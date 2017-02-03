// Copyright (c) 2010-2013 SharpDX - Alexandre Mutel
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
	
    [StructLayout(LayoutKind.Sequential, Size = 4)]
    public struct RawBool {
        private int boolValue;

        public RawBool(bool boolValue)  {
            this.boolValue = boolValue ? 1 : 0;
        }

        public static implicit operator bool(RawBool booleanValue) {
            return booleanValue.boolValue != 0;
        }

        public static implicit operator RawBool(bool boolValue) {
            return new RawBool(boolValue);
        }
    }
    
    [StructLayout(LayoutKind.Sequential)]
	public struct LockedRectangle {
    	
    	/// <summary> Gets the number of bytes per row. </summary>
		public int Pitch;
		
		/// <summary> Pointer to the data. </summary>
		public IntPtr DataPointer;
	}
    
    [StructLayout(LayoutKind.Sequential)]
    public struct D3DRect {
    	public int Left;
    	public int Top;
    	public int Right;
    	public int Bottom;
    }
}
