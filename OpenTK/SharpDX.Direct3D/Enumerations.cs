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
	
	[Flags]
	public enum BlendCaps : int {
		Zero = 1,
		One = 2,
		SourceColor = 4,
		InverseSourceColor = 8,
		SourceAlpha = 16,
		InverseSourceAlpha = 32,
		DestinationAlpha = 64,
		InverseDestinationAlpha = 128,
		DestinationColor = 256,
		InverseDestinationColor = 512,
		SourceAlphaSaturated = 1024,
		Bothsrcalpha = 2048,
		BothInverseSourceAlpha = 4096,
		BlendFactor = 8192,
		SourceColor2 = 16384,
		InverseSourceColor2 = 32768,
	}
	
	public enum BlendOperation : int {
		Add = 1,
		Subtract = 2,
		ReverseSubtract = 3,
		Minimum = 4,
		Maximum = 5,
	}
	
	[Flags]
	public enum Caps2 : int {
		FullScreenGamma = 131072,
		CanCalibrateGamma = 1048576,
		CanManageResource = 268435456,
		DynamicTextures = 536870912,
		CanAutoGenerateMipMap = 1073741824,
		CanShareResource = unchecked((int)-2147483648),
		None = 0,
	}
	
	[Flags]
	public enum Caps3 : int {
		AlphaFullScreenFlipOrDiscard = 32,
		LinearToSrgbPresentation = 128,
		CopyToVideoMemory = 256,
		CopyToSystemMemory = 512,
		DXVAHd = 1024,
		None = 0,
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
	public enum CompareCaps : int {
		Never = 1,
		Less = 2,
		Equal = 4,
		LessEqual = 8,
		Greater = 16,
		NotEqual = 32,
		GreaterEqual = 64,
		Always = 128,
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
	
	[Flags]
	public enum DeviceCaps : int {
		ExecuteSystemMemory = 16,
		ExecuteVideoMemory = 32,
		TLVertexSystemMemory = 64,
		TLVertexVideoMemory = 128,
		TextureSystemMemory = 256,
		TextureVideoMemory = 512,
		DrawPrimTLVertex = 1024,
		CanRenderAfterFlip = 2048,
		TextureNonLocalVideoMemory = 4096,
		DrawPrimitives2 = 8192,
		SeparateTextureMemory = 16384,
		DrawPrimitives2Extended = 32768,
		HWTransformAndLight = 65536,
		CanBlitSysToNonLocal = 131072,
		HWRasterization = 524288,
		PureDevice = 1048576,
		QuinticRTPatches = 2097152,
		RTPatches = 4194304,
		RTPatchHandleZero = 8388608,
		NPatches = 16777216,
	}
	
	[Flags]
	public enum DeviceCaps2 : int {
		StreamOffset = 1,
		DMapNPatch = 2,
		AdaptiveTessRTPatch = 4,
		AdaptiveTessNPatch = 8,
		CanStretchRectFromTextures = 16,
		PresampledMapNPatch = 32,
		VertexElementsCanShareStreamOffset = 64,
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
		VertexData = 100,
		Index16 = 101,
		Index32 = 102,
	}

	[Flags]
	public enum LineCaps : int {
		Texture = 1,
		DepthTest = 2,
		Blend = 4,
		AlphaCompare = 8,
		Fog = 16,
		Antialias = 32,
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
		Two = 2,
		Three = 4,
		Four = 8,
		Immediate = unchecked((int)-2147483648),
	}
	
	[Flags]
	public enum PrimitiveMiscCaps : int {
		MaskZ = 2,
		CullNone = 16,
		CullCW = 32,
		CullCCW = 64,
		ColorWriteEnable = 128,
		ClipPlanesScaledPoints = 256,
		ClipTLVertices = 512,
		TssArgTemp = 1024,
		BlendOperation = 2048,
		NullReference = 4096,
		IndependentWriteMasks = 16384,
		PerStageConstant = 32768,
		FogAndSpecularAlpha = 65536,
		SeparateAlphaBlend = 131072,
		MrtIndependentBitDepths = 262144,
		MrtPostPixelShaderBlending = 524288,
		FogVertexClamped = 1048576,
		PostBlendSrgbConvert = 2097152,
	}
	
	public enum PrimitiveType : int {
		PointList = 1,
		LineList = 2,
		LineStrip = 3,
		TriangleList = 4,
		TriangleStrip = 5,
		TriangleFan = 6,
	}
	
	[Flags]
	public enum RasterCaps : int {
		Dither = 1,
		DepthTest = 16,
		FogVertex = 128,
		FogTable = 256,
		MipMapLodBias = 8192,
		ZBufferLessHsr = 32768,
		FogRange = 65536,
		Anisotropy = 131072,
		WBuffer = 262144,
		WFog = 1048576,
		ZFog = 2097152,
		ColorPerspective = 4194304,
		ScissorTest = 16777216,
		SlopeScaleDepthBias = 33554432,
		DepthBias = 67108864,
		MultisampleToggle = 134217728,
	}
	
	public enum RenderState : int {
		ZEnable = 7,
		FillMode = 8,
		ShadeMode = 9,
		ZWriteEnable = 14,
		AlphaTestEnable = 15,
		LastPixel = 16,
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
		RangeFogEnable = 48,
		TextureFactor = 60,
		Clipping = 136,
		Lighting = 137,
		Ambient = 139,
		FogVertexMode = 140,
		ColorVertex = 141,
		LocalViewer = 142,
		PatchEdgeStyle = 163,
		DebugMonitorToken = 165,
		PointSizeMax = 166,
		IndexedVertexBlendEnable = 167,
		ColorWriteEnable = 168,
		BlendOperation = 171,
		PositionDegree = 172,
		NormalDegree = 173,
		ScissorTestEnable = 174,
		SlopeScaleDepthBias = 175,
		AntialiasedLineEnable = 176,
	}
	
	public enum SamplerState : int {
		MagFilter = 5,
		MinFilter = 6,
		MipFilter = 7,
		MipMapLODBias  = 8,
		MaxMipLevel = 9,
	}

	[Flags]
	public enum ShadeCaps : int {
		ColorGouraudRgb = 8,
		SpecularGouraudRgb = 512,
		AlphaGouraudBlend = 16384,
		FogGouraud = 524288,
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
	
	[Flags]
	public enum TextureAddressCaps : int {
		Wrap = 1,
		Mirror = 2,
		Clamp = 4,
		Border = 8,
		IndependentUV = 16,
		MirrorOnce = 32,
	}
	
	[Flags]
	public enum TextureCaps : int {
		Perspective = 1,
		Pow2 = 2,
		Alpha = 4,
		SquareOnly = 32,
		TextureRepeatNotScaledBySize = 64,
		AlphaPalette = 128,
		NonPow2Conditional = 256,
		Projected = 1024,
		CubeMap = 2048,
		VolumeMap = 8192,
		MipMap = 16384,
		MipVolumeMap = 32768,
		MipCubeMap = 65536,
		CubeMapPow2 = 131072,
		VolumeMapPow2 = 262144,
		NoProjectedBumpEnvironment = 2097152,
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
	
	[Flags]
	public enum VertexFormatCaps : int {
		TextureCoordCountMask = 65535,
		DoNotStripElements = 524288,
		PointSize = 1048576,
	}
}
