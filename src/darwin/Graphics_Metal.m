#define CC_DYNAMIC_VBS_ARE_STATIC
#define CC_SCRATCH_VBS_ARE_DYNAMIC
#include "../Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_METAL
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"
#include <QuartzCore/CAMetalLayer.h>
#include <Metal/Metal.h>

static GfxResourceID white_square;
static id<MTLDevice>  gfx_device;
static CAMetalLayer*  gfx_layer;
static id<MTLLibrary> gfx_library;
static id<MTLTexture> gfx_depthTex;

static id<MTLCommandQueue> cmd_queue;
static id<MTLCommandBuffer> cmd_buf;
static id<MTLRenderCommandEncoder> ren_enc;

static void UpdateDirtyState(void);
static int dirty_bits;
static MTLScissorRect scissor_rect;
static MTLViewport viewport_rect;

#define DIRTY_SCISSOR  (1 << 0)
#define DIRTY_VIEWPORT (1 << 1)
#define DIRTY_PIPELINE (1 << 2)
#define DIRTY_DEPTH    (1 << 3)
#define DIRTY_MVP      (1 << 4)

// vertex shader attributes
#define VSHDR_ATTR_MVP_MATRIX 1
#define VSHDR_ATTR_TEX_OFFSET 2

// fragment shader attributes
#define FSHDR_ATTR_FOGCOLOR 1

static void Gfx_RestoreState(void) {
    InitDefaultResources();

    // 1x1 dummy white texture
    struct Bitmap bmp;
    BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
    Bitmap_Init(bmp, 1, 1, pixels);
    white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
    FreeDefaultResources();
    Gfx_DeleteTexture(&white_square);
}

static int ComputeMaxTextureSize(void) {
    // https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
    // TODO tvOS max
    // TODO: is this even right
#if defined CC_BUILD_IOS
    if ([gfx_device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily3_v1]) return 16384;
    if ([gfx_device supportsFeatureSet:MTLFeatureSet_iOS_GPUFamily1_v2]) return 8192;
    
#elif defined CC_BUILD_MACOS
    if ([gfx_device supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v1]) return 16384;
#else
    #error "Don't know how to get real max 2D size! Add it here"
#endif
    return 4096;
}

void Gfx_Create(void) {
    int size = ComputeMaxTextureSize();
    Gfx.MaxTexWidth  = size;
    Gfx.MaxTexHeight = size;
    Gfx.Created      = true;
    
    cmd_queue = [gfx_device newCommandQueue];
    dirty_bits = ~0; // set all states as dirty
}

void Gfx_Free(void) {
    Gfx_FreeState();
    // TODO: implement
}

void Gfx_InitForLayer(CAMetalLayer* layer) {
    gfx_layer  = [layer retain];
    gfx_device = MTLCreateSystemDefaultDevice();

    [layer setDevice:gfx_device];
    [layer setPixelFormat:MTLPixelFormatBGRA8Unorm];
    [layer setFramebufferOnly:YES];
    
    gfx_library = [gfx_device newDefaultLibrary]; // TODO: release
}


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
static cc_bool gfx_R = true, gfx_G = true, gfx_B = true, gfx_A = true;
static cc_bool gfx_depthTest, gfx_depthWrite;

void Gfx_SetFog(cc_bool enabled)    { }// TODO: implement
void Gfx_SetFogCol(PackedCol col)   { }// TODO: implement
void Gfx_SetFogDensity(float value) { }// TODO: implement
void Gfx_SetFogEnd(float value)     { }// TODO: implement
void Gfx_SetFogMode(FogFunc func)   { }// TODO: implement

void Gfx_SetFaceCulling(cc_bool enabled) {
    [ren_enc setCullMode:enabled ? MTLCullModeFront : MTLCullModeNone];
}

static void SetAlphaTest(cc_bool enabled)  { dirty_bits |= DIRTY_PIPELINE; }
static void SetAlphaBlend(cc_bool enabled) { dirty_bits |= DIRTY_PIPELINE; }

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearBuffers(GfxBuffers buffers) {
    // TODO: implement
}

