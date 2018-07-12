using System;
using System.Collections.Generic;
using System.Drawing;
using OpenTK.Platform.MacOS.Carbon;

namespace OpenTK.Platform.MacOS {
	static class QuartzDisplayDevice {
		internal static IntPtr MainDisplay;

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
			fixed (IntPtr* displayPtr = displays) {
				CG.CGGetActiveDisplayList(maxDisplayCount, displayPtr, out displayCount);
			}

			Debug.Print("CoreGraphics reported {0} display(s).", displayCount);

			for (int i = 0; i < displayCount; i++) {
				IntPtr curDisplay = displays[i];

				// according to docs, first element in the array is always the main display.
				bool primary = (i == 0);
				if (primary) MainDisplay = curDisplay;

				// gets current settings
				int curWidth = CG.CGDisplayPixelsWide(curDisplay);
				int curHeight = CG.CGDisplayPixelsHigh(curDisplay);
				Debug.Print("Display {0} is at  {1}x{2}", i, curWidth, curHeight);
				
				IntPtr modes = CG.CGDisplayAvailableModes(curDisplay);
				int modesCount = CF.CFArrayGetCount(modes);
				Debug.Print("Supports {0} display modes.", modesCount);

				DisplayResolution opentk_curRes = null;
				IntPtr curMode = CG.CGDisplayCurrentMode(curDisplay);
				for (int j = 0; j < modesCount; j++) {
					IntPtr mode = CF.CFArrayGetValueAtIndex(modes, j);

					int width  = (int)CF.DictGetNumber(mode, "Width");
					int height = (int)CF.DictGetNumber(mode, "Height");
					int bpp  = (int)CF.DictGetNumber(mode, "BitsPerPixel");
					int freq = (int)CF.DictGetNumber(mode, "RefreshRate");

					if (mode == curMode) {
						opentk_curRes = new DisplayResolution(width, height, bpp, freq);
					}
				}

				HIRect bounds = CG.CGDisplayBounds(curDisplay);
				Rectangle newRect = new Rectangle(
					(int)bounds.Origin.X, (int)bounds.Origin.Y, (int)bounds.Size.X, (int)bounds.Size.Y);

				Debug.Print("Display {0} bounds: {1}", i, newRect);

				DisplayDevice opentk_dev = new DisplayDevice(opentk_curRes, primary);
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
