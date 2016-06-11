#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
#endregion

using System;
using System.Runtime.InteropServices;
using OpenTK.Graphics;

namespace OpenTK.Platform.Windows {
	
	/// \internal
	/// <summary>
	/// Provides methods to create and control an opengl context on the Windows platform.
	/// This class supports OpenTK, and is not intended for use by OpenTK programs.
	/// </summary>
	internal sealed class WinGLContext : GraphicsContextBase
	{
		static IntPtr opengl32Handle;
		const string opengl32Name = "OPENGL32.DLL";
		bool vsync_supported;

		static WinGLContext() {
			// Dynamically load the OpenGL32.dll in order to use the extension loading capabilities of Wgl.
			if (opengl32Handle == IntPtr.Zero) {
				opengl32Handle = API.LoadLibrary(opengl32Name);
				if (opengl32Handle == IntPtr.Zero)
					throw new ApplicationException(String.Format("LoadLibrary(\"{0}\") call failed with code {1}",
					                                             opengl32Name, Marshal.GetLastWin32Error()));
				Debug.Print( "Loaded opengl32.dll: {0}", opengl32Handle );
			}
		}

		public WinGLContext(GraphicsMode format, WinWindowInfo window) {
			if (window == null)
				throw new ArgumentNullException("window", "Must point to a valid window.");
			if (window.WindowHandle == IntPtr.Zero)
				throw new ArgumentException("window", "Must be a valid window.");

			Debug.Print( "OpenGL will be bound to handle: {0}", window.WindowHandle );
			SelectGraphicsModePFD(format, (WinWindowInfo)window);
			Debug.Print( "Setting pixel format... " );
			SetGraphicsModePFD(format, (WinWindowInfo)window);

			ContextHandle = Wgl.wglCreateContext(window.DeviceContext);
			if (ContextHandle == IntPtr.Zero)
				ContextHandle = Wgl.wglCreateContext(window.DeviceContext);
			if (ContextHandle == IntPtr.Zero)
				throw new GraphicsContextException(
					String.Format("Context creation failed. Wgl.CreateContext() error: {0}.",
					              Marshal.GetLastWin32Error()));
			
			Debug.Print( "success! (id: {0})", ContextHandle );
		}

		public override void SwapBuffers() {
			if (!API.SwapBuffers(dc))
				throw new GraphicsContextException(String.Format(
					"Failed to swap buffers for context {0} current. Error: {1}", this, Marshal.GetLastWin32Error()));
		}

		IntPtr dc;
		public override void MakeCurrent(IWindowInfo window) {
			bool success;

			if (window != null) {
				if (((WinWindowInfo)window).WindowHandle == IntPtr.Zero)
					throw new ArgumentException("window", "Must point to a valid window.");

				success = Wgl.wglMakeCurrent(((WinWindowInfo)window).DeviceContext, ContextHandle);
			} else {
				success = Wgl.wglMakeCurrent(IntPtr.Zero, IntPtr.Zero);
			}

			if (!success)
				throw new GraphicsContextException(String.Format(
					"Failed to make context {0} current. Error: {1}", this, Marshal.GetLastWin32Error()));
			dc = Wgl.wglGetCurrentDC();
		}

		public override bool IsCurrent {
			get { return Wgl.wglGetCurrentContext() == ContextHandle; }
		}

		/// <summary> Gets or sets a System.Boolean indicating whether SwapBuffer calls are synced to the screen refresh rate. </summary>
		public override bool VSync {
			get { return vsync_supported && Wgl.wglGetSwapIntervalEXT() != 0; }
			set {
				if (vsync_supported)
					Wgl.wglSwapIntervalEXT(value ? 1 : 0);
			}
		}

		public override void LoadAll() {
			new Wgl().LoadEntryPoints();
			vsync_supported = Wgl.wglGetSwapIntervalEXT != null
				&& Wgl.wglSwapIntervalEXT != null;
			new OpenTK.Graphics.OpenGL.GL().LoadEntryPoints( this );
		}

		public override IntPtr GetAddress(string funcName) {
			IntPtr dynAddress = Wgl.wglGetProcAddress(funcName);
			if( !BindingsBase.IsInvalidAddress( dynAddress ) )
				return dynAddress;
			return API.GetProcAddress( opengl32Handle, funcName );
		}


