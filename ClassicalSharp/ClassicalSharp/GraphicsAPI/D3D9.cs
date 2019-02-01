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

	public enum Direct3DError {
		Ok = 0,
		NotFound          = 2150 | unchecked((int)0x88760000),
		MoreData          = 2151 | unchecked((int)0x88760000),
		DeviceLost        = 2152 | unchecked((int)0x88760000),
		DeviceNotReset    = 2153 | unchecked((int)0x88760000),
		NotAvailable      = 2154 | unchecked((int)0x88760000),
		OutOfVideoMemory  =  380 | unchecked((int)0x88760000),
		InvalidDevice     = 2155 | unchecked((int)0x88760000),
		InvalidCall       = 2156 | unchecked((int)0x88760000),
		DriverInvalidCall = 2157 | unchecked((int)0x88760000),
		WasStillDrawing   =  540 | unchecked((int)0x88760000),
	}
	
	public class SharpDXException : Exception {
		public SharpDXException(int result) : base(Format(result)) {
			HResult = result;
		}
		
		static string Format(int code) {
			Direct3DError err = (Direct3DError)code;
			return String.Format("HRESULT: [0x{0:X}], D3D error type: {1}", code, err);
		}
	}
	
	[InteropPatch]
	public unsafe static class Direct3D {
		
		public const int SdkVersion = 32;
		[DllImport("d3d9.dll")]
		public static extern IntPtr Direct3DCreate9(int sdkVersion);
		
		public static AdapterDetails GetAdapterIdentifier(IntPtr ptr, int adapter) {
			AdapterDetails identifier = new AdapterDetails();
			int res = Interop.Calli(ptr, adapter, 0, (IntPtr)(void*)&identifier,(*(IntPtr**)ptr)[5]);
			if (res < 0) { throw new SharpDXException(res); }
			return identifier;
		}
		
		public static bool CheckDeviceType(IntPtr ptr, int adapter, DeviceType devType, 
		                                   Format adapterFormat, Format backBufferFormat, bool bWindowed) {
			return Interop.Calli(ptr, adapter, (int)devType, (int)adapterFormat,
			                     (int)backBufferFormat, bWindowed ? 1 : 0, (*(IntPtr**)ptr)[9]) == 0;
		}
		
		public static bool CheckDepthStencilMatch(IntPtr ptr, int adapter, DeviceType deviceType, 
		                                          Format adapterFormat, Format renderTargetFormat, Format depthStencilFormat) {
			return Interop.Calli(ptr, adapter, (int)deviceType, (int)adapterFormat,
			                     (int)renderTargetFormat, (int)depthStencilFormat, (*(IntPtr**)ptr)[12]) == 0;
		}
		
		public static IntPtr CreateDevice(IntPtr ptr, int adapter, DeviceType deviceType, IntPtr hFocusWindow, 
		                                  CreateFlags behaviorFlags, PresentParameters* presentParams) {
			IntPtr devicePtr = IntPtr.Zero;
			int res = Interop.Calli(ptr, adapter, (int)deviceType, hFocusWindow, (int)behaviorFlags, (IntPtr)presentParams,
			                        (IntPtr)(void*)&devicePtr, (*(IntPtr**)ptr)[16]);
			
			if (res < 0) { throw new SharpDXException(res); }
			return devicePtr;
		}
	}

	[InteropPatch]
	public unsafe static class Device {

		public static int TestCooperativeLevel(IntPtr ptr) {
			return Interop.Calli(ptr, (*(IntPtr**)ptr)[3]);
		}
		
		public static uint GetAvailableTextureMemory(IntPtr ptr) {
			return (uint)Interop.Calli(ptr, (*(IntPtr**)ptr)[4]);
		}
		
		public static void GetCapabilities(IntPtr ptr, Capabilities* caps) {
			int res = Interop.Calli(ptr, (IntPtr)caps, (*(IntPtr**)ptr)[7]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static int Reset(IntPtr ptr, PresentParameters* presentParams) {
			return Interop.Calli(ptr, (IntPtr)presentParams, (*(IntPtr**)ptr)[16]);
		}
		
		public static int Present(IntPtr ptr) {
			return Interop.Calli(ptr, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, (*(IntPtr**)ptr)[17]);
		}
		
		public static void GetBackBuffer(IntPtr ptr, int iSwapChain, int iBackBuffer, BackBufferType type, IntPtr* backBuffer) {
			int res = Interop.Calli(ptr, iSwapChain, iBackBuffer, (int)type, (IntPtr)backBuffer,(*(IntPtr**)ptr)[18]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void CreateTexture(IntPtr ptr, int width, int height, int levels, Usage usage, Format format, Pool pool, IntPtr* tex) {
			int res = Interop.Calli(ptr, width, height, levels, (int)usage, (int)format, (int)pool, (IntPtr)tex, IntPtr.Zero, (*(IntPtr**)ptr)[23]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static int CreateVertexBuffer(IntPtr ptr, int length, Usage usage, VertexFormat vertexFormat, Pool pool, IntPtr* vb) {
			return Interop.Calli(ptr, length, (int)usage, (int)vertexFormat, (int)pool, (IntPtr)vb, IntPtr.Zero, (*(IntPtr**)ptr)[26]);
		}
		
		public static void CreateIndexBuffer(IntPtr ptr, int length, Usage usage, Format format, Pool pool, IntPtr* ib) {
			int res = Interop.Calli(ptr, length, (int)usage, (int)format, (int)pool, (IntPtr)ib, IntPtr.Zero, (*(IntPtr**)ptr)[27]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void UpdateTexture(IntPtr ptr, IntPtr srcTex, IntPtr dstTex) {
			int res = Interop.Calli(ptr, srcTex, dstTex, (*(IntPtr**)ptr)[31]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void GetRenderTargetData(IntPtr ptr, IntPtr renderTarget, IntPtr dstSurface) {
			int res = Interop.Calli(ptr, renderTarget, dstSurface, (*(IntPtr**)ptr)[32]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void CreateOffscreenPlainSurface(IntPtr ptr, int width, int height, Format format, Pool pool, IntPtr* surface) {
			int res = Interop.Calli(ptr, width, height, (int)format, (int)pool, (IntPtr)surface, IntPtr.Zero, (*(IntPtr**)ptr)[36]);
			if (res < 0) { throw new SharpDXException(res); }
		}

		public static void BeginScene(IntPtr ptr) {
			int res = Interop.Calli(ptr, (*(IntPtr**)ptr)[41]);
			if (res < 0) { throw new SharpDXException(res); }
		}

		public static void EndScene(IntPtr ptr) {
			int res = Interop.Calli(ptr, (*(IntPtr**)ptr)[42]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void Clear(IntPtr ptr, ClearFlags flags, int colorBGRA, float z, int stencil) {
			int res = Interop.Calli(ptr, 0, IntPtr.Zero, (int)flags, colorBGRA, z, stencil, (*(IntPtr**)ptr)[43]);
			if (res < 0) { throw new SharpDXException(res); }
		}

		public static void SetTransform(IntPtr ptr, TransformState state, ref Matrix4 matrixRef) {
			int res;
			fixed (void* matrixRef_ = &matrixRef)
				res = Interop.Calli(ptr, (int)state, (IntPtr)matrixRef_, (*(IntPtr**)ptr)[44]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void SetRenderState(IntPtr ptr, RenderState renderState, bool enable) {
			SetRenderState(ptr, renderState, enable ? 1 : 0);
		}

		public static void SetRenderState(IntPtr ptr, RenderState renderState, float value) {
			SetRenderState(ptr, renderState, *(int*)&value);
		}

		public static void SetRenderState(IntPtr ptr, RenderState state, int value) {
			int res = Interop.Calli(ptr, (int)state, value, (*(IntPtr**)ptr)[57]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void SetTexture(IntPtr ptr, int stage, IntPtr texture) {
			int res = Interop.Calli(ptr, stage, texture, (*(IntPtr**)ptr)[65]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void SetTextureStageState(IntPtr ptr, int stage, TextureStage type, int value) {
			int res = Interop.Calli(ptr, stage, (int)type, value, (*(IntPtr**)ptr)[67]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void SetSamplerState(IntPtr ptr, int sampler, SamplerState type, int value) {
			int res = Interop.Calli(ptr, sampler, (int)type, value, (*(IntPtr**)ptr)[69]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void DrawPrimitives(IntPtr ptr, PrimitiveType type, int startVertex, int primitiveCount) {
			int res = Interop.Calli(ptr, (int)type, startVertex, primitiveCount,(*(IntPtr**)ptr)[81]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void DrawIndexedPrimitives(IntPtr ptr, PrimitiveType type, int baseVertexIndex, int minVertexIndex, int numVertices, int startIndex, int primCount) {
			int res = Interop.Calli(ptr, (int)type, baseVertexIndex, minVertexIndex, numVertices, startIndex, primCount, (*(IntPtr**)ptr)[82]);
			if (res < 0) { throw new SharpDXException(res); }
		}

		public static void SetVertexFormat(IntPtr ptr, VertexFormat vertexFormat) {
			int res = Interop.Calli(ptr, (int)vertexFormat,(*(IntPtr**)ptr)[89]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void SetStreamSource(IntPtr ptr, int streamNumber, IntPtr streamData, int offsetInBytes, int stride) {
			int res = Interop.Calli(ptr, streamNumber, streamData, offsetInBytes, stride, (*(IntPtr**)ptr)[100]);
			if (res < 0) { throw new SharpDXException(res); }
		}
		
		public static void SetIndices(IntPtr ptr, IntPtr indexData) {
			int res = Interop.Calli(ptr, indexData, (*(IntPtr**)ptr)[104]);
			if (res < 0) { throw new SharpDXException(res); }
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public unsafe struct AdapterDetails {
		public fixed byte RawDriver[512];
		public fixed byte RawDescription[512];
		public fixed byte RawDeviceName[32];
		public long RawDriverVersion;
		public int VendorId;
		public int DeviceId;
		public int SubsystemId;
		public int Revision;
		public Guid DeviceIdentifier;
		public int WhqlLevel;
		
		public Version DriverVersion {
			get {
				return new Version((int)(RawDriverVersion >> 48) & 0xFFFF, (int)(RawDriverVersion >> 32) & 0xFFFF,
				                   (int)(RawDriverVersion >> 16) & 0xFFFF, (int)(RawDriverVersion >> 0) & 0xFFFF);
			}
		}
		
		public string Description {
			get {
				fixed (byte* ptr = RawDescription) return new string((sbyte*)ptr);
			}
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
		Software = 32, // SoftwareVertexProcessing
		Hardware = 64, // HardwareVertexProcessing
		Mixed = 128, // MixedVertexProcessing
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
		LineList = 2,
		TriangleList = 4,
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
		FogEnd = 37,
		FogDensity = 38,
		Lighting = 137,
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
	public unsafe static class DataBuffser { // Either 'VertexBuffer' or 'IndexBuffer
		
		public static IntPtr Lock(IntPtr ptr, int offsetToLock, int sizeToLock, LockFlags flags) {
			IntPtr data;
			int res = Interop.Calli(ptr, offsetToLock, sizeToLock, (IntPtr)(void*)&data, (int)flags, (*(IntPtr**)ptr)[11]);
			if (res < 0) { throw new SharpDXException(res); }
			return data;
		}
		
		public static void SetData(IntPtr ptr, IntPtr data, int bytes, LockFlags flags) {
			IntPtr dst = Lock(ptr, 0, bytes, flags);
			MemUtils.memcpy(data, dst, bytes);
			Unlock(ptr);
		}
		
		public static void Unlock(IntPtr ptr) {
			int res = Interop.Calli(ptr, (*(IntPtr**)ptr)[12]);
			if (res < 0) { throw new SharpDXException(res); }
		}
	}
	
	[InteropPatch]
	public unsafe static class Surface {
		
		public static LockedRectangle LockRectangle(IntPtr ptr, LockFlags flags) {
			LockedRectangle rect = new LockedRectangle();
			int res = Interop.Calli(ptr, (IntPtr)(void*)&rect, IntPtr.Zero, (int)flags, (*(IntPtr**)ptr)[13]);
			if (res < 0) { throw new SharpDXException(res); }
			return rect;
		}
		
		public static void UnlockRectangle(IntPtr ptr) {
			int res = Interop.Calli(ptr,(*(IntPtr**)ptr)[14]);
			if (res < 0) { throw new SharpDXException(res); }
		}
	}
	
	[InteropPatch]
	public unsafe static class Texture {

		public static LockedRectangle LockRectangle(IntPtr ptr, int level, LockFlags flags) {
			LockedRectangle rect = new LockedRectangle();
			int res = Interop.Calli(ptr, level, (IntPtr)(void*)&rect, IntPtr.Zero, (int)flags, (*(IntPtr**)ptr)[19]);
			if (res < 0) { throw new SharpDXException(res); }
			return rect;
		}
		
		public static LockedRectangle LockRectangle(IntPtr ptr, int level, D3DRect region, LockFlags flags) {
			LockedRectangle rect = new LockedRectangle();
			int res = Interop.Calli(ptr, level, (IntPtr)(void*)&rect, (IntPtr)(void*)&region, (int)flags, (*(IntPtr**)ptr)[19]);
			if (res < 0) { throw new SharpDXException(res); }
			return rect;
		}
		
		public static void SetData(IntPtr ptr, int level, LockFlags flags, IntPtr data, int bytes) {
			LockedRectangle rect = LockRectangle(ptr, level, flags);
			MemUtils.memcpy(data, rect.DataPointer, bytes);
			UnlockRectangle(ptr, level);
		}
		
		public static void SetPartData(IntPtr ptr, int level, LockFlags flags, IntPtr data, int x, int y, int width, int height) {
			D3DRect region;
			region.Left = x; region.Top = y;
			region.Right = x + width; region.Bottom = y + height;
			LockedRectangle rect = LockRectangle(ptr, level, region, flags);
			
			// We need to copy scanline by scanline, as generally rect.stride != data.stride
			byte* src = (byte*)data, dst = (byte*)rect.DataPointer;
			for(int yy = 0; yy < height; yy++) {
				MemUtils.memcpy((IntPtr)src, (IntPtr)dst, width * 4);
				src += width * 4;
				dst += rect.Pitch;
			}
			UnlockRectangle(ptr, level);
		}
		
		public static void UnlockRectangle(IntPtr ptr, int level) {
			int res = Interop.Calli(ptr, level, (*(IntPtr**)ptr)[20]);
			if (res < 0) { throw new SharpDXException(res); }
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