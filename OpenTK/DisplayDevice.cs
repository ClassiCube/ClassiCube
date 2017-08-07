#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK team.
 * This notice may not be removed.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;
using System.Collections.Generic;
using System.Drawing;

namespace OpenTK {
	
	/// <summary> Defines a display device on the underlying system, and provides
	/// methods to query and change its display parameters. </summary>
	public class DisplayDevice {
		// TODO: Add support for refresh rate queries and switches.
		// TODO: Check whether bits_per_pixel works correctly under Mono/X11.
		// TODO: Add properties that describe the 'usable' size of the Display, i.e. the maximized size without the taskbar etc.
		// TODO: Does not detect changes to primary device.

		DisplayResolution current_resolution = new DisplayResolution();
		bool primary;
		Rectangle bounds;
		static DisplayDevice primary_display;

		static DisplayDevice() {
			Platform.Factory.Default.InitDisplayDeviceDriver();
		}

		internal DisplayDevice() { AvailableDisplays.Add(this); }

		internal DisplayDevice(DisplayResolution currentResolution, bool primary,
		                       IEnumerable<DisplayResolution> resolutions, Rectangle bounds) : this() {
			#warning "Consolidate current resolution with bounds? Can they fall out of sync right now?"
			this.current_resolution = currentResolution;
			IsPrimary = primary;
			AvailableResolutions.AddRange(resolutions);
			this.bounds = bounds == Rectangle.Empty ? currentResolution.Bounds : bounds;

			Debug.Print("DisplayDevice {0} ({1}) supports {2} resolutions.",
			            AvailableDisplays.Count, primary ? "primary" : "secondary", AvailableResolutions.Count);
		}

		/// <summary> Returns bounds of this instance in pixel coordinates. </summary>
		public Rectangle Bounds {
			get { return bounds; }
			internal set {
				bounds = value;
				current_resolution.Height = bounds.Height;
				current_resolution.Width = bounds.Width;
			}
		}

		/// <summary> Returns width of this display in pixels. </summary>
		public int Width { get { return current_resolution.Width; } }

		/// <summary> Returns height of this display in pixels. </summary>
		public int Height { get { return current_resolution.Height; } }

		/// <summary> Returns number of bits per pixel of this display. Typical values include 8, 16, 24 and 32. </summary>
		public int BitsPerPixel {
			get { return current_resolution.BitsPerPixel; }
			internal set { current_resolution.BitsPerPixel = value; }
		}

		/// <summary> Returns vertical refresh rate of this display. </summary>
		public float RefreshRate {
			get { return current_resolution.RefreshRate; }
			internal set { current_resolution.RefreshRate = value; }
		}

		/// <summary> Returns whether this Display is the primary Display in systems with multiple Displays.</summary>
		public bool IsPrimary {
			get { return primary; }
			internal set {
				if (value && primary_display != null && primary_display != this)
					primary_display.IsPrimary = false;

				primary = value;
				if (value)
					primary_display = this;
			}
		}
		
		/// <summary> Data unique to this Display. </summary>
		public object Metadata;

		/// <summary> The list of <see cref="DisplayResolution"/> objects available on this device. </summary>
		public List<DisplayResolution> AvailableResolutions = new List<DisplayResolution>();

		/// <summary> The list of available <see cref="DisplayDevice"/> objects. </summary>
		public static List<DisplayDevice> AvailableDisplays = new List<DisplayDevice>();

		/// <summary>Gets the default (primary) display of this system.</summary>
		public static DisplayDevice Default { get { return primary_display; } }

		/// <summary> Returns a System.String representing this DisplayDevice. </summary>
		/// <returns>A System.String representing this DisplayDevice.</returns>
		public override string ToString() {
			return String.Format("{0}: {1} ({2} modes available)", IsPrimary ? "Primary" : "Secondary",
			                     Bounds.ToString(), AvailableResolutions.Count);
		}
	}
}
