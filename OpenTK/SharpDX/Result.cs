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

namespace SharpDX {
	
	public enum Direct3DError : uint {
		Ok = 0,		
		NotFound = 2150 | 0x88760000u,
		MoreData = 2151 | 0x88760000u,
		DeviceLost = 2152 | 0x88760000u,
		DeviceNotReset = 2153 | 0x88760000u,
		NotAvailable = 2154 | 0x88760000u,
		OutOfVideoMemory = 380 | 0x88760000u,
		InvalidDevice = 2155 | 0x88760000u,
		InvalidCall = 2156 | 0x88760000u,
		DriverInvalidCall = 2157 | 0x88760000u,
		WasStillDrawing = 540 | 0x88760000u,
	}
	
	public class SharpDXException : Exception {
		
        public int Code;

        public SharpDXException(int result) : base(Format(result)) {
            HResult = result;
            Code = result;
        }
        
        static string Format( int code ) {
        	Direct3DError err = (Direct3DError)code;
        	return String.Format( "HRESULT: [0x{0:X}], D3D error type: {1}", code, err );
		}
    }
}