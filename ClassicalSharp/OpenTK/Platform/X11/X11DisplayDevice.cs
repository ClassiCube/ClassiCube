#region License
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;

namespace OpenTK.Platform.X11 {
	
	internal static class X11DisplayDevice {
		
		internal static void Init() {
			IntPtr display = API.DefaultDisplay;
			int screen = API.DefaultScreen;
			DisplayDevice device = new DisplayDevice();
			
			/* TODO: Use Xinerama and XRandR for querying these */			
			device.Bounds.Width  = API.XDisplayWidth(display, screen);
			device.Bounds.Height = API.XDisplayHeight(display, screen);
			device.BitsPerPixel  = API.XDefaultDepth(display, screen);		
			DisplayDevice.Default = device;
		}
	}
}
