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

namespace SharpDX
{
	public class ComObject : IDisposable
	{
		public IntPtr comPointer;
		
		public ComObject(IntPtr pointer) {
			comPointer = pointer;
		}

		protected ComObject() {
		}

		public bool IsDisposed;

		public void Dispose() {
			CheckAndDispose( true );
		}
		
		~ComObject() {
			CheckAndDispose( false );
		}

		unsafe void CheckAndDispose( bool disposing ) {
			if( IsDisposed ) return;
			
			if( comPointer != IntPtr.Zero ) {
				if( !disposing ) {
					string text = String.Format( "Warning: Live ComObject [0x{0:X}], potential memory leak: {1}", 
					                            comPointer.ToInt64(), GetType().Name );
					Console.WriteLine( text );
				}
				
				int refCount = Marshal.Release( comPointer );
				if( refCount > 0 ) {
					string text = String.Format( "Warning: ComObject [0x{0:X}] still has some references, potential memory leak: {1} ({2})", 
					                            comPointer.ToInt64(), GetType().Name, refCount );
				}
				comPointer = IntPtr.Zero;
			}
			GC.SuppressFinalize( this );
			IsDisposed = true;
		}
	}
}