#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * Contributions from Erik Ylvisaker
 * See license.txt for license info
 */
#endregion

#region --- Using Directives ---

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;

using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;

#endregion

namespace OpenTK.Platform.Windows
{
	/// \internal
	/// <summary>
	/// Provides methods to create and control an opengl context on the Windows platform.
	/// This class supports OpenTK, and is not intended for use by OpenTK programs.
	/// </summary>
	internal sealed class WinGLContext : DesktopGraphicsContext
	{
		static object SyncRoot = new object();

		static IntPtr opengl32Handle;
		const string opengl32Name = "OPENGL32.DLL";

		bool vsync_supported;

		#region --- Contructors ---

		static WinGLContext()
		{
			lock (SyncRoot)
			{
				// Dynamically load the OpenGL32.dll in order to use the extension loading capabilities of Wgl.
				if (opengl32Handle == IntPtr.Zero)
				{
					opengl32Handle = Functions.LoadLibrary(opengl32Name);
					if (opengl32Handle == IntPtr.Zero)
						throw new ApplicationException(String.Format("LoadLibrary(\"{0}\") call failed with code {1}",
						                                             opengl32Name, Marshal.GetLastWin32Error()));
					Debug.WriteLine(String.Format("Loaded opengl32.dll: {0}", opengl32Handle));
				}
			}
		}

		public WinGLContext(GraphicsMode format, WinWindowInfo window,
		                    int major, int minor)
		{
			// There are many ways this code can break when accessed by multiple threads. The biggest offender is
			// the sharedContext stuff, which will only become valid *after* this constructor returns.
			// The easiest solution is to serialize all context construction - hence the big lock, below.
			lock (SyncRoot)
			{
				if (window == null)
					throw new ArgumentNullException("window", "Must point to a valid window.");
				if (window.WindowHandle == IntPtr.Zero)
					throw new ArgumentException("window", "Must be a valid window.");

				Debug.Print("OpenGL will be bound to handle: {0}", window.WindowHandle);
				SelectGraphicsModePFD(format, (WinWindowInfo)window);
				Debug.Write("Setting pixel format... ");
				SetGraphicsModePFD(format, (WinWindowInfo)window);

				Debug.Write("Falling back to GL2... ");
				Handle = new ContextHandle(Wgl.Imports.CreateContext(window.DeviceContext));
				if (Handle == ContextHandle.Zero)
					Handle = new ContextHandle(Wgl.Imports.CreateContext(window.DeviceContext));
				if (Handle == ContextHandle.Zero)
					throw new GraphicsContextException(
						String.Format("Context creation failed. Wgl.CreateContext() error: {0}.",
						              Marshal.GetLastWin32Error()));
				
				Debug.WriteLine(String.Format("success! (id: {0})", Handle));
			}
		}

		#endregion

		#region --- IGraphicsContext Members ---

		#region SwapBuffers

		public override void SwapBuffers()
		{
			if (!Functions.SwapBuffers(Wgl.Imports.GetCurrentDC()))
				throw new GraphicsContextException(String.Format(
					"Failed to swap buffers for context {0} current. Error: {1}", this, Marshal.GetLastWin32Error()));
		}

		#endregion

		#region MakeCurrent

		public override void MakeCurrent(IWindowInfo window)
		{
			bool success;

			if (window != null)
			{
				if (((WinWindowInfo)window).WindowHandle == IntPtr.Zero)
					throw new ArgumentException("window", "Must point to a valid window.");

				success = Wgl.Imports.MakeCurrent(((WinWindowInfo)window).DeviceContext, Handle.Handle);
			}
			else
				success = Wgl.Imports.MakeCurrent(IntPtr.Zero, IntPtr.Zero);

			if (!success)
				throw new GraphicsContextException(String.Format(
					"Failed to make context {0} current. Error: {1}", this, Marshal.GetLastWin32Error()));

		}
		#endregion

		#region IsCurrent

		public override bool IsCurrent
		{
			get { return Wgl.Imports.GetCurrentContext() == Handle.Handle; }
		}

		#endregion

		#region public bool VSync

		/// <summary>
		/// Gets or sets a System.Boolean indicating whether SwapBuffer calls are synced to the screen refresh rate.
		/// </summary>
		public override bool VSync
		{
			get
			{
				return vsync_supported && Wgl.Ext.GetSwapInterval() != 0;
			}
			set
			{
				if (vsync_supported)
					Wgl.Ext.SwapInterval(value ? 1 : 0);
			}
		}

		#endregion

		#region void LoadAll()

