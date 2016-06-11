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
using OpenTK;

namespace SharpDX.Direct3D9 {

	[InteropPatch]
	public unsafe class Resource : ComObject {
		
		public Resource( IntPtr nativePtr ) : base( nativePtr ) {
		}

		public int SetPriority(int priorityNew) {
			return Interop.Calli(comPointer, priorityNew,(*(IntPtr**)comPointer)[7]);
		}
		
		public int GetPriority() {
			return Interop.Calli(comPointer,(*(IntPtr**)comPointer)[8]);
		}
		
		public void PreLoad() {
			Interop.Calli_V(comPointer,(*(IntPtr**)comPointer)[9]);
		}
	}
	
	[InteropPatch]
	public unsafe class DataBuffer : Resource { // Either 'VertexBuffer' or 'IndexBuffer
		
		public DataBuffer(IntPtr nativePtr) : base(nativePtr) {
		}
		
		public IntPtr Lock( int offsetToLock, int sizeToLock, LockFlags flags ) {
			IntPtr pOut;
			int res = Interop.Calli(comPointer, offsetToLock, sizeToLock, (IntPtr)(void*)&pOut, (int)flags, (*(IntPtr**)comPointer)[11]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return pOut;
		}
		
		public void SetData( IntPtr data, int bytes, LockFlags flags ) {
			IntPtr dst = Lock( 0, bytes, flags );
			MemUtils.memcpy( data, dst, bytes );
			Unlock();
		}
		
		public void SetData<T>( T[] data, int bytes, LockFlags flags ) where T : struct {
			IntPtr src = Interop.Fixed( ref data[0] );	
			IntPtr dst = Lock( 0, bytes, flags );
			MemUtils.memcpy( src, dst, bytes );
			Unlock();
		}
		
		public void Unlock() {
			int res = Interop.Calli(comPointer, (*(IntPtr**)comPointer)[12]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
	}
	
	public unsafe class DynamicDataBuffer : DataBuffer {
		
		public VertexFormat Format;
		public int MaxSize;
		
		public DynamicDataBuffer(IntPtr nativePtr) : base(nativePtr) {
		}
	}
	
	[InteropPatch]
	public unsafe class Surface : Resource {
		
		public Surface(IntPtr nativePtr) : base(nativePtr) {
		}
		
		public LockedRectangle LockRectangle(LockFlags flags) {
			LockedRectangle lockedRect = new LockedRectangle();
			int res = Interop.Calli(comPointer, (IntPtr)(void*)&lockedRect, IntPtr.Zero, (int)flags,(*(IntPtr**)comPointer)[13]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return lockedRect;
		}
		
		public void UnlockRectangle() {
			int res = Interop.Calli(comPointer,(*(IntPtr**)comPointer)[14]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
	}
	
	[InteropPatch]
	public unsafe class Texture : Resource {
		
		public Texture(IntPtr nativePtr) : base(nativePtr) {
		}

		public LockedRectangle LockRectangle(int level, LockFlags flags) {
			LockedRectangle lockedRect = new LockedRectangle();
			int res = Interop.Calli(comPointer, level, (IntPtr)(void*)&lockedRect, IntPtr.Zero, (int)flags,(*(IntPtr**)comPointer)[19]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return lockedRect;
		}
		
		public LockedRectangle LockRectangle(int level, D3DRect rect, LockFlags flags) {
			LockedRectangle lockedRect = new LockedRectangle();
			int res = Interop.Calli(comPointer, level, (IntPtr)(void*)&lockedRect, (IntPtr)(void*)&rect, (int)flags,(*(IntPtr**)comPointer)[19]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return lockedRect;
		}
		
		public void SetData( int level, LockFlags flags, IntPtr data, int bytes ) {
			LockedRectangle rect = LockRectangle( level, flags );
			MemUtils.memcpy( data, rect.DataPointer, bytes );
			UnlockRectangle( level );
		}
		
		public void SetPartData( int level, LockFlags flags, IntPtr data, int x, int y, int width, int height ) {
			D3DRect partRect;
			partRect.Left = x; partRect.Top = y;
			partRect.Right = x + width; partRect.Bottom = y + height;
			LockedRectangle rect = LockRectangle( level, partRect, flags );
			
			// We need to copy scanline by scanline, as generally rect.stride != data.stride
			byte* src = (byte*)data, dst = (byte*)rect.DataPointer;
			for( int yy = 0; yy < height; yy++ ) {
				MemUtils.memcpy( (IntPtr)src, (IntPtr)dst, width * 4 );
				src += width * 4;
				dst += rect.Pitch;			
			}
			UnlockRectangle( level );
		}
		
		public void UnlockRectangle(int level) {
			int res = Interop.Calli(comPointer, level,(*(IntPtr**)comPointer)[20]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
	}
}