static float clearR, clearG, clearB;
void Gfx_ClearColor(PackedCol color) {
    clearR = PackedCol_R(color) / 255.0f;
    clearG = PackedCol_G(color) / 255.0f;
    clearB = PackedCol_B(color) / 255.0f;
}

void Gfx_SetDepthTest(cc_bool enabled) {
    gfx_depthTest = enabled;
    dirty_bits |= DIRTY_DEPTH;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
    gfx_depthWrite = enabled;
    dirty_bits |= DIRTY_DEPTH;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
    gfx_R = r; gfx_G = g; gfx_B = b; gfx_A = a;
    dirty_bits |= DIRTY_PIPELINE;
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
    cc_bool enabled = !depthOnly;
    SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1],
                  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
}


/*########################################################################################################################*
*---------------------------------------------------------Depth state-----------------------------------------------------*
*#########################################################################################################################*/
#define DEPTHSTATE_FLAG_DEPTH_TEST  (1 << 0)
#define DEPTHSTATE_FLAG_DEPTH_WRITE (1 << 1)

#define DEPTHSTATE_STATES_COUNT (2 * DEPTHSTATE_FLAG_DEPTH_WRITE)
static id<MTLDepthStencilState> depthStates[DEPTHSTATE_STATES_COUNT];

static void DepthState_Build(int idx) {
    int depthTest  = (idx & DEPTHSTATE_FLAG_DEPTH_TEST);
    int depthWrite = (idx & DEPTHSTATE_FLAG_DEPTH_WRITE);
    
    MTLDepthStencilDescriptor* desc = [[MTLDepthStencilDescriptor alloc] init];
    desc.depthWriteEnabled    = depthWrite ? YES : NO;
    desc.depthCompareFunction = depthTest ? MTLCompareFunctionLessEqual : MTLCompareFunctionAlways;
    
    id<MTLDepthStencilState> dstate = [gfx_device newDepthStencilStateWithDescriptor:desc];
    if (dstate == nil) Process_Abort("depth state failure"); // TODO: log error
    
    [desc autorelease];
    depthStates[idx] = dstate;
}

static void DepthState_Update(void) {
    int idx =
    (gfx_depthTest  ? DEPTHSTATE_FLAG_DEPTH_TEST  : 0) |
    (gfx_depthWrite ? DEPTHSTATE_FLAG_DEPTH_WRITE : 0);
    
    if (depthStates[idx] == nil) DepthState_Build(idx);
    [ren_enc setDepthStencilState:depthStates[idx]];
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void UploadTexture(id<MTLTexture> dst, struct Bitmap* src, int x, int y, int rowWidth) {
    id<MTLCommandBuffer> buf  = [cmd_queue commandBuffer];
    if (buf == nil) Process_Abort("no temp command buffer");
    
    int size = rowWidth * src->height * 4;
    id<MTLBuffer> tmp = [gfx_device newBufferWithBytes:src->scan0 length:size options:MTLResourceStorageModeShared];
    if (tmp == nil) Process_Abort("no mem for temp blit buffer");
    
    id<MTLBlitCommandEncoder> enc = [buf blitCommandEncoder];
    if (enc == nil) Process_Abort("no temp encoder");
    
    [enc copyFromBuffer:tmp sourceOffset:0 sourceBytesPerRow:rowWidth*4 sourceBytesPerImage:0 sourceSize:MTLSizeMake(src->width, src->height, 1) toTexture:dst destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake(x, y, 0)];
    [enc endEncoding];
    
    [buf addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull buffer) {
        [tmp autorelease];
    }];
    [buf commit];
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
    // TODO: implement
    // TODO: mipmaps
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                    width:bmp->width height:bmp->height mipmapped:NO];
    if (desc == nil) return NULL;
    
    desc.storageMode = MTLStorageModePrivate; // TODO: needed?
    id<MTLTexture> tex = [gfx_device newTextureWithDescriptor:desc];
    if (tex == NULL) return NULL;
    
    UploadTexture(tex, bmp, 0, 0, rowWidth);
    return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
    id<MTLTexture> tex = (id<MTLTexture>)texId;
    UploadTexture(tex, part, x, y, rowWidth);
}

