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

		DisplayResolution current_resolution = new DisplayResolution(), original_resolution;
		List<DisplayResolution> available_resolutions = new List<DisplayResolution>();
		IList<DisplayResolution> available_resolutions_readonly;
		bool primary;
		Rectangle bounds;

		static readonly List<DisplayDevice> available_displays = new List<DisplayDevice>();
		static readonly IList<DisplayDevice> available_displays_readonly;
		static DisplayDevice primary_display;
		static Platform.IDisplayDeviceDriver implementation;

		static DisplayDevice() {
			implementation = Platform.Factory.Default.CreateDisplayDeviceDriver();
			available_displays_readonly = available_displays.AsReadOnly();
		}

		internal DisplayDevice() {
			available_displays.Add(this);
			available_resolutions_readonly = available_resolutions.AsReadOnly();
		}

		internal DisplayDevice(DisplayResolution currentResolution, bool primary,
		                       IEnumerable<DisplayResolution> availableResolutions, Rectangle bounds) : this() {
			#warning "Consolidate current resolution with bounds? Can they fall out of sync right now?"
			this.current_resolution = currentResolution;
			IsPrimary = primary;
			this.available_resolutions.AddRange(availableResolutions);
			this.bounds = bounds == Rectangle.Empty ? currentResolution.Bounds : bounds;

			Debug.Print("DisplayDevice {0} ({1}) supports {2} resolutions.",
			            available_displays.Count, primary ? "primary" : "secondary", available_resolutions.Count);
		}

		/// <summary> Gets the bounds of this instance in pixel coordinates. </summary>
		public Rectangle Bounds {
			get { return bounds; }
			internal set {
				bounds = value;
				current_resolution.Height = bounds.Height;
				current_resolution.Width = bounds.Width;
			}
		}

		/// <summary>Gets a System.Int32 that contains the width of this display in pixels.</summary>
		public int Width { get { return current_resolution.Width; } }

		/// <summary>Gets a System.Int32 that contains the height of this display in pixels.</summary>
		public int Height { get { return current_resolution.Height; } }

		/// <summary>Gets a System.Int32 that contains number of bits per pixel of this display. Typical values include 8, 16, 24 and 32.</summary>
		public int BitsPerPixel {
			get { return current_resolution.BitsPerPixel; }
			internal set { current_resolution.BitsPerPixel = value; }
		}

		/// <summary> Gets a System.Single representing the vertical refresh rate of this display. </summary>
		public float RefreshRate {
			get { return current_resolution.RefreshRate; }
			internal set { current_resolution.RefreshRate = value; }
		}

		/// <summary>Gets a System.Boolean that indicates whether this Display is the primary Display in systems with multiple Displays.</summary>
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

		/// <summary> Selects an available resolution that matches the specified parameters. </summary>
		/// <param name="width">The width of the requested resolution in pixels.</param>
		/// <param name="height">The height of the requested resolution in pixels.</param>
		/// <param name="bitsPerPixel">The bits per pixel of the requested resolution.</param>
		/// <param name="refreshRate">The refresh rate of the requested resolution in hertz.</param>
		/// <returns>The requested DisplayResolution or null if the parameters cannot be met.</returns>
		/// <remarks> <para>If a matching resolution is not found, this function will retry ignoring the specified refresh rate,
		/// bits per pixel and resolution, in this order. If a matching resolution still doesn't exist, this function will
		/// return the current resolution.</para>
		/// <para>A parameter set to 0 or negative numbers will not be used in the search (e.g. if refreshRate is 0,
		/// any refresh rate will be considered valid).</para>
		/// <para>This function allocates memory.</para> </remarks>
		public DisplayResolution SelectResolution(int width, int height, int bitsPerPixel, float refreshRate) {
			DisplayResolution resolution = FindResolution(width, height, bitsPerPixel, refreshRate);
			if (resolution == null)
				resolution = FindResolution(width, height, bitsPerPixel, 0);
			if (resolution == null)
				resolution = FindResolution(width, height, 0, 0);
			if (resolution == null)
				return current_resolution;
			return resolution;
		}

		/// <summary> Gets the list of <see cref="DisplayResolution"/> objects available on this device. </summary>
		public IList<DisplayResolution> AvailableResolutions {
			get { return available_resolutions_readonly; }
			internal set
			{
				available_resolutions = (List<DisplayResolution>)value;
				available_resolutions_readonly = available_resolutions.AsReadOnly();
			}
		}

		/// <summary>Changes the resolution of the DisplayDevice.</summary>
		/// <param name="resolution">The resolution to set. <see cref="DisplayDevice.SelectResolution"/></param>
		/// <exception cref="Graphics.GraphicsModeException">Thrown if the requested resolution could not be set.</exception>
		/// <remarks>If the specified resolution is null, this function will restore the original DisplayResolution.</remarks>
		public void ChangeResolution(DisplayResolution resolution) {
			if (resolution == null)
				this.RestoreResolution();

			if (resolution == current_resolution)
				return;

			if (implementation.TryChangeResolution(this, resolution)) {
				if (original_resolution == null)
					original_resolution = current_resolution;
				current_resolution = resolution;
			} else
				throw new Graphics.GraphicsModeException(String.Format("Device {0}: Failed to change resolution to {1}.",
				                                                       this, resolution));
		}

		/// <summary>Changes the resolution of the DisplayDevice.</summary>
		/// <param name="width">The new width of the DisplayDevice.</param>
		/// <param name="height">The new height of the DisplayDevice.</param>
		/// <param name="bitsPerPixel">The new bits per pixel of the DisplayDevice.</param>
		/// <param name="refreshRate">The new refresh rate of the DisplayDevice.</param>
		/// <exception cref="Graphics.GraphicsModeException">Thrown if the requested resolution could not be set.</exception>
		public void ChangeResolution(int width, int height, int bitsPerPixel, float refreshRate) {
			this.ChangeResolution(this.SelectResolution(width, height, bitsPerPixel, refreshRate));
		}

		/// <summary>Restores the original resolution of the DisplayDevice.</summary>
		/// <exception cref="Graphics.GraphicsModeException">Thrown if the original resolution could not be restored.</exception>
		public void RestoreResolution() {
			if (original_resolution != null) {
				if (implementation.TryRestoreResolution(this)) {
					current_resolution = original_resolution;
					original_resolution = null;
				}  else
					throw new Graphics.GraphicsModeException(String.Format("Device {0}: Failed to restore resolution.", this));
			}
		}

		/// <summary> Gets the list of available <see cref="DisplayDevice"/> objects. </summary>
		public static IList<DisplayDevice> AvailableDisplays {
			get { return available_displays_readonly; }
		}

		/// <summary>Gets the default (primary) display of this system.</summary>
		public static DisplayDevice Default { get { return primary_display; } }

		DisplayResolution FindResolution(int width, int height, int bitsPerPixel, float refreshRate) {
			for( int i = 0; i < available_resolutions.Count; i++ ) {
				DisplayResolution res = available_resolutions[i];
				bool match = ((width > 0 && width == res.Width) || width == 0) &&
					((height > 0 && height == res.Height) || height == 0) &&
					((bitsPerPixel > 0 && bitsPerPixel == res.BitsPerPixel) || bitsPerPixel == 0) &&
					((refreshRate > 0 && System.Math.Abs(refreshRate - res.RefreshRate) < 1.0) || refreshRate == 0);
				
				if( match ) return res;
			}
			return null;
		}

		/// <summary> Returns a System.String representing this DisplayDevice. </summary>
		/// <returns>A System.String representing this DisplayDevice.</returns>
		public override string ToString() {
			return String.Format("{0}: {1} ({2} modes available)", IsPrimary ? "Primary" : "Secondary",
			                     Bounds.ToString(), available_resolutions.Count);
		}
	}
}
