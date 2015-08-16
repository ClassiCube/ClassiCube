// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

using System;
using System.Collections.Generic;

namespace SharpDX.Direct3D9 {

	public class AdapterInformation
	{
		readonly Direct3D d3d;

		internal AdapterInformation(Direct3D direct3D, int adapter) {
			d3d = direct3D;
			Adapter = adapter;
			Details = direct3D.GetAdapterIdentifier(adapter);
		}

		/// <summary> Gets the capabilities of this adapter. </summary>
		public Capabilities GetCaps(DeviceType type) {
			return d3d.GetDeviceCaps(Adapter, type);
		}

		/// <summary> Gets the display modes supported by this adapter. </summary>
		public List<DisplayMode> GetDisplayModes( Format format ) {
			int count = d3d.GetAdapterModeCount( Adapter, format );
			List<DisplayMode> items = new List<DisplayMode>( count );
			
			for( int i = 0; i < count; i++ )
				items.Add( d3d.EnumAdapterModes( Adapter, format, i ) );
			return items;
		}

		/// <summary> Gets the adapter ordinal. </summary>
		public int Adapter;

		/// <summary> Gets the current display mode. </summary>
		public DisplayMode CurrentDisplayMode {
			get { return d3d.GetAdapterDisplayMode(Adapter); }
		}

		/// <summary> Gets the details. </summary>
		public AdapterDetails Details;

		/// <summary> Gets the monitor. </summary>
		public IntPtr Monitor {
			get { return d3d.GetAdapterMonitor(Adapter); }
		}
	}
}