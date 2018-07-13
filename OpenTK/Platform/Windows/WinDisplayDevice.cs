#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK team.
 * This notice may not be removed.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Collections.Generic;

namespace OpenTK.Platform.Windows {
	
	internal static class WinDisplayDevice {

		/// <summary>Queries available display devices and display resolutions.</summary>
		internal static void Init() {
			// Get available video adapters and enumerate all monitors
			int displayNum = 0;
			WindowsDisplayDevice display = new WindowsDisplayDevice();
			
			while (API.EnumDisplayDevices(null, displayNum++, display, 0)) {
				if ((display.StateFlags & DisplayDeviceStateFlags.AttachedToDesktop) == 0) continue;
				DeviceMode mode = new DeviceMode();

				// The second function should only be executed when the first one fails (e.g. when the monitor is disabled)
				if (API.EnumDisplaySettings(display.DeviceName, (int)DisplayModeSettings.Current, mode) ||
				    API.EnumDisplaySettings(display.DeviceName, (int)DisplayModeSettings.Registry, mode)) {			
				} else {
					mode.BitsPerPel = 0;
				}
				
				if (mode.BitsPerPel == 0) continue;
				DisplayDevice device = new DisplayDevice();
				
				device.Bounds.Width = mode.PelsWidth;
				device.Bounds.Height = mode.PelsHeight;
				device.BitsPerPixel = mode.BitsPerPel;
				device.RefreshRate = mode.DisplayFrequency;
				device.IsPrimary = (display.StateFlags & DisplayDeviceStateFlags.PrimaryDevice) != 0;
			}
		}
	}
}
