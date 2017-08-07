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
using System.Runtime.InteropServices;

namespace OpenTK.Platform.X11 {
	
	internal static class X11DisplayDevice {
		static bool xinerama_supported, xrandr_supported, xf86_supported;
		
		internal static void Init() {
			List<DisplayDevice> devices = new List<DisplayDevice>();
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
					dev.IsPrimary = i == API.XDefaultScreen(API.DefaultDisplay);
					dev.Metadata = i;
					devices.Add(dev);
				}
			}

			try {
				xrandr_supported = QueryXRandR(devices);
			} catch { }

			if (!xrandr_supported) {
				Debug.Print("XRandR query failed, falling back to XF86.");
				try {
					xf86_supported = QueryXF86(devices);
				}  catch { }

				if (!xf86_supported) {
					Debug.Print("XF86 query failed, no DisplayDevice support available.");
				}
			}
		}
		
		static bool QueryXinerama(List<DisplayDevice> devices) {
			// Try to use Xinerama to obtain the geometry of all output devices.
			int event_base, error_base;
			if (NativeMethods.XineramaQueryExtension(API.DefaultDisplay, out event_base, out error_base) &&
			    NativeMethods.XineramaIsActive(API.DefaultDisplay)) {
				XineramaScreenInfo[] screens = NativeMethods.XineramaQueryScreens(API.DefaultDisplay);
				bool first = true;
				foreach (XineramaScreenInfo screen in screens) {
					DisplayDevice dev = new DisplayDevice();
					dev.Bounds = new Rectangle(screen.X, screen.Y, screen.Width, screen.Height);
					if (first) {
						// We consider the first device returned by Xinerama as the primary one.
						// Makes sense conceptually, but is there a way to verify this?
						dev.IsPrimary = true;
						first = false;
					}
					devices.Add(dev);
					// It seems that all X screens are equal to 0 is Xinerama is enabled, at least on Nvidia (verify?)
					dev.Metadata = 0; /*screen.ScreenNumber*/
				}
			}
			return true;
		}

		static bool QueryXRandR(List<DisplayDevice> devices) {
			// Get available resolutions. Then, for each resolution get all available rates.
			foreach (DisplayDevice dev in devices) {
				int screen = (int)dev.Metadata;
				XRRScreenSize[] sizes = FindAvailableResolutions(screen);				
				IntPtr screenConfig = API.XRRGetScreenInfo(API.DefaultDisplay, API.XRootWindow(API.DefaultDisplay, screen));
				
				ushort curRotation;
				int curSizesIndex = API.XRRConfigCurrentConfiguration(screenConfig, out curRotation);
				float curRefreshRate = API.XRRConfigCurrentRate(screenConfig);
				int curDepth = API.XDefaultDepth(API.DefaultDisplay, screen);
				API.XRRFreeScreenConfigInfo(screenConfig);
				
				if (dev.Bounds == Rectangle.Empty)
					dev.Bounds = new Rectangle(0, 0, sizes[curSizesIndex].Width, sizes[curSizesIndex].Height);
				dev.BitsPerPixel = curDepth;
				dev.RefreshRate = curRefreshRate;
			}
			return true;
		}

		static bool QueryXF86(List<DisplayDevice> devices) {
			return false;
		}
		
		static XRRScreenSize[] FindAvailableResolutions(int screen) {
			XRRScreenSize[] resolutions = API.XRRSizes(API.DefaultDisplay, screen);
			if (resolutions == null)
				throw new NotSupportedException("XRandR extensions not available.");
			return resolutions;
		}
		
		static class NativeMethods {
			const string Xinerama = "libXinerama";

			[DllImport(Xinerama)]
			public static extern bool XineramaQueryExtension(IntPtr dpy, out int event_basep, out int error_basep);

			[DllImport(Xinerama)]
			public static extern bool XineramaIsActive(IntPtr dpy);

			[DllImport(Xinerama)]
			static extern IntPtr XineramaQueryScreens(IntPtr dpy, out int count);

			public unsafe static XineramaScreenInfo[] XineramaQueryScreens(IntPtr dpy) {
				int count;
				IntPtr screen_ptr = XineramaQueryScreens(dpy, out count);
				XineramaScreenInfo* ptr = (XineramaScreenInfo*)screen_ptr;
				
				XineramaScreenInfo[] screens = new XineramaScreenInfo[count];
				for( int i = 0; i < screens.Length; i++ ) {
					screens[i] = *ptr++;
				}
				return screens;
			}
		}

		[StructLayout(LayoutKind.Sequential, Pack = 1)]
		struct XineramaScreenInfo {
			public int ScreenNumber;
			public short X;
			public short Y;
			public short Width;
			public short Height;
		}
	}
}
