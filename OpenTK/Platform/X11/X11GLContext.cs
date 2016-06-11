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
		
		IntPtr Display;
		X11WindowInfo currentWindow;
		bool vsync_supported;
		int vsync_interval;

		public X11GLContext(GraphicsMode mode, IWindowInfo window) {
			if (mode == null)
				throw new ArgumentNullException("mode");
			if (window == null)
				throw new ArgumentNullException("window");

			Debug.Print( "Creating X11GLContext context: " );
			currentWindow = (X11WindowInfo)window;
			Display = API.DefaultDisplay;			
			XVisualInfo info = currentWindow.VisualInfo;
			Mode = GetGraphicsMode( info );
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

		public override void SwapBuffers() {
			if (Display == IntPtr.Zero || currentWindow.WindowHandle == IntPtr.Zero)
				throw new InvalidOperationException(
					String.Format("Window is invalid. Display ({0}), Handle ({1}).", Display, currentWindow.WindowHandle));
			Glx.glXSwapBuffers(Display, currentWindow.WindowHandle);
		}

		public override void MakeCurrent(IWindowInfo window) {
			if (window == currentWindow && IsCurrent)
				return;

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
		
		internal static GraphicsMode SelectGraphicsMode( GraphicsMode template, out XVisualInfo info ) {
			int[] attribs = GetVisualAttribs( template.ColorFormat, template.Depth, template.Stencil, template.Buffers );
			IntPtr visual = SelectVisual( attribs );
			if( visual == IntPtr.Zero )
				throw new GraphicsModeException( "Requested GraphicsMode not available." );
			
			info = (XVisualInfo)Marshal.PtrToStructure( visual, typeof( XVisualInfo ) );
			API.XFree( visual );
			return GetGraphicsMode( info );
		}
		
		internal static GraphicsMode GetGraphicsMode( XVisualInfo info ) {
			// See what we *really* got:
			int r, g, b, a, depth, stencil, buffers;
			IntPtr display = API.DefaultDisplay;
			Glx.glXGetConfig( display, ref info, GLXAttribute.ALPHA_SIZE, out a );
			Glx.glXGetConfig( display, ref info, GLXAttribute.RED_SIZE, out r );
			Glx.glXGetConfig( display, ref info, GLXAttribute.GREEN_SIZE, out g );
			Glx.glXGetConfig( display, ref info, GLXAttribute.BLUE_SIZE, out b );
			Glx.glXGetConfig( display, ref info, GLXAttribute.DEPTH_SIZE, out depth );
			Glx.glXGetConfig( display, ref info, GLXAttribute.STENCIL_SIZE, out stencil );
			Glx.glXGetConfig( display, ref info, GLXAttribute.DOUBLEBUFFER, out buffers );
			++buffers;
			// the above lines returns 0 - false and 1 - true.
			return new GraphicsMode( info.VisualID, new ColorFormat(r, g, b, a), depth, stencil, buffers );
		}

		// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig
		// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree
		// for the attribute declarations. Note that the attributes are different than those used in Glx.ChooseVisual.
		static unsafe IntPtr SelectVisual( int[] visualAttribs ) {
			int major = 0, minor = 0;
			if( !Glx.glXQueryVersion( API.DefaultDisplay, ref major, ref minor ) )
				throw new InvalidOperationException( "glXQueryVersion failed, potentially corrupt OpenGL driver" );		
			int screen = API.XDefaultScreen( API.DefaultDisplay );
			
			if( major >= 1 && minor >= 3 ) {
				Debug.Print( "Getting FB config." );
				int fbcount;
				// Note that ChooseFBConfig returns an array of GLXFBConfig opaque structures (i.e. mapped to IntPtrs).
				IntPtr* fbconfigs = Glx.glXChooseFBConfig( API.DefaultDisplay, screen, visualAttribs, out fbcount );
				if( fbcount > 0 && fbconfigs != null ) {
					// We want to use the first GLXFBConfig from the fbconfigs array (the first one is the best match).
					IntPtr visual = Glx.glXGetVisualFromFBConfig( API.DefaultDisplay, *fbconfigs );
					API.XFree( (IntPtr)fbconfigs );
					return visual;
				}
			}
			Debug.Print( "Falling back to glXChooseVisual." );
			return Glx.glXChooseVisual( API.DefaultDisplay, screen, visualAttribs );
		}
		
		static int[] GetVisualAttribs( ColorFormat color, int depth, int stencil, int buffers ) {
			int[] attribs = new int[16];
			int index = 0;
			Debug.Print("Bits per pixel: {0}", color.BitsPerPixel);
			Debug.Print("Depth: {0}", depth);
			
			if( !color.IsIndexed )
				attribs[index++] = (int)GLXAttribute.RGBA;
			attribs[index++] = (int)GLXAttribute.RED_SIZE;
			attribs[index++] = color.Red;
			attribs[index++] = (int)GLXAttribute.GREEN_SIZE;
			attribs[index++] = color.Green;
			attribs[index++] = (int)GLXAttribute.BLUE_SIZE;
			attribs[index++] = color.Blue;
			attribs[index++] = (int)GLXAttribute.ALPHA_SIZE;
			attribs[index++] = color.Alpha;

			if( depth > 0 ) {
				attribs[index++] = (int)GLXAttribute.DEPTH_SIZE;
				attribs[index++] = depth;
			}
			if( stencil > 0 ) {
				attribs[index++] = (int)GLXAttribute.STENCIL_SIZE;
				attribs[index++] = stencil;
			}
			if( buffers > 1 )
				attribs[index++] = (int)GLXAttribute.DOUBLEBUFFER;

			attribs[index++] = 0;
			return attribs;
		}
	}
}
