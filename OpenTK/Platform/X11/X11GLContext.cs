#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
using System.Runtime.InteropServices;
using OpenTK.Graphics;

namespace OpenTK.Platform.X11 {
	
	internal sealed class X11GLContext : IGraphicsContext {
		X11Window cur;
		bool vsync_supported;
		int vsync_interval;

		public X11GLContext(GraphicsMode mode, X11Window window) {
			Debug.Print("Creating X11GLContext context: ");
			cur = window;
			XVisualInfo info = cur.VisualInfo;
			Mode = GetGraphicsMode(info);
			
			// Cannot pass a Property by reference.
			IntPtr display = API.DefaultDisplay;
			ContextHandle = Glx.glXCreateContext(display, ref info, IntPtr.Zero, true);

			if (ContextHandle == IntPtr.Zero) {
				Debug.Print("failed. Trying indirect... ");
				ContextHandle = Glx.glXCreateContext(display, ref info, IntPtr.Zero, false);
			}
			
			if (ContextHandle != IntPtr.Zero) {
				Debug.Print("Context created (id: {0}).", ContextHandle);
			} else {
				throw new GraphicsContextException("Context creation failed. Error: NULL returned");
			}
			
			if (!Glx.glXIsDirect(display, ContextHandle))
				Debug.Print("Warning: Context is not direct.");
		}

		public override void SwapBuffers() {
			Glx.glXSwapBuffers(API.DefaultDisplay, cur.WinHandle);
		}

		public override void MakeCurrent(INativeWindow window) {
			Debug.Print("Making context {0} current (Screen: {1}, Window: {2})... ",
			            ContextHandle, API.DefaultScreen, window.WinHandle);
			
			if (!Glx.glXMakeCurrent(API.DefaultDisplay, window.WinHandle, ContextHandle))
				throw new GraphicsContextException("Failed to make context current.");
			cur = (X11Window)window;
		}

		public override bool IsCurrent {
			get { return Glx.glXGetCurrentContext() == ContextHandle; }
		}

		public override bool VSync {
			get { return vsync_supported && vsync_interval != 0; }
			set {
				if (vsync_supported) {
					int result = Glx.glXSwapIntervalSGI(value ? 1 : 0);
					if (result != 0)
						Debug.Print("VSync = {0} failed, error code: {1}.", value, result);
					vsync_interval = value ? 1 : 0;
				}
			}
		}

		public override IntPtr GetAddress(string function) {
			return Glx.GetAddress(function);
		}

		public override void LoadAll() {
			Glx.LoadEntryPoints();
			vsync_supported = Glx.glXSwapIntervalSGI != null;
			Debug.Print("Context supports vsync: {0}.", vsync_supported);
		}

		protected override void Dispose(bool manuallyCalled) {
			if (ContextHandle == IntPtr.Zero) return;
			
			if (manuallyCalled) {
				IntPtr display = API.DefaultDisplay;
				if (IsCurrent) {
					Glx.glXMakeCurrent(display, IntPtr.Zero, IntPtr.Zero);
				}
				Glx.glXDestroyContext(display, ContextHandle);
			} else {
				Debug.Print("=== [Warning] OpenGL context leaked ===");
			}
			ContextHandle = IntPtr.Zero;
		}
		
		internal static GraphicsMode SelectGraphicsMode(GraphicsMode template, out XVisualInfo info) {
			int[] attribs = GetVisualAttribs(template.ColorFormat, template.Depth, template.Stencil, template.Buffers);
			IntPtr visual = SelectVisual(attribs);
			if (visual == IntPtr.Zero)
				throw new GraphicsModeException("Requested GraphicsMode not available.");
			
			info = (XVisualInfo)Marshal.PtrToStructure(visual, typeof(XVisualInfo));
			API.XFree(visual);
			return GetGraphicsMode(info);
		}
		
		internal static GraphicsMode GetGraphicsMode(XVisualInfo info) {
			// See what we *really* got:
			int r, g, b, a, depth, stencil, buffers;
			IntPtr display = API.DefaultDisplay;
			Glx.glXGetConfig(display, ref info, GLXAttribute.ALPHA_SIZE, out a);
			Glx.glXGetConfig(display, ref info, GLXAttribute.RED_SIZE, out r);
			Glx.glXGetConfig(display, ref info, GLXAttribute.GREEN_SIZE, out g);
			Glx.glXGetConfig(display, ref info, GLXAttribute.BLUE_SIZE, out b);
			Glx.glXGetConfig(display, ref info, GLXAttribute.DEPTH_SIZE, out depth);
			Glx.glXGetConfig(display, ref info, GLXAttribute.STENCIL_SIZE, out stencil);
			Glx.glXGetConfig(display, ref info, GLXAttribute.DOUBLEBUFFER, out buffers);
			++buffers;
			// the above lines returns 0 - false and 1 - true.
			return new GraphicsMode(new ColorFormat(r, g, b, a), depth, stencil, buffers);
		}

		// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig
		// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree
		// for the attribute declarations. Note that the attributes are different than those used in Glx.ChooseVisual.
		static unsafe IntPtr SelectVisual(int[] attribs) {
			int major = 0, minor = 0;
			if (!Glx.glXQueryVersion(API.DefaultDisplay, ref major, ref minor))
				throw new InvalidOperationException("glXQueryVersion failed, potentially corrupt OpenGL driver");
			
			if (major >= 1 && minor >= 3) {
				Debug.Print("Getting FB config.");
				int fbcount;
				// Note that ChooseFBConfig returns an array of GLXFBConfig opaque structures (i.e. mapped to IntPtrs).
				IntPtr* fbconfigs = Glx.glXChooseFBConfig(API.DefaultDisplay, API.DefaultScreen, attribs, out fbcount);
				if (fbcount > 0 && fbconfigs != null) {
					// We want to use the first GLXFBConfig from the fbconfigs array (the first one is the best match).
					IntPtr visual = Glx.glXGetVisualFromFBConfig(API.DefaultDisplay, *fbconfigs);
					API.XFree((IntPtr)fbconfigs);
					return visual;
				}
			}
			Debug.Print("Falling back to glXChooseVisual.");
			return Glx.glXChooseVisual(API.DefaultDisplay, API.DefaultScreen, attribs);
		}
		
		static int[] GetVisualAttribs(ColorFormat color, int depth, int stencil, int buffers) {
			int[] attribs = new int[16];
			int index = 0;
			Debug.Print("Bits per pixel: {0}", color.BitsPerPixel);
			Debug.Print("Depth: {0}", depth);
			
			if (!color.IsIndexed)
				attribs[index++] = (int)GLXAttribute.RGBA;
			attribs[index++] = (int)GLXAttribute.RED_SIZE;
			attribs[index++] = color.Red;
			attribs[index++] = (int)GLXAttribute.GREEN_SIZE;
			attribs[index++] = color.Green;
			attribs[index++] = (int)GLXAttribute.BLUE_SIZE;
			attribs[index++] = color.Blue;
			attribs[index++] = (int)GLXAttribute.ALPHA_SIZE;
			attribs[index++] = color.Alpha;

			if (depth > 0) {
				attribs[index++] = (int)GLXAttribute.DEPTH_SIZE;
				attribs[index++] = depth;
			}
			if (stencil > 0) {
				attribs[index++] = (int)GLXAttribute.STENCIL_SIZE;
				attribs[index++] = stencil;
			}
			if (buffers > 1)
				attribs[index++] = (int)GLXAttribute.DOUBLEBUFFER;

			attribs[index++] = 0;
			return attribs;
		}
	}
}
