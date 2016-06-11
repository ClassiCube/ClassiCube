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
using System.Runtime.InteropServices;
using OpenTK;

namespace SharpDX.Direct3D9 {
	
	[InteropPatch]
	public unsafe class Direct3D : ComObject {
		
		public Direct3D() {
			comPointer = Direct3DCreate9( SdkVersion );
			
			int count = GetAdapterCount();
			Adapters = new List<AdapterInformation>( count );
			for( int i = 0; i < count; i++ )
				Adapters.Add( new AdapterInformation( this, i ) );
		}

		public List<AdapterInformation> Adapters;
		
		const int SdkVersion = 32;
		[DllImport( "d3d9.dll" )]
		static extern IntPtr Direct3DCreate9( int sdkVersion );
		
		public int GetAdapterCount() {
			return Interop.Calli(comPointer,(*(IntPtr**)comPointer)[4]);
		}
		
		public AdapterDetails GetAdapterIdentifier( int adapter ) {			
			AdapterDetails.Native identifierNative = new AdapterDetails.Native();
			int res = Interop.Calli(comPointer, adapter, 0, (IntPtr)(void*)&identifierNative,(*(IntPtr**)comPointer)[5]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			
			AdapterDetails identifier = new AdapterDetails();
			identifier.MarshalFrom(ref identifierNative);			
			return identifier;
		}
		
		public int GetAdapterModeCount(int adapter, Format format) {
			return Interop.Calli(comPointer, adapter, (int)format,(*(IntPtr**)comPointer)[6]);
		}
		
		public DisplayMode EnumAdapterModes(int adapter, Format format, int mode) {
			DisplayMode modeRef = new DisplayMode();
			int res = Interop.Calli(comPointer, adapter, (int)format, mode, (IntPtr)(void*)&modeRef,(*(IntPtr**)comPointer)[7]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return modeRef;
		}
		
		public DisplayMode GetAdapterDisplayMode(int adapter) {
			DisplayMode modeRef = new DisplayMode();
			int res = Interop.Calli(comPointer, adapter, (IntPtr)(void*)&modeRef,(*(IntPtr**)comPointer)[8]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return modeRef;
		}
		
		public bool CheckDeviceType(int adapter, DeviceType devType, Format adapterFormat, Format backBufferFormat, bool bWindowed) {
			return Interop.Calli(comPointer, adapter, (int)devType, (int)adapterFormat, 
			                             (int)backBufferFormat, bWindowed ? 1 : 0,(*(IntPtr**)comPointer)[9]) == 0;
		}
		
		public bool CheckDepthStencilMatch(int adapter, DeviceType deviceType, Format adapterFormat, Format renderTargetFormat, Format depthStencilFormat) {
			return Interop.Calli(comPointer, adapter, (int)deviceType, (int)adapterFormat, 
			                             (int)renderTargetFormat, (int)depthStencilFormat,(*(IntPtr**)comPointer)[12]) == 0;
		}
		
		public Capabilities GetDeviceCaps(int adapter, DeviceType deviceType) {
			Capabilities capsRef = new Capabilities();
			int res = Interop.Calli(comPointer, adapter, (int)deviceType, (IntPtr)(void*)&capsRef,(*(IntPtr**)comPointer)[14]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return capsRef;
		}
		
		public IntPtr GetAdapterMonitor(int adapter) {
			return Interop.Calli_IntPtr(comPointer, adapter,(*(IntPtr**)comPointer)[15]);
		}
		
		public Device CreateDevice(int adapter, DeviceType deviceType, IntPtr hFocusWindow, CreateFlags behaviorFlags,  PresentParameters presentParams) {
			IntPtr devicePtr = IntPtr.Zero;
			int res = Interop.Calli(comPointer, adapter, (int)deviceType, hFocusWindow, (int)behaviorFlags, (IntPtr)(void*)&presentParams, 
			                           (IntPtr)(void*)&devicePtr,(*(IntPtr**)comPointer)[16]);
			
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new Device( devicePtr );
		}
	}
}
