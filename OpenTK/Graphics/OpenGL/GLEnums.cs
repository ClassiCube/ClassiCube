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

    public enum ArbVertexBufferObject : int
    {
        BufferSizeArb = ((int)0x8764),
        BufferUsageArb = ((int)0x8765),
        ArrayBufferArb = ((int)0x8892),
        ElementArrayBufferArb = ((int)0x8893),
        ArrayBufferBindingArb = ((int)0x8894),
        ElementArrayBufferBindingArb = ((int)0x8895),
        VertexArrayBufferBindingArb = ((int)0x8896),

        ColorArrayBufferBindingArb = ((int)0x8898),

        TextureCoordArrayBufferBindingArb = ((int)0x889A),

        ReadOnlyArb = ((int)0x88B8),
        WriteOnlyArb = ((int)0x88B9),
        ReadWriteArb = ((int)0x88BA),
        BufferAccessArb = ((int)0x88BB),
        BufferMappedArb = ((int)0x88BC),
        BufferMapPointerArb = ((int)0x88BD),
        StreamDrawArb = ((int)0x88E0),
        StreamReadArb = ((int)0x88E1),
        StreamCopyArb = ((int)0x88E2),
        StaticDrawArb = ((int)0x88E4),
        StaticReadArb = ((int)0x88E5),
        StaticCopyArb = ((int)0x88E6),
        DynamicDrawArb = ((int)0x88E8),
        DynamicReadArb = ((int)0x88E9),
        DynamicCopyArb = ((int)0x88EA),
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

    public enum BlendEquationMode : int
    {
        FuncAdd = ((int)0x8006),
        Min = ((int)0x8007),
        Max = ((int)0x8008),
        FuncSubtract = ((int)0x800A),
        FuncReverseSubtract = ((int)0x800B),
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

    public enum BufferAccess : int
    {
        ReadOnly = ((int)0x88B8),
        WriteOnly = ((int)0x88B9),
        ReadWrite = ((int)0x88BA),
    }

    public enum BufferAccessArb : int
    {
        ReadOnly = ((int)0x88B8),
        WriteOnly = ((int)0x88B9),
        ReadWrite = ((int)0x88BA),
    }

    [Flags]
    public enum BufferAccessMask : int
    {
        MapReadBit = ((int)0x0001),
        MapWriteBit = ((int)0x0002),
        MapInvalidateRangeBit = ((int)0x0004),
        MapInvalidateBufferBit = ((int)0x0008),
        MapFlushExplicitBit = ((int)0x0010),
        MapUnsynchronizedBit = ((int)0x0020),
    }

    public enum BufferParameterName : int
    {
        BufferSize = ((int)0x8764),
        BufferUsage = ((int)0x8765),
        BufferAccess = ((int)0x88BB),
        BufferMapped = ((int)0x88BC),
    }

    public enum BufferParameterNameArb : int
    {
        BufferSize = ((int)0x8764),
        BufferUsage = ((int)0x8765),
        BufferAccess = ((int)0x88BB),
        BufferMapped = ((int)0x88BC),
    }

    public enum BufferPointer : int
    {
        BufferMapPointer = ((int)0x88BD),
    }

    public enum BufferPointerNameArb : int
    {
        BufferMapPointer = ((int)0x88BD),
    }

    public enum BufferTarget : int
    {
        ArrayBuffer = ((int)0x8892),
        ElementArrayBuffer = ((int)0x8893),
        PixelPackBuffer = ((int)0x88EB),
        PixelUnpackBuffer = ((int)0x88EC),
        UniformBuffer = ((int)0x8A11),
        TextureBuffer = ((int)0x8C2A),
        TransformFeedbackBuffer = ((int)0x8C8E),
        CopyReadBuffer = ((int)0x8F36),
        CopyWriteBuffer = ((int)0x8F37),
    }

    public enum BufferTargetArb : int
    {
        ArrayBuffer = ((int)0x8892),
        ElementArrayBuffer = ((int)0x8893),
    }

    public enum BufferUsageArb : int
    {
        StreamDraw = ((int)0x88E0),
        StreamRead = ((int)0x88E1),
        StreamCopy = ((int)0x88E2),
        StaticDraw = ((int)0x88E4),
        StaticRead = ((int)0x88E5),
        StaticCopy = ((int)0x88E6),
        DynamicDraw = ((int)0x88E8),
        DynamicRead = ((int)0x88E9),
        DynamicCopy = ((int)0x88EA),
    }

    public enum BufferUsageHint : int
    {
        StreamDraw = ((int)0x88E0),
        StreamRead = ((int)0x88E1),
        StreamCopy = ((int)0x88E2),
        StaticDraw = ((int)0x88E4),
        StaticRead = ((int)0x88E5),
        StaticCopy = ((int)0x88E6),
        DynamicDraw = ((int)0x88E8),
        DynamicRead = ((int)0x88E9),
        DynamicCopy = ((int)0x88EA),
    }

    [Flags]
    public enum ClearBufferMask : int
    {
        DepthBufferBit = ((int)0x00000100),
        AccumBufferBit = ((int)0x00000200),
        StencilBufferBit = ((int)0x00000400),
        ColorBufferBit = ((int)0x00004000),
    }

    public enum ColorMaterialFace : int
    {
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        FrontAndBack = ((int)0x0408),
    }

    public enum ColorMaterialParameter : int
    {
        Ambient = ((int)0x1200),
        Diffuse = ((int)0x1201),
        Specular = ((int)0x1202),
        Emission = ((int)0x1600),
        AmbientAndDiffuse = ((int)0x1602),
    }

    public enum ColorPointerType : int
    {
        Byte = ((int)0x1400),
        UnsignedByte = ((int)0x1401),
        Short = ((int)0x1402),
        UnsignedShort = ((int)0x1403),
        Int = ((int)0x1404),
        UnsignedInt = ((int)0x1405),
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

        AutoNormal = ((int)0x0D80),

        Texture2D = ((int)0x0DE1),
        PolygonOffsetPoint = ((int)0x2A01),
        PolygonOffsetLine = ((int)0x2A02),

        PolygonOffsetFill = ((int)0x8037),

        VertexArray = ((int)0x8074),
        NormalArray = ((int)0x8075),
        ColorArray = ((int)0x8076),
        IndexArray = ((int)0x8077),
        TextureCoordArray = ((int)0x8078),
       
        ColorSum = ((int)0x8458),
       
        DepthClamp = ((int)0x864F),

        RasterizerDiscard = ((int)0x8C89),
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
        InvalidFramebufferOperation = ((int)0x0506),
        InvalidFramebufferOperationExt = ((int)0x0506),
        TableTooLargeExt = ((int)0x8031),
        TextureTooLargeExt = ((int)0x8065),
    }

    public enum ExtBgra : int
    {
        BgrExt = ((int)0x80E0),
        BgraExt = ((int)0x80E1),
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

        ListMode = ((int)0x0B30),
        MaxListNesting = ((int)0x0B31),
        ListBase = ((int)0x0B32),
        ListIndex = ((int)0x0B33),
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

        AuxBuffers = ((int)0x0C00),

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

        AutoNormal = ((int)0x0D80),
       
        Texture2D = ((int)0x0DE1),

        PolygonOffsetUnits = ((int)0x2A00),
        PolygonOffsetPoint = ((int)0x2A01),
        PolygonOffsetLine = ((int)0x2A02),
        
        BlendColorExt = ((int)0x8005),
        BlendEquationExt = ((int)0x8009),
        BlendEquationRgb = ((int)0x8009),
       
        PolygonOffsetFill = ((int)0x8037),
        PolygonOffsetFactor = ((int)0x8038),
        PolygonOffsetBiasExt = ((int)0x8039),
        RescaleNormalExt = ((int)0x803A),

        TextureBinding2D = ((int)0x8069),

        VertexArray = ((int)0x8074),
        NormalArray = ((int)0x8075),
        ColorArray = ((int)0x8076),

        TextureCoordArray = ((int)0x8078),

        VertexArraySize = ((int)0x807A),
        VertexArrayType = ((int)0x807B),
        VertexArrayStride = ((int)0x807C),
        VertexArrayCountExt = ((int)0x807D),

        ColorArraySize = ((int)0x8081),
        ColorArrayType = ((int)0x8082),
        ColorArrayStride = ((int)0x8083),
        ColorArrayCountExt = ((int)0x8084),

        TextureCoordArraySize = ((int)0x8088),
        TextureCoordArrayType = ((int)0x8089),
        TextureCoordArrayStride = ((int)0x808A),
        TextureCoordArrayCountExt = ((int)0x808B),
       
        BlendDstRgb = ((int)0x80C8),
        BlendSrcRgb = ((int)0x80C9),
        BlendDstAlpha = ((int)0x80CA),
        BlendSrcAlpha = ((int)0x80CB),

     
        MajorVersion = ((int)0x821B),
        MinorVersion = ((int)0x821C),
        NumExtensions = ((int)0x821D),
        ContextFlags = ((int)0x821E),
        
        ColorSum = ((int)0x8458),
        
        TextureCompressionHint = ((int)0x84EF),
        TextureBindingRectangle = ((int)0x84F6),
        MaxRectangleTextureSize = ((int)0x84F8),
        MaxTextureLodBias = ((int)0x84FD),

        VertexArrayBinding = ((int)0x85B5),

        DepthClamp = ((int)0x864F),

        RgbaFloatMode = ((int)0x8820),
       
        BlendEquationAlpha = ((int)0x883D),

        PointSprite = ((int)0x8861),

        MaxTextureCoords = ((int)0x8871),

        ArrayBufferBinding = ((int)0x8894),
        ElementArrayBufferBinding = ((int)0x8895),
        VertexArrayBufferBinding = ((int)0x8896),
        NormalArrayBufferBinding = ((int)0x8897),
        ColorArrayBufferBinding = ((int)0x8898),
        IndexArrayBufferBinding = ((int)0x8899),
        TextureCoordArrayBufferBinding = ((int)0x889A),

        MaxArrayTextureLayers = ((int)0x88FF),

        MaxColorAttachments = ((int)0x8CDF),
        MaxColorAttachmentsExt = ((int)0x8CDF),
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
        TextureCompressionHint = ((int)0x84EF),
    }

    
    public enum InterleavedArrayFormat : int
    {
        V2f = ((int)0x2A20),
        V3f = ((int)0x2A21),
        C4ubV2f = ((int)0x2A22),
        C4ubV3f = ((int)0x2A23),
        C3fV3f = ((int)0x2A24),
        N3fV3f = ((int)0x2A25),
        C4fN3fV3f = ((int)0x2A26),
        T2fV3f = ((int)0x2A27),
        T4fV4f = ((int)0x2A28),
        T2fC4ubV3f = ((int)0x2A29),
        T2fC3fV3f = ((int)0x2A2A),
        T2fN3fV3f = ((int)0x2A2B),
        T2fC4fN3fV3f = ((int)0x2A2C),
        T4fC4fN3fV4f = ((int)0x2A2D),
    }

    public enum ListMode : int
    {
        Compile = ((int)0x1300),
        CompileAndExecute = ((int)0x1301),
    }

    public enum ListNameType : int
    {
        Byte = ((int)0x1400),
        UnsignedByte = ((int)0x1401),
        Short = ((int)0x1402),
        UnsignedShort = ((int)0x1403),
        Int = ((int)0x1404),
        UnsignedInt = ((int)0x1405),
        Float = ((int)0x1406),
        Gl2Bytes = ((int)0x1407),
        Gl3Bytes = ((int)0x1408),
        Gl4Bytes = ((int)0x1409),
    }

    public enum ListParameterName : int
    {
        ListPrioritySgix = ((int)0x8182),
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
        Red = ((int)0x1903),
        Green = ((int)0x1904),
        Blue = ((int)0x1905),
        Alpha = ((int)0x1906),
        Rgb = ((int)0x1907),
        Rgba = ((int)0x1908),

        Bgr = ((int)0x80E0),
        Bgra = ((int)0x80E1),

        RgbInteger = ((int)0x8D98),
        RgbaInteger = ((int)0x8D99),
        BgrInteger = ((int)0x8D9A),
        BgraInteger = ((int)0x8D9B),
    }

    public enum PixelInternalFormat : int
    {
        Rgb = ((int)0x1907),
        Rgba = ((int)0x1908),

        Rgb8 = ((int)0x8051),

        Rgb16 = ((int)0x8054),
        Rgba8 = ((int)0x8058),
        Rgba16 = ((int)0x805B),

        Rgba32ui = ((int)0x8D70),
        Rgb32ui = ((int)0x8D71),
        Rgba16ui = ((int)0x8D76),
        Rgb16ui = ((int)0x8D77),
        Rgba8ui = ((int)0x8D7C),
        Rgb8ui = ((int)0x8D7D),
        Rgba32i = ((int)0x8D82),
        Rgb32i = ((int)0x8D83),
        Rgba16i = ((int)0x8D88),
        Rgb16i = ((int)0x8D89),
        Rgba8i = ((int)0x8D8E),
        Rgb8i = ((int)0x8D8F),

        One = ((int)1),
        Two = ((int)2),
        Three = ((int)3),
        Four = ((int)4),
    }

    public enum PixelType : int
    {
        Byte = ((int)0x1400),
        UnsignedByte = ((int)0x1401),
        Short = ((int)0x1402),
        UnsignedShort = ((int)0x1403),
        Int = ((int)0x1404),
        UnsignedInt = ((int)0x1405),
        Float = ((int)0x1406),
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

    public enum TexCoordPointerType : int
    {
        Short = ((int)0x1402),
        Int = ((int)0x1404),
        Float = ((int)0x1406),
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

    public enum VertexPointerType : int
    {
        Short = ((int)0x1402),
        Int = ((int)0x1404),
        Float = ((int)0x1406),
    }
}
