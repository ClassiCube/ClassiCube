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

    public enum AccumOp : int
    {
        Accum = ((int)0x0100),
        Load = ((int)0x0101),
        Return = ((int)0x0102),
        Mult = ((int)0x0103),
        Add = ((int)0x0104),
    }

    public enum ActiveAttribType : int
    {
        Float = ((int)0x1406),
        FloatVec2 = ((int)0x8B50),
        FloatVec3 = ((int)0x8B51),
        FloatVec4 = ((int)0x8B52),
        FloatMat2 = ((int)0x8B5A),
        FloatMat3 = ((int)0x8B5B),
        FloatMat4 = ((int)0x8B5C),
    }

    public enum ActiveUniformBlockParameter : int
    {
        UniformBlockBinding = ((int)0x8A3F),
        UniformBlockDataSize = ((int)0x8A40),
        UniformBlockNameLength = ((int)0x8A41),
        UniformBlockActiveUniforms = ((int)0x8A42),
        UniformBlockActiveUniformIndices = ((int)0x8A43),
        UniformBlockReferencedByVertexShader = ((int)0x8A44),
        UniformBlockReferencedByFragmentShader = ((int)0x8A46),
    }

    public enum ActiveUniformParameter : int
    {
        UniformType = ((int)0x8A37),
        UniformSize = ((int)0x8A38),
        UniformNameLength = ((int)0x8A39),
        UniformBlockIndex = ((int)0x8A3A),
        UniformOffset = ((int)0x8A3B),
        UniformArrayStride = ((int)0x8A3C),
        UniformMatrixStride = ((int)0x8A3D),
        UniformIsRowMajor = ((int)0x8A3E),
    }

    public enum ActiveUniformType : int
    {
        Int = ((int)0x1404),
        Float = ((int)0x1406),
        FloatVec2 = ((int)0x8B50),
        FloatVec3 = ((int)0x8B51),
        FloatVec4 = ((int)0x8B52),
        IntVec2 = ((int)0x8B53),
        IntVec3 = ((int)0x8B54),
        IntVec4 = ((int)0x8B55),
        Bool = ((int)0x8B56),
        BoolVec2 = ((int)0x8B57),
        BoolVec3 = ((int)0x8B58),
        BoolVec4 = ((int)0x8B59),
        FloatMat2 = ((int)0x8B5A),
        FloatMat3 = ((int)0x8B5B),
        FloatMat4 = ((int)0x8B5C),
        Sampler1D = ((int)0x8B5D),
        Sampler2D = ((int)0x8B5E),
        Sampler3D = ((int)0x8B5F),
        SamplerCube = ((int)0x8B60),
        Sampler1DShadow = ((int)0x8B61),
        Sampler2DShadow = ((int)0x8B62),
        Sampler2DRect = ((int)0x8B63),
        Sampler2DRectShadow = ((int)0x8B64),
        FloatMat2x3 = ((int)0x8B65),
        FloatMat2x4 = ((int)0x8B66),
        FloatMat3x2 = ((int)0x8B67),
        FloatMat3x4 = ((int)0x8B68),
        FloatMat4x2 = ((int)0x8B69),
        FloatMat4x3 = ((int)0x8B6A),
        Sampler1DArray = ((int)0x8DC0),
        Sampler2DArray = ((int)0x8DC1),
        SamplerBuffer = ((int)0x8DC2),
        Sampler1DArrayShadow = ((int)0x8DC3),
        Sampler2DArrayShadow = ((int)0x8DC4),
        SamplerCubeShadow = ((int)0x8DC5),
        UnsignedIntVec2 = ((int)0x8DC6),
        UnsignedIntVec3 = ((int)0x8DC7),
        UnsignedIntVec4 = ((int)0x8DC8),
        IntSampler1D = ((int)0x8DC9),
        IntSampler2D = ((int)0x8DCA),
        IntSampler3D = ((int)0x8DCB),
        IntSamplerCube = ((int)0x8DCC),
        IntSampler2DRect = ((int)0x8DCD),
        IntSampler1DArray = ((int)0x8DCE),
        IntSampler2DArray = ((int)0x8DCF),
        IntSamplerBuffer = ((int)0x8DD0),
        UnsignedIntSampler1D = ((int)0x8DD1),
        UnsignedIntSampler2D = ((int)0x8DD2),
        UnsignedIntSampler3D = ((int)0x8DD3),
        UnsignedIntSamplerCube = ((int)0x8DD4),
        UnsignedIntSampler2DRect = ((int)0x8DD5),
        UnsignedIntSampler1DArray = ((int)0x8DD6),
        UnsignedIntSampler2DArray = ((int)0x8DD7),
        UnsignedIntSamplerBuffer = ((int)0x8DD8),
        Sampler2DMultisample = ((int)0x9108),
        IntSampler2DMultisample = ((int)0x9109),
        UnsignedIntSampler2DMultisample = ((int)0x910A),
        Sampler2DMultisampleArray = ((int)0x910B),
        IntSampler2DMultisampleArray = ((int)0x910C),
        UnsignedIntSampler2DMultisampleArray = ((int)0x910D),
    }

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
        NormalArray = ((int)0x8075),
        ColorArray = ((int)0x8076),
        IndexArray = ((int)0x8077),
        TextureCoordArray = ((int)0x8078),
    }

    [Flags]
    public enum AttribMask : int
    {
        CurrentBit = ((int)0x00000001),
        PointBit = ((int)0x00000002),
        LineBit = ((int)0x00000004),
        PolygonBit = ((int)0x00000008),
        PolygonStippleBit = ((int)0x00000010),
        PixelModeBit = ((int)0x00000020),
        LightingBit = ((int)0x00000040),
        FogBit = ((int)0x00000080),
        DepthBufferBit = ((int)0x00000100),
        AccumBufferBit = ((int)0x00000200),
        StencilBufferBit = ((int)0x00000400),
        ViewportBit = ((int)0x00000800),
        TransformBit = ((int)0x00001000),
        EnableBit = ((int)0x00002000),
        ColorBufferBit = ((int)0x00004000),
        HintBit = ((int)0x00008000),
        EvalBit = ((int)0x00010000),
        ListBit = ((int)0x00020000),
        TextureBit = ((int)0x00040000),
        ScissorBit = ((int)0x00080000),
        MultisampleBit = ((int)0x20000000),
        AllAttribBits = unchecked((int)0xFFFFFFFF),
    }

    public enum BeginFeedbackMode : int
    {
        Points = ((int)0x0000),
        Lines = ((int)0x0001),
        Triangles = ((int)0x0004),
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
        Polygon = ((int)0x0009),
        LinesAdjacency = ((int)0xA),
        LineStripAdjacency = ((int)0xB),
        TrianglesAdjacency = ((int)0xC),
        TriangleStripAdjacency = ((int)0xD),
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
        SrcColor = ((int)0x0300),
        OneMinusSrcColor = ((int)0x0301),
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
        Double = ((int)0x140A),
        HalfFloat = ((int)0x140B),
    }

    public enum CullFaceMode : int
    {
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        FrontAndBack = ((int)0x0408),
    }

    public enum DataType : int
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
        Double = ((int)0x140A),
        DoubleExt = ((int)0x140A),
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

    public enum DrawBufferMode : int
    {
        None = ((int)0),
        FrontLeft = ((int)0x0400),
        FrontRight = ((int)0x0401),
        BackLeft = ((int)0x0402),
        BackRight = ((int)0x0403),
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        Left = ((int)0x0406),
        Right = ((int)0x0407),
        FrontAndBack = ((int)0x0408),
        Aux0 = ((int)0x0409),
        Aux1 = ((int)0x040A),
        Aux2 = ((int)0x040B),
        Aux3 = ((int)0x040C),
        ColorAttachment0 = ((int)0x8CE0),
        ColorAttachment1 = ((int)0x8CE1),
        ColorAttachment2 = ((int)0x8CE2),
        ColorAttachment3 = ((int)0x8CE3),
        ColorAttachment4 = ((int)0x8CE4),
        ColorAttachment5 = ((int)0x8CE5),
        ColorAttachment6 = ((int)0x8CE6),
        ColorAttachment7 = ((int)0x8CE7),
        ColorAttachment8 = ((int)0x8CE8),
        ColorAttachment9 = ((int)0x8CE9),
        ColorAttachment10 = ((int)0x8CEA),
        ColorAttachment11 = ((int)0x8CEB),
        ColorAttachment12 = ((int)0x8CEC),
        ColorAttachment13 = ((int)0x8CED),
        ColorAttachment14 = ((int)0x8CEE),
        ColorAttachment15 = ((int)0x8CEF),
    }

    public enum DrawBuffersEnum : int
    {
        None = ((int)0),
        FrontLeft = ((int)0x0400),
        FrontRight = ((int)0x0401),
        BackLeft = ((int)0x0402),
        BackRight = ((int)0x0403),
        Aux0 = ((int)0x0409),
        Aux1 = ((int)0x040A),
        Aux2 = ((int)0x040B),
        Aux3 = ((int)0x040C),
        ColorAttachment0 = ((int)0x8CE0),
        ColorAttachment1 = ((int)0x8CE1),
        ColorAttachment2 = ((int)0x8CE2),
        ColorAttachment3 = ((int)0x8CE3),
        ColorAttachment4 = ((int)0x8CE4),
        ColorAttachment5 = ((int)0x8CE5),
        ColorAttachment6 = ((int)0x8CE6),
        ColorAttachment7 = ((int)0x8CE7),
        ColorAttachment8 = ((int)0x8CE8),
        ColorAttachment9 = ((int)0x8CE9),
        ColorAttachment10 = ((int)0x8CEA),
        ColorAttachment11 = ((int)0x8CEB),
        ColorAttachment12 = ((int)0x8CEC),
        ColorAttachment13 = ((int)0x8CED),
        ColorAttachment14 = ((int)0x8CEE),
        ColorAttachment15 = ((int)0x8CEF),
    }

    public enum DrawElementsType : int
    {
        UnsignedByte = ((int)0x1401),
        UnsignedShort = ((int)0x1403),
        UnsignedInt = ((int)0x1405),
    }

    public enum EnableCap : int
    {
        PointSmooth = ((int)0x0B10),
        LineSmooth = ((int)0x0B20),
        LineStipple = ((int)0x0B24),
        PolygonSmooth = ((int)0x0B41),
        PolygonStipple = ((int)0x0B42),
        CullFace = ((int)0x0B44),
        Lighting = ((int)0x0B50),
        ColorMaterial = ((int)0x0B57),
        Fog = ((int)0x0B60),
        DepthTest = ((int)0x0B71),
        StencilTest = ((int)0x0B90),
        Normalize = ((int)0x0BA1),
        AlphaTest = ((int)0x0BC0),
        Dither = ((int)0x0BD0),
        Blend = ((int)0x0BE2),
        IndexLogicOp = ((int)0x0BF1),
        ColorLogicOp = ((int)0x0BF2),
        ScissorTest = ((int)0x0C11),
        TextureGenS = ((int)0x0C60),
        TextureGenT = ((int)0x0C61),
        TextureGenR = ((int)0x0C62),
        TextureGenQ = ((int)0x0C63),
        AutoNormal = ((int)0x0D80),

        Texture1D = ((int)0x0DE0),
        Texture2D = ((int)0x0DE1),
        PolygonOffsetPoint = ((int)0x2A01),
        PolygonOffsetLine = ((int)0x2A02),

        PolygonOffsetFill = ((int)0x8037),
        RescaleNormal = ((int)0x803A),
        RescaleNormalExt = ((int)0x803A),
        Texture3DExt = ((int)0x806F),
        VertexArray = ((int)0x8074),
        NormalArray = ((int)0x8075),
        ColorArray = ((int)0x8076),
        IndexArray = ((int)0x8077),
        TextureCoordArray = ((int)0x8078),
        EdgeFlagArray = ((int)0x8079),

        FogCoordArray = ((int)0x8457),
        ColorSum = ((int)0x8458),
        SecondaryColorArray = ((int)0x845E),
        TextureCubeMap = ((int)0x8513),
        ProgramPointSize = ((int)0x8642),
        VertexProgramPointSize = ((int)0x8642),
        VertexProgramTwoSide = ((int)0x8643),
        DepthClamp = ((int)0x864F),
        TextureCubeMapSeamless = ((int)0x884F),
        PointSprite = ((int)0x8861),
        RasterizerDiscard = ((int)0x8C89),
        FramebufferSrgb = ((int)0x8DB9),
        SampleMask = ((int)0x8E51),
        PrimitiveRestart = ((int)0x8F9D),
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
    }

    public enum FogMode : int
    {
        Exp = ((int)0x0800),
        Exp2 = ((int)0x0801),
        Linear = ((int)0x2601),
        FogFuncSgis = ((int)0x812A),
        FogCoord = ((int)0x8451),
        FragmentDepth = ((int)0x8452),
    }

    public enum FogParameter : int
    {
        FogIndex = ((int)0x0B61),
        FogDensity = ((int)0x0B62),
        FogStart = ((int)0x0B63),
        FogEnd = ((int)0x0B64),
        FogMode = ((int)0x0B65),
        FogColor = ((int)0x0B66),
        FogOffsetValueSgix = ((int)0x8199),
        FogCoordSrc = ((int)0x8450),
    }

    public enum FogPointerType : int
    {
        Float = ((int)0x1406),
        Double = ((int)0x140A),
        HalfFloat = ((int)0x140B),
    }

    public enum FrontFaceDirection : int
    {
        Cw = ((int)0x0900),
        Ccw = ((int)0x0901),
    }

    public enum GenerateMipmapTarget : int
    {
        Texture1D = ((int)0x0DE0),
        Texture2D = ((int)0x0DE1),
        Texture3D = ((int)0x806F),
        TextureCubeMap = ((int)0x8513),
        Texture1DArray = ((int)0x8C18),
        Texture2DArray = ((int)0x8C1A),
        Texture2DMultisample = ((int)0x9100),
        Texture2DMultisampleArray = ((int)0x9102),
    }

    public enum GetPName : int
    {
        CurrentColor = ((int)0x0B00),
        CurrentIndex = ((int)0x0B01),
        CurrentNormal = ((int)0x0B02),
        CurrentTextureCoords = ((int)0x0B03),
        CurrentRasterColor = ((int)0x0B04),
        CurrentRasterIndex = ((int)0x0B05),
        CurrentRasterTextureCoords = ((int)0x0B06),
        CurrentRasterPosition = ((int)0x0B07),
        CurrentRasterPositionValid = ((int)0x0B08),
        CurrentRasterDistance = ((int)0x0B09),
        PointSmooth = ((int)0x0B10),
        PointSize = ((int)0x0B11),
        PointSizeRange = ((int)0x0B12),
        SmoothPointSizeRange = ((int)0x0B12),
        PointSizeGranularity = ((int)0x0B13),
        SmoothPointSizeGranularity = ((int)0x0B13),
        LineSmooth = ((int)0x0B20),
        LineWidth = ((int)0x0B21),
        LineWidthRange = ((int)0x0B22),
        SmoothLineWidthRange = ((int)0x0B22),
        LineWidthGranularity = ((int)0x0B23),
        SmoothLineWidthGranularity = ((int)0x0B23),
        LineStipple = ((int)0x0B24),
        LineStipplePattern = ((int)0x0B25),
        LineStippleRepeat = ((int)0x0B26),
        ListMode = ((int)0x0B30),
        MaxListNesting = ((int)0x0B31),
        ListBase = ((int)0x0B32),
        ListIndex = ((int)0x0B33),
        PolygonMode = ((int)0x0B40),
        PolygonSmooth = ((int)0x0B41),
        PolygonStipple = ((int)0x0B42),
        EdgeFlag = ((int)0x0B43),
        CullFace = ((int)0x0B44),
        CullFaceMode = ((int)0x0B45),
        FrontFace = ((int)0x0B46),
        Lighting = ((int)0x0B50),
        LightModelLocalViewer = ((int)0x0B51),
        LightModelTwoSide = ((int)0x0B52),
        LightModelAmbient = ((int)0x0B53),
        ShadeModel = ((int)0x0B54),
        ColorMaterialFace = ((int)0x0B55),
        ColorMaterialParameter = ((int)0x0B56),
        ColorMaterial = ((int)0x0B57),
        Fog = ((int)0x0B60),
        FogIndex = ((int)0x0B61),
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
        AccumClearValue = ((int)0x0B80),

        MatrixMode = ((int)0x0BA0),
        Normalize = ((int)0x0BA1),
        Viewport = ((int)0x0BA2),
        ModelviewStackDepth = ((int)0x0BA3),
        ProjectionStackDepth = ((int)0x0BA4),
        TextureStackDepth = ((int)0x0BA5),
        ModelviewMatrix = ((int)0x0BA6),
        ProjectionMatrix = ((int)0x0BA7),
        TextureMatrix = ((int)0x0BA8),
        AttribStackDepth = ((int)0x0BB0),
        ClientAttribStackDepth = ((int)0x0BB1),
        AlphaTest = ((int)0x0BC0),
        AlphaTestFunc = ((int)0x0BC1),
        AlphaTestRef = ((int)0x0BC2),
        Dither = ((int)0x0BD0),
        BlendDst = ((int)0x0BE0),
        BlendSrc = ((int)0x0BE1),
        Blend = ((int)0x0BE2),
        LogicOpMode = ((int)0x0BF0),
        IndexLogicOp = ((int)0x0BF1),
        LogicOp = ((int)0x0BF1),
        ColorLogicOp = ((int)0x0BF2),
        AuxBuffers = ((int)0x0C00),
        DrawBuffer = ((int)0x0C01),
        ReadBuffer = ((int)0x0C02),
        ScissorBox = ((int)0x0C10),
        ScissorTest = ((int)0x0C11),
        IndexClearValue = ((int)0x0C20),
        IndexWritemask = ((int)0x0C21),
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
        TextureGenS = ((int)0x0C60),
        TextureGenT = ((int)0x0C61),
        TextureGenR = ((int)0x0C62),
        TextureGenQ = ((int)0x0C63),
        PixelMapIToISize = ((int)0x0CB0),
        PixelMapSToSSize = ((int)0x0CB1),
        PixelMapIToRSize = ((int)0x0CB2),
        PixelMapIToGSize = ((int)0x0CB3),
        PixelMapIToBSize = ((int)0x0CB4),
        PixelMapIToASize = ((int)0x0CB5),
        PixelMapRToRSize = ((int)0x0CB6),
        PixelMapGToGSize = ((int)0x0CB7),
        PixelMapBToBSize = ((int)0x0CB8),
        PixelMapAToASize = ((int)0x0CB9),
        UnpackSwapBytes = ((int)0x0CF0),
        UnpackLsbFirst = ((int)0x0CF1),
        UnpackRowLength = ((int)0x0CF2),
        UnpackSkipRows = ((int)0x0CF3),
        UnpackSkipPixels = ((int)0x0CF4),
        UnpackAlignment = ((int)0x0CF5),
        PackSwapBytes = ((int)0x0D00),
        PackLsbFirst = ((int)0x0D01),
        PackRowLength = ((int)0x0D02),
        PackSkipRows = ((int)0x0D03),
        PackSkipPixels = ((int)0x0D04),
        PackAlignment = ((int)0x0D05),
        MapColor = ((int)0x0D10),
        MapStencil = ((int)0x0D11),
        IndexShift = ((int)0x0D12),
        IndexOffset = ((int)0x0D13),
        RedScale = ((int)0x0D14),
        RedBias = ((int)0x0D15),
        ZoomX = ((int)0x0D16),
        ZoomY = ((int)0x0D17),
        GreenScale = ((int)0x0D18),
        GreenBias = ((int)0x0D19),
        BlueScale = ((int)0x0D1A),
        BlueBias = ((int)0x0D1B),
        AlphaScale = ((int)0x0D1C),
        AlphaBias = ((int)0x0D1D),
        DepthScale = ((int)0x0D1E),
        DepthBias = ((int)0x0D1F),
        MaxEvalOrder = ((int)0x0D30),
        MaxLights = ((int)0x0D31),
        MaxClipPlanes = ((int)0x0D32),
        MaxTextureSize = ((int)0x0D33),
        MaxPixelMapTable = ((int)0x0D34),
        MaxAttribStackDepth = ((int)0x0D35),
        MaxModelviewStackDepth = ((int)0x0D36),
        MaxNameStackDepth = ((int)0x0D37),
        MaxProjectionStackDepth = ((int)0x0D38),
        MaxTextureStackDepth = ((int)0x0D39),
        MaxViewportDims = ((int)0x0D3A),
        MaxClientAttribStackDepth = ((int)0x0D3B),
        SubpixelBits = ((int)0x0D50),
        IndexBits = ((int)0x0D51),
        RedBits = ((int)0x0D52),
        GreenBits = ((int)0x0D53),
        BlueBits = ((int)0x0D54),
        AlphaBits = ((int)0x0D55),
        DepthBits = ((int)0x0D56),
        StencilBits = ((int)0x0D57),
        AccumRedBits = ((int)0x0D58),
        AccumGreenBits = ((int)0x0D59),
        AccumBlueBits = ((int)0x0D5A),
        AccumAlphaBits = ((int)0x0D5B),
        NameStackDepth = ((int)0x0D70),
        AutoNormal = ((int)0x0D80),
      
        Texture1D = ((int)0x0DE0),
        Texture2D = ((int)0x0DE1),
        FeedbackBufferSize = ((int)0x0DF1),
        FeedbackBufferType = ((int)0x0DF2),
        SelectionBufferSize = ((int)0x0DF4),
        PolygonOffsetUnits = ((int)0x2A00),
        PolygonOffsetPoint = ((int)0x2A01),
        PolygonOffsetLine = ((int)0x2A02),       
        
        PolygonOffsetFill = ((int)0x8037),
        PolygonOffsetFactor = ((int)0x8038),
        PolygonOffsetBiasExt = ((int)0x8039),
        RescaleNormalExt = ((int)0x803A),
        TextureBinding1D = ((int)0x8068),
        TextureBinding2D = ((int)0x8069),
        Texture3DBindingExt = ((int)0x806A),
        TextureBinding3D = ((int)0x806A),
        PackSkipImagesExt = ((int)0x806B),
        PackImageHeightExt = ((int)0x806C),
        UnpackSkipImagesExt = ((int)0x806D),
        UnpackImageHeightExt = ((int)0x806E),
        Texture3DExt = ((int)0x806F),
        Max3DTextureSize = ((int)0x8073),
        Max3DTextureSizeExt = ((int)0x8073),
           
        MajorVersion = ((int)0x821B),
        MinorVersion = ((int)0x821C),
        NumExtensions = ((int)0x821D),
        ContextFlags = ((int)0x821E),
        
        ActiveTexture = ((int)0x84E0),
        ClientActiveTexture = ((int)0x84E1),
        MaxTextureUnits = ((int)0x84E2),
        TransposeModelviewMatrix = ((int)0x84E3),
        TransposeProjectionMatrix = ((int)0x84E4),
        TransposeTextureMatrix = ((int)0x84E5),
        TransposeColorMatrix = ((int)0x84E6),
        MaxRenderbufferSize = ((int)0x84E8),
        MaxRenderbufferSizeExt = ((int)0x84E8),
        TextureCompressionHint = ((int)0x84EF),
        TextureBindingRectangle = ((int)0x84F6),
        MaxRectangleTextureSize = ((int)0x84F8),
        MaxTextureLodBias = ((int)0x84FD),
        TextureCubeMap = ((int)0x8513),
        TextureBindingCubeMap = ((int)0x8514),
        
        BlendEquationAlpha = ((int)0x883D),
        TextureCubeMapSeamless = ((int)0x884F),
        PointSprite = ((int)0x8861),
        MaxVertexAttribs = ((int)0x8869),
        MaxTextureCoords = ((int)0x8871),
        MaxTextureImageUnits = ((int)0x8872),
        ArrayBufferBinding = ((int)0x8894),
        ElementArrayBufferBinding = ((int)0x8895),
        VertexArrayBufferBinding = ((int)0x8896),
        NormalArrayBufferBinding = ((int)0x8897),
        ColorArrayBufferBinding = ((int)0x8898),
        IndexArrayBufferBinding = ((int)0x8899),
        TextureCoordArrayBufferBinding = ((int)0x889A),
        EdgeFlagArrayBufferBinding = ((int)0x889B),
        SecondaryColorArrayBufferBinding = ((int)0x889C),
        FogCoordArrayBufferBinding = ((int)0x889D),
        WeightArrayBufferBinding = ((int)0x889E),
        VertexAttribArrayBufferBinding = ((int)0x889F),
        PixelPackBufferBinding = ((int)0x88ED),
        PixelUnpackBufferBinding = ((int)0x88EF),
        MaxArrayTextureLayers = ((int)0x88FF),
        MinProgramTexelOffset = ((int)0x8904),
        MaxProgramTexelOffset = ((int)0x8905),
        ClampVertexColor = ((int)0x891A),
        ClampFragmentColor = ((int)0x891B),
        ClampReadColor = ((int)0x891C),
        MaxVertexUniformBlocks = ((int)0x8A2B),
        MaxGeometryUniformBlocks = ((int)0x8A2C),
        MaxFragmentUniformBlocks = ((int)0x8A2D),
        MaxCombinedUniformBlocks = ((int)0x8A2E),
        MaxUniformBufferBindings = ((int)0x8A2F),
        MaxUniformBlockSize = ((int)0x8A30),
        MaxCombinedVertexUniformComponents = ((int)0x8A31),
        MaxCombinedGeometryUniformComponents = ((int)0x8A32),
        MaxCombinedFragmentUniformComponents = ((int)0x8A33),
        UniformBufferOffsetAlignment = ((int)0x8A34),
        MaxFragmentUniformComponents = ((int)0x8B49),
        MaxVertexUniformComponents = ((int)0x8B4A),
        MaxVaryingComponents = ((int)0x8B4B),
        MaxVaryingFloats = ((int)0x8B4B),
        MaxVertexTextureImageUnits = ((int)0x8B4C),
        MaxCombinedTextureImageUnits = ((int)0x8B4D),
        FragmentShaderDerivativeHint = ((int)0x8B8B),
        CurrentProgram = ((int)0x8B8D),
        TextureBinding1DArray = ((int)0x8C1C),
        TextureBinding2DArray = ((int)0x8C1D),
        MaxGeometryTextureImageUnits = ((int)0x8C29),
        MaxTransformFeedbackSeparateComponents = ((int)0x8C80),
        MaxTransformFeedbackInterleavedComponents = ((int)0x8C8A),
        MaxTransformFeedbackSeparateAttribs = ((int)0x8C8B),
        StencilBackRef = ((int)0x8CA3),
        StencilBackValueMask = ((int)0x8CA4),
        StencilBackWritemask = ((int)0x8CA5),
        DrawFramebufferBinding = ((int)0x8CA6),
        FramebufferBinding = ((int)0x8CA6),
        FramebufferBindingExt = ((int)0x8CA6),
        RenderbufferBinding = ((int)0x8CA7),
        RenderbufferBindingExt = ((int)0x8CA7),
        ReadFramebufferBinding = ((int)0x8CAA),
        MaxColorAttachments = ((int)0x8CDF),
        MaxColorAttachmentsExt = ((int)0x8CDF),
        MaxSamples = ((int)0x8D57),
        FramebufferSrgb = ((int)0x8DB9),
        MaxGeometryVaryingComponents = ((int)0x8DDD),
        MaxVertexVaryingComponents = ((int)0x8DDE),
        MaxGeometryUniformComponents = ((int)0x8DDF),
        MaxGeometryOutputVertices = ((int)0x8DE0),
        MaxGeometryTotalOutputComponents = ((int)0x8DE1),
        QuadsFollowProvokingVertexConvention = ((int)0x8E4C),
        ProvokingVertex = ((int)0x8E4F),
        SampleMask = ((int)0x8E51),
        MaxSampleMaskWords = ((int)0x8E59),
        TextureBinding2DMultisample = ((int)0x9104),
        TextureBinding2DMultisampleArray = ((int)0x9105),
        MaxColorTextureSamples = ((int)0x910E),
        MaxDepthTextureSamples = ((int)0x910F),
        MaxIntegerSamples = ((int)0x9110),
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
        PackCmykHintExt = ((int)0x800E),
        UnpackCmykHintExt = ((int)0x800F),
        TextureMultiBufferHintSgix = ((int)0x812E),
        GenerateMipmapHint = ((int)0x8192),
        GenerateMipmapHintSgis = ((int)0x8192),
        ConvolutionHintSgix = ((int)0x8316),
        VertexPreclipHintSgix = ((int)0x83EF),
        TextureCompressionHint = ((int)0x84EF),
        FragmentShaderDerivativeHint = ((int)0x8B8B),
    }

 
    public enum LogicOp : int
    {
        Clear = ((int)0x1500),
        And = ((int)0x1501),
        AndReverse = ((int)0x1502),
        Copy = ((int)0x1503),
        AndInverted = ((int)0x1504),
        Noop = ((int)0x1505),
        Xor = ((int)0x1506),
        Or = ((int)0x1507),
        Nor = ((int)0x1508),
        Equiv = ((int)0x1509),
        Invert = ((int)0x150A),
        OrReverse = ((int)0x150B),
        CopyInverted = ((int)0x150C),
        OrInverted = ((int)0x150D),
        Nand = ((int)0x150E),
        Set = ((int)0x150F),
    }

    public enum MatrixMode : int
    {
        Modelview = ((int)0x1700),
        Projection = ((int)0x1701),
        Texture = ((int)0x1702),
        Color = ((int)0x1800),
    }

    public enum PixelCopyType : int
    {
        Color = ((int)0x1800),
        Depth = ((int)0x1801),
        Stencil = ((int)0x1802),
    }

    public enum PixelFormat : int
    {
        ColorIndex = ((int)0x1900),
        StencilIndex = ((int)0x1901),
        DepthComponent = ((int)0x1902),
        Red = ((int)0x1903),
        Green = ((int)0x1904),
        Blue = ((int)0x1905),
        Alpha = ((int)0x1906),
        Rgb = ((int)0x1907),
        Rgba = ((int)0x1908),
        Luminance = ((int)0x1909),
        LuminanceAlpha = ((int)0x190A),
        AbgrExt = ((int)0x8000),
        CmykExt = ((int)0x800C),
        CmykaExt = ((int)0x800D),
        Bgr = ((int)0x80E0),
        Bgra = ((int)0x80E1),
        Ycrcb422Sgix = ((int)0x81BB),
        Ycrcb444Sgix = ((int)0x81BC),
        Rg = ((int)0x8227),
        RgInteger = ((int)0x8228),
        DepthStencil = ((int)0x84F9),
        RedInteger = ((int)0x8D94),
        GreenInteger = ((int)0x8D95),
        BlueInteger = ((int)0x8D96),
        AlphaInteger = ((int)0x8D97),
        RgbInteger = ((int)0x8D98),
        RgbaInteger = ((int)0x8D99),
        BgrInteger = ((int)0x8D9A),
        BgraInteger = ((int)0x8D9B),
    }

    public enum PixelInternalFormat : int
    {
        DepthComponent = ((int)0x1902),
        Alpha = ((int)0x1906),
        Rgb = ((int)0x1907),
        Rgba = ((int)0x1908),
        Luminance = ((int)0x1909),
        LuminanceAlpha = ((int)0x190A),
        R3G3B2 = ((int)0x2A10),
        Alpha4 = ((int)0x803B),
        Alpha8 = ((int)0x803C),
        Alpha12 = ((int)0x803D),
        Alpha16 = ((int)0x803E),
        Luminance4 = ((int)0x803F),
        Luminance8 = ((int)0x8040),
        Luminance12 = ((int)0x8041),
        Luminance16 = ((int)0x8042),
        Luminance4Alpha4 = ((int)0x8043),
        Luminance6Alpha2 = ((int)0x8044),
        Luminance8Alpha8 = ((int)0x8045),
        Luminance12Alpha4 = ((int)0x8046),
        Luminance12Alpha12 = ((int)0x8047),
        Luminance16Alpha16 = ((int)0x8048),
        Intensity = ((int)0x8049),
        Intensity4 = ((int)0x804A),
        Intensity8 = ((int)0x804B),
        Intensity12 = ((int)0x804C),
        Intensity16 = ((int)0x804D),
        Rgb2Ext = ((int)0x804E),
        Rgb4 = ((int)0x804F),
        Rgb5 = ((int)0x8050),
        Rgb8 = ((int)0x8051),
        Rgb10 = ((int)0x8052),
        Rgb12 = ((int)0x8053),
        Rgb16 = ((int)0x8054),
        Rgba2 = ((int)0x8055),
        Rgba4 = ((int)0x8056),
        Rgb5A1 = ((int)0x8057),
        Rgba8 = ((int)0x8058),
        Rgb10A2 = ((int)0x8059),
        Rgba12 = ((int)0x805A),
        Rgba16 = ((int)0x805B),
        DualAlpha4Sgis = ((int)0x8110),
        DualAlpha8Sgis = ((int)0x8111),
        DualAlpha12Sgis = ((int)0x8112),
        DualAlpha16Sgis = ((int)0x8113),
        DualLuminance4Sgis = ((int)0x8114),
        DualLuminance8Sgis = ((int)0x8115),
        DualLuminance12Sgis = ((int)0x8116),
        DualLuminance16Sgis = ((int)0x8117),
        DualIntensity4Sgis = ((int)0x8118),
        DualIntensity8Sgis = ((int)0x8119),
        DualIntensity12Sgis = ((int)0x811A),
        DualIntensity16Sgis = ((int)0x811B),
        DualLuminanceAlpha4Sgis = ((int)0x811C),
        DualLuminanceAlpha8Sgis = ((int)0x811D),
        QuadAlpha4Sgis = ((int)0x811E),
        QuadAlpha8Sgis = ((int)0x811F),
        QuadLuminance4Sgis = ((int)0x8120),
        QuadLuminance8Sgis = ((int)0x8121),
        QuadIntensity4Sgis = ((int)0x8122),
        QuadIntensity8Sgis = ((int)0x8123),
        DepthComponent16 = ((int)0x81a5),
        DepthComponent16Sgix = ((int)0x81A5),
        DepthComponent24 = ((int)0x81a6),
        DepthComponent24Sgix = ((int)0x81A6),
        DepthComponent32 = ((int)0x81a7),
        DepthComponent32Sgix = ((int)0x81A7),
        CompressedRed = ((int)0x8225),
        CompressedRg = ((int)0x8226),
        R8 = ((int)0x8229),
        R16 = ((int)0x822A),
        Rg8 = ((int)0x822B),
        Rg16 = ((int)0x822C),
        R16f = ((int)0x822D),
        R32f = ((int)0x822E),
        Rg16f = ((int)0x822F),
        Rg32f = ((int)0x8230),
        R8i = ((int)0x8231),
        R8ui = ((int)0x8232),
        R16i = ((int)0x8233),
        R16ui = ((int)0x8234),
        R32i = ((int)0x8235),
        R32ui = ((int)0x8236),
        Rg8i = ((int)0x8237),
        Rg8ui = ((int)0x8238),
        Rg16i = ((int)0x8239),
        Rg16ui = ((int)0x823A),
        Rg32i = ((int)0x823B),
        Rg32ui = ((int)0x823C),
        CompressedRgbS3tcDxt1Ext = ((int)0x83F0),
        CompressedRgbaS3tcDxt1Ext = ((int)0x83F1),
        CompressedRgbaS3tcDxt3Ext = ((int)0x83F2),
        CompressedRgbaS3tcDxt5Ext = ((int)0x83F3),
        CompressedAlpha = ((int)0x84E9),
        CompressedLuminance = ((int)0x84EA),
        CompressedLuminanceAlpha = ((int)0x84EB),
        CompressedIntensity = ((int)0x84EC),
        CompressedRgb = ((int)0x84ED),
        CompressedRgba = ((int)0x84EE),
        DepthStencil = ((int)0x84F9),
        Rgba32f = ((int)0x8814),
        Rgb32f = ((int)0x8815),
        Rgba16f = ((int)0x881A),
        Rgb16f = ((int)0x881B),
        Depth24Stencil8 = ((int)0x88F0),
        R11fG11fB10f = ((int)0x8C3A),
        Rgb9E5 = ((int)0x8C3D),
        Srgb = ((int)0x8C40),
        Srgb8 = ((int)0x8C41),
        SrgbAlpha = ((int)0x8C42),
        Srgb8Alpha8 = ((int)0x8C43),
        SluminanceAlpha = ((int)0x8C44),
        Sluminance8Alpha8 = ((int)0x8C45),
        Sluminance = ((int)0x8C46),
        Sluminance8 = ((int)0x8C47),
        CompressedSrgb = ((int)0x8C48),
        CompressedSrgbAlpha = ((int)0x8C49),
        CompressedSluminance = ((int)0x8C4A),
        CompressedSluminanceAlpha = ((int)0x8C4B),
        CompressedSrgbS3tcDxt1Ext = ((int)0x8C4C),
        CompressedSrgbAlphaS3tcDxt1Ext = ((int)0x8C4D),
        CompressedSrgbAlphaS3tcDxt3Ext = ((int)0x8C4E),
        CompressedSrgbAlphaS3tcDxt5Ext = ((int)0x8C4F),
        DepthComponent32f = ((int)0x8CAC),
        Depth32fStencil8 = ((int)0x8CAD),
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
        Float32UnsignedInt248Rev = ((int)0x8DAD),
        CompressedRedRgtc1 = ((int)0x8DBB),
        CompressedSignedRedRgtc1 = ((int)0x8DBC),
        CompressedRgRgtc2 = ((int)0x8DBD),
        CompressedSignedRgRgtc2 = ((int)0x8DBE),
        One = ((int)1),
        Two = ((int)2),
        Three = ((int)3),
        Four = ((int)4),
    }

    public enum PixelStoreParameter : int
    {
        UnpackSwapBytes = ((int)0x0CF0),
        UnpackLsbFirst = ((int)0x0CF1),
        UnpackRowLength = ((int)0x0CF2),
        UnpackSkipRows = ((int)0x0CF3),
        UnpackSkipPixels = ((int)0x0CF4),
        UnpackAlignment = ((int)0x0CF5),
        PackSwapBytes = ((int)0x0D00),
        PackLsbFirst = ((int)0x0D01),
        PackRowLength = ((int)0x0D02),
        PackSkipRows = ((int)0x0D03),
        PackSkipPixels = ((int)0x0D04),
        PackAlignment = ((int)0x0D05),
        PackSkipImages = ((int)0x806B),
        PackSkipImagesExt = ((int)0x806B),
        PackImageHeight = ((int)0x806C),
        PackImageHeightExt = ((int)0x806C),
        UnpackSkipImages = ((int)0x806D),
        UnpackSkipImagesExt = ((int)0x806D),
        UnpackImageHeight = ((int)0x806E),
        UnpackImageHeightExt = ((int)0x806E),
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
        HalfFloat = ((int)0x140B),
        Bitmap = ((int)0x1A00),
        UnsignedByte332 = ((int)0x8032),
        UnsignedByte332Ext = ((int)0x8032),
        UnsignedShort4444 = ((int)0x8033),
        UnsignedShort4444Ext = ((int)0x8033),
        UnsignedShort5551 = ((int)0x8034),
        UnsignedShort5551Ext = ((int)0x8034),
        UnsignedInt8888 = ((int)0x8035),
        UnsignedInt8888Ext = ((int)0x8035),
        UnsignedInt1010102 = ((int)0x8036),
        UnsignedInt1010102Ext = ((int)0x8036),
        UnsignedByte233Reversed = ((int)0x8362),
        UnsignedShort565 = ((int)0x8363),
        UnsignedShort565Reversed = ((int)0x8364),
        UnsignedShort4444Reversed = ((int)0x8365),
        UnsignedShort1555Reversed = ((int)0x8366),
        UnsignedInt8888Reversed = ((int)0x8367),
        UnsignedInt2101010Reversed = ((int)0x8368),
        UnsignedInt248 = ((int)0x84FA),
        UnsignedInt10F11F11FRev = ((int)0x8C3B),
        UnsignedInt5999Rev = ((int)0x8C3E),
        Float32UnsignedInt248Rev = ((int)0x8DAD),
    }

    public enum PointParameterName : int
    {
        PointSizeMin = ((int)0x8126),
        PointSizeMax = ((int)0x8127),
        PointFadeThresholdSize = ((int)0x8128),
        PointDistanceAttenuation = ((int)0x8129),
        PointSpriteCoordOrigin = ((int)0x8CA0),
    }

    public enum PointParameterNameSgis : int
    {
        PointSizeMinSgis = ((int)0x8126),
        PointSizeMaxSgis = ((int)0x8127),
        PointFadeThresholdSizeSgis = ((int)0x8128),
        DistanceAttenuationSgis = ((int)0x8129),
    }

    public enum PointSpriteCoordOriginParameter : int
    {
        LowerLeft = ((int)0x8CA1),
        UpperLeft = ((int)0x8CA2),
    }

    public enum PolygonMode : int
    {
        Point = ((int)0x1B00),
        Line = ((int)0x1B01),
        Fill = ((int)0x1B02),
    }

    public enum ProgramParameter : int
    {
        ActiveUniformBlockMaxNameLength = ((int)0x8A35),
        ActiveUniformBlocks = ((int)0x8A36),
        DeleteStatus = ((int)0x8B80),
        LinkStatus = ((int)0x8B82),
        ValidateStatus = ((int)0x8B83),
        InfoLogLength = ((int)0x8B84),
        AttachedShaders = ((int)0x8B85),
        ActiveUniforms = ((int)0x8B86),
        ActiveUniformMaxLength = ((int)0x8B87),
        ActiveAttributes = ((int)0x8B89),
        ActiveAttributeMaxLength = ((int)0x8B8A),
        TransformFeedbackVaryingMaxLength = ((int)0x8C76),
        TransformFeedbackBufferMode = ((int)0x8C7F),
        TransformFeedbackVaryings = ((int)0x8C83),
        GeometryVerticesOut = ((int)0x8DDA),
        GeometryInputType = ((int)0x8DDB),
        GeometryOutputType = ((int)0x8DDC),
    }

    public enum ProvokingVertexMode : int
    {
        FirstVertexConvention = ((int)0x8E4D),
        LastVertexConvention = ((int)0x8E4E),
    }

    public enum QueryTarget : int
    {
        SamplesPassed = ((int)0x8914),
        PrimitivesGenerated = ((int)0x8C87),
        TransformFeedbackPrimitivesWritten = ((int)0x8C88),
    }

    public enum ReadBufferMode : int
    {
        FrontLeft = ((int)0x0400),
        FrontRight = ((int)0x0401),
        BackLeft = ((int)0x0402),
        BackRight = ((int)0x0403),
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        Left = ((int)0x0406),
        Right = ((int)0x0407),
        Aux0 = ((int)0x0409),
        Aux1 = ((int)0x040A),
        Aux2 = ((int)0x040B),
        Aux3 = ((int)0x040C),
        ColorAttachment0 = ((int)0x8CE0),
        ColorAttachment1 = ((int)0x8CE1),
        ColorAttachment2 = ((int)0x8CE2),
        ColorAttachment3 = ((int)0x8CE3),
        ColorAttachment4 = ((int)0x8CE4),
        ColorAttachment5 = ((int)0x8CE5),
        ColorAttachment6 = ((int)0x8CE6),
        ColorAttachment7 = ((int)0x8CE7),
        ColorAttachment8 = ((int)0x8CE8),
        ColorAttachment9 = ((int)0x8CE9),
        ColorAttachment10 = ((int)0x8CEA),
        ColorAttachment11 = ((int)0x8CEB),
        ColorAttachment12 = ((int)0x8CEC),
        ColorAttachment13 = ((int)0x8CED),
        ColorAttachment14 = ((int)0x8CEE),
        ColorAttachment15 = ((int)0x8CEF),
    }

    public enum RenderbufferParameterName : int
    {
        RenderbufferSamples = ((int)0x8CAB),
        RenderbufferWidth = ((int)0x8D42),
        RenderbufferWidthExt = ((int)0x8D42),
        RenderbufferHeight = ((int)0x8D43),
        RenderbufferHeightExt = ((int)0x8D43),
        RenderbufferInternalFormat = ((int)0x8D44),
        RenderbufferInternalFormatExt = ((int)0x8D44),
        RenderbufferRedSize = ((int)0x8D50),
        RenderbufferRedSizeExt = ((int)0x8D50),
        RenderbufferGreenSize = ((int)0x8D51),
        RenderbufferGreenSizeExt = ((int)0x8D51),
        RenderbufferBlueSize = ((int)0x8D52),
        RenderbufferBlueSizeExt = ((int)0x8D52),
        RenderbufferAlphaSize = ((int)0x8D53),
        RenderbufferAlphaSizeExt = ((int)0x8D53),
        RenderbufferDepthSize = ((int)0x8D54),
        RenderbufferDepthSizeExt = ((int)0x8D54),
        RenderbufferStencilSize = ((int)0x8D55),
        RenderbufferStencilSizeExt = ((int)0x8D55),
    }

    public enum RenderbufferStorage : int
    {
        R3G3B2 = ((int)0x2A10),
        Alpha4 = ((int)0x803B),
        Alpha8 = ((int)0x803C),
        Alpha12 = ((int)0x803D),
        Alpha16 = ((int)0x803E),
        Rgb4 = ((int)0x804F),
        Rgb5 = ((int)0x8050),
        Rgb8 = ((int)0x8051),
        Rgb10 = ((int)0x8052),
        Rgb12 = ((int)0x8053),
        Rgb16 = ((int)0x8054),
        Rgba2 = ((int)0x8055),
        Rgba4 = ((int)0x8056),
        Rgba8 = ((int)0x8058),
        Rgb10A2 = ((int)0x8059),
        Rgba12 = ((int)0x805A),
        Rgba16 = ((int)0x805B),
        DepthComponent16 = ((int)0x81a5),
        DepthComponent24 = ((int)0x81a6),
        DepthComponent32 = ((int)0x81a7),
        R8 = ((int)0x8229),
        R16 = ((int)0x822A),
        Rg8 = ((int)0x822B),
        Rg16 = ((int)0x822C),
        R16f = ((int)0x822D),
        R32f = ((int)0x822E),
        Rg16f = ((int)0x822F),
        Rg32f = ((int)0x8230),
        R8i = ((int)0x8231),
        R8ui = ((int)0x8232),
        R16i = ((int)0x8233),
        R16ui = ((int)0x8234),
        R32i = ((int)0x8235),
        R32ui = ((int)0x8236),
        Rg8i = ((int)0x8237),
        Rg8ui = ((int)0x8238),
        Rg16i = ((int)0x8239),
        Rg16ui = ((int)0x823A),
        Rg32i = ((int)0x823B),
        Rg32ui = ((int)0x823C),
        Rgba32f = ((int)0x8814),
        Rgb32f = ((int)0x8815),
        Rgba16f = ((int)0x881A),
        Rgb16f = ((int)0x881B),
        Depth24Stencil8 = ((int)0x88F0),
        R11fG11fB10f = ((int)0x8C3A),
        Rgb9E5 = ((int)0x8C3D),
        Srgb8 = ((int)0x8C41),
        Srgb8Alpha8 = ((int)0x8C43),
        DepthComponent32f = ((int)0x8CAC),
        Depth32fStencil8 = ((int)0x8CAD),
        StencilIndex1 = ((int)0x8D46),
        StencilIndex1Ext = ((int)0x8D46),
        StencilIndex4 = ((int)0x8D47),
        StencilIndex4Ext = ((int)0x8D47),
        StencilIndex8 = ((int)0x8D48),
        StencilIndex8Ext = ((int)0x8D48),
        StencilIndex16 = ((int)0x8D49),
        StencilIndex16Ext = ((int)0x8D49),
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
    }

    public enum RenderbufferTarget : int
    {
        Renderbuffer = ((int)0x8D41),
        RenderbufferExt = ((int)0x8D41),
    }

    public enum RenderingMode : int
    {
        Render = ((int)0x1C00),
        Feedback = ((int)0x1C01),
        Select = ((int)0x1C02),
    }

    public enum RendScreenCoordinates : int
    {
        ScreenCoordinatesRend = ((int)0x8490),
        InvertedScreenWRend = ((int)0x8491),
    }

    public enum S3S3tc : int
    {
        RgbS3tc = ((int)0x83A0),
        Rgb4S3tc = ((int)0x83A1),
        RgbaS3tc = ((int)0x83A2),
        Rgba4S3tc = ((int)0x83A3),
    }

    public enum SamplePatternSgis : int
    {
        Gl1PassSgis = ((int)0x80A1),
        Gl2Pass0Sgis = ((int)0x80A2),
        Gl2Pass1Sgis = ((int)0x80A3),
        Gl4Pass0Sgis = ((int)0x80A4),
        Gl4Pass1Sgis = ((int)0x80A5),
        Gl4Pass2Sgis = ((int)0x80A6),
        Gl4Pass3Sgis = ((int)0x80A7),
    }

    public enum SeparableTarget : int
    {
        Separable2D = ((int)0x8012),
    }

    public enum SeparableTargetExt : int
    {
        Separable2DExt = ((int)0x8012),
    }

  

    public enum ShaderParameter : int
    {
        ShaderType = ((int)0x8B4F),
        DeleteStatus = ((int)0x8B80),
        CompileStatus = ((int)0x8B81),
        InfoLogLength = ((int)0x8B84),
        ShaderSourceLength = ((int)0x8B88),
    }

    public enum ShaderType : int
    {
        FragmentShader = ((int)0x8B30),
        VertexShader = ((int)0x8B31),
        GeometryShader = ((int)0x8DD9),
        GeometryShaderExt = ((int)0x8DD9),
    }

    public enum ShadingModel : int
    {
        Flat = ((int)0x1D00),
        Smooth = ((int)0x1D01),
    }

    public enum StencilFace : int
    {
        Front = ((int)0x0404),
        Back = ((int)0x0405),
        FrontAndBack = ((int)0x0408),
    }

    public enum StencilFunction : int
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

    public enum StencilOp : int
    {
        Zero = ((int)0),
        Invert = ((int)0x150A),
        Keep = ((int)0x1E00),
        Replace = ((int)0x1E01),
        Incr = ((int)0x1E02),
        Decr = ((int)0x1E03),
        IncrWrap = ((int)0x8507),
        DecrWrap = ((int)0x8508),
    }

    public enum StringName : int
    {
        Vendor = ((int)0x1F00),
        Renderer = ((int)0x1F01),
        Version = ((int)0x1F02),
        Extensions = ((int)0x1F03),
        ShadingLanguageVersion = ((int)0x8B8C),
    }

    public enum SunConvolutionBorderModes : int
    {
        WrapBorderSun = ((int)0x81D4),
    }

    public enum SunGlobalAlpha : int
    {
        GlobalAlphaSun = ((int)0x81D9),
        GlobalAlphaFactorSun = ((int)0x81DA),
    }

    public enum SunMeshArray : int
    {
        QuadMeshSun = ((int)0x8614),
        TriangleMeshSun = ((int)0x8615),
    }

    public enum SunSliceAccum : int
    {
        SliceAccumSun = ((int)0x85CC),
    }

    public enum SunTriangleList : int
    {
        RestartSun = ((int)0x0001),
        ReplaceMiddleSun = ((int)0x0002),
        ReplaceOldestSun = ((int)0x0003),
        TriangleListSun = ((int)0x81D7),
        ReplacementCodeSun = ((int)0x81D8),
        ReplacementCodeArraySun = ((int)0x85C0),
        ReplacementCodeArrayTypeSun = ((int)0x85C1),
        ReplacementCodeArrayStrideSun = ((int)0x85C2),
        ReplacementCodeArrayPointerSun = ((int)0x85C3),
        R1uiV3fSun = ((int)0x85C4),
        R1uiC4ubV3fSun = ((int)0x85C5),
        R1uiC3fV3fSun = ((int)0x85C6),
        R1uiN3fV3fSun = ((int)0x85C7),
        R1uiC4fN3fV3fSun = ((int)0x85C8),
        R1uiT2fV3fSun = ((int)0x85C9),
        R1uiT2fN3fV3fSun = ((int)0x85CA),
        R1uiT2fC4fN3fV3fSun = ((int)0x85CB),
    }

    public enum SunVertex : int
    {
    }

    public enum SunxConstantData : int
    {
        UnpackConstantDataSunx = ((int)0x81D5),
        TextureConstantDataSunx = ((int)0x81D6),
    }

    public enum TexCoordPointerType : int
    {
        Short = ((int)0x1402),
        Int = ((int)0x1404),
        Float = ((int)0x1406),
        Double = ((int)0x140A),
        HalfFloat = ((int)0x140B),
    }

    public enum TextureBufferTarget : int
    {
        TextureBuffer = ((int)0x8C2A),
    }

    public enum TextureCompareMode : int
    {
        CompareRefToTexture = ((int)0x884E),
        CompareRToTexture = ((int)0x884E),
    }

    public enum TextureCoordName : int
    {
        S = ((int)0x2000),
        T = ((int)0x2001),
        R = ((int)0x2002),
        Q = ((int)0x2003),
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
        TextureBorderColor = ((int)0x1004),
        Red = ((int)0x1903),
        TextureMagFilter = ((int)0x2800),
        TextureMinFilter = ((int)0x2801),
        TextureWrapS = ((int)0x2802),
        TextureWrapT = ((int)0x2803),
        TexturePriority = ((int)0x8066),
        TextureDepth = ((int)0x8071),
        TextureWrapR = ((int)0x8072),
        TextureWrapRExt = ((int)0x8072),
        DetailTextureLevelSgis = ((int)0x809A),
        DetailTextureModeSgis = ((int)0x809B),
        ShadowAmbientSgix = ((int)0x80BF),
        TextureCompareFailValue = ((int)0x80BF),
        DualTextureSelectSgis = ((int)0x8124),
        QuadTextureSelectSgis = ((int)0x8125),
        ClampToBorder = ((int)0x812D),
        ClampToEdge = ((int)0x812F),
        TextureWrapQSgis = ((int)0x8137),
        TextureMinLod = ((int)0x813A),
        TextureMaxLod = ((int)0x813B),
        TextureBaseLevel = ((int)0x813C),
        TextureMaxLevel = ((int)0x813D),
        TextureClipmapCenterSgix = ((int)0x8171),
        TextureClipmapFrameSgix = ((int)0x8172),
        TextureClipmapOffsetSgix = ((int)0x8173),
        TextureClipmapVirtualDepthSgix = ((int)0x8174),
        TextureClipmapLodOffsetSgix = ((int)0x8175),
        TextureClipmapDepthSgix = ((int)0x8176),
        PostTextureFilterBiasSgix = ((int)0x8179),
        PostTextureFilterScaleSgix = ((int)0x817A),
        TextureLodBiasSSgix = ((int)0x818E),
        TextureLodBiasTSgix = ((int)0x818F),
        TextureLodBiasRSgix = ((int)0x8190),
        GenerateMipmap = ((int)0x8191),
        GenerateMipmapSgis = ((int)0x8191),
        TextureCompareSgix = ((int)0x819A),
        TextureCompareOperatorSgix = ((int)0x819B),
        TextureMaxClampSSgix = ((int)0x8369),
        TextureMaxClampTSgix = ((int)0x836A),
        TextureMaxClampRSgix = ((int)0x836B),
        TextureLodBias = ((int)0x8501),
        DepthTextureMode = ((int)0x884B),
        TextureCompareMode = ((int)0x884C),
        TextureCompareFunc = ((int)0x884D),
    }

    public enum TextureTarget : int
    {
        Texture1D = ((int)0x0DE0),
        Texture2D = ((int)0x0DE1),
        ProxyTexture1D = ((int)0x8063),
        ProxyTexture2D = ((int)0x8064),
        Texture3D = ((int)0x806F),
        ProxyTexture3D = ((int)0x8070),
        DetailTexture2DSgis = ((int)0x8095),
        Texture4DSgis = ((int)0x8134),
        ProxyTexture4DSgis = ((int)0x8135),
        TextureMinLod = ((int)0x813A),
        TextureMaxLod = ((int)0x813B),
        TextureBaseLevel = ((int)0x813C),
        TextureMaxLevel = ((int)0x813D),
        TextureRectangle = ((int)0x84F5),
        TextureRectangleArb = ((int)0x84F5),
        TextureRectangleNv = ((int)0x84F5),
        ProxyTextureRectangle = ((int)0x84F7),
        TextureCubeMap = ((int)0x8513),
        TextureBindingCubeMap = ((int)0x8514),
        TextureCubeMapPositiveX = ((int)0x8515),
        TextureCubeMapNegativeX = ((int)0x8516),
        TextureCubeMapPositiveY = ((int)0x8517),
        TextureCubeMapNegativeY = ((int)0x8518),
        TextureCubeMapPositiveZ = ((int)0x8519),
        TextureCubeMapNegativeZ = ((int)0x851A),
        ProxyTextureCubeMap = ((int)0x851B),
        Texture1DArray = ((int)0x8C18),
        ProxyTexture1DArray = ((int)0x8C19),
        Texture2DArray = ((int)0x8C1A),
        ProxyTexture2DArray = ((int)0x8C1B),
        TextureBuffer = ((int)0x8C2A),
        Texture2DMultisample = ((int)0x9100),
        ProxyTexture2DMultisample = ((int)0x9101),
        Texture2DMultisampleArray = ((int)0x9102),
        ProxyTexture2DMultisampleArray = ((int)0x9103),
    }

    public enum TextureTargetMultisample : int
    {
        Texture2DMultisample = ((int)0x9100),
        ProxyTexture2DMultisample = ((int)0x9101),
        Texture2DMultisampleArray = ((int)0x9102),
        ProxyTexture2DMultisampleArray = ((int)0x9103),
    }

    public enum TextureUnit : int
    {
        Texture0 = ((int)0x84C0),
        Texture1 = ((int)0x84C1),
        Texture2 = ((int)0x84C2),
        Texture3 = ((int)0x84C3),
        Texture4 = ((int)0x84C4),
        Texture5 = ((int)0x84C5),
        Texture6 = ((int)0x84C6),
        Texture7 = ((int)0x84C7),
        Texture8 = ((int)0x84C8),
        Texture9 = ((int)0x84C9),
        Texture10 = ((int)0x84CA),
        Texture11 = ((int)0x84CB),
        Texture12 = ((int)0x84CC),
        Texture13 = ((int)0x84CD),
        Texture14 = ((int)0x84CE),
        Texture15 = ((int)0x84CF),
        Texture16 = ((int)0x84D0),
        Texture17 = ((int)0x84D1),
        Texture18 = ((int)0x84D2),
        Texture19 = ((int)0x84D3),
        Texture20 = ((int)0x84D4),
        Texture21 = ((int)0x84D5),
        Texture22 = ((int)0x84D6),
        Texture23 = ((int)0x84D7),
        Texture24 = ((int)0x84D8),
        Texture25 = ((int)0x84D9),
        Texture26 = ((int)0x84DA),
        Texture27 = ((int)0x84DB),
        Texture28 = ((int)0x84DC),
        Texture29 = ((int)0x84DD),
        Texture30 = ((int)0x84DE),
        Texture31 = ((int)0x84DF),
    }

    public enum TextureWrapMode : int
    {
        Clamp = ((int)0x2900),
        Repeat = ((int)0x2901),
        ClampToBorder = ((int)0x812D),
        ClampToEdge = ((int)0x812F),
        MirroredRepeat = ((int)0x8370),
    }

    public enum TransformFeedbackMode : int
    {
        InterleavedAttribs = ((int)0x8C8C),
        SeparateAttribs = ((int)0x8C8D),
    }

    public enum VertexAttribIPointerType : int
    {
        Byte = ((int)0x1400),
        UnsignedByte = ((int)0x1401),
        Short = ((int)0x1402),
        UnsignedShort = ((int)0x1403),
        Int = ((int)0x1404),
        UnsignedInt = ((int)0x1405),
    }

    public enum VertexAttribParameter : int
    {
        ArrayEnabled = ((int)0x8622),
        ArraySize = ((int)0x8623),
        ArrayStride = ((int)0x8624),
        ArrayType = ((int)0x8625),
        CurrentVertexAttrib = ((int)0x8626),
        ArrayNormalized = ((int)0x886A),
        VertexAttribArrayInteger = ((int)0x88FD),
    }

    public enum VertexAttribPointerParameter : int
    {
        ArrayPointer = ((int)0x8645),
    }

    public enum VertexAttribPointerType : int
    {
        Byte = ((int)0x1400),
        UnsignedByte = ((int)0x1401),
        Short = ((int)0x1402),
        UnsignedShort = ((int)0x1403),
        Int = ((int)0x1404),
        UnsignedInt = ((int)0x1405),
        Float = ((int)0x1406),
    }

    public enum VertexPointerType : int
    {
        Short = ((int)0x1402),
        Int = ((int)0x1404),
        Float = ((int)0x1406),
    }
}
