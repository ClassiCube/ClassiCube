#if USE_DX
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
using OpenTK;

namespace SharpDX.Direct3D9 {

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
	
	public class ComObject : IDisposable {
		public IntPtr comPointer;
		
		public ComObject(IntPtr pointer) { comPointer = pointer; }
		public ComObject() { }

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}
		
		~ComObject() { Dispose(false); }

		public bool IsDisposed;
		unsafe void Dispose( bool disposing ) {
			if( IsDisposed ) return;
			if( comPointer == IntPtr.Zero ) return;
			
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
			IsDisposed = true;
		}
	}
	
	[InteropPatch]
	public unsafe class Direct3D : ComObject {
		
		public Direct3D() {
			comPointer = Direct3DCreate9(SdkVersion);
			int count = GetAdapterCount();
			Adapters = new AdapterDetails[count];
			
			for (int i = 0; i < count; i++) {
				Adapters[i] = GetAdapterIdentifier(i);
			}
		}

		public AdapterDetails[] Adapters;
		
		const int SdkVersion = 32;
		[DllImport( "d3d9.dll" )]
		static extern IntPtr Direct3DCreate9(int sdkVersion);
		
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
		
		public bool CheckDeviceType(int adapter, DeviceType devType, Format adapterFormat, Format backBufferFormat, bool bWindowed) {
			return Interop.Calli(comPointer, adapter, (int)devType, (int)adapterFormat,
			                     (int)backBufferFormat, bWindowed ? 1 : 0,(*(IntPtr**)comPointer)[9]) == 0;
		}
		
		public bool CheckDepthStencilMatch(int adapter, DeviceType deviceType, Format adapterFormat, Format renderTargetFormat, Format depthStencilFormat) {
			return Interop.Calli(comPointer, adapter, (int)deviceType, (int)adapterFormat,
			                     (int)renderTargetFormat, (int)depthStencilFormat,(*(IntPtr**)comPointer)[12]) == 0;
		}
		
		public Device CreateDevice(int adapter, DeviceType deviceType, IntPtr hFocusWindow, CreateFlags behaviorFlags,  PresentParameters presentParams) {
			IntPtr devicePtr = IntPtr.Zero;
			int res = Interop.Calli(comPointer, adapter, (int)deviceType, hFocusWindow, (int)behaviorFlags, (IntPtr)(void*)&presentParams,
			                        (IntPtr)(void*)&devicePtr,(*(IntPtr**)comPointer)[16]);
			
			if( res < 0 ) { throw new SharpDXException( res ); }
			return new Device( devicePtr );
		}
	}

	[InteropPatch]
	public unsafe class Device : ComObject {
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
		
		public int Reset(PresentParameters presentParams) {
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
		
		public void UpdateTexture(Texture srcTex, Texture dstTex) {
			int res = Interop.Calli(comPointer, srcTex.comPointer, dstTex.comPointer,(*(IntPtr**)comPointer)[31]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
		
		public void GetRenderTargetData(Surface renderTarget, Surface dstSurface) {
			int res = Interop.Calli(comPointer, renderTarget.comPointer, dstSurface.comPointer,(*(IntPtr**)comPointer)[32]);
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
		
		public void SetSamplerState(int sampler, SamplerState type, int value) {
			int res = Interop.Calli(comPointer, sampler, (int)type, value, (*(IntPtr**)comPointer)[69]);
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
	
	public enum BackBufferType : int {
		Mono = 0,
		Left = 1,
		Right = 2,
	}
	
	public enum Blend : int {
		Zero = 1,
		One = 2,

		SourceAlpha = 5,
		InverseSourceAlpha = 6,
		DestinationAlpha = 7,
		InverseDestinationAlpha = 8,
	}
	
	public enum BlendOperation : int {
		Add = 1,
		Subtract = 2,
		ReverseSubtract = 3,
		Minimum = 4,
		Maximum = 5,
	}
	
	[Flags]
	public enum ClearFlags : int {
		Target = 1,
		ZBuffer = 2,
	}
	
	[Flags]
	public enum ColorWriteEnable : int {
		Red = 1,
		Green = 2,
		Blue = 4,
		Alpha = 8,
	}
	
	public enum Compare : int {
		Never = 1,
		Less = 2,
		Equal = 3,
		LessEqual = 4,
		Greater = 5,
		NotEqual = 6,
		GreaterEqual = 7,
		Always = 8,
	}
	
	[Flags]
	public enum CreateFlags : int {
		FpuPreserve = 2,
		Multithreaded = 4,
		PureDevice = 16,
		SoftwareVertexProcessing = 32,
		HardwareVertexProcessing = 64,
		MixedVertexProcessing = 128,
		DisableDriverManagement = 256,
		AdapterGroupDevice = 512,
		DisableExtendedDriverManagement = 1024,
		NoWindowChanges = 2048,
		None = 0,
	}
	
	public enum Cull : int {
		None = 1,
		Clockwise = 2,
		Counterclockwise = 3,
	}

	public enum DeviceType : int {
		Hardware = 1,
		Reference = 2,
		Software = 3,
		NullReference = 4,
	}
	
	public enum FillMode : int {
		Point = 1,
		Wireframe = 2,
		Solid = 3,
	}
	
	public enum FogMode : int {
		None = 0,
		Exponential = 1,
		ExponentialSquared = 2,
		Linear = 3,
	}
	
	public enum Format : int {
		Unknown = 0,
		R8G8B8 = 20,
		A8R8G8B8 = 21,
		X8R8G8B8 = 22,
		R5G6B5 = 23,
		X1R5G5B5 = 24,
		A1R5G5B5 = 25,
		A4R4G4B4 = 26,
		X4R4G4B4 = 30,
		D32 = 71,
		D15S1 = 73,
		D24S8 = 75,
		D24X8 = 77,
		D24X4S4 = 79,
		D16 = 80,
		D32F_Lockable = 82,
		VertexData = 100,
		Index16 = 101,
		Index32 = 102,
	}
	
	[Flags]
	public enum LockFlags : int {
		ReadOnly = 16,
		Discard = 8192,
		NoOverwrite = 4096,
		NoSystemLock = 2048,
		DoNotWait = 16384,
		NoDirtyUpdate = 32768,
		DoNotCopyData = 1,
		None = 0,
	}
	
	public enum Pool : int {
		Default = 0,
		Managed = 1,
		SystemMemory = 2,
		Scratch = 3,
	}
	
	[Flags]
	public enum PresentFlags : int {
		LockableBackBuffer = 1,
		DiscardDepthStencil = 2,
		DeviceClip = 4,
		Video = 16,
		None = 0,
	}
	
	[Flags]
	public enum PresentInterval : int {
		Default = 0,
		One = 1,
		Immediate = unchecked((int)-2147483648),
	}
	
	public enum PrimitiveType : int {
		PointList = 1,
		LineList = 2,
		LineStrip = 3,
		TriangleList = 4,
		TriangleStrip = 5,
		TriangleFan = 6,
	}
	
	public enum RenderState : int {
		ZEnable = 7,
		FillMode = 8,
		ShadeMode = 9,
		ZWriteEnable = 14,
		AlphaTestEnable = 15,
		SourceBlend = 19,
		DestinationBlend = 20,
		CullMode = 22,
		ZFunc = 23,
		AlphaRef = 24,
		AlphaFunc = 25,
		DitherEnable = 26,
		AlphaBlendEnable = 27,
		FogEnable = 28,
		SpecularEnable = 29,
		FogColor = 34,
		FogTableMode = 35,
		FogStart = 36,
		FogEnd = 37,
		FogDensity = 38,
		Clipping = 136,
		Lighting = 137,
		Ambient = 139,
		FogVertexMode = 140,
		ColorVertex = 141,
		LocalViewer = 142,
		DebugMonitorToken = 165,
		ColorWriteEnable = 168,
	}
	
	public enum SamplerState : int {
		MagFilter = 5,
		MinFilter = 6,
		MipFilter = 7,
		MipMapLODBias  = 8,
		MaxMipLevel = 9,
	}
	
	public enum ShadeMode : int {
		Flat = 1,
		Gouraud = 2,
		Phong = 3,
	}
	
	public enum SwapEffect : int {
		Discard = 1,
		Flip = 2,
		Copy = 3,
	}
	
	public enum TextureFilter : int {
		None = 0,
		Point = 1,
		Linear = 2,
	}
	
	public enum TextureStage : int {
		AlphaOperation = 4,
		TextureTransformFlags = 24,
	}
	
	public enum TextureOp : int {
		SelectArg1 = 2,
		Modulate = 4,
	}
	
	[Flags]
	public enum TextureTransform : int {
		Disable = 0,
		Count1 = 1,
		Count2 = 2,
	}
	
	public enum TransformState : int {
		View = 2,
		Projection = 3,
		World = 256,
		Texture0 = 16,
	}
	
	[Flags]
	public enum Usage : int {
		DepthStencil = 2,
		Dynamic = 512,
		WriteOnly = 8,
		SoftwareProcessing = 16,
		None = 0,
		AutoGenMipmap = 1024,
	}
	
	[Flags]
	public enum VertexFormat : int {
		Position = 2,
		Diffuse = 64,
		Texture0 = 0, // really means '0 texture coordinates'
		Texture1 = 256, // really means '1 texture coordinate'
		Texture2 = 512,
		None = 0,
	}
	
	[InteropPatch]
	public unsafe class DataBuffer : ComObject { // Either 'VertexBuffer' or 'IndexBuffer
		
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
		
		public void Unlock() {
			int res = Interop.Calli(comPointer, (*(IntPtr**)comPointer)[12]);
			if( res < 0 ) { throw new SharpDXException( res ); }
		}
	}
	
	[InteropPatch]
	public unsafe class Surface : ComObject {
		
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
	public unsafe class Texture : ComObject {
		
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

	[StructLayout(LayoutKind.Sequential)]
	public struct LockedRectangle {
		public int Pitch;
		public IntPtr DataPointer;
	}
	
	[StructLayout(LayoutKind.Sequential)]
	public struct D3DRect {
		public int Left, Top, Right, Bottom;
	}
	
	[StructLayout(LayoutKind.Sequential)]
	public struct PresentParameters {
		public int BackBufferWidth, BackBufferHeight;
		public Format BackBufferFormat;
		public int BackBufferCount;
		public int MultiSampleType, MultiSampleQuality;
		public SwapEffect SwapEffect;
		public IntPtr DeviceWindowHandle;
		public int Windowed;
		public int EnableAutoDepthStencil;
		public Format AutoDepthStencilFormat;
		public PresentFlags PresentFlags;
		public int FullScreenRefreshRateInHz;
		public PresentInterval PresentationInterval;
	}
	
	[StructLayout(LayoutKind.Sequential)]
	public struct Capabilities {
		public DeviceType DeviceType;
		public int AdapterOrdinal;
		public int Caps, Caps2, Caps3;
		public PresentInterval PresentationIntervals;
		public int CursorCaps;
		public int DeviceCaps;
		public int PrimitiveMiscCaps;
		public int RasterCaps;
		public int DepthCompareCaps;
		public int SourceBlendCaps;
		public int DestinationBlendCaps;
		public int AlpaCompareCaps;
		public int ShadeCaps;
		public int TextureCaps;
		public int TextureFilterCaps;
		public int CubeTextureFilterCaps;
		public int VolumeTextureFilterCaps;
		public int TextureAddressCaps;
		public int VolumeTextureAddressCaps;
		public int LineCaps;
		public int MaxTextureWidth, MaxTextureHeight;
		public int MaxVolumeExtent;
		public int MaxTextureRepeat;
		public int MaxTextureAspectRatio;
		public int MaxAnisotropy;
		public float MaxVertexW;
		public float GuardBandLeft, GuardBandTop;
		public float GuardBandRight, GuardBandBottom;
		public float ExtentsAdjust;
		public int StencilCaps;
		public int FVFCaps;
		public int TextureOperationCaps;
		public int MaxTextureBlendStages;
		public int MaxSimultaneousTextures;
		public int VertexProcessingCaps;
		public int MaxActiveLights;
		public int MaxUserClipPlanes;
		public int MaxVertexBlendMatrices;
		public int MaxVertexBlendMatrixIndex;
		public float MaxPointSize;
		public int MaxPrimitiveCount;
		public int MaxVertexIndex;
		public int MaxStream, MaxStreamStride;
		internal int VertexShaderVersion_;
		public int MaxVertexShaderConst;
		internal int PixelShaderVersion_;
		public float PixelShader1xMaxValue;
		public int DeviceCaps2;
		public float MaxNpatchTessellationLevel;
		internal int Reserved5;
		public int MasterAdapterOrdinal;
		public int AdapterOrdinalInGroup;
		public int NumberOfAdaptersInGroup;
		public int DeclarationTypes;
		public int SimultaneousRTCount;
		public int StretchRectFilterCaps;
		public int VS20Caps;
		public int VS20DynamicFlowControlDepth, VS20NumTemps;
		public int VS20StaticFlowControlDepth;
		public int PS20Caps;
		public int PS20DynamicFlowControlDepth, PS20NumTemps;
		public int PS20StaticFlowControlDepth;
		public int PS20NumInstructionSlots;
		public int VertexTextureFilterCaps;
		public int MaxVShaderInstructionsExecuted;
		public int MaxPShaderInstructionsExecuted;
		public int MaxVertexShader30InstructionSlots;
		public int MaxPixelShader30InstructionSlots;
	}
}
#endif