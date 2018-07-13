using System;
using System.Collections.Generic;
using System.Drawing;

namespace OpenTK.Platform.MacOS {
	static class QuartzDisplayDevice {
		internal static IntPtr MainDisplay;

		internal unsafe static void Init() {
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

				DisplayDevice device = new DisplayDevice();
				IntPtr curMode = CG.CGDisplayCurrentMode(curDisplay);
				
				for (int j = 0; j < modesCount; j++) {
					IntPtr mode = CF.CFArrayGetValueAtIndex(modes, j);
					if (mode != curMode) continue;
					
					device.Bounds.Width  = (int)CF.DictGetNumber(mode, "Width");
					device.Bounds.Height = (int)CF.DictGetNumber(mode, "Height");
					device.BitsPerPixel  = (int)CF.DictGetNumber(mode, "BitsPerPixel");
					device.RefreshRate   = (int)CF.DictGetNumber(mode, "RefreshRate");
				}

				HIRect bounds = CG.CGDisplayBounds(curDisplay);
				device.Bounds = new Rectangle(
					(int)bounds.Origin.X, (int)bounds.Origin.Y, (int)bounds.Size.X, (int)bounds.Size.Y);
				Debug.Print("Display {0} bounds: {1}", i, device.Bounds);
				
				device.Metadata = curDisplay;
				device.IsPrimary = primary;
			}
		}
	}
}
