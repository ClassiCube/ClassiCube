//  Created by Erik Ylvisaker on 3/17/08.
//  Copyright 2008. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using OpenTK.Graphics;
using OpenTK.Platform.MacOS.Carbon;
using AGLPixelFormat = System.IntPtr;

namespace OpenTK.Platform.MacOS {

	class AglContext : GraphicsContextBase {
		
		bool mVSync = false;
		// Todo: keep track of which display adapter was specified when the context was created.
		// IntPtr displayID;
		
		CarbonWindowInfo carbonWindow;
		bool mIsFullscreen = false;

		public AglContext(GraphicsMode mode, IWindowInfo window)
		{
			Debug.Print("Window info: {0}", window);

			Mode = mode;
			this.carbonWindow = (CarbonWindowInfo)window;

			CreateContext(mode, carbonWindow, true);
		}

		private void AddPixelAttrib(List<int> attribs, Agl.PixelFormatAttribute name, int value)
		{
			attribs.Add((int)name);
			attribs.Add(value);
		}
		
		void CreateContext(GraphicsMode mode, CarbonWindowInfo carbonWindow, bool fullscreen)
		{
			List<int> attribs = new List<int>();

			attribs.Add((int)Agl.PixelFormatAttribute.AGL_RGBA);
			attribs.Add((int)Agl.PixelFormatAttribute.AGL_DOUBLEBUFFER);
			AddPixelAttrib(attribs, Agl.PixelFormatAttribute.AGL_RED_SIZE, mode.ColorFormat.Red);
			AddPixelAttrib(attribs, Agl.PixelFormatAttribute.AGL_GREEN_SIZE, mode.ColorFormat.Green);
			AddPixelAttrib(attribs, Agl.PixelFormatAttribute.AGL_BLUE_SIZE, mode.ColorFormat.Blue);
			AddPixelAttrib(attribs, Agl.PixelFormatAttribute.AGL_ALPHA_SIZE, mode.ColorFormat.Alpha);

			if (mode.Depth > 0)
				AddPixelAttrib(attribs, Agl.PixelFormatAttribute.AGL_DEPTH_SIZE, mode.Depth);

			if (mode.Stencil > 0)
				AddPixelAttrib(attribs, Agl.PixelFormatAttribute.AGL_STENCIL_SIZE, mode.Stencil);

			if (fullscreen)
				attribs.Add((int)Agl.PixelFormatAttribute.AGL_FULLSCREEN);
			attribs.Add((int)Agl.PixelFormatAttribute.AGL_NONE);

			Debug.Print("AGL Attribute array:  ");
			for (int i = 0; i < attribs.Count; i++)
				Debug.Print(attribs[i].ToString() + "  ");
			Debug.Print("");

			AGLPixelFormat aglPixelFormat;

			// Choose a pixel format with the attributes we specified.
			if (fullscreen) {
				IntPtr gdevice;
				IntPtr cgdevice = GetQuartzDevice(carbonWindow);

				if (cgdevice == IntPtr.Zero)
					cgdevice = QuartzDisplayDeviceDriver.MainDisplay;

				OSStatus status = API.DMGetGDeviceByDisplayID(cgdevice, out gdevice, false);
				
				if (status != OSStatus.NoError)
					throw new MacOSException(status, "DMGetGDeviceByDisplayID failed.");

				aglPixelFormat = Agl.aglChoosePixelFormat(ref gdevice, 1, attribs.ToArray());
				Agl.AglError err = Agl.aglGetError();

				if (err == Agl.AglError.BadPixelFormat) {
					Debug.Print("Failed to create full screen pixel format.");
					Debug.Print("Trying again to create a non-fullscreen pixel format.");

					CreateContext(mode, carbonWindow, false);
					return;
				}
			} else {
				aglPixelFormat = Agl.aglChoosePixelFormat(IntPtr.Zero, 0, attribs.ToArray());
				Agl.CheckReturnValue( 0, "aglChoosePixelFormat" );
			}


			Debug.Print("Creating AGL context.");

			// create the context and share it with the share reference.
			ContextHandle = Agl.aglCreateContext(aglPixelFormat, IntPtr.Zero);
			Agl.CheckReturnValue( 0, "aglCreateContext" );

			// Free the pixel format from memory.
			Agl.aglDestroyPixelFormat(aglPixelFormat);
			Agl.CheckReturnValue( 0, "aglDestroyPixelFormat" );
			
			SetDrawable(carbonWindow);
			Update(carbonWindow);
			
			MakeCurrent(carbonWindow);

			Debug.Print("context: {0}", ContextHandle);
		}

		private IntPtr GetQuartzDevice(CarbonWindowInfo carbonWindow)
		{
			IntPtr windowRef = carbonWindow.WindowRef;

			if (!CarbonGLNative.WindowRefMap.ContainsKey(windowRef))
				return IntPtr.Zero;

			WeakReference nativeRef = CarbonGLNative.WindowRefMap[windowRef];
			if (!nativeRef.IsAlive)
				return IntPtr.Zero;

			CarbonGLNative window = nativeRef.Target as CarbonGLNative;

			if (window == null)
				return IntPtr.Zero;

			return QuartzDisplayDeviceDriver.HandleTo(window.TargetDisplayDevice);

		}
		
		void SetDrawable(CarbonWindowInfo carbonWindow)
		{
			IntPtr windowPort = API.GetWindowPort(carbonWindow.WindowRef);
			//Debug.Print("Setting drawable for context {0} to window port: {1}", Handle.Handle, windowPort);

			byte code = Agl.aglSetDrawable(ContextHandle, windowPort);
			Agl.CheckReturnValue( code, "aglSetDrawable" );
		}
		