void Gfx_EnableMipmaps(void)  { }// TODO: implement
void Gfx_DisableMipmaps(void) { }// TODO: implement

void Gfx_BindTexture(GfxResourceID texId) {
    if (!texId) texId = white_square;
    id<MTLTexture> tex = (id<MTLTexture>)texId;
    
    [ren_enc setFragmentTexture:tex atIndex:0];
}
        
void Gfx_DeleteTexture(GfxResourceID* texId) {
    id<MTLTexture> tex = (id<MTLTexture>)(*texId);
    if (tex) { [tex autorelease]; }
    *texId = NULL;
}



/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static void BlitBuffer(void* src, int size, id<MTLBuffer> dst) {
    id<MTLCommandBuffer> buf  = [cmd_queue commandBuffer];
    if (buf == nil) Process_Abort("no temp command buffer");
    
    id<MTLBuffer> tmp = [gfx_device newBufferWithBytes:src length:size options:MTLResourceStorageModeShared];
    if (tmp == nil) Process_Abort("no mem for temp blit buffer");
    
    id<MTLBlitCommandEncoder> enc = [buf blitCommandEncoder];
    if (enc == nil) Process_Abort("no temp encoder");
    
    [enc copyFromBuffer:tmp sourceOffset:0 toBuffer:dst destinationOffset:0 size:size];
    [enc endEncoding];
    
    [buf addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull buffer) {
        [tmp autorelease];
    }];
    [buf commit];
}

static void DeleteBuffer(GfxResourceID* obj) {
    id<MTLBuffer> buf = (id<MTLBuffer>)(*obj);
    if (buf) { [buf autorelease]; }
    *obj = NULL;
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static id<MTLBuffer> gfx_IB;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
    cc_uint16 indices[GFX_MAX_INDICES];
    fillFunc(indices, count, obj);
    int size = count * 2;
    
    id<MTLBuffer> buf = [gfx_device newBufferWithLength:size options:MTLResourceStorageModePrivate];
    if (buf == nil) return 0;
    
    BlitBuffer(indices, size, buf);
    return buf;
}

void Gfx_BindIb(GfxResourceID ib) { gfx_IB = ib; }

void Gfx_DeleteIb(GfxResourceID* ib) { DeleteBuffer(ib); }


/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
static int gfx_vOffset;

// TODO: use this instead of index buffer offset. after scratch VBs
static void SetVertexOffset(int offset) {
    if (gfx_vOffset == offset) return;
    gfx_vOffset = offset;
    [ren_enc setVertexBufferOffset:offset atIndex:0];
}

static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
    int size = count * strideSizes[fmt];
    return [gfx_device newBufferWithLength:size options:MTLResourceStorageModePrivate];
}

void Gfx_BindVb(GfxResourceID vb) {
    id<MTLBuffer> buf = (id<MTLBuffer>)vb;
    [ren_enc setVertexBuffer:buf offset:0 atIndex:0];
    gfx_vOffset = 0;
}

void Gfx_DeleteVb(GfxResourceID* vb) { DeleteBuffer(vb); }

static void* tmpPtr;
static int tmpSize;
void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
    tmpPtr = Mem_Alloc(count, strideSizes[fmt], "staging VB memory");
    tmpSize = count * strideSizes[fmt];
    return tmpPtr;
}

void Gfx_UnlockVb(GfxResourceID vb) {
    BlitBuffer(tmpPtr, tmpSize, (id<MTLBuffer>)vb);
    Gfx_BindVb(vb);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, _mvp;
static cc_bool texOffseting;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
    if (type == MATRIX_VIEW) _view = *matrix;
    if (type == MATRIX_PROJ) _proj = *matrix;

    Matrix_Mul(&_mvp, &_view, &_proj);
    dirty_bits |= DIRTY_MVP;
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
    _view = *view;
    _proj = *proj;

    Matrix_Mul(mvp, view, proj);
    Mem_Copy(&_mvp, mvp, sizeof(struct Matrix));
    dirty_bits |= DIRTY_MVP;
}