		int modeIndex;
		void SetGraphicsModePFD(GraphicsMode mode, WinWindowInfo window) {
			// Find out what we really got as a format:
			IntPtr deviceContext = window.DeviceContext;
			PixelFormatDescriptor pfd = new PixelFormatDescriptor();
			pfd.Size = PixelFormatDescriptor.DefaultSize;
			pfd.Version = PixelFormatDescriptor.DefaultVersion;
			API.DescribePixelFormat(deviceContext, modeIndex, pfd.Size, ref pfd);
			
			Mode = new GraphicsMode(
				(IntPtr)modeIndex, new ColorFormat(pfd.RedBits, pfd.GreenBits, pfd.BlueBits, pfd.AlphaBits),
				pfd.DepthBits, pfd.StencilBits,
				(pfd.Flags & PixelFormatDescriptorFlags.DOUBLEBUFFER) != 0 ? 2 : 1);
			
			Debug.Print(modeIndex);
			if (!API.SetPixelFormat(window.DeviceContext, modeIndex, ref pfd))
				throw new GraphicsContextException(String.Format(
					"Requested GraphicsMode not available. SetPixelFormat error: {0}", Marshal.GetLastWin32Error()));
		}

		void SelectGraphicsModePFD(GraphicsMode format, WinWindowInfo window) {
			IntPtr deviceContext = window.DeviceContext;
			Debug.Print("Device context: {0}", deviceContext);
			ColorFormat color = format.ColorFormat;

			Debug.Print("Selecting pixel format PFD... ");
			PixelFormatDescriptor pfd = new PixelFormatDescriptor();
			pfd.Size = PixelFormatDescriptor.DefaultSize;
			pfd.Version = PixelFormatDescriptor.DefaultVersion;
			pfd.Flags =
				PixelFormatDescriptorFlags.SUPPORT_OPENGL |
				PixelFormatDescriptorFlags.DRAW_TO_WINDOW;
			pfd.ColorBits = (byte)(color.Red + color.Green + color.Blue);

			pfd.PixelType = color.IsIndexed ? PixelType.INDEXED : PixelType.RGBA;
			pfd.RedBits = (byte)color.Red;
			pfd.GreenBits = (byte)color.Green;
			pfd.BlueBits = (byte)color.Blue;
			pfd.AlphaBits = (byte)color.Alpha;

			pfd.DepthBits = (byte)format.Depth;
			pfd.StencilBits = (byte)format.Stencil;

			if (format.Depth <= 0) pfd.Flags |= PixelFormatDescriptorFlags.DEPTH_DONTCARE;
			if (format.Buffers > 1) pfd.Flags |= PixelFormatDescriptorFlags.DOUBLEBUFFER;

			modeIndex = API.ChoosePixelFormat(deviceContext, ref pfd);
			if (modeIndex == 0)
				throw new GraphicsModeException("The requested GraphicsMode is not available.");
		}

		public override string ToString() {
			return ContextHandle.ToString();
		}

		protected override void Dispose(bool calledManually) {
			if (IsDisposed) return;
			
			if (calledManually) {
				DestroyContext();
			} else {
				Debug.Print("[Warning] OpenGL context {0} leaked. Did you forget to call IGraphicsContext.Dispose()?",  ContextHandle);
			}
			IsDisposed = true;
		}

		private void DestroyContext() {
			if (ContextHandle == IntPtr.Zero) return;
			
			try {
				// This will fail if the user calls Dispose() on thread X when the context is current on thread Y.
				if (!Wgl.wglDeleteContext(ContextHandle))
					Debug.Print("Failed to destroy OpenGL context {0}. Error: {1}",
					            ContextHandle.ToString(), Marshal.GetLastWin32Error());
			} catch (AccessViolationException e) {
				Debug.Print("An access violation occured while destroying the OpenGL context. Please report at http://www.opentk.com.");
				Debug.Print("Marshal.GetLastWin32Error(): {0}", Marshal.GetLastWin32Error().ToString());
				Debug.Print(e.ToString());
			}
			ContextHandle = IntPtr.Zero;
		}
	}
}
