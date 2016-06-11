using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Platform.MacOS.Carbon;

namespace OpenTK.Platform.MacOS
{
	class QuartzDisplayDeviceDriver : IDisplayDeviceDriver
	{
		static Dictionary<DisplayDevice, IntPtr> displayMap =
			new Dictionary<DisplayDevice, IntPtr>();

		static IntPtr mainDisplay;
		internal static IntPtr MainDisplay { get { return mainDisplay; } }

		unsafe static QuartzDisplayDeviceDriver() {
			// To minimize the need to add static methods to OpenTK.Graphics.DisplayDevice
			// we only allow settings to be set through its constructor.
			// Thus, we save all necessary parameters in temporary variables
			// and construct the device when every needed detail is available.
			// The main DisplayDevice constructor adds the newly constructed device
			// to the list of available devices.
			const int maxDisplayCount = 20;
			IntPtr[] displays = new IntPtr[maxDisplayCount];
			int displayCount;
			fixed(IntPtr* displayPtr = displays) {
				CG.CGGetActiveDisplayList(maxDisplayCount, displayPtr, out displayCount);
			}

			Debug.Print("CoreGraphics reported {0} display(s).", displayCount);

			for (int i = 0; i < displayCount; i++) {
				IntPtr curDisplay = displays[i];

				// according to docs, first element in the array is always the main display.
				bool primary = (i == 0);
				if (primary)
					mainDisplay = curDisplay;

				// gets current settings
				int currentWidth = CG.CGDisplayPixelsWide(curDisplay);
				int currentHeight = CG.CGDisplayPixelsHigh(curDisplay);
				Debug.Print("Display {0} is at  {1}x{2}", i, currentWidth, currentHeight);

				IntPtr displayModesPtr = CG.CGDisplayAvailableModes(curDisplay);
				CFArray displayModes = new CFArray(displayModesPtr);
				Debug.Print("Supports {0} display modes.", displayModes.Count);

				DisplayResolution opentk_dev_current_res = null;
				List<DisplayResolution> opentk_dev_available_res = new List<DisplayResolution>();
				IntPtr currentModePtr = CG.CGDisplayCurrentMode(curDisplay);
				CFDictionary currentMode = new CFDictionary(currentModePtr);

				for (int j = 0; j < displayModes.Count; j++)
				{
					CFDictionary dict = new CFDictionary(displayModes[j]);
					
					int width = (int) dict.GetNumberValue("Width");
					int height = (int) dict.GetNumberValue("Height");
					int bpp = (int) dict.GetNumberValue("BitsPerPixel");
					double freq = dict.GetNumberValue("RefreshRate");
					bool current = currentMode.DictRef == dict.DictRef;

					//if (current) Debug.Write("  * ");
					//else Debug.Write("    ");

					//Debug.Print("Mode {0} is {1}x{2}x{3} @ {4}.", j, width, height, bpp, freq);

					DisplayResolution thisRes = new DisplayResolution(0, 0, width, height, bpp, (float)freq);
					opentk_dev_available_res.Add(thisRes);

					if (current)
						opentk_dev_current_res = thisRes;
				}

				HIRect bounds = CG.CGDisplayBounds(curDisplay);
				Rectangle newRect = new Rectangle(
					(int)bounds.Origin.X, (int)bounds.Origin.Y, (int)bounds.Size.X, (int)bounds.Size.Y);

				Debug.Print("Display {0} bounds: {1}", i, newRect);

				DisplayDevice opentk_dev =
					new DisplayDevice(opentk_dev_current_res, primary, opentk_dev_available_res, newRect);

				displayMap.Add(opentk_dev, curDisplay);
			}
		}

		internal static IntPtr HandleTo(DisplayDevice displayDevice) {
			if (displayMap.ContainsKey(displayDevice))
				return displayMap[displayDevice];
			else
				return IntPtr.Zero;
		}

		#region IDisplayDeviceDriver Members

		Dictionary<IntPtr, IntPtr> storedModes = new Dictionary<IntPtr, IntPtr>();
		List<IntPtr> displaysCaptured = new List<IntPtr>();
		
		public bool TryChangeResolution(DisplayDevice device, DisplayResolution resolution)
		{
			IntPtr display = displayMap[device];
			IntPtr currentModePtr = CG.CGDisplayCurrentMode(display);

			if (!storedModes.ContainsKey(display)) {
				storedModes.Add(display, currentModePtr);
			}

			IntPtr displayModesPtr = CG.CGDisplayAvailableModes(display);
			CFArray displayModes = new CFArray(displayModesPtr);

			for (int j = 0; j < displayModes.Count; j++) {
				CFDictionary dict = new CFDictionary(displayModes[j]);

				int width = (int)dict.GetNumberValue("Width");
				int height = (int)dict.GetNumberValue("Height");
				int bpp = (int)dict.GetNumberValue("BitsPerPixel");
				double freq = dict.GetNumberValue("RefreshRate");

				if (width == resolution.Width && height == resolution.Height &&
				    bpp == resolution.BitsPerPixel && Math.Abs(freq - resolution.RefreshRate) < 1e-6) {
					if (!displaysCaptured.Contains(display)) {
						CG.CGDisplayCapture(display);
					}

					Debug.Print("Changing resolution to {0}x{1}x{2}@{3}.", width, height, bpp, freq);
					CG.CGDisplaySwitchToMode(display, displayModes[j]);
					return true;
				}

			}
			return false;
		}

		public bool TryRestoreResolution(DisplayDevice device) {
			IntPtr display = displayMap[device];

			if (storedModes.ContainsKey(display)) {
				Debug.Print("Restoring resolution.");

				CG.CGDisplaySwitchToMode(display, storedModes[display]);
				CG.CGDisplayRelease(display);
				displaysCaptured.Remove(display);

				return true;
			}

			return false;
		}

		#endregion

	}
}
