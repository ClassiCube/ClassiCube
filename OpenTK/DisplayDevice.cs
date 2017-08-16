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
	
	/// <summary> Contains information regarding a monitor's display resolution. </summary>
	public class DisplayResolution {		
		internal DisplayResolution() { }
		
		// Creates a new DisplayResolution object for the primary DisplayDevice.
		internal DisplayResolution(int width, int height, int bitsPerPixel, float refreshRate)  {
			Width = width; Height = height; 
			BitsPerPixel = bitsPerPixel; RefreshRate = refreshRate;
		}

		/// <summary> The width of this display in pixels. </summary>
		public int Width;

		/// <summary> The height of this display in pixels. </summary>
		public int Height;
		
		/// <summary> The number of bits per pixel of this display. Typical values include 8, 16, 24 and 32. </summary>
		public int BitsPerPixel;
		
		/// <summary> The vertical refresh rate of this display. </summary>
		public float RefreshRate;
	}
	
	/// <summary> Defines a display device on the underlying system. </summary>
	public class DisplayDevice {
		// TODO: Add support for refresh rate queries and switches.
		// TODO: Check whether bits_per_pixel works correctly under Mono/X11.
		// TODO: Add properties that describe the 'usable' size of the Display, i.e. the maximized size without the taskbar etc.
		// TODO: Does not detect changes to primary device.

		DisplayResolution curResolution = new DisplayResolution();
		Rectangle bounds;

		static DisplayDevice() {
			Platform.Factory.Default.InitDisplayDeviceDriver();
		}

		internal DisplayDevice() { }

		internal DisplayDevice(DisplayResolution curResolution, bool primary) {
			#warning "Consolidate current resolution with bounds? Can they fall out of sync right now?"
			this.curResolution = curResolution;
			IsPrimary = primary;
			this.bounds = new Rectangle(0, 0, curResolution.Width, curResolution.Height);
		}

		/// <summary> Returns bounds of this instance in pixel coordinates. </summary>
		public Rectangle Bounds {
			get { return bounds; }
			internal set {
				bounds = value;
				curResolution.Height = bounds.Height;
				curResolution.Width = bounds.Width;
			}
		}

		/// <summary> Returns width of this display in pixels. </summary>
		public int Width { get { return curResolution.Width; } }

		/// <summary> Returns height of this display in pixels. </summary>
		public int Height { get { return curResolution.Height; } }

		/// <summary> Returns number of bits per pixel of this display. Typical values include 8, 16, 24 and 32. </summary>
		public int BitsPerPixel {
			get { return curResolution.BitsPerPixel; }
			internal set { curResolution.BitsPerPixel = value; }
		}

		/// <summary> Returns vertical refresh rate of this display. </summary>
		public float RefreshRate {
			get { return curResolution.RefreshRate; }
			internal set { curResolution.RefreshRate = value; }
		}

		/// <summary> Returns whether this Display is the primary Display in systems with multiple Displays.</summary>
		public bool IsPrimary { set { if (value) Primary = this; } }
		
		/// <summary> Data unique to this Display. </summary>
		public object Metadata;

		/// <summary>Gets the default (primary) display of this system.</summary>
		public static DisplayDevice Primary;
		
		public static DisplayDevice Default { get { return Primary; } }
	}
}
