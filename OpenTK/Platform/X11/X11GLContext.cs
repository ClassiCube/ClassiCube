#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
using System.Runtime.InteropServices;
using OpenTK.Graphics;

namespace OpenTK.Platform.X11 {
	
	#if USE_DX
	public class IGraphicsContext { }
	#endif
	
	internal sealed class X11GLContext : IGraphicsContext {
		#if !USE_DX
		X11Window cur;
		bool vsync_supported;
		int vsync_interval;

		public X11GLContext(GraphicsMode mode, X11Window window) {
			Debug.Print("Creating X11GLContext context: ");
			cur = window;
			XVisualInfo info = cur.VisualInfo;
			
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
			if (!Glx.glXMakeCurrent(API.DefaultDisplay, window.WinHandle, ContextHandle))
				throw new GraphicsContextException("Failed to make context current.");
			
			Glx.LoadEntryPoints();
			vsync_supported = Glx.glXSwapIntervalSGI != null;
		}

		public override void SwapBuffers() {
			Glx.glXSwapBuffers(API.DefaultDisplay, cur.WinHandle);
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

		protected override void Dispose(bool manuallyCalled) {
			if (ContextHandle == IntPtr.Zero) return;
			
			if (manuallyCalled) {
				IntPtr display = API.DefaultDisplay;
				if (Glx.glXGetCurrentContext() == ContextHandle) {
					Glx.glXMakeCurrent(display, IntPtr.Zero, IntPtr.Zero);
				}
				Glx.glXDestroyContext(display, ContextHandle);
			} else {
				Debug.Print("=== [Warning] OpenGL context leaked ===");
			}
			ContextHandle = IntPtr.Zero;
		}
		#endif

		internal unsafe static XVisualInfo SelectGraphicsMode(GraphicsMode template) {
			int[] attribs = GetVisualAttribs(template);
			int major = 0, minor = 0, fbcount;
			if (!Glx.glXQueryVersion(API.DefaultDisplay, ref major, ref minor))
				throw new InvalidOperationException("glXQueryVersion failed");
			
			IntPtr visual = IntPtr.Zero;
			if (major >= 1 && minor >= 3) {
				Debug.Print("Getting FB config.");
				// ChooseFBConfig returns an array of GLXFBConfig opaque structures (i.e. mapped to IntPtrs).
				IntPtr* fbconfigs = Glx.glXChooseFBConfig(API.DefaultDisplay, API.DefaultScreen, attribs, out fbcount);
				if (fbcount > 0 && fbconfigs != null) {
					// Use the first GLXFBConfig from the fbconfigs array (best match)
					visual = Glx.glXGetVisualFromFBConfig(API.DefaultDisplay, *fbconfigs);
					API.XFree((IntPtr)fbconfigs);
				}
			}
			
			if (visual == IntPtr.Zero) {
				Debug.Print("Falling back to glXChooseVisual.");
				visual = Glx.glXChooseVisual(API.DefaultDisplay, API.DefaultScreen, attribs);
			}
			if (visual == IntPtr.Zero)
				throw new GraphicsModeException("Requested GraphicsMode not available.");
			
			XVisualInfo info = (XVisualInfo)Marshal.PtrToStructure(visual, typeof(XVisualInfo));
			API.XFree(visual);
			return info;
		}
		
		static int[] GetVisualAttribs(GraphicsMode mode) {
			// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_61/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm%23glxchoosefbconfig
			// See http://www-01.ibm.com/support/knowledgecenter/ssw_aix_71/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm%23b5c84be452rree
			// for the attribute declarations. Note that the attributes are different than those used in Glx.ChooseVisual.
			int[] attribs = new int[20];
			int i = 0;
			ColorFormat color = mode.ColorFormat;
			
			Debug.Print("Bits per pixel: {0}", color.BitsPerPixel);
			Debug.Print("Depth: {0}", mode.Depth);
			
			if (!color.IsIndexed) {
				attribs[i++] = (int)GLXAttribute.RGBA;
			}
			attribs[i++] = (int)GLXAttribute.RED_SIZE;
			attribs[i++] = color.Red;
			attribs[i++] = (int)GLXAttribute.GREEN_SIZE;
			attribs[i++] = color.Green;
			attribs[i++] = (int)GLXAttribute.BLUE_SIZE;
			attribs[i++] = color.Blue;
			attribs[i++] = (int)GLXAttribute.ALPHA_SIZE;
			attribs[i++] = color.Alpha;

			if (mode.Depth > 0) {
				attribs[i++] = (int)GLXAttribute.DEPTH_SIZE;
				attribs[i++] = mode.Depth;
			}
			if (mode.Stencil > 0) {
				attribs[i++] = (int)GLXAttribute.STENCIL_SIZE;
				attribs[i++] = mode.Stencil;
			}
			if (mode.Buffers > 1) {
				attribs[i++] = (int)GLXAttribute.DOUBLEBUFFER;
			}

			attribs[i++] = 0;
			return attribs;
		}
	}
}