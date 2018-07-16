#region License
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Collections.Generic;
using System.Drawing;

namespace OpenTK.Platform.X11 {
	
	internal static class X11DisplayDevice {
		
		internal static void Init() {
			List<DisplayDevice> devices = new List<DisplayDevice>();
			bool xinerama_supported = false, xrandr_supported = false;
			try {
				xinerama_supported = QueryXinerama(devices);
			} catch {
				Debug.Print("Xinerama query failed.");
			}

			if (!xinerama_supported) {
				// We assume that devices are equivalent to the number of available screens.
				// Note: this won't work correctly in the case of distinct X servers.
				for (int i = 0; i < API.ScreenCount; i++) {
					DisplayDevice dev = new DisplayDevice();
					dev.IsPrimary = i == API.DefaultScreen;
					dev.Metadata = (IntPtr)i;
					devices.Add(dev);
				}
			}

			try {
				xrandr_supported = QueryXRandR(devices);
			} catch { }

			if (!xrandr_supported) {
				Debug.Print("XRandR query failed, no DisplayDevice support available.");
			}
		}
		
		unsafe static bool QueryXinerama(List<DisplayDevice> devices) {
			// Try to use Xinerama to obtain the geometry of all output devices.
			int a, b;
			if (API.XineramaQueryExtension(API.DefaultDisplay, out a, out b) && API.XineramaIsActive(API.DefaultDisplay)) {
				int count = 0;
				XineramaScreenInfo* screens = API.XineramaQueryScreens(API.DefaultDisplay, out count);
				
				for (int i = 0; i < count; i++) {
					XineramaScreenInfo screen = screens[i];
					DisplayDevice dev = new DisplayDevice();
					dev.Bounds = new Rectangle(screen.X, screen.Y, screen.Width, screen.Height);
					
					// We consider the first device returned by Xinerama as the primary one.
					// Makes sense conceptually, but is there a way to verify this?
					dev.IsPrimary = i == 0;
					
					devices.Add(dev);
					// It seems that all X screens are equal to 0 is Xinerama is enabled, at least on Nvidia (verify?)
					dev.Metadata = (IntPtr)0; /*screen.ScreenNumber*/
				}
				return count > 0;
			}
			return false;
		}

		unsafe static bool QueryXRandR(List<DisplayDevice> devices) {
			// Get available resolutions. Then, for each resolution get all available rates.
			foreach (DisplayDevice dev in devices) {
				int screen = (int)dev.Metadata, count = 0;
				XRRScreenSize* sizes = API.XRRSizes(API.DefaultDisplay, screen, &count);
				if (count == 0)
					throw new NotSupportedException("XRandR extensions not available.");
				
				IntPtr screenConfig = API.XRRGetScreenInfo(API.DefaultDisplay, API.XRootWindow(API.DefaultDisplay, screen));
				
				ushort curRotation;
				int idx = API.XRRConfigCurrentConfiguration(screenConfig, out curRotation);
				int curRefreshRate = API.XRRConfigCurrentRate(screenConfig);
				int curDepth = API.XDefaultDepth(API.DefaultDisplay, screen);
				API.XRRFreeScreenConfigInfo(screenConfig);
				
				if (dev.Bounds == Rectangle.Empty)
					dev.Bounds = new Rectangle(0, 0, sizes[idx].Width, sizes[idx].Height);
				dev.BitsPerPixel = curDepth;
				dev.RefreshRate = curRefreshRate;
			}
			return true;
		}
	}
}
