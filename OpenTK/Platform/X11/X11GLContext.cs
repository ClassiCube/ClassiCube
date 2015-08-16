#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;

using OpenTK.Graphics;

namespace OpenTK.Platform.X11
{
	/// \internal
	/// <summary>
	/// Provides methods to create and control an opengl context on the X11 platform.
	/// This class supports OpenTK, and is not intended for use by OpenTK programs.
	/// </summary>
	internal sealed class X11GLContext : GraphicsContextBase
	{
		#region Fields

		// We assume that we cannot move a GL context to a different display connection.
		// For this reason, we'll "lock" onto the display of the window used in the context
		// constructor and we'll throw an exception if the user ever tries to make the context
		// current on window originating from a different display.
		IntPtr display;
		X11WindowInfo currentWindow;
		bool vsync_supported;
		int vsync_interval;

		#endregion

		#region --- Constructors ---

		public X11GLContext(GraphicsMode mode, IWindowInfo window)
		{
			if (mode == null)
				throw new ArgumentNullException("mode");
			if (window == null)
				throw new ArgumentNullException("window");

			Mode = mode;

			// Do not move this lower, as almost everything requires the Display
			// property to be correctly set.
			Display = ((X11WindowInfo)window).Display;
			
			currentWindow = (X11WindowInfo)window;
			currentWindow.VisualInfo = SelectVisual(mode, currentWindow);
			
			Debug.Write("Creating X11GLContext context: ");			
			Debug.Write("Using legacy context creation... ");
			
			XVisualInfo info = currentWindow.VisualInfo;
			// Cannot pass a Property by reference.
			ContextHandle = Glx.CreateContext(Display, ref info, IntPtr.Zero, true);

			if (ContextHandle == IntPtr.Zero)
			{
				Debug.WriteLine("failed. Trying indirect... ");
				ContextHandle = Glx.CreateContext(Display, ref info, IntPtr.Zero, false);
			}
			
			if (ContextHandle != IntPtr.Zero)
				Debug.Print("Context created (id: {0}).", ContextHandle);
			else
				throw new GraphicsContextException("Failed to create OpenGL context. Glx.CreateContext call returned 0.");

			if (!Glx.IsDirect(Display, ContextHandle))
				Debug.Print("Warning: Context is not direct.");
		}

		#endregion

		#region --- Private Methods ---

		IntPtr Display
		{
			get { return display; }
			set
			{
				if (value == IntPtr.Zero)
					throw new ArgumentOutOfRangeException();
				if (display != IntPtr.Zero)
					throw new InvalidOperationException("The display connection may not be changed after being set.");
				display = value;
			}
		}

		#region XVisualInfo SelectVisual(GraphicsMode mode, X11WindowInfo currentWindow)

		XVisualInfo SelectVisual(GraphicsMode mode, X11WindowInfo currentWindow)
		{
			XVisualInfo info = new XVisualInfo();
			info.VisualID = (IntPtr)mode.Index;
			info.Screen = currentWindow.Screen;
			int items;
			
			lock (API.Lock)
			{
				IntPtr vs = Functions.XGetVisualInfo(Display, XVisualInfoMask.ID | XVisualInfoMask.Screen, ref info, out items);
				if (items == 0)
					throw new GraphicsModeException(String.Format("Invalid GraphicsMode specified ({0}).", mode));

				info = (XVisualInfo)Marshal.PtrToStructure(vs, typeof(XVisualInfo));
				Functions.XFree(vs);
			}

			return info;
		}

		#endregion

		#endregion

		#region --- IGraphicsContext Members ---

		#region SwapBuffers()

		public override void SwapBuffers()
		{
			if (Display == IntPtr.Zero || currentWindow.WindowHandle == IntPtr.Zero)
				throw new InvalidOperationException(
					String.Format("Window is invalid. Display ({0}), Handle ({1}).", Display, currentWindow.WindowHandle));
			Glx.SwapBuffers(Display, currentWindow.WindowHandle);
		}

		#endregion

		#region MakeCurrent

		public override void MakeCurrent(IWindowInfo window)
		{
			if (window == currentWindow && IsCurrent)
				return;

			if (window != null && ((X11WindowInfo)window).Display != Display)
				throw new InvalidOperationException("MakeCurrent() may only be called on windows originating from the same display that spawned this GL context.");

			if (window == null)
			{
				Debug.Write(String.Format("Releasing context {0} (Display: {1})... ", ContextHandle, Display));

				bool result = Glx.MakeCurrent(Display, IntPtr.Zero, IntPtr.Zero);
				if (result) {
					currentWindow = null;
				}
				Debug.Print("{0}", result ? "done!" : "failed.");
			}
			else
			{
				X11WindowInfo w = (X11WindowInfo)window;
				Debug.Write(String.Format("Making context {0} current (Display: {1}, Screen: {2}, Window: {3})... ",
				                          ContextHandle, Display, w.Screen, w.WindowHandle));

				if (Display == IntPtr.Zero || w.WindowHandle == IntPtr.Zero || ContextHandle == IntPtr.Zero)
					throw new InvalidOperationException("Invalid display, window or context.");

				bool result = Glx.MakeCurrent(Display, w.WindowHandle, ContextHandle);
				if (result) {
					currentWindow = w;
				}

				if (!result)
					throw new GraphicsContextException("Failed to make context current.");
				else
					Debug.WriteLine("done!");
			}

			currentWindow = (X11WindowInfo)window;
		}

		#endregion

		#region IsCurrent

		public override bool IsCurrent {
			get { return Glx.GetCurrentContext() == ContextHandle; }
		}

		#endregion

		#region VSync

		public override bool VSync
		{
			get
			{
				return vsync_supported && vsync_interval != 0;
			}
			set
			{
				if (vsync_supported)
				{
					GLXErrorCode error_code = Glx.glXSwapIntervalSGI(value ? 1 : 0);
					if (error_code != X11.GLXErrorCode.NO_ERROR)
						Debug.Print("VSync = {0} failed, error code: {1}.", value, error_code);
					vsync_interval = value ? 1 : 0;
				}
			}
		}

		#endregion

		#region GetAddress

		public override IntPtr GetAddress(string function) {
			return Glx.GetProcAddress(function);
		}

		#endregion

		#region LoadAll

		public override void LoadAll()
		{
			new Glx().LoadEntryPoints();
			vsync_supported = Glx.glXSwapIntervalSGI != null;
			Debug.Print("Context supports vsync: {0}.", vsync_supported);
			new OpenTK.Graphics.OpenGL.GL().LoadEntryPoints( this );
		}

		#endregion

		#endregion

		#region --- IGLContextInternal Members ---

		#region IWindowInfo IGLContextInternal.Info

		//IWindowInfo IGraphicsContextInternal.Info { get { return window; } }

		#endregion

		#endregion

		#region --- IDisposable Members ---

		public override void Dispose()
		{
			this.Dispose(true);
			GC.SuppressFinalize(this);
		}

		private void Dispose(bool manuallyCalled)
		{
			if (!IsDisposed)
			{
				if (manuallyCalled)
				{
					IntPtr display = Display;

					if (IsCurrent) {
						Glx.MakeCurrent(display, IntPtr.Zero, IntPtr.Zero);
					}
					Glx.DestroyContext(display, ContextHandle);
				}
			}
			else
			{
				Debug.Print("[Warning] {0} leaked.", this.GetType().Name);
			}
			IsDisposed = true;
		}
		

		~X11GLContext()
		{
			this.Dispose(false);
		}

		#endregion
	}
}
