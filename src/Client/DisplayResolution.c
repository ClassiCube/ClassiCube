#region-- - License-- -
/* Licensed under the MIT/X11 license.
* Copyright (c) 2006-2008 the OpenTK team.
* This notice may not be removed.
* See license.txt for licensing detailed licensing details.
*/
#endregion

using System;

namespace OpenTK {

	/// <summary> Contains information regarding a monitor's display resolution. </summary>
	public class DisplayResolution {
		internal DisplayResolution() { }

		// Creates a new DisplayResolution object for the primary DisplayDevice.
		internal DisplayResolution(int width, int height, int bitsPerPixel, float refreshRate) {
			// Refresh rate may be zero, since this information may not be available on some platforms.
			if (width <= 0) throw new ArgumentOutOfRangeException("width", "Must be greater than zero.");
			if (height <= 0) throw new ArgumentOutOfRangeException("height", "Must be greater than zero.");
			if (bitsPerPixel <= 0) throw new ArgumentOutOfRangeException("bitsPerPixel", "Must be greater than zero.");
			if (refreshRate < 0) throw new ArgumentOutOfRangeException("refreshRate", "Must be greater than, or equal to zero.");

			Width = width; Height = height;
			BitsPerPixel = bitsPerPixel;
			RefreshRate = refreshRate;
		}

		/// <summary> Returns the width of this display in pixels. </summary>
		public int Width;

		/// <summary> Returns the height of this display in pixels. </summary>
		public int Height;

		/// <summary> Returns the number of bits per pixel of this display. Typical values include 8, 16, 24 and 32. </summary>
		public int BitsPerPixel;

		/// <summary> Returns the vertical refresh rate of this display. </summary>
		public float RefreshRate;
	}
}
