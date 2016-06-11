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
	public struct Capabilities {
		public DeviceType DeviceType;
		public int AdapterOrdinal;
		public int Caps;
		public Caps2 Caps2;
		public Caps3 Caps3;
		public PresentInterval PresentationIntervals;
		public int CursorCaps;
		public DeviceCaps DeviceCaps;
		public PrimitiveMiscCaps PrimitiveMiscCaps;
		public RasterCaps RasterCaps;
		public CompareCaps DepthCompareCaps;
		public BlendCaps SourceBlendCaps;
		public BlendCaps DestinationBlendCaps;
		public CompareCaps AlpaCompareCaps;
		public ShadeCaps ShadeCaps;
		public TextureCaps TextureCaps;
		public int TextureFilterCaps;
		public int CubeTextureFilterCaps;
		public int VolumeTextureFilterCaps;
		public TextureAddressCaps TextureAddressCaps;
		public TextureAddressCaps VolumeTextureAddressCaps;
		public LineCaps LineCaps;
		public int MaxTextureWidth;
		public int MaxTextureHeight;
		public int MaxVolumeExtent;
		public int MaxTextureRepeat;
		public int MaxTextureAspectRatio;
		public int MaxAnisotropy;
		public float MaxVertexW;
		public float GuardBandLeft;
		public float GuardBandTop;
		public float GuardBandRight;
		public float GuardBandBottom;
		public float ExtentsAdjust;
		public int StencilCaps;
		public VertexFormatCaps FVFCaps;
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
		public int MaxStreams;
		public int MaxStreamStride;
		internal int VertexShaderVersion_;
		public int MaxVertexShaderConst;
		internal int PixelShaderVersion_;
		public float PixelShader1xMaxValue;
		public DeviceCaps2 DeviceCaps2;
		public float MaxNpatchTessellationLevel;
		internal int Reserved5;
		public int MasterAdapterOrdinal;
		public int AdapterOrdinalInGroup;
		public int NumberOfAdaptersInGroup;
		public int DeclarationTypes;
		public int SimultaneousRTCount;
		public int StretchRectFilterCaps;
		public int VS20Caps;
		public int VS20DynamicFlowControlDepth;
		public int VS20NumTemps;
		public int VS20StaticFlowControlDepth;
		public int PS20Caps;
		public int PS20DynamicFlowControlDepth;
		public int PS20NumTemps;
		public int PS20StaticFlowControlDepth;
		public int PS20NumInstructionSlots;
		public int VertexTextureFilterCaps;
		public int MaxVShaderInstructionsExecuted;
		public int MaxPShaderInstructionsExecuted;
		public int MaxVertexShader30InstructionSlots;
		public int MaxPixelShader30InstructionSlots;
	}
}