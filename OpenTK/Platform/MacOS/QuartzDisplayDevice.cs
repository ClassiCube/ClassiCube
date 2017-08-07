using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Platform.MacOS.Carbon;

namespace OpenTK.Platform.MacOS
{
	static class QuartzDisplayDevice
	{
		static IntPtr mainDisplay;
		internal static IntPtr MainDisplay { get { return mainDisplay; } }

		internal unsafe static void Init() {
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
				IntPtr currentModePtr = CG.CGDisplayCurrentMode(curDisplay);
				CFDictionary currentMode = new CFDictionary(currentModePtr);

				for (int j = 0; j < displayModes.Count; j++) {
					CFDictionary dict = new CFDictionary(displayModes[j]);
					
					int width = (int) dict.GetNumberValue("Width");
					int height = (int) dict.GetNumberValue("Height");
					int bpp = (int) dict.GetNumberValue("BitsPerPixel");
					double freq = dict.GetNumberValue("RefreshRate");
					bool current = currentMode.DictRef == dict.DictRef;

					if (current) {
						opentk_dev_current_res = new DisplayResolution(width, height, bpp, (float)freq);
					}
				}

				HIRect bounds = CG.CGDisplayBounds(curDisplay);
				Rectangle newRect = new Rectangle(
					(int)bounds.Origin.X, (int)bounds.Origin.Y, (int)bounds.Size.X, (int)bounds.Size.Y);

				Debug.Print("Display {0} bounds: {1}", i, newRect);

				DisplayDevice opentk_dev = new DisplayDevice(opentk_dev_current_res, primary);
				opentk_dev.Bounds = newRect;
				opentk_dev.Metadata = curDisplay;
			}
		}

		internal static IntPtr HandleTo(DisplayDevice device) {
			if (device == null || device.Metadata == null) return IntPtr.Zero;
			return (IntPtr)device.Metadata;
		}
	}
}
