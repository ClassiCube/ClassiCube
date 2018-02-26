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
	
	[StructLayout(LayoutKind.Sequential)]
	public struct PresentParameters {		
		public int BackBufferWidth;		
		public int BackBufferHeight;		
		public Format BackBufferFormat;		
		public int BackBufferCount;		
		public int MultiSampleType;		
		public int MultiSampleQuality;		
		public SwapEffect SwapEffect;		
		public IntPtr DeviceWindowHandle;		
		public RawBool Windowed;		
		public RawBool EnableAutoDepthStencil;
		public Format AutoDepthStencilFormat;		
		public PresentFlags PresentFlags;		
		public int FullScreenRefreshRateInHz;		
		public PresentInterval PresentationInterval;

		public void InitDefaults() {
			AutoDepthStencilFormat = Format.D24X8;
			BackBufferWidth = 800;
			BackBufferHeight = 600;
			BackBufferFormat = Format.X8R8G8B8;
			BackBufferCount = 1;
			DeviceWindowHandle = IntPtr.Zero;
			EnableAutoDepthStencil = true;
			PresentFlags = PresentFlags.None;
			PresentationInterval = PresentInterval.Immediate;
			SwapEffect = SwapEffect.Discard;
			Windowed = true;
		}
	}
}