void Gfx_EnableTextureOffset(float x, float y) {
    Vec2 texOffset = { x, y };
    texOffseting = true;
    dirty_bits |= DIRTY_PIPELINE;
    
    [ren_enc setVertexBytes:&texOffset length:8 atIndex:VSHDR_ATTR_TEX_OFFSET];
}

void Gfx_DisableTextureOffset(void) {
    // TODO: implement
    texOffseting = false;
    dirty_bits |= DIRTY_PIPELINE;
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
    /* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
    /*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
    /* NOTE: This calculation is shared with Direct3D 11 backend */
    *matrix = Matrix_Identity;

    matrix->row1.x =  2.0f / width;
    matrix->row2.y = -2.0f / height;
    matrix->row3.z =  1.0f / (zNear - zFar);

    matrix->row4.x = -1.0f;
    matrix->row4.y =  1.0f;
    matrix->row4.z = zNear / (zNear - zFar);
    // TODO: implement
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
    float zNear = 0.1f;

    /* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
    /* NOTE: This calculation is shared with Direct3D 11 backend */
    float c = Cotangent(0.5f * fov);
    *matrix = Matrix_Identity;

    // TODO: is this right without swapped Znear/zfar?
    matrix->row1.x =  c / aspect;
    matrix->row2.y =  c;
    matrix->row3.z = zFar / (zNear - zFar);
    matrix->row3.w = -1.0f;
    matrix->row4.z = (zNear * zFar) / (zNear - zFar);
    matrix->row4.w =  0.0f;
    // TODO: implement
}


/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
    gfx_format = fmt;
    gfx_stride = strideSizes[fmt];
    dirty_bits |= DIRTY_PIPELINE;
}

void Gfx_DrawVb_Lines(int verticesCount) {
    if (dirty_bits) UpdateDirtyState();
    // TODO: implement
    [ren_enc drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:verticesCount];
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
    if (dirty_bits) UpdateDirtyState();
    // TODO: implement
    [ren_enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:ICOUNT(verticesCount) indexType:MTLIndexTypeUInt16
                       indexBuffer:gfx_IB indexBufferOffset:ICOUNT(startVertex)*sizeof(ushort)];
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
    if (dirty_bits) UpdateDirtyState();
    // TODO: implement
    [ren_enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:ICOUNT(verticesCount) indexType:MTLIndexTypeUInt16
                       indexBuffer:gfx_IB indexBufferOffset:0];
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
    if (dirty_bits) UpdateDirtyState();
    // TODO: implement
    [ren_enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:ICOUNT(verticesCount) indexType:MTLIndexTypeUInt16
                       indexBuffer:gfx_IB indexBufferOffset:ICOUNT(startVertex)*sizeof(ushort)];
}


/*########################################################################################################################*
*----------------------------------------------------------Pipelines------------------------------------------------------*
*#########################################################################################################################*/
#define PIPELINE_FLAG_TEXTURED    (1 << 0)
#define PIPELINE_FLAG_TEXOFFSET   (1 << 1)
#define PIPELINE_FLAG_ALPHA_TEST  (1 << 2)
#define PIPELINE_FLAG_ALPHA_BLEND (1 << 3)
#define PIPELINE_FLAG_R_WRITE     (1 << 4)
#define PIPELINE_FLAG_G_WRITE     (1 << 5)
#define PIPELINE_FLAG_B_WRITE     (1 << 6)
#define PIPELINE_FLAG_A_WRITE     (1 << 7)

#define PIPELINE_STATES_COUNT (2 * PIPELINE_FLAG_A_WRITE)
static id<MTLRenderPipelineState> pipelines[PIPELINE_STATES_COUNT];

