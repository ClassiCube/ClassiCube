#if !USE_DX
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
	
	internal sealed class WinGLContext : IGraphicsContext {
		static IntPtr opengl32Handle;
		bool vsync_supported;
		IntPtr dc;

		static WinGLContext() {
			// Dynamically load the OpenGL32.dll in order to use the extension loading capabilities of Wgl.
			if (opengl32Handle == IntPtr.Zero) {
				opengl32Handle = API.LoadLibrary("OPENGL32.DLL");
				if (opengl32Handle == IntPtr.Zero) {
					throw new ApplicationException("LoadLibrary failed. " +
					                              "Error: " + Marshal.GetLastWin32Error());
				}
				Debug.Print("Loaded opengl32.dll: {0}", opengl32Handle);
			}
		}

		public WinGLContext(GraphicsMode format, WinWindow window) {
			Debug.Print("OpenGL will be bound to handle: {0}", window.WinHandle);
			SelectGraphicsModePFD(format, window);
			Debug.Print("Setting pixel format... ");
			SetGraphicsModePFD(format, window);

			ContextHandle = Wgl.wglCreateContext(window.DeviceContext);
			if (ContextHandle == IntPtr.Zero)
				ContextHandle = Wgl.wglCreateContext(window.DeviceContext);
			if (ContextHandle == IntPtr.Zero)
				throw new GraphicsContextException("Context creation failed. Error: " + Marshal.GetLastWin32Error());
			
			if (!Wgl.wglMakeCurrent(window.DeviceContext, ContextHandle)) {
				throw new GraphicsContextException("Failed to make context current. " +
				                                  "Error: " + Marshal.GetLastWin32Error());
			}
			
			dc = Wgl.wglGetCurrentDC();		
			Wgl.LoadEntryPoints();
			vsync_supported = Wgl.wglGetSwapIntervalEXT != null && Wgl.wglSwapIntervalEXT != null;
		}

		public override void SwapBuffers() {
			if (!API.SwapBuffers(dc))
				throw new GraphicsContextException("Failed to swap buffers for context. " +
				                                  "Error: " + Marshal.GetLastWin32Error());
		}

		public override bool VSync {
			get { return vsync_supported && Wgl.wglGetSwapIntervalEXT() != 0; }
			set {
				if (vsync_supported)
					Wgl.wglSwapIntervalEXT(value ? 1 : 0);
			}
		}

		public override IntPtr GetAddress(string funcName) {
			IntPtr address = Wgl.GetAddress(funcName);
			if (address != IntPtr.Zero) return address;
			return API.GetProcAddress(opengl32Handle, funcName);
		}


		int modeIndex;
		void SetGraphicsModePFD(GraphicsMode mode, WinWindow window) {
			// Find out what we really got as a format:
			IntPtr deviceContext = window.DeviceContext;
			PixelFormatDescriptor pfd = new PixelFormatDescriptor();
			pfd.Size = PixelFormatDescriptor.DefaultSize;
			pfd.Version = PixelFormatDescriptor.DefaultVersion;
			API.DescribePixelFormat(deviceContext, modeIndex, pfd.Size, ref pfd);
			
			Debug.Print("WGL mode index: " + modeIndex.ToString());
			if (!API.SetPixelFormat(window.DeviceContext, modeIndex, ref pfd))
				throw new GraphicsContextException("SetPixelFormat failed. " +
				                                  "Error: " + Marshal.GetLastWin32Error());
		}

		void SelectGraphicsModePFD(GraphicsMode format, WinWindow window) {
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
			if (ContextHandle == IntPtr.Zero) return;
			
			if (calledManually) {
				// This will fail if the user calls Dispose() on thread X when the context is current on thread Y.
				Wgl.wglDeleteContext(ContextHandle);
			} else {
				Debug.Print("=== [Warning] OpenGL context leaked ===");
			}
			ContextHandle = IntPtr.Zero;
		}
	}
}
#endif