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
	
	internal class X11DisplayDevice : IDisplayDeviceDriver {
		// Store a mapping between resolutions and their respective
		// size_index (needed for XRRSetScreenConfig). The size_index
		// is simply the sequence number of the resolution as returned by
		// XRRSizes. This is done per available screen.
		static List<Dictionary<DisplayResolution, int>> screenResolutionToIndex =
			new List<Dictionary<DisplayResolution, int>>();
		// Store a mapping between DisplayDevices and their default resolutions.
		static Dictionary<DisplayDevice, int> deviceToDefaultResolution = new Dictionary<DisplayDevice, int>();
		// Store a mapping between DisplayDevices and X11 screens.
		static Dictionary<DisplayDevice, int> deviceToScreen = new Dictionary<DisplayDevice, int>();
		// Keep the time when the config of each screen was last updated.
		static List<IntPtr> lastConfigUpdate = new List<IntPtr>();

		static bool xinerama_supported, xrandr_supported, xf86_supported;
		
		static X11DisplayDevice() {
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
					devices.Add(dev);
					deviceToScreen.Add(dev, i);
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

		internal X11DisplayDevice() { }
		
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
					deviceToScreen.Add(dev, 0 /*screen.ScreenNumber*/);
				}
			}
			return true;
		}

		static bool QueryXRandR(List<DisplayDevice> devices) {
			// Get available resolutions. Then, for each resolution get all available rates.
			foreach (DisplayDevice dev in devices) {
				int screen = deviceToScreen[dev];

				IntPtr lastUpdateTimestamp;
				API.XRRTimes(API.DefaultDisplay, screen, out lastUpdateTimestamp);
				lastConfigUpdate.Add(lastUpdateTimestamp);
				List<DisplayResolution> available_res = new List<DisplayResolution>();

				// Add info for a new screen.
				screenResolutionToIndex.Add(new Dictionary<DisplayResolution, int>());
				int[] depths = API.XListDepths(API.DefaultDisplay, screen);

				int resolution_count = 0;
				foreach (XRRScreenSize size in FindAvailableResolutions(screen)) {
					if (size.Width == 0 || size.Height == 0) {
						Debug.Print("[Warning] XRandR returned an invalid resolution ({0}) for display device {1}", size, screen);
						continue;
					}
					short[] rates = API.XRRRates(API.DefaultDisplay, screen, resolution_count);

					// It seems that XRRRates returns 0 for modes that are larger than the screen
					// can support, as well as for all supported modes. On Ubuntu 7.10 the tool
					// "Screens and Graphics" does report these modes, though.
					foreach (short rate in rates) {
						// Note: some X servers (like Xming on Windows) do not report any rates other than 0.
						// If we only have 1 rate, add it even if it is 0.
						if (rate != 0 || rates.Length == 1)
							foreach (int depth in depths)
								available_res.Add(new DisplayResolution(0, 0, size.Width, size.Height, depth, rate));
					}
					// Keep the index of this resolution - we will need it for resolution changes later.
					foreach (int depth in depths) {
						// Note that Xinerama may return multiple devices for a single screen. XRandR will
						// not distinguish between the two as far as resolutions are supported (since XRandR
						// operates on X screens, not display devices) - we need to be careful not to add the
						// same resolution twice!
						DisplayResolution res = new DisplayResolution(0, 0, size.Width, size.Height, depth, 0);
						if (!screenResolutionToIndex[screen].ContainsKey(res))
							screenResolutionToIndex[screen].Add(res, resolution_count);
					}
					++resolution_count;
				}
				
				IntPtr screenConfig = API.XRRGetScreenInfo(API.DefaultDisplay, API.XRootWindow(API.DefaultDisplay, screen));			
				ushort curRotation;
				int curResolutionIndex = API.XRRConfigCurrentConfiguration(screenConfig, out curRotation);
				float curRefreshRate = API.XRRConfigCurrentRate(screenConfig);
				int curDepth = API.XDefaultDepth(API.DefaultDisplay, screen);
				API.XRRFreeScreenConfigInfo(screenConfig);
				
				if (dev.Bounds == Rectangle.Empty)
					dev.Bounds = new Rectangle(0, 0, available_res[curResolutionIndex].Width, available_res[curResolutionIndex].Height);
				dev.BitsPerPixel = curDepth;
				dev.RefreshRate = curRefreshRate;
				dev.AvailableResolutions = available_res;

				deviceToDefaultResolution.Add(dev, curResolutionIndex);
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
		
		static bool ChangeResolutionXRandR(DisplayDevice device, DisplayResolution resolution) {
			int screen = deviceToScreen[device];
			IntPtr root = API.XRootWindow(API.DefaultDisplay, screen);
			IntPtr screen_config = API.XRRGetScreenInfo(API.DefaultDisplay, root);

			ushort current_rotation;
			int current_resolution_index = API.XRRConfigCurrentConfiguration(screen_config, out current_rotation);
			int new_resolution_index;
			if (resolution != null)
				new_resolution_index = screenResolutionToIndex[screen]
					[new DisplayResolution(0, 0, resolution.Width, resolution.Height, resolution.BitsPerPixel, 0)];
			else
				new_resolution_index = deviceToDefaultResolution[device];

			Debug.Print("Changing size of screen {0} from {1} to {2}",
			            screen, current_resolution_index, new_resolution_index);

			return 0 == API.XRRSetScreenConfigAndRate(API.DefaultDisplay, screen_config, root, new_resolution_index,
			                                          current_rotation, (short)(resolution != null ? resolution.RefreshRate : 0), lastConfigUpdate[screen]);
		}

		static bool ChangeResolutionXF86(DisplayDevice device, DisplayResolution resolution) {
			return false;
		}
		
		public bool TryChangeResolution(DisplayDevice device, DisplayResolution resolution) {
			// If resolution is null, restore the default resolution (new_resolution_index = 0).
			if (xrandr_supported) {
				return ChangeResolutionXRandR(device, resolution);
			} else if (xf86_supported) {
				return ChangeResolutionXF86(device, resolution);
			}
			return false;
		}

		public bool TryRestoreResolution(DisplayDevice device) {
			return TryChangeResolution(device, null);
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
