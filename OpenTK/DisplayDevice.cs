#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK team.
 * This notice may not be removed.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Drawing;

namespace OpenTK {
	
	/// <summary> Defines a display device on the underlying system. </summary>
	public class DisplayDevice {
		// TODO: Add support for refresh rate queries and switches.
		// TODO: Check whether bits_per_pixel works correctly under Mono/X11.
		// TODO: Add properties that describe the 'usable' size of the Display, i.e. the maximized size without the taskbar etc.
		// TODO: Does not detect changes to primary device.

		static DisplayDevice() {
			Platform.Factory.Default.InitDisplayDeviceDriver();
		}

		public Rectangle Bounds;
		public int BitsPerPixel;
		public static DisplayDevice Default;
	}
}