static void Pipelines_FillVertexDeclaration(MTLVertexDescriptor* desc, VertexFormat fmt) {
    desc.attributes[0].format      = MTLVertexFormatFloat3;
    desc.attributes[0].offset      = 0;
    desc.attributes[0].bufferIndex = 0;

    desc.attributes[1].format      = MTLVertexFormatUChar4Normalized;
    desc.attributes[1].offset      = 12;
    desc.attributes[1].bufferIndex = 0;
    
    if (fmt == VERTEX_FORMAT_TEXTURED) {
        desc.attributes[2].format      = MTLVertexFormatFloat2;
        desc.attributes[2].offset      = 16;
        desc.attributes[2].bufferIndex = 0;
    }

    desc.layouts[0].stride       = fmt == VERTEX_FORMAT_TEXTURED ? SIZEOF_VERTEX_TEXTURED : SIZEOF_VERTEX_COLOURED;
    desc.layouts[0].stepRate     = 1;
    desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
}

static void Pipeline_Build(int idx) {
    VertexFormat fmt = (idx & PIPELINE_FLAG_TEXTURED) ? VERTEX_FORMAT_TEXTURED : VERTEX_FORMAT_COLOURED;
    int texOffset    = (idx & PIPELINE_FLAG_TEXOFFSET);
    int alphaTest    = (idx & PIPELINE_FLAG_ALPHA_TEST);
    int alphaBlend   = (idx & PIPELINE_FLAG_ALPHA_BLEND);
    
    MTLVertexDescriptor* vdesc = [MTLVertexDescriptor vertexDescriptor];
    Pipelines_FillVertexDeclaration(vdesc, fmt);
    
    NSString* vfunc = texOffset ? (fmt == VERTEX_FORMAT_TEXTURED ? @"vertex_textured_main_offset" : @"vertex_coloured_main")
                                : (fmt == VERTEX_FORMAT_TEXTURED ? @"vertex_textured_main"        : @"vertex_coloured_main");
    NSString* ffunc = alphaTest ? (fmt == VERTEX_FORMAT_TEXTURED ? @"fragment_textured_main_at" : @"fragment_coloured_main_at")
                                : (fmt == VERTEX_FORMAT_TEXTURED ? @"fragment_textured_main"    : @"fragment_coloured_main");
    
    MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
    MTLRenderPipelineColorAttachmentDescriptor* fb = desc.colorAttachments[0];
    
    desc.vertexDescriptor = vdesc;
    desc.vertexFunction   = [gfx_library newFunctionWithName:vfunc];
    desc.fragmentFunction = [gfx_library newFunctionWithName:ffunc];
    fb.pixelFormat = MTLPixelFormatBGRA8Unorm; // TODO: nil frag on depth only pass
    
    fb.blendingEnabled             = alphaBlend ? YES : NO;
    fb.sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
    fb.sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
    fb.destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
    fb.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    fb.writeMask =
    (idx & PIPELINE_FLAG_R_WRITE ? MTLColorWriteMaskRed   : 0) |
    (idx & PIPELINE_FLAG_G_WRITE ? MTLColorWriteMaskGreen : 0) |
    (idx & PIPELINE_FLAG_B_WRITE ? MTLColorWriteMaskBlue  : 0) |
    (idx & PIPELINE_FLAG_A_WRITE ? MTLColorWriteMaskAlpha : 0);
    if (fb.writeMask == MTLColorWriteMaskNone) desc.fragmentFunction = nil;
    
    desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    NSError* err = nil;
    id<MTLRenderPipelineState> pso = [gfx_device newRenderPipelineStateWithDescriptor:desc error:&err];
    if (pso == nil) Process_Abort("pipeline failure"); // TODO: log error
    
    [desc autorelease];
    pipelines[idx] = pso;
}

