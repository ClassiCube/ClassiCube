using System;
using System.Drawing;

namespace OpenTK.Platform.MacOS {
	static class QuartzDisplayDevice {

		internal unsafe static void Init() {
			IntPtr display = CG.CGMainDisplayID();
			DisplayDevice device = new DisplayDevice();
			HIRect bounds = CG.CGDisplayBounds(display);
			
			device.Bounds = new Rectangle(
				(int)bounds.Origin.X, (int)bounds.Origin.Y, (int)bounds.Size.X, (int)bounds.Size.Y);
			device.BitsPerPixel = CG.CGDisplayBitsPerPixel(display);			
			DisplayDevice.Default = device;
		}
	}
}
