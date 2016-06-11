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
	
	internal class WinDisplayDeviceDriver : IDisplayDeviceDriver {
		
		static Dictionary<DisplayDevice, string> available_device_names =
			new Dictionary<DisplayDevice, string>(); // Needed for ChangeDisplaySettingsEx

		/// <summary>Queries available display devices and display resolutions.</summary>
		static WinDisplayDeviceDriver() {
			// To minimize the need to add static methods to OpenTK.Graphics.DisplayDevice
			// we only allow settings to be set through its constructor.
			// Thus, we save all necessary parameters in temporary variables
			// and construct the device when every needed detail is available.
			// The main DisplayDevice constructor adds the newly constructed device
			// to the list of available devices.
			DisplayResolution currentRes = null;
			List<DisplayResolution> availableRes = new List<DisplayResolution>();
			bool devPrimary = false;
			int deviceNum = 0;

			// Get available video adapters and enumerate all monitors
			WindowsDisplayDevice winDev = new WindowsDisplayDevice();
			while (API.EnumDisplayDevices(null, deviceNum++, winDev, 0)) {
				if ((winDev.StateFlags & DisplayDeviceStateFlags.AttachedToDesktop) == 0)
					continue;

				DeviceMode monitor_mode = new DeviceMode();

				// The second function should only be executed when the first one fails (e.g. when the monitor is disabled)
				if (API.EnumDisplaySettings(winDev.DeviceName, (int)DisplayModeSettings.Current, monitor_mode) ||
				    API.EnumDisplaySettings(winDev.DeviceName, (int)DisplayModeSettings.Registry, monitor_mode)) {
					currentRes = new DisplayResolution(
						monitor_mode.Position.X, monitor_mode.Position.Y,
						monitor_mode.PelsWidth, monitor_mode.PelsHeight,
						monitor_mode.BitsPerPel, monitor_mode.DisplayFrequency);
					devPrimary = (winDev.StateFlags & DisplayDeviceStateFlags.PrimaryDevice) != 0;
				}

				availableRes.Clear();
				int i = 0;
				while (API.EnumDisplaySettings(winDev.DeviceName, i++, monitor_mode)) {
					availableRes.Add(new DisplayResolution(
						monitor_mode.Position.X, monitor_mode.Position.Y,
						monitor_mode.PelsWidth, monitor_mode.PelsHeight,
						monitor_mode.BitsPerPel, monitor_mode.DisplayFrequency));
				}

				// Construct the OpenTK DisplayDevice through the accumulated parameters.
				// The constructor automatically adds the DisplayDevice to the list of available devices.
				DisplayDevice device = new DisplayDevice(currentRes, devPrimary, availableRes, currentRes.Bounds);
				available_device_names.Add(device, winDev.DeviceName);
			}
		}

		public WinDisplayDeviceDriver() {
		}

		public bool TryChangeResolution(DisplayDevice device, DisplayResolution resolution) {
			DeviceMode mode = null;

			if (resolution != null)  {
				mode = new DeviceMode();
				mode.PelsWidth = resolution.Width;
				mode.PelsHeight = resolution.Height;
				mode.BitsPerPel = resolution.BitsPerPixel;
				mode.DisplayFrequency = (int)resolution.RefreshRate;
				mode.Fields = Constants.DM_BITSPERPEL | Constants.DM_PELSWIDTH
					| Constants.DM_PELSHEIGHT | Constants.DM_DISPLAYFREQUENCY;
			}

			return Constants.DISP_CHANGE_SUCCESSFUL ==
				API.ChangeDisplaySettingsEx(available_device_names[device], mode, IntPtr.Zero,
				                                  ChangeDisplaySettingsEnum.Fullscreen, IntPtr.Zero);
		}

		public bool TryRestoreResolution(DisplayDevice device) {
			return TryChangeResolution(device, null);
		}
	}
}
