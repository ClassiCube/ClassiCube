#if !USE_DX
//  Created by Erik Ylvisaker on 3/17/08.
//  Copyright 2008. All rights reserved.

using System;
using System.Runtime.InteropServices;
using OpenTK.Graphics;

namespace OpenTK.Platform.MacOS {

	class AglContext : IGraphicsContext {
		
		bool mVSync = false;
		// Todo: keep track of which display adapter was specified when the context was created.
		// IntPtr displayID;
		bool mIsFullscreen = false;

		public AglContext(GraphicsMode mode, CarbonWindow window) {
			Debug.Print("Window info: {0}", window);
			CreateContext(mode, window, true);
			MakeCurrent(window);
		}
		
		int[] GetAttribs(GraphicsMode mode, bool fullscreen) {
			int[] attribs = new int[20];
			int i = 0;
			ColorFormat color = mode.ColorFormat;
			
			Debug.Print("Bits per pixel: {0}", color.BitsPerPixel);
			Debug.Print("Depth: {0}", mode.Depth);
			
			attribs[i++] = (int)AglAttribute.RGBA;
			attribs[i++] = (int)AglAttribute.DOUBLEBUFFER;

			attribs[i++] = (int)AglAttribute.RED_SIZE;
			attribs[i++] = color.Red;
			attribs[i++] = (int)AglAttribute.GREEN_SIZE;
			attribs[i++] = color.Green;
			attribs[i++] = (int)AglAttribute.BLUE_SIZE;
			attribs[i++] = color.Blue;
			attribs[i++] = (int)AglAttribute.ALPHA_SIZE;
			attribs[i++] = color.Alpha;

			if (mode.Depth > 0) {
				attribs[i++] = (int)AglAttribute.DEPTH_SIZE;
				attribs[i++] = mode.Depth;
			}
			if (mode.Stencil > 0) {
				attribs[i++] = (int)AglAttribute.STENCIL_SIZE;
				attribs[i++] = mode.Stencil;
			}
			if (fullscreen) {
				attribs[i++] = (int)AglAttribute.FULLSCREEN;
			}
			
			attribs[i++] = 0;
			return attribs;
		}
		
		void CreateContext(GraphicsMode mode, CarbonWindow wind, bool fullscreen) {
			int[] attribs = GetAttribs(mode, fullscreen);
			IntPtr pixelFormat;

			// Choose a pixel format with the attributes we specified.
			if (fullscreen) {
				IntPtr gdevice;
				IntPtr cgdevice = GetQuartzDevice(wind);

				if (cgdevice == IntPtr.Zero)
					cgdevice = QuartzDisplayDevice.MainDisplay;

				OSStatus status = API.DMGetGDeviceByDisplayID(cgdevice, out gdevice, false);
				
				if (status != OSStatus.NoError)
					throw new MacOSException(status, "DMGetGDeviceByDisplayID failed.");

				pixelFormat = Agl.aglChoosePixelFormat(ref gdevice, 1, attribs);
				int err = Agl.aglGetError();

				if (err == Agl.AGL_BAD_PIXEL_FORMAT) {
					Debug.Print("Failed to create full screen pixel format.");
					Debug.Print("Trying again to create a non-fullscreen pixel format.");

					CreateContext(mode, wind, false);
					return;
				}
			} else {
				pixelFormat = Agl.aglChoosePixelFormat(IntPtr.Zero, 0, attribs);
				Agl.CheckReturnValue(0, "aglChoosePixelFormat");
			}

			Debug.Print("Creating AGL context.");

			// create the context and share it with the share reference.
			ContextHandle = Agl.aglCreateContext(pixelFormat, IntPtr.Zero);
			Agl.CheckReturnValue(0, "aglCreateContext");

			// Free the pixel format from memory.
			Agl.aglDestroyPixelFormat(pixelFormat);
			Agl.CheckReturnValue(0, "aglDestroyPixelFormat");
			
			SetDrawable(wind);
			Update(wind);
			
			MakeCurrent(wind);

			Debug.Print("context: {0}", ContextHandle);
		}

		IntPtr GetQuartzDevice(CarbonWindow window) { return window.Display.Metadata; }
		
		void SetDrawable(CarbonWindow window) {
			IntPtr windowPort = API.GetWindowPort(window.WinHandle);
			//Debug.Print("Setting drawable for context {0} to window port: {1}", Handle.Handle, windowPort);

			byte code = Agl.aglSetDrawable(ContextHandle, windowPort);
			Agl.CheckReturnValue(code, "aglSetDrawable");
		}
		