static void Pipeline_Update(void) {
    int idx =
    (gfx_format == VERTEX_FORMAT_TEXTURED ? PIPELINE_FLAG_TEXTURED : 0) |
    (texOffseting   ? PIPELINE_FLAG_TEXOFFSET   : 0) |
    (gfx_alphaTest  ? PIPELINE_FLAG_ALPHA_TEST  : 0) |
    (gfx_alphaBlend ? PIPELINE_FLAG_ALPHA_BLEND : 0) |
    (gfx_R          ? PIPELINE_FLAG_R_WRITE     : 0) |
    (gfx_G          ? PIPELINE_FLAG_G_WRITE     : 0) |
    (gfx_B          ? PIPELINE_FLAG_B_WRITE     : 0) |
    (gfx_A          ? PIPELINE_FLAG_A_WRITE     : 0);
    
    if (pipelines[idx] == nil) Pipeline_Build(idx);
    [ren_enc setRenderPipelineState:pipelines[idx]];
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
static void UpdateDirtyState(void) {
    if (dirty_bits & DIRTY_SCISSOR) {
        [ren_enc setScissorRect:scissor_rect];
    }
    if (dirty_bits & DIRTY_VIEWPORT) {
        [ren_enc setViewport:viewport_rect];
    }
    if (dirty_bits & DIRTY_DEPTH) {
        DepthState_Update();
    }
    if (dirty_bits & DIRTY_MVP) {
        [ren_enc setVertexBytes:&_mvp length:sizeof(struct Matrix) atIndex:VSHDR_ATTR_MVP_MATRIX];
    }
    if (dirty_bits & DIRTY_PIPELINE) {
        Pipeline_Update();
    }
    dirty_bits = 0;
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
    return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

static id<CAMetalDrawable> drawable;
void Gfx_BeginFrame(void) {
    // TODO: implement
    cmd_buf  = [[cmd_queue commandBuffer] retain];
    drawable = [gfx_layer nextDrawable];
    if (drawable == nil) Process_Abort("No metal drawable");
    
    MTLRenderPassDescriptor* desc = [[MTLRenderPassDescriptor alloc] init];
    desc.colorAttachments[0].texture     = [drawable texture];
    desc.colorAttachments[0].loadAction  = MTLLoadActionClear;
    desc.colorAttachments[0].clearColor  = MTLClearColorMake(clearR, clearG, clearB, 1.0f);
    desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    desc.depthAttachment.texture     = gfx_depthTex;
    desc.depthAttachment.loadAction  = MTLLoadActionClear;
    desc.depthAttachment.storeAction = MTLStoreActionDontCare;
    desc.depthAttachment.clearDepth  = 1.0f;
    
    ren_enc = [cmd_buf renderCommandEncoderWithDescriptor:desc];
    [desc autorelease];
}

void Gfx_EndFrame(void) {
    [ren_enc endEncoding];
    ren_enc = nil;
    
    [cmd_buf presentDrawable:drawable];
    [cmd_buf commit];
    
    [cmd_buf autorelease];
    cmd_buf = nil;
    // TODO: implement
}

void Gfx_SetVSync(cc_bool vsync) {
    gfx_vsync = vsync;
#ifdef CC_BUILD_MACOS
    [gfx_layer setDisplaySyncEnabled:vsync];
#endif
}

void Gfx_OnWindowResize(int width, int height) {
    if (gfx_depthTex) { [gfx_depthTex autorelease]; }
    
    // TODO: depth32Float ?
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                    width:width height:height mipmapped:NO];
    desc.storageMode = MTLStorageModePrivate;
    desc.usage       = MTLTextureUsageRenderTarget;
    if (desc == nil) Process_Abort("no depth texture");
    
    gfx_depthTex = [gfx_device newTextureWithDescriptor:desc];
    if (gfx_depthTex == nil) Process_Abort("no depth texture");

    // TODO: needed?
    Gfx_SetViewport(0, 0, width, height);
    Gfx_SetScissor (0, 0, width, height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
    viewport_rect.originX = x;
    viewport_rect.originY = y;
    viewport_rect.width   = w;
    viewport_rect.height  = h;
    viewport_rect.znear   = 0.0f;
    viewport_rect.zfar    = 1.0f;
    dirty_bits |= DIRTY_VIEWPORT;
}

void Gfx_SetScissor (int x, int y, int w, int h) {
    scissor_rect.x      = x;
    scissor_rect.y      = y;
    scissor_rect.width  = w;
    scissor_rect.height = h;
    dirty_bits |= DIRTY_SCISSOR;
}

void Gfx_GetApiInfo(cc_string* info) {
    NSString* str = [gfx_device name];
    if (str) {
        const char* name = [str UTF8String];
        String_Format1(info, "Device: %c\n", name);
    }
    
    PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
