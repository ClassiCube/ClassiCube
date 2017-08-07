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
			// To minimize the need to add static methods to OpenTK.Graphics.DisplayDevice
			// we only allow settings to be set through its constructor.
			// Thus, we save all necessary parameters in temporary variables
			// and construct the device when every needed detail is available.
			// The main DisplayDevice constructor adds the newly constructed device
			// to the list of available devices.
			DisplayResolution currentRes = null;
			bool devPrimary = false;
			int deviceNum = 0;

			// Get available video adapters and enumerate all monitors
			WindowsDisplayDevice winDev = new WindowsDisplayDevice();
			while (API.EnumDisplayDevices(null, deviceNum++, winDev, 0)) {
				if ((winDev.StateFlags & DisplayDeviceStateFlags.AttachedToDesktop) == 0)
					continue;

				DeviceMode mode = new DeviceMode();

				// The second function should only be executed when the first one fails (e.g. when the monitor is disabled)
				if (API.EnumDisplaySettings(winDev.DeviceName, (int)DisplayModeSettings.Current, mode) ||
				    API.EnumDisplaySettings(winDev.DeviceName, (int)DisplayModeSettings.Registry, mode)) {
					if (mode.BitsPerPel > 0) {
						currentRes = new DisplayResolution(
							mode.PelsWidth, mode.PelsHeight,
							mode.BitsPerPel, mode.DisplayFrequency);
						devPrimary = (winDev.StateFlags & DisplayDeviceStateFlags.PrimaryDevice) != 0;
					}
				}
				
				// This device has no valid resolution, ignore it
				if (currentRes == null) continue;

				// Construct the OpenTK DisplayDevice through the accumulated parameters.
				// The constructor automatically adds the DisplayDevice to the list of available devices.
				DisplayDevice device = new DisplayDevice(currentRes, devPrimary);
				currentRes = null;
			}
		}
	}
}
