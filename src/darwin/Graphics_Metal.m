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
id<MTLDevice> gfx_device;
static id<MTLCommandQueue> cmd_queue;
static id<MTLCommandBuffer> cmd_buf;

id<MTLDrawable> MetalContext_NextDrawable(void);
id<MTLTexture>  MetalContext_GetDrawableTexture(id<MTLDrawable> d);
extern void     MetalContext_SetVSync(cc_bool vsync);

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

void Gfx_Create(void) {
    Gfx.MaxTexWidth  = 4096;
    Gfx.MaxTexHeight = 4096;
    Gfx.Created      = true;
    Gfx.Limitations  = GFX_LIMIT_MINIMAL;
    
    cmd_queue = [gfx_device newCommandQueue];
}

void Gfx_Free(void) {
    Gfx_FreeState();
    // TODO: implement
}


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)    { }// TODO: implement
void Gfx_SetFogCol(PackedCol col)   { }// TODO: implement
void Gfx_SetFogDensity(float value) { }// TODO: implement
void Gfx_SetFogEnd(float value)     { }// TODO: implement
void Gfx_SetFogMode(FogFunc func)   { }// TODO: implement

void Gfx_SetFaceCulling(cc_bool enabled) {
    // TODO: implement
}

static void SetAlphaTest(cc_bool enabled) {
    // TODO: implement
}

static void SetAlphaBlend(cc_bool enabled) {
    // TODO: implement
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearBuffers(GfxBuffers buffers) {
    // TODO: implement
}

void Gfx_ClearColor(PackedCol color) {
    // TODO: implement
}

void Gfx_SetDepthTest(cc_bool enabled) {
    // TODO: implement
}

void Gfx_SetDepthWrite(cc_bool enabled) {
    // TODO: implement
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
    // TODO: implement
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
    // TODO: implement
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
    // TODO: implement
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
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
    int size = count * strideSizes[fmt];
    return [gfx_device newBufferWithLength:size options:MTLResourceStorageModePrivate];
}

void Gfx_BindVb(GfxResourceID vb) { }// TODO: implement

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
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static float texOffsetX, texOffsetY;
static struct Matrix _view, _proj, _mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
    if (type == MATRIX_VIEW) _view = *matrix;
    if (type == MATRIX_PROJ) _proj = *matrix;

    Matrix_Mul(&_mvp, &_view, &_proj);
    // TODO: implement
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
    _view = *view;
    _proj = *proj;

    Matrix_Mul(mvp, view, proj);
    // TODO: implement
}

void Gfx_EnableTextureOffset(float x, float y) {
    // TODO: implement
}

void Gfx_DisableTextureOffset(void) {
    // TODO: implement
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
}

void Gfx_DrawVb_Lines(int verticesCount) {
    // TODO: implement
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
    // TODO: implement
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
    // TODO: implement
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
    // TODO: implement
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
    return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

static int F;
static id<MTLDrawable> drawable;
void Gfx_BeginFrame(void) {
    // TODO: implement
    cmd_buf  = [[cmd_queue commandBuffer] retain];
    drawable = MetalContext_NextDrawable();
    if (drawable == nil) Process_Abort("No metal drawable");
    
    MTLRenderPassDescriptor* desc = [[MTLRenderPassDescriptor alloc] init];
    desc.colorAttachments[0].texture     = MetalContext_GetDrawableTexture(drawable);
    desc.colorAttachments[0].loadAction  = MTLLoadActionClear;
    desc.colorAttachments[0].clearColor  = MTLClearColorMake(0.0f + (F % 100) * 0.01f, 0.5f, 0.7f, 1.0f); F++;
    desc.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    id<MTLRenderCommandEncoder> enc = [cmd_buf renderCommandEncoderWithDescriptor:desc];
    [enc endEncoding];
    [desc autorelease];
}

void Gfx_EndFrame(void) {
    [cmd_buf presentDrawable:drawable];
    [cmd_buf commit];
    
    [cmd_buf autorelease];
    cmd_buf = nil;
    // TODO: implement
}

void Gfx_SetVSync(cc_bool vsync) {
    gfx_vsync = vsync;
    MetalContext_SetVSync(vsync);
}

void Gfx_OnWindowResize(int width, int height) {
    // TODO: implement

    Gfx_SetViewport(0, 0, width, height);
    Gfx_SetScissor (0, 0, width, height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
    // TODO: implement
}

void Gfx_SetScissor (int x, int y, int w, int h) {
    // TODO: implement
}

void Gfx_GetApiInfo(cc_string* info) {
    // TODO: implement
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
