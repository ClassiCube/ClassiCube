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
using System.Runtime.InteropServices;

namespace SharpDX.Direct3D9 {	
	public class AdapterInformation {
		readonly Direct3D d3d;

		internal AdapterInformation(Direct3D direct3D, int adapter) {
			d3d = direct3D;
			Adapter = adapter;
			Details = direct3D.GetAdapterIdentifier(adapter);
		}

		/// <summary> Gets the adapter ordinal. </summary>
		public int Adapter;

		/// <summary> Gets the details. </summary>
		public AdapterDetails Details;
	}
	
	public unsafe class AdapterDetails {
		public Version DriverVersion {
			get {
				return new Version((int)(RawDriverVersion >> 48) & 0xFFFF, (int)(RawDriverVersion >> 32) & 0xFFFF, 
				                   (int)(RawDriverVersion >> 16) & 0xFFFF, (int)(RawDriverVersion >> 0) & 0xFFFF);
			}
		}
				
		public string Driver;		
		public string Description;		
		public string DeviceName;		
		internal long RawDriverVersion;
		
		public int VendorId;	
		public int DeviceId;	
		public int SubsystemId;	
		public int Revision;
		
		public Guid DeviceIdentifier;		
		public int WhqlLevel;

		[StructLayout(LayoutKind.Sequential)]
		internal struct Native {
			public fixed byte Driver[512];
			public fixed byte Description[512];
			public fixed byte DeviceName[32];
			public long RawDriverVersion;
			public int VendorId;
			public int DeviceId;
			public int SubsystemId;
			public int Revision;
			public Guid DeviceIdentifier;
			public int WhqlLevel;
		}
		
		internal void MarshalFrom(ref Native native) {
			fixed (void* __ptr = native.Driver)
				Driver = Marshal.PtrToStringAnsi((IntPtr)__ptr);
			fixed (void* __ptr = native.Description)
				Description = Marshal.PtrToStringAnsi((IntPtr)__ptr);
			fixed (void* __ptr = native.DeviceName)
				DeviceName = Marshal.PtrToStringAnsi((IntPtr)__ptr);
			
			RawDriverVersion = native.RawDriverVersion;
			VendorId = native.VendorId;
			DeviceId = native.DeviceId;
			SubsystemId = native.SubsystemId;
			Revision = native.Revision;
			DeviceIdentifier = native.DeviceIdentifier;
			WhqlLevel = native.WhqlLevel;
		}
	}
}