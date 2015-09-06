#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

using OpenTK.Graphics;

namespace OpenTK.Platform.X11
{
	class X11GraphicsMode : IGraphicsMode
	{
		// Todo: Add custom visual selection algorithm, instead of ChooseFBConfig/ChooseVisual.
		// It seems the Choose* methods do not take multisampling into account (at least on some
		// drivers).

		#region IGraphicsMode Members

		public override GraphicsMode SelectGraphicsMode(ColorFormat color, int depth, int stencil, int buffers)
		{
			GraphicsMode gfx;
			// The actual GraphicsMode that will be selected.
			IntPtr visual = IntPtr.Zero;
			IntPtr display = API.DefaultDisplay;
			
			// Try to select a visual using Glx.ChooseFBConfig and Glx.GetVisualFromFBConfig.
			// This is only supported on GLX 1.3 - if it fails, fall back to Glx.ChooseVisual.
			int[] attribs = GetVisualAttribs(color, depth, stencil, buffers);
			visual = SelectVisualUsingFBConfig(attribs);
			
			if (visual == IntPtr.Zero)
				visual = SelectVisualUsingChooseVisual(attribs);
			
			if (visual == IntPtr.Zero)
				throw new GraphicsModeException("Requested GraphicsMode not available.");
			
			XVisualInfo info = (XVisualInfo)Marshal.PtrToStructure(visual, typeof(XVisualInfo));
			
			// See what we *really* got:
			int r, g, b, a;
			Glx.glXGetConfig(display, ref info, GLXAttribute.ALPHA_SIZE, out a);
			Glx.glXGetConfig(display, ref info, GLXAttribute.RED_SIZE, out r);
			Glx.glXGetConfig(display, ref info, GLXAttribute.GREEN_SIZE, out g);
			Glx.glXGetConfig(display, ref info, GLXAttribute.BLUE_SIZE, out b);
			Glx.glXGetConfig(display, ref info, GLXAttribute.DEPTH_SIZE, out depth);
			Glx.glXGetConfig(display, ref info, GLXAttribute.STENCIL_SIZE, out stencil);
			Glx.glXGetConfig(display, ref info, GLXAttribute.DOUBLEBUFFER, out buffers);
			++buffers;
			// the above lines returns 0 - false and 1 - true.
			
			gfx = new GraphicsMode(info.VisualID, new ColorFormat(r, g, b, a), depth, stencil, buffers);
			API.XFree(visual);
			return gfx;
		}

		#endregion

		#region Private Members

		// See http://publib.boulder.ibm.com/infocenter/systems/index.jsp?topic=/com.ibm.aix.opengl/doc/openglrf/glXChooseFBConfig.htm
		// for the attribute declarations. Note that the attributes are different than those used in Glx.ChooseVisual.
		IntPtr SelectVisualUsingFBConfig( int[] visualAttribs ) {
			// Select a visual that matches the parameters set by the user.
			IntPtr display = API.DefaultDisplay;
			IntPtr visual = IntPtr.Zero;
			try {
				int screen = API.XDefaultScreen(display);
				IntPtr root = API.XRootWindow(display, screen);
				Debug.Print("Display: {0}, Screen: {1}, RootWindow: {2}", display, screen, root);
				
				unsafe
				{
					Debug.Print("Getting FB config.");
					int fbcount;
					// Note that ChooseFBConfig returns an array of GLXFBConfig opaque structures (i.e. mapped to IntPtrs).
					IntPtr* fbconfigs = Glx.glXChooseFBConfig(display, screen, visualAttribs, out fbcount);
					if (fbcount > 0 && fbconfigs != null)
					{
						// We want to use the first GLXFBConfig from the fbconfigs array (the first one is the best match).
						visual = Glx.glXGetVisualFromFBConfig(display, *fbconfigs);
						API.XFree((IntPtr)fbconfigs);
					}
				}
			} catch (EntryPointNotFoundException) {
				Debug.Print("Function glXChooseFBConfig not supported.");
				return IntPtr.Zero;
			}
			return visual;
		}

		// See http://publib.boulder.ibm.com/infocenter/systems/index.jsp?topic=/com.ibm.aix.opengl/doc/openglrf/glXChooseVisual.htm
		IntPtr SelectVisualUsingChooseVisual( int[] visualAttribs ) {
			Debug.Print("Falling back to glXChooseVisual.");
			IntPtr display = API.DefaultDisplay;
			return Glx.glXChooseVisual(display, API.XDefaultScreen(display), visualAttribs);
		}
		
		int[] GetVisualAttribs( ColorFormat color, int depth, int stencil, int buffers ) {
			List<int> attribs = new List<int>();
			Debug.Print("Bits per pixel: {0}", color.BitsPerPixel);

			if (color.BitsPerPixel > 0) {
				if (!color.IsIndexed)
					attribs.Add((int)GLXAttribute.RGBA);
				attribs.Add((int)GLXAttribute.RED_SIZE);
				attribs.Add(color.Red);
				attribs.Add((int)GLXAttribute.GREEN_SIZE);
				attribs.Add(color.Green);
				attribs.Add((int)GLXAttribute.BLUE_SIZE);
				attribs.Add(color.Blue);
				attribs.Add((int)GLXAttribute.ALPHA_SIZE);
				attribs.Add(color.Alpha);
			}

			Debug.Print("Depth: {0}", depth);

			if (depth > 0) {
				attribs.Add((int)GLXAttribute.DEPTH_SIZE);
				attribs.Add(depth);
			}

			if (buffers > 1)
				attribs.Add((int)GLXAttribute.DOUBLEBUFFER);

			if (stencil > 1) {
				attribs.Add((int)GLXAttribute.STENCIL_SIZE);
				attribs.Add(stencil);
			}

			attribs.Add(0);
			return attribs.ToArray();
		}

		#endregion
	}
}
