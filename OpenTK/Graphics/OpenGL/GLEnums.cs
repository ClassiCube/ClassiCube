#region License
//
// The Open Toolkit Library License
//
// Copyright (c) 2006 - 2009 the Open Toolkit library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
#endregion

using System;

namespace OpenTK.Graphics.OpenGL
{

    public enum ArrayCap : int {
        VertexArray = 0x8074,
        ColorArray = 0x8076,
        TextureCoordArray = 0x8078,
    }

    public enum BeginMode : int {
        Points = 0x0000,
        Lines = 0x0001,
        LineLoop = 0x0002,
        LineStrip = 0x0003,
        Triangles = 0x0004,
        TriangleStrip = 0x0005,
        TriangleFan = 0x0006,
    }

    public enum BlendingFactor : int {
        Zero = 0,
        SrcAlpha = 0x0302,
        OneMinusSrcAlpha = 0x0303,
        DstAlpha = 0x0304,
        OneMinusDstAlpha = 0x0305,
        One = 1,
    }

    public enum BufferTarget : int {
        ArrayBuffer = 0x8892,
        ElementArrayBuffer = 0x8893,
    }

    public enum BufferUsage : int {
        StreamDraw = 0x88E0,
        StaticDraw = 0x88E4,
        DynamicDraw = 0x88E8,
    }

    [Flags]
    public enum ClearBufferMask : int {
        DepthBufferBit = 0x00000100,
        ColorBufferBit = 0x00004000,
    }

    public enum PointerType : int {
        UnsignedByte = 0x1401,
        Float = 0x1406,
    }

    public enum CullFaceMode : int {
        Front = 0x0404,
        Back = 0x0405,
        FrontAndBack = 0x0408,
    }

    public enum Compare : int {
        Never = 0x0200,
        Less = 0x0201,
        Equal = 0x0202,
        Lequal = 0x0203,
        Greater = 0x0204,
        Notequal = 0x0205,
        Gequal = 0x0206,
        Always = 0x0207,
    }

    public enum DrawElementsType : int {
        UnsignedShort = 0x1403,
    }

    public enum EnableCap : int {
        CullFace = 0x0B44,
        Fog = 0x0B60,
        DepthTest = 0x0B71,
        StencilTest = 0x0B90,
        Normalize = 0x0BA1,
        AlphaTest = 0x0BC0,
        Dither = 0x0BD0,
        Blend = 0x0BE2,
        Texture2D = 0x0DE1,

        VertexArray = 0x8074,
        ColorArray = 0x8076,
        TextureCoordArray = 0x8078,
    }

    public enum ErrorCode : int {
        NoError = 0,
        InvalidEnum = 0x0500,
        InvalidValue = 0x0501,
        InvalidOperation = 0x0502,
        StackOverflow = 0x0503,
        StackUnderflow = 0x0504,
        OutOfMemory = 0x0505,

        TextureTooLargeExt = 0x8065,
    }

    public enum ExtVertexArrayBgra : int {
        Bgra = 0x80E1,
    }

    public enum FogMode : int {
        Exp = 0x0800,
        Exp2 = 0x0801,
        Linear = 0x2601,
    }

    public enum FogParameter : int {
        FogDensity = 0x0B62,
        FogStart = 0x0B63,
        FogEnd = 0x0B64,
        FogMode = 0x0B65,
        FogColor = 0x0B66,
    }

    public enum FrontFaceDirection : int {
        Cw = 0x0900,
        Ccw = 0x0901,
    }

    public enum GetPName : int {
        CurrentColor = 0x0B00,

        PolygonMode = 0x0B40,
        PolygonSmooth = 0x0B41,
        FrontFace = 0x0B46,
        ShadeModel = 0x0B54,
        Fog = 0x0B60,
        
        FogDensity = 0x0B62,
        FogStart = 0x0B63,
        FogEnd = 0x0B64,
        FogMode = 0x0B65,
        FogColor = 0x0B66,
        DepthRange = 0x0B70,
        DepthTest = 0x0B71,
        DepthWritemask = 0x0B72,
        DepthClearValue = 0x0B73,
        DepthFunc = 0x0B74,

        MatrixMode = 0x0BA0,
        Normalize = 0x0BA1,
        Viewport = 0x0BA2,
        ModelviewStackDepth = 0x0BA3,
        ProjectionStackDepth = 0x0BA4,
        TextureStackDepth = 0x0BA5,
        ModelviewMatrix = 0x0BA6,
        ProjectionMatrix = 0x0BA7,
        TextureMatrix = 0x0BA8,

        AlphaTest = 0x0BC0,
        AlphaTestFunc = 0x0BC1,
        AlphaTestRef = 0x0BC2,
        Dither = 0x0BD0,
        BlendDst = 0x0BE0,
        BlendSrc = 0x0BE1,
        Blend = 0x0BE2,

        ColorClearValue = 0x0C22,
        ColorWritemask = 0x0C23,
        IndexMode = 0x0C30,
        RgbaMode = 0x0C31,
        Doublebuffer = 0x0C32,
        Stereo = 0x0C33,
        RenderMode = 0x0C40,
        PerspectiveCorrectionHint = 0x0C50,
        PointSmoothHint = 0x0C51,
        LineSmoothHint = 0x0C52,
        PolygonSmoothHint = 0x0C53,
        FogHint = 0x0C54,
       
        UnpackAlignment = 0x0CF5,

        PackAlignment = 0x0D05,
       
        DepthScale = 0x0D1E,
        DepthBias = 0x0D1F,

        MaxTextureSize = 0x0D33,

        MaxModelviewStackDepth = 0x0D36,

        MaxProjectionStackDepth = 0x0D38,
        MaxTextureStackDepth = 0x0D39,
        MaxViewportDims = 0x0D3A,

        SubpixelBits = 0x0D50,
        IndexBits = 0x0D51,
        RedBits = 0x0D52,
        GreenBits = 0x0D53,
        BlueBits = 0x0D54,
        AlphaBits = 0x0D55,
        DepthBits = 0x0D56,
       
        Texture2D = 0x0DE1,
        TextureBinding2D = 0x8069,
    }

    public enum HintMode : int {
        DontCare = 0x1100,
        Fastest = 0x1101,
        Nicest = 0x1102,
    }

    public enum HintTarget : int {
        PerspectiveCorrectionHint = 0x0C50,
        PointSmoothHint = 0x0C51,
        LineSmoothHint = 0x0C52,
        PolygonSmoothHint = 0x0C53,
        FogHint = 0x0C54,

        GenerateMipmapHint = 0x8192,
    }

    public enum MatrixMode : int {
        Modelview = 0x1700,
        Projection = 0x1701,
        Texture = 0x1702,
        Color = 0x1800,
    }

    public enum PixelFormat : int {
        Rgba = 0x1908,
        Bgra = 0x80E1,
    }

    public enum PixelInternalFormat : int {
        Rgba = 0x1908,
        Rgba8 = 0x8058,
    }

    public enum PixelType : int {
        UnsignedByte = 0x1401,
    }

    public enum ShadingModel : int {
        Flat = 0x1D00,
        Smooth = 0x1D01,
    }

    public enum StringName : int {
        Vendor = 0x1F00,
        Renderer = 0x1F01,
        Version = 0x1F02,
        Extensions = 0x1F03,
    }

    public enum TextureFilter : int {
        Nearest = 0x2600,
        Linear = 0x2601,
    }

    public enum TextureParameterName : int {
        MagFilter = 0x2800,
        MinFilter = 0x2801,
    }

    public enum TextureTarget : int {
        Texture2D = 0x0DE1,
    }
}