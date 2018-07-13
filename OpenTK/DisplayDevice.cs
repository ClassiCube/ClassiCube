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

		internal DisplayDevice() { }

		/// <summary> Bounds of this Display in pixel coordinates. </summary>
		public Rectangle Bounds;

		/// <summary> Width of this display in pixels. </summary>
		public int Width { get { return Bounds.Width; } }

		/// <summary> Height of this display in pixels. </summary>
		public int Height { get { return Bounds.Height; } }

		/// <summary> Number of bits per pixel of this display. Typical values include 8, 16, 24 and 32. </summary>
		public int BitsPerPixel;

		/// <summary> Vertical refresh rate of this display. </summary>
		public int RefreshRate;

		/// <summary> Whether this Display is the primary Display in systems with multiple Displays.</summary>
		public bool IsPrimary { set { if (value) Primary = this; } }
		
		/// <summary> Data unique to this Display. </summary>
		public IntPtr Metadata;

		/// <summary>Gets the default (primary) display of this system.</summary>
		public static DisplayDevice Primary;
		
		public static DisplayDevice Default { get { return Primary; } }
	}
}
