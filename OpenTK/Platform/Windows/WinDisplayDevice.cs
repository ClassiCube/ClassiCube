#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK team.
 * This notice may not be removed.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;

namespace OpenTK.Platform.Windows {
	
	internal static class WinDisplayDevice {

		internal static void Init() {
			IntPtr dc = API.GetDC(IntPtr.Zero);
			const int SM_CXSCREEN = 0, SM_CYSCREEN = 1, BITSPIXEL = 12;
			DisplayDevice device = new DisplayDevice();
			
			device.Bounds.Width  = API.GetSystemMetrics(SM_CXSCREEN);
			device.Bounds.Height = API.GetSystemMetrics(SM_CYSCREEN);
			device.BitsPerPixel  = API.GetDeviceCaps(dc, BITSPIXEL);
			DisplayDevice.Default = device;
			
			API.ReleaseDC(IntPtr.Zero, dc);
		}
	}
}
