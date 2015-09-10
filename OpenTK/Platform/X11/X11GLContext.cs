#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
using System.Runtime.InteropServices;
using OpenTK.Graphics;

namespace OpenTK.Platform.X11 {
	
	/// \internal
	/// <summary> Provides methods to create and control an opengl context on the X11 platform.
	/// This class supports OpenTK, and is not intended for use by OpenTK programs. </summary>
	internal sealed class X11GLContext : GraphicsContextBase {
		
		// We assume that we cannot move a GL context to a different display connection.
		// For this reason, we'll "lock" onto the display of the window used in the context
		// constructor and we'll throw an exception if the user ever tries to make the context
		// current on window originating from a different display.
		IntPtr display;
		X11WindowInfo currentWindow;
		bool vsync_supported;
		int vsync_interval;

		public X11GLContext(GraphicsMode mode, IWindowInfo window) {
			if (mode == null)
				throw new ArgumentNullException("mode");
			if (window == null)
				throw new ArgumentNullException("window");

			Mode = mode;
			currentWindow = (X11WindowInfo)window;
			Display = currentWindow.Display;
			currentWindow.VisualInfo = SelectVisual(mode);
			
			Debug.Print("Creating X11GLContext context: ");
			XVisualInfo info = currentWindow.VisualInfo;
			// Cannot pass a Property by reference.
			ContextHandle = Glx.glXCreateContext(Display, ref info, IntPtr.Zero, true);

			if (ContextHandle == IntPtr.Zero) {
				Debug.Print("failed. Trying indirect... ");
				ContextHandle = Glx.glXCreateContext(Display, ref info, IntPtr.Zero, false);
			}
			
			if (ContextHandle != IntPtr.Zero)
				Debug.Print("Context created (id: {0}).", ContextHandle);
			else
				throw new GraphicsContextException("Failed to create OpenGL context. Glx.CreateContext call returned 0.");

			if (!Glx.glXIsDirect(Display, ContextHandle))
				Debug.Print("Warning: Context is not direct.");
		}

		IntPtr Display {
			get { return display; }
			set {
				if (value == IntPtr.Zero)
					throw new ArgumentOutOfRangeException();
				if (display != IntPtr.Zero)
					throw new InvalidOperationException("The display connection may not be changed after being set.");
				display = value;
			}
		}

		XVisualInfo SelectVisual(GraphicsMode mode) {
			XVisualInfo info = new XVisualInfo();
			info.VisualID = (IntPtr)mode.Index;
			info.Screen = currentWindow.Screen;
			int items;
			
			lock (API.Lock) {
				IntPtr vs = API.XGetVisualInfo(Display, XVisualInfoMask.ID | XVisualInfoMask.Screen, ref info, out items);
				if (items == 0)
					throw new GraphicsModeException(String.Format("Invalid GraphicsMode specified ({0}).", mode));

				info = (XVisualInfo)Marshal.PtrToStructure(vs, typeof(XVisualInfo));
				API.XFree(vs);
			}
			return info;
		}

		public override void SwapBuffers() {
			if (Display == IntPtr.Zero || currentWindow.WindowHandle == IntPtr.Zero)
				throw new InvalidOperationException(
					String.Format("Window is invalid. Display ({0}), Handle ({1}).", Display, currentWindow.WindowHandle));
			Glx.glXSwapBuffers(Display, currentWindow.WindowHandle);
		}

		public override void MakeCurrent(IWindowInfo window) {
			if (window == currentWindow && IsCurrent)
				return;

			if (window != null && ((X11WindowInfo)window).Display != Display)
				throw new InvalidOperationException("MakeCurrent() may only be called on windows originating from the same display that spawned this GL context.");

			if (window == null) {
				Debug.Print("Releasing context {0} (Display: {1})... ", ContextHandle, Display);
				if (!Glx.glXMakeCurrent(Display, IntPtr.Zero, IntPtr.Zero))
					Debug.Print("failed to release context");
			} else {
				X11WindowInfo w = (X11WindowInfo)window;
				Debug.Print("Making context {0} current (Display: {1}, Screen: {2}, Window: {3})... ", ContextHandle, Display, w.Screen, w.WindowHandle);

				if (Display == IntPtr.Zero || w.WindowHandle == IntPtr.Zero || ContextHandle == IntPtr.Zero)
					throw new InvalidOperationException("Invalid display, window or context.");

				if (!Glx.glXMakeCurrent(Display, w.WindowHandle, ContextHandle))
					throw new GraphicsContextException("Failed to make context current.");

			}
			currentWindow = (X11WindowInfo)window;
		}

		public override bool IsCurrent {
			get { return Glx.glXGetCurrentContext() == ContextHandle; }
		}

		public override bool VSync {
			get { return vsync_supported && vsync_interval != 0; }
			set {
				if (vsync_supported) {
					GLXErrorCode error_code = Glx.glXSwapIntervalSGI(value ? 1 : 0);
					if (error_code != X11.GLXErrorCode.NO_ERROR)
						Debug.Print("VSync = {0} failed, error code: {1}.", value, error_code);
					vsync_interval = value ? 1 : 0;
				}
			}
		}

		public override IntPtr GetAddress(string function) {
			return Glx.glXGetProcAddress(function);
		}

		public override void LoadAll() {
			new Glx().LoadEntryPoints();
			vsync_supported = Glx.glXSwapIntervalSGI != null;
			Debug.Print("Context supports vsync: {0}.", vsync_supported);
			new OpenTK.Graphics.OpenGL.GL().LoadEntryPoints( this );
		}

		protected override void Dispose(bool manuallyCalled) {
			if (!IsDisposed) {
				if (manuallyCalled) {
					IntPtr display = Display;

					if (IsCurrent) {
						Glx.glXMakeCurrent(display, IntPtr.Zero, IntPtr.Zero);
					}
					Glx.glXDestroyContext(display, ContextHandle);
				}
			} else {
				Debug.Print("[Warning] {0} leaked.", this.GetType().Name);
			}
			IsDisposed = true;
		}
	}
}