		public override void LoadAll()
		{
			Wgl.LoadAll();
			vsync_supported = Wgl.Arb.SupportsExtension(this, "WGL_EXT_swap_control") &&
				Wgl.Delegates.wglGetSwapIntervalEXT != null && Wgl.Delegates.wglSwapIntervalEXT != null;

			base.LoadAll();
		}

		#endregion

		#endregion

		#region --- IGLContextInternal Members ---

		#region IWindowInfo IGLContextInternal.Info
		/*
        IWindowInfo IGraphicsContextInternal.Info
        {
            get { return (IWindowInfo)windowInfo; }
        }
		 */
		#endregion

		#region GetAddress

		public override IntPtr GetAddress(string function_string)
		{
			return Wgl.Imports.GetProcAddress(function_string);
		}

		#endregion

		#endregion

		#region --- Private Methods ---

		int modeIndex;
		void SetGraphicsModePFD(GraphicsMode mode, WinWindowInfo window) {
			// Find out what we really got as a format:
			IntPtr deviceContext = window.DeviceContext;
			PixelFormatDescriptor pfd = new PixelFormatDescriptor();
			pfd.Size = API.PixelFormatDescriptorSize;
			pfd.Version = API.PixelFormatDescriptorVersion;
			Functions.DescribePixelFormat(deviceContext, modeIndex, API.PixelFormatDescriptorSize, ref pfd);
			
			Mode = new GraphicsMode(
				(IntPtr)modeIndex, new ColorFormat(pfd.RedBits, pfd.GreenBits, pfd.BlueBits, pfd.AlphaBits),
				pfd.DepthBits, pfd.StencilBits,
				(pfd.Flags & PixelFormatDescriptorFlags.DOUBLEBUFFER) != 0 ? 2 : 1);
			
			Debug.WriteLine(modeIndex);
			if (!Functions.SetPixelFormat(window.DeviceContext, modeIndex, ref pfd))
				throw new GraphicsContextException(String.Format(
					"Requested GraphicsMode not available. SetPixelFormat error: {0}", Marshal.GetLastWin32Error()));
		}

		void SelectGraphicsModePFD(GraphicsMode format, WinWindowInfo window) {
			IntPtr deviceContext = window.DeviceContext;
			Debug.WriteLine(String.Format("Device context: {0}", deviceContext));
			ColorFormat color = format.ColorFormat;

			Debug.Write("Selecting pixel format PFD... ");
			PixelFormatDescriptor pfd = new PixelFormatDescriptor();
			pfd.Size = API.PixelFormatDescriptorSize;
			pfd.Version = API.PixelFormatDescriptorVersion;
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

			modeIndex = Functions.ChoosePixelFormat(deviceContext, ref pfd);
			if (modeIndex == 0)
				throw new GraphicsModeException("The requested GraphicsMode is not available.");
		}

		#endregion

		#region --- Internal Methods ---

		#region internal IntPtr DeviceContext

		internal IntPtr DeviceContext
		{
			get { return Wgl.Imports.GetCurrentDC(); }
		}


		#endregion

		#endregion

		#region --- Overrides ---

		/// <summary>Returns a System.String describing this OpenGL context.</summary>
		/// <returns>A System.String describing this OpenGL context.</returns>
		public override string ToString()
		{
			return (this as IGraphicsContextInternal).Context.ToString();
		}

		#endregion

		#region --- IDisposable Members ---

		public override void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		private void Dispose(bool calledManually)
		{
			if (!IsDisposed)
			{
				if (calledManually)
				{
					DestroyContext();
				}
				else
				{
					Debug.Print("[Warning] OpenGL context {0} leaked. Did you forget to call IGraphicsContext.Dispose()?",
					            Handle.Handle);
				}
				IsDisposed = true;
			}
		}

		~WinGLContext()
		{
			Dispose(false);
		}

		#region private void DestroyContext()

		private void DestroyContext()
		{
			if (Handle != ContextHandle.Zero)
			{
				try
				{
					// This will fail if the user calls Dispose() on thread X when the context is current on thread Y.
					if (!Wgl.Imports.DeleteContext(Handle.Handle))
						Debug.Print("Failed to destroy OpenGL context {0}. Error: {1}",
						            Handle.ToString(), Marshal.GetLastWin32Error());
				}
				catch (AccessViolationException e)
				{
					Debug.WriteLine("An access violation occured while destroying the OpenGL context. Please report at http://www.opentk.com.");
					Debug.Indent();
					Debug.Print("Marshal.GetLastWin32Error(): {0}", Marshal.GetLastWin32Error().ToString());
					Debug.WriteLine(e.ToString());
					Debug.Unindent();
				}
				Handle = ContextHandle.Zero;
			}
		}

		#endregion

		#endregion
	}
}
