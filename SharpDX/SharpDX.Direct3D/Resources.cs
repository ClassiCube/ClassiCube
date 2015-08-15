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
	public unsafe class IndexBuffer : Resource {
		
		public IndexBuffer(IntPtr nativePtr) : base(nativePtr) {
		}
		
		public IntPtr Lock( int offsetToLock, int sizeToLock, LockFlags flags ) {
			IntPtr pOut;
			Result res = Interop.Calli(comPointer, offsetToLock, sizeToLock, (IntPtr)(void*)&pOut, (int)flags, (*(IntPtr**)comPointer)[11]);
			res.CheckError();
			return pOut;
		}
		
		public void Unlock() {
			Result res = Interop.Calli(comPointer, (*(IntPtr**)comPointer)[12]);
			res.CheckError();
		}
	}
	
	[InteropPatch]
	public unsafe class Surface : Resource {
		
		public Surface(IntPtr nativePtr) : base(nativePtr) {
		}
		
		public LockedRectangle LockRectangle(LockFlags flags) {
			LockedRectangle lockedRect = new LockedRectangle();
			Result res = Interop.Calli(comPointer, (IntPtr)(void*)&lockedRect, IntPtr.Zero, (int)flags,(*(IntPtr**)comPointer)[13]);
			res.CheckError();
			return lockedRect;
		}
		
		public void UnlockRectangle() {
			Result res = Interop.Calli(comPointer,(*(IntPtr**)comPointer)[14]);
			res.CheckError();
		}
	}
	
	[InteropPatch]
	public unsafe class Texture : Resource {
		
		public Texture(IntPtr nativePtr) : base(nativePtr) {
		}

		public LockedRectangle LockRectangle(int level, LockFlags flags) {
			LockedRectangle lockedRect = new LockedRectangle();
			Result res = Interop.Calli(comPointer, level, (IntPtr)(void*)&lockedRect, IntPtr.Zero, (int)flags,(*(IntPtr**)comPointer)[19]);
			res.CheckError();
			return lockedRect;
		}
		
		public void UnlockRectangle(int level) {
			Result res = Interop.Calli(comPointer, level,(*(IntPtr**)comPointer)[20]);
			res.CheckError();
		}
	}
	
	[InteropPatch]
	public unsafe class VertexBuffer : Resource {
		
		public VertexBuffer(IntPtr nativePtr) : base(nativePtr) {
		}
		
		public IntPtr Lock( int offsetToLock, int sizeToLock, LockFlags flags ) {
			IntPtr pOut;
			Result res = Interop.Calli(comPointer, offsetToLock, sizeToLock, (IntPtr)(void*)&pOut, (int)flags, (*(IntPtr**)comPointer)[11]);
			res.CheckError();
			return pOut;
		}
		
		public void Unlock() {
			Result res = Interop.Calli(comPointer,(*(IntPtr**)comPointer)[12]);
			res.CheckError();
		}
	}
}