		public override void Update(IWindowInfo window)
		{
			CarbonWindowInfo carbonWindow = (CarbonWindowInfo)window;

			if (carbonWindow.goFullScreenHack)
			{
				carbonWindow.goFullScreenHack = false;
				CarbonGLNative wind = GetCarbonWindow(carbonWindow);

				if (wind != null)
					wind.SetFullscreen(this);
				else
					Debug.Print("Could not find window!");

				return;
			}
			else if (carbonWindow.goWindowedHack)
			{
				carbonWindow.goWindowedHack = false;
				CarbonGLNative wind = GetCarbonWindow(carbonWindow);

				if (wind != null)
					wind.UnsetFullscreen(this);
				else
					Debug.Print("Could not find window!");

			}

			if (mIsFullscreen)
				return;
			
			SetDrawable(carbonWindow);

			Agl.aglUpdateContext(ContextHandle);
		}

		private CarbonGLNative GetCarbonWindow(CarbonWindowInfo carbonWindow)
		{
			WeakReference r = CarbonGLNative.WindowRefMap[carbonWindow.WindowRef];
			return r.IsAlive ? (CarbonGLNative)r.Target : null;
		}

		bool firstFullScreen = false;
		internal void SetFullScreen(CarbonWindowInfo info, out int width, out int height)
		{
			CarbonGLNative wind = GetCarbonWindow(info);

			Debug.Print("Switching to full screen {0}x{1} on context {2}",
			            wind.TargetDisplayDevice.Width, wind.TargetDisplayDevice.Height, ContextHandle);

			CG.CGDisplayCapture(GetQuartzDevice(info));
			byte code = Agl.aglSetFullScreen(ContextHandle, wind.TargetDisplayDevice.Width, wind.TargetDisplayDevice.Height, 0, 0);
			Agl.CheckReturnValue(code, "aglSetFullScreen");
			MakeCurrent(info);

			width = wind.TargetDisplayDevice.Width;
			height = wind.TargetDisplayDevice.Height;

			// This is a weird hack to workaround a bug where the first time a context
			// is made fullscreen, we just end up with a blank screen.  So we undo it as fullscreen
			// and redo it as fullscreen.
			if (!firstFullScreen) {
				firstFullScreen = true;
				UnsetFullScreen(info);
				SetFullScreen(info, out width, out height);
			}

			mIsFullscreen = true;
		}
		
		internal void UnsetFullScreen(CarbonWindowInfo windowInfo)
		{
			Debug.Print("Unsetting AGL fullscreen.");
			byte code = Agl.aglSetDrawable(ContextHandle, IntPtr.Zero);
			Agl.CheckReturnValue( code, "aglSetDrawable" );
			code = Agl.aglUpdateContext(ContextHandle);
			Agl.CheckReturnValue( code, "aglUpdateContext" );
			
			CG.CGDisplayRelease(GetQuartzDevice(windowInfo));
			Debug.Print("Resetting drawable.");
			SetDrawable(windowInfo);

			mIsFullscreen = false;
		}


		#region IGraphicsContext Members

		public override void SwapBuffers() {
			Agl.aglSwapBuffers(ContextHandle);
			Agl.CheckReturnValue( 0, "aglSwapBuffers" );
		}
		
		public override void MakeCurrent( IWindowInfo window ) {
			byte code = Agl.aglSetCurrentContext( ContextHandle );
			Agl.CheckReturnValue(code, "aglSetCurrentContext" );
		}

		public override bool IsCurrent {
			get {  return ContextHandle == Agl.aglGetCurrentContext(); }
		}

		public override bool VSync {
			get { return mVSync; }
			set {
				int intVal = value ? 1 : 0;
				Agl.aglSetInteger(ContextHandle, Agl.ParameterNames.AGL_SWAP_INTERVAL, ref intVal);
				mVSync = value;
			}
		}

		#endregion

		#region IDisposable Members

		protected override void Dispose(bool disposing)
		{
			if (IsDisposed || ContextHandle == IntPtr.Zero)
				return;

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
				Agl.CheckReturnValue(code, "aglDestroyContext" );
				ContextHandle = IntPtr.Zero;
				Debug.Print("Context destruction completed successfully.");
			} catch( MacOSException ) {
				Debug.Print("Failed to destroy context.");
				if( disposing )
					throw;
			}
			IsDisposed = true;
		}

		#endregion

		#region IGraphicsContextInternal Members

		private const string Library = "libdl.dylib";

		[DllImport(Library, EntryPoint = "NSIsSymbolNameDefined")]
		private static extern bool NSIsSymbolNameDefined(string s);
		[DllImport(Library, EntryPoint = "NSLookupAndBindSymbol")]
		private static extern IntPtr NSLookupAndBindSymbol(string s);
		[DllImport(Library, EntryPoint = "NSAddressOfSymbol")]
		private static extern IntPtr NSAddressOfSymbol(IntPtr symbol);

		public override IntPtr GetAddress(string function)
		{
			string fname = "_" + function;
			if (!NSIsSymbolNameDefined(fname))
				return IntPtr.Zero;

			IntPtr symbol = NSLookupAndBindSymbol(fname);
			if (symbol != IntPtr.Zero)
				symbol = NSAddressOfSymbol(symbol);

			return symbol;
		}
		
		public override void LoadAll() {
			new OpenTK.Graphics.OpenGL.GL().LoadEntryPoints( this );
		}

		#endregion
	}
}