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

namespace SharpDX.Direct3D9
{
	[InteropPatch]
	public unsafe class Device : ComObject
	{
		public Device(IntPtr nativePtr) : base(nativePtr) {
		}

		public int TestCooperativeLevel() {
			return Interop.Calli(comPointer,(*(IntPtr**)comPointer)[3]);
		}
		
		public uint AvailableTextureMemory {
			get { return (uint)Interop.Calli(comPointer,(*(IntPtr**)comPointer)[4]); }
		}
		
		public void EvictManagedResources() {
			int res = Interop.Calli(comPointer,(*(IntPtr**)comPointer)[5]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public Capabilities Capabilities {
			get {
				Capabilities caps = new Capabilities();
				int res = Interop.Calli(comPointer, (IntPtr)(void*)&caps,(*(IntPtr**)comPointer)[7]);
				if( res < 0 ) { throw new SharpDXException( res ); }
				return caps;
			}
		}
		
		public DisplayMode GetDisplayMode(int iSwapChain) {
			DisplayMode modeRef = new DisplayMode();
			int res = Interop.Calli(comPointer, iSwapChain, (IntPtr)(void*)&modeRef,(*(IntPtr**)comPointer)[8]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return modeRef;
		}
		
		public int Reset( PresentParameters presentParams ) {
			return Interop.Calli(comPointer, (IntPtr)(void*)&presentParams,(*(IntPtr**)comPointer)[16]);
		}
		
		public int Present() {
			return Interop.Calli(comPointer, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero,(*(IntPtr**)comPointer)[17]);
		}
		
		public Surface GetBackBuffer(int iSwapChain, int iBackBuffer, BackBufferType type) {
			IntPtr backBufferOut = IntPtr.Zero;
			int res = Interop.Calli(comPointer, iSwapChain, iBackBuffer, (int)type, (IntPtr)(void*)&backBufferOut,(*(IntPtr**)comPointer)[18]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return ( backBufferOut == IntPtr.Zero ) ? null : new Surface( backBufferOut );
		}
		
		public Texture CreateTexture(int width, int height, int levels, Usage usage, Format format, Pool pool) {
			IntPtr pOut = IntPtr.Zero;
			int res = Interop.Calli(comPointer, width, height, levels, (int)usage, (int)format, (int)pool, (IntPtr)(void*)&pOut, IntPtr.Zero,(*(IntPtr**)comPointer)[23]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new Texture( pOut );
		}
		
		// Really the same as CreateVertexBuffer - but we need a separate return type so we make a new method.
		public DynamicDataBuffer CreateDynamicVertexBuffer(int length, VertexFormat vertexFormat) {
			IntPtr pOut = IntPtr.Zero;
			int res = Interop.Calli(comPointer, length, (int)Usage.Dynamic, (int)vertexFormat, (int)Pool.Default, (IntPtr)(void*)&pOut, IntPtr.Zero,(*(IntPtr**)comPointer)[26]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new DynamicDataBuffer( pOut );
		}
		
		public DataBuffer CreateVertexBuffer(int length, Usage usage, VertexFormat vertexFormat, Pool pool) {
			IntPtr pOut = IntPtr.Zero;
			int res = Interop.Calli(comPointer, length, (int)usage, (int)vertexFormat, (int)pool, (IntPtr)(void*)&pOut, IntPtr.Zero,(*(IntPtr**)comPointer)[26]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new DataBuffer( pOut );
		}
		
		public DataBuffer CreateIndexBuffer(int length, Usage usage, Format format, Pool pool) {
			IntPtr pOut = IntPtr.Zero;
			int res = Interop.Calli(comPointer, length, (int)usage, (int)format, (int)pool, (IntPtr)(void*)&pOut, IntPtr.Zero,(*(IntPtr**)comPointer)[27]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new DataBuffer( pOut );
		}
		
		public void GetRenderTargetData(Surface renderTarget, Surface destSurface) {
			int res = Interop.Calli(comPointer, renderTarget.comPointer, destSurface.comPointer,(*(IntPtr**)comPointer)[32]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public Surface CreateOffscreenPlainSurface(int width, int height, Format format, Pool pool) {
			IntPtr pOut = IntPtr.Zero;
			int res = Interop.Calli(comPointer, width, height, (int)format, (int)pool, (IntPtr)(void*)&pOut, IntPtr.Zero,(*(IntPtr**)comPointer)[36]);
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new Surface(pOut);
		}

		public void BeginScene() {
			int res = Interop.Calli(comPointer,(*(IntPtr**)comPointer)[41]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}

		public void EndScene() {
			int res = Interop.Calli(comPointer,(*(IntPtr**)comPointer)[42]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void Clear(ClearFlags flags, int colorBGRA, float z, int stencil) {
			int res = Interop.Calli(comPointer, 0, IntPtr.Zero, (int)flags, colorBGRA, z, stencil, (*(IntPtr**)comPointer)[43]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}

		public void SetTransform(TransformState state, ref Matrix4 matrixRef) {
			int res;
			fixed (void* matrixRef_ = &matrixRef)
				res = Interop.Calli(comPointer, (int)state, (IntPtr)matrixRef_,(*(IntPtr**)comPointer)[44]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void SetRenderState(RenderState renderState, bool enable) {
			SetRenderState(renderState, enable ? 1 : 0);
		}

		public void SetRenderState(RenderState renderState, float value) {
			SetRenderState(renderState, *(int*)&value);
		}

		public void SetRenderState(RenderState state, int value) {
			int res = Interop.Calli(comPointer, (int)state, value,(*(IntPtr**)comPointer)[57]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void SetTexture(int stage, Texture texture) {
			int res = Interop.Calli(comPointer, stage, (texture == null)?IntPtr.Zero:texture.comPointer,(*(IntPtr**)comPointer)[65]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void SetTextureStageState(int stage, TextureStage type, int value) {
			int res = Interop.Calli(comPointer, stage, (int)type, value,(*(IntPtr**)comPointer)[67]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void DrawPrimitives(PrimitiveType type, int startVertex, int primitiveCount) {
			int res = Interop.Calli(comPointer, (int)type, startVertex, primitiveCount,(*(IntPtr**)comPointer)[81]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void DrawIndexedPrimitives(PrimitiveType type, int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int primCount) {
			int res = Interop.Calli(comPointer, (int)type, baseVertexIndex, minVertexIndex, numVertices, startIndex, primCount,(*(IntPtr**)comPointer)[82]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}

		public void SetVertexFormat(VertexFormat vertexFormat) {
			int res = Interop.Calli(comPointer, (int)vertexFormat,(*(IntPtr**)comPointer)[89]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void SetStreamSource(int streamNumber, DataBuffer streamData, int offsetInBytes, int stride) {
			int res = Interop.Calli(comPointer, streamNumber,(streamData == null)?IntPtr.Zero:streamData.comPointer,offsetInBytes, stride,(*(IntPtr**)comPointer)[100]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void SetIndices(DataBuffer indexData) {
			int res = Interop.Calli(comPointer,(indexData == null)?IntPtr.Zero:indexData.comPointer,(*(IntPtr**)comPointer)[104]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
	}
}