		public override void Update(INativeWindow window) {
			CarbonWindow wind = (CarbonWindow)window;

			if (wind.goFullScreenHack) {
				wind.goFullScreenHack = false;
				wind.SetFullscreen(this);
				return;
			} else if (wind.goWindowedHack) {
				wind.goWindowedHack = false;
				wind.UnsetFullscreen(this);
			}

			if (mIsFullscreen) return;
			SetDrawable(wind);
			Agl.aglUpdateContext(ContextHandle);
		}

		bool firstFullScreen = false;
		internal void SetFullScreen(CarbonWindow wind, out int width, out int height) {
			Debug.Print("Switching to full screen {0}x{1} on context {2}",
			            wind.Display.Width, wind.Display.Height, ContextHandle);

			CG.CGDisplayCapture(GetQuartzDevice(wind));
			byte code = Agl.aglSetFullScreen(ContextHandle, wind.Display.Width, wind.Display.Height, 0, 0);
			Agl.CheckReturnValue(code, "aglSetFullScreen");
			MakeCurrent(wind);

			width = wind.Display.Width;
			height = wind.Display.Height;

			// This is a weird hack to workaround a bug where the first time a context
			// is made fullscreen, we just end up with a blank screen.  So we undo it as fullscreen
			// and redo it as fullscreen.
			if (!firstFullScreen) {
				firstFullScreen = true;
				UnsetFullScreen(wind);
				SetFullScreen(wind, out width, out height);
			}
			mIsFullscreen = true;
		}
		
		internal void UnsetFullScreen(CarbonWindow window) {
			Debug.Print("Unsetting AGL fullscreen.");
			byte code = Agl.aglSetDrawable(ContextHandle, IntPtr.Zero);
			Agl.CheckReturnValue(code, "aglSetDrawable");
			code = Agl.aglUpdateContext(ContextHandle);
			Agl.CheckReturnValue(code, "aglUpdateContext");
			
			CG.CGDisplayRelease(GetQuartzDevice(window));
			Debug.Print("Resetting drawable.");
			SetDrawable(window);

			mIsFullscreen = false;
		}

		public override void SwapBuffers() {
			Agl.aglSwapBuffers(ContextHandle);
			Agl.CheckReturnValue(0, "aglSwapBuffers");
		}
		
		void MakeCurrent(INativeWindow window) {
			byte code = Agl.aglSetCurrentContext(ContextHandle);
			Agl.CheckReturnValue(code, "aglSetCurrentContext");
		}

		public override bool VSync {
			get { return mVSync; }
			set {
				int intVal = value ? 1 : 0;
				Agl.aglSetInteger(ContextHandle, Agl.AGL_SWAP_INTERVAL, ref intVal);
				mVSync = value;
			}
		}

		bool IsDisposed;
		protected override void Dispose(bool disposing) {
			if (IsDisposed || ContextHandle == IntPtr.Zero) return;

			Debug.Print("Disposing of AGL context.");
			Agl.aglSetCurrentContext(IntPtr.Zero);
			
			//Debug.Print("Setting drawable to null for context {0}.", Handle.Handle);
			//Agl.aglSetDrawable(Handle.Handle, IntPtr.Zero);

			// I do not know MacOS allows us to destroy a context from a separate thread,
			// like the finalizer thread.  It's untested, but worst case is probably
			// an exception on application exit, which would be logged to the console.
			Debug.Print("Destroying context");
			byte code = Agl.aglDestroyContext(ContextHandle);
			try {
				Agl.CheckReturnValue(code, "aglDestroyContext");
				ContextHandle = IntPtr.Zero;
				Debug.Print("Context destruction completed successfully.");
			} catch (MacOSException) {
				Debug.Print("Failed to destroy context.");
				if (disposing) throw;
			}
			IsDisposed = true;
		}

		const string lib = "libdl.dylib";
		[DllImport(lib, EntryPoint = "NSIsSymbolNameDefined")]
		static extern bool NSIsSymbolNameDefined(string s);
		[DllImport(lib, EntryPoint = "NSLookupAndBindSymbol")]
		static extern IntPtr NSLookupAndBindSymbol(string s);
		[DllImport(lib, EntryPoint = "NSAddressOfSymbol")]
		static extern IntPtr NSAddressOfSymbol(IntPtr symbol);

		public override IntPtr GetAddress(string function) {
			string fname = "_" + function;
			if (!NSIsSymbolNameDefined(fname)) return IntPtr.Zero;

			IntPtr symbol = NSLookupAndBindSymbol(fname);
			if (symbol != IntPtr.Zero)
				symbol = NSAddressOfSymbol(symbol);
			return symbol;
		}
	}
}
#endif