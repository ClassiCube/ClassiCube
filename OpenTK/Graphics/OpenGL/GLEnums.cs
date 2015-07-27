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
    #pragma warning disable 1591

    public enum AlphaFunction : int
    {
        Never = ((int)0x0200),
        Less = ((int)0x0201),
        Equal = ((int)0x0202),
        Lequal = ((int)0x0203),
        Greater = ((int)0x0204),
        Notequal = ((int)0x0205),
        Gequal = ((int)0x0206),
        Always = ((int)0x0207),
    }

    public enum ArrayCap : int
    {
        VertexArray = ((int)0x8074),
        ColorArray = ((int)0x8076),
        TextureCoordArray = ((int)0x8078),
    }

    public enum BeginMode : int
    {
        Points = ((int)0x0000),
        Lines = ((int)0x0001),
        LineLoop = ((int)0x0002),
        LineStrip = ((int)0x0003),
        Triangles = ((int)0x0004),
        TriangleStrip = ((int)0x0005),
        TriangleFan = ((int)0x0006),
        Quads = ((int)0x0007),
        QuadStrip = ((int)0x0008),
    }

    public enum BlendingFactor : int
    {
        Zero = ((int)0),
        SrcAlpha = ((int)0x0302),
        OneMinusSrcAlpha = ((int)0x0303),
        DstAlpha = ((int)0x0304),
        OneMinusDstAlpha = ((int)0x0305),
        One = ((int)1),
    }

    public enum Boolean : int
    {
        False = ((int)0),
        True = ((int)1),
    }

    public enum BufferTarget : int
    {
        ArrayBuffer = ((int)0x8892),
        ElementArrayBuffer = ((int)0x8893),
    }

    public enum BufferUsageHint : int
    {
        StreamDraw = ((int)0x88E0),
        StaticDraw = ((int)0x88E4),
        DynamicDraw = ((int)0x88E8),
    }

    [Flags]
    public enum ClearBufferMask : int
    {
        DepthBufferBit = ((int)0x00000100),
        AccumBufferBit = ((int)0x00000200),
        StencilBufferBit = ((int)0x00000400),
        ColorBufferBit = ((int)0x00004000),
    }

    public enum PointerType : int
    {
        UnsignedByte = ((int)0x1401),
        Float = ((int)0x1406),
    }

    public enum CullFaceMode : int
    {
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        FrontAndBack = ((int)0x0408),
    }

    public enum DepthFunction : int
    {
        Never = ((int)0x0200),
        Less = ((int)0x0201),
        Equal = ((int)0x0202),
        Lequal = ((int)0x0203),
        Greater = ((int)0x0204),
        Notequal = ((int)0x0205),
        Gequal = ((int)0x0206),
        Always = ((int)0x0207),
    }

    public enum DrawElementsType : int
    {
        UnsignedByte = ((int)0x1401),
        UnsignedShort = ((int)0x1403),
        UnsignedInt = ((int)0x1405),
    }

    public enum EnableCap : int
    {
        CullFace = ((int)0x0B44),

        Fog = ((int)0x0B60),
        DepthTest = ((int)0x0B71),
        StencilTest = ((int)0x0B90),
        Normalize = ((int)0x0BA1),
        AlphaTest = ((int)0x0BC0),
        Dither = ((int)0x0BD0),
        Blend = ((int)0x0BE2),

        Texture2D = ((int)0x0DE1),

        VertexArray = ((int)0x8074),
        NormalArray = ((int)0x8075),
        ColorArray = ((int)0x8076),
        IndexArray = ((int)0x8077),
        TextureCoordArray = ((int)0x8078),
       
        DepthClamp = ((int)0x864F),
    }

    public enum ErrorCode : int
    {
        NoError = ((int)0),
        InvalidEnum = ((int)0x0500),
        InvalidValue = ((int)0x0501),
        InvalidOperation = ((int)0x0502),
        StackOverflow = ((int)0x0503),
        StackUnderflow = ((int)0x0504),
        OutOfMemory = ((int)0x0505),

        TableTooLargeExt = ((int)0x8031),
        TextureTooLargeExt = ((int)0x8065),
    }

    public enum ExtVertexArrayBgra : int
    {
        Bgra = ((int)0x80E1),
    }

    public enum FogMode : int
    {
        Exp = ((int)0x0800),
        Exp2 = ((int)0x0801),
        Linear = ((int)0x2601),
    }

    public enum FogParameter : int
    {
        FogDensity = ((int)0x0B62),
        FogStart = ((int)0x0B63),
        FogEnd = ((int)0x0B64),
        FogMode = ((int)0x0B65),
        FogColor = ((int)0x0B66),
    }

    public enum FrontFaceDirection : int
    {
        Cw = ((int)0x0900),
        Ccw = ((int)0x0901),
    }

    public enum GetPName : int
    {
        CurrentColor = ((int)0x0B00),

        PointSize = ((int)0x0B11),

        LineWidth = ((int)0x0B21),

        PolygonMode = ((int)0x0B40),
        PolygonSmooth = ((int)0x0B41),

        CullFace = ((int)0x0B44),
        CullFaceMode = ((int)0x0B45),
        FrontFace = ((int)0x0B46),

        ShadeModel = ((int)0x0B54),

        Fog = ((int)0x0B60),
        
        FogDensity = ((int)0x0B62),
        FogStart = ((int)0x0B63),
        FogEnd = ((int)0x0B64),
        FogMode = ((int)0x0B65),
        FogColor = ((int)0x0B66),
        DepthRange = ((int)0x0B70),
        DepthTest = ((int)0x0B71),
        DepthWritemask = ((int)0x0B72),
        DepthClearValue = ((int)0x0B73),
        DepthFunc = ((int)0x0B74),

        MatrixMode = ((int)0x0BA0),
        Normalize = ((int)0x0BA1),
        Viewport = ((int)0x0BA2),
        ModelviewStackDepth = ((int)0x0BA3),
        ProjectionStackDepth = ((int)0x0BA4),
        TextureStackDepth = ((int)0x0BA5),
        ModelviewMatrix = ((int)0x0BA6),
        ProjectionMatrix = ((int)0x0BA7),
        TextureMatrix = ((int)0x0BA8),

        AlphaTest = ((int)0x0BC0),
        AlphaTestFunc = ((int)0x0BC1),
        AlphaTestRef = ((int)0x0BC2),
        Dither = ((int)0x0BD0),
        BlendDst = ((int)0x0BE0),
        BlendSrc = ((int)0x0BE1),
        Blend = ((int)0x0BE2),

        ColorClearValue = ((int)0x0C22),
        ColorWritemask = ((int)0x0C23),
        IndexMode = ((int)0x0C30),
        RgbaMode = ((int)0x0C31),
        Doublebuffer = ((int)0x0C32),
        Stereo = ((int)0x0C33),
        RenderMode = ((int)0x0C40),
        PerspectiveCorrectionHint = ((int)0x0C50),
        PointSmoothHint = ((int)0x0C51),
        LineSmoothHint = ((int)0x0C52),
        PolygonSmoothHint = ((int)0x0C53),
        FogHint = ((int)0x0C54),
       
        UnpackAlignment = ((int)0x0CF5),

        PackAlignment = ((int)0x0D05),
       
        DepthScale = ((int)0x0D1E),
        DepthBias = ((int)0x0D1F),

        MaxTextureSize = ((int)0x0D33),

        MaxModelviewStackDepth = ((int)0x0D36),

        MaxProjectionStackDepth = ((int)0x0D38),
        MaxTextureStackDepth = ((int)0x0D39),
        MaxViewportDims = ((int)0x0D3A),

        SubpixelBits = ((int)0x0D50),
        IndexBits = ((int)0x0D51),
        RedBits = ((int)0x0D52),
        GreenBits = ((int)0x0D53),
        BlueBits = ((int)0x0D54),
        AlphaBits = ((int)0x0D55),
        DepthBits = ((int)0x0D56),
       
        Texture2D = ((int)0x0DE1),

        TextureBinding2D = ((int)0x8069),
        
        TextureBindingRectangle = ((int)0x84F6),
        MaxRectangleTextureSize = ((int)0x84F8),

        DepthClamp = ((int)0x864F),
    }

    public enum HintMode : int
    {
        DontCare = ((int)0x1100),
        Fastest = ((int)0x1101),
        Nicest = ((int)0x1102),
    }

    public enum HintTarget : int
    {
        PerspectiveCorrectionHint = ((int)0x0C50),
        PointSmoothHint = ((int)0x0C51),
        LineSmoothHint = ((int)0x0C52),
        PolygonSmoothHint = ((int)0x0C53),
        FogHint = ((int)0x0C54),

        GenerateMipmapHint = ((int)0x8192),
    }

    public enum MaterialFace : int
    {
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        FrontAndBack = ((int)0x0408),
    }

    public enum MatrixMode : int
    {
        Modelview = ((int)0x1700),
        Projection = ((int)0x1701),
        Texture = ((int)0x1702),
        Color = ((int)0x1800),
    }

    public enum PixelFormat : int
    {
        Rgba = ((int)0x1908),

        Bgra = ((int)0x80E1),
    }

    public enum PixelInternalFormat : int
    {
        Rgba = ((int)0x1908),

        Rgba8 = ((int)0x8058),
    }

    public enum PixelType : int
    {
        UnsignedByte = ((int)0x1401),
    }

    public enum PolygonMode : int
    {
        Point = ((int)0x1B00),
        Line = ((int)0x1B01),
        Fill = ((int)0x1B02),
    }

    public enum ShadingModel : int
    {
        Flat = ((int)0x1D00),
        Smooth = ((int)0x1D01),
    }

    public enum StringName : int
    {
        Vendor = ((int)0x1F00),
        Renderer = ((int)0x1F01),
        Version = ((int)0x1F02),
        Extensions = ((int)0x1F03),
    }

    public enum TextureMagFilter : int
    {
        Nearest = ((int)0x2600),
        Linear = ((int)0x2601),
    }

    public enum TextureMinFilter : int
    {
        Nearest = ((int)0x2600),
        Linear = ((int)0x2601),
        NearestMipmapNearest = ((int)0x2700),
        LinearMipmapNearest = ((int)0x2701),
        NearestMipmapLinear = ((int)0x2702),
        LinearMipmapLinear = ((int)0x2703),
    }

    public enum TextureParameterName : int
    {
        TextureMagFilter = ((int)0x2800),
        TextureMinFilter = ((int)0x2801),
        TextureWrapS = ((int)0x2802),
        TextureWrapT = ((int)0x2803),

        ClampToBorder = ((int)0x812D),
        ClampToEdge = ((int)0x812F),

        GenerateMipmap = ((int)0x8191),
    }

    public enum TextureTarget : int
    {
        Texture2D = ((int)0x0DE1),

        ProxyTexture2D = ((int)0x8064),
        
        TextureRectangle = ((int)0x84F5),

        ProxyTextureRectangle = ((int)0x84F7),
    }

    public enum TextureWrapMode : int
    {
        Clamp = ((int)0x2900),
        Repeat = ((int)0x2901),
        ClampToBorder = ((int)0x812D),
        ClampToEdge = ((int)0x812F),
    }
}
