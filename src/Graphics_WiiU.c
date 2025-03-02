#include "Core.h"
#ifdef CC_BUILD_WIIU
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <gx2/clear.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/draw.h>
#include <gx2/event.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/shaders.h>
#include <gx2/state.h>
#include <gx2/surface.h>
#include <gx2/swap.h>
#include <gx2/temp.h>
#include <gx2/utils.h>
#include <gx2r/draw.h>
#include <gx2r/mem.h>
#include <gx2r/buffer.h>
#include <whb/gfx.h>
#include <coreinit/memdefaultheap.h>
#include "../build-wiiu/coloured_gsh.h"
#include "../build-wiiu/textured_gsh.h"

static WHBGfxShaderGroup colorShader;
static WHBGfxShaderGroup textureShader;
static GX2Sampler sampler;
static GfxResourceID white_square;
static WHBGfxShaderGroup* group;

static void InitGfx(void) {
   	GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_WRAP, GX2_TEX_XY_FILTER_MODE_POINT);
	
	WHBGfxLoadGFDShaderGroup(&colorShader, 0, coloured_gsh);
	WHBGfxInitShaderAttribute(&colorShader, "in_pos", 0,  0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
	WHBGfxInitShaderAttribute(&colorShader, "in_col", 0, 12, GX2_ATTRIB_FORMAT_UNORM_8_8_8_8);
	WHBGfxInitFetchShader(&colorShader);
	
	WHBGfxLoadGFDShaderGroup(&textureShader, 0, textured_gsh);
	WHBGfxInitShaderAttribute(&textureShader, "in_pos", 0,  0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
	WHBGfxInitShaderAttribute(&textureShader, "in_col", 0, 12, GX2_ATTRIB_FORMAT_UNORM_8_8_8_8);
	WHBGfxInitShaderAttribute(&textureShader, "in_uv",  0, 16, GX2_ATTRIB_FORMAT_FLOAT_32_32);
	WHBGfxInitFetchShader(&textureShader);
}

void Gfx_Create(void) {
	if (!Gfx.Created) InitGfx();
	
	Gfx.Created      = true;
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
}

static void Gfx_FreeState(void) { 
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

static void Gfx_RestoreState(void) {
	Gfx_SetFaceCulling(false);
	InitDefaultResources();
	gfx_format = -1;
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static GX2Texture* pendingTex;

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	GX2Texture* tex = Mem_TryAllocCleared(1, sizeof(GX2Texture));
	if (!tex) return NULL;

	// TODO handle out of memory better
	int width = bmp->width, height = bmp->height;
	tex->surface.width    = width;
	tex->surface.height   = height;
	tex->surface.depth    = 1;
	tex->surface.dim      = GX2_SURFACE_DIM_TEXTURE_2D;
	tex->surface.format   = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
	tex->surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
	tex->viewNumSlices    = 1;
	tex->compMap          = GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A);
	GX2CalcSurfaceSizeAndAlignment(&tex->surface);
	GX2InitTextureRegs(tex);

	tex->surface.image = MEMAllocFromDefaultHeapEx(tex->surface.imageSize, tex->surface.alignment);
	if (!tex->surface.image) { Mem_Free(tex); return NULL; }
  
	CopyTextureData(tex->surface.image, tex->surface.pitch << 2, 
					bmp, rowWidth * BITMAPCOLOR_SIZE);
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, tex->surface.image, tex->surface.imageSize);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	GX2Texture* tex = (GX2Texture*)texId;	
	uint32_t* dst   = (uint32_t*)tex->surface.image + (y * tex->surface.pitch) + x;
	
	CopyTextureData(dst, tex->surface.pitch << 2, 
					part, rowWidth * BITMAPCOLOR_SIZE);
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, tex->surface.image, tex->surface.imageSize);
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	pendingTex = (GX2Texture*)texId;
	// Texture is bound to active shader, so might need to defer it in
	//  case a call to Gfx_BindTexture was called even though vertex format wasn't textured
	// TODO: Track as dirty uniform flag instead?
}

static void BindPendingTexture(void) {
	if (!pendingTex || group != &textureShader) return;
	
	GX2SetPixelTexture(pendingTex, group->pixelShader->samplerVars[0].location);
	GX2SetPixelSampler(&sampler,   group->pixelShader->samplerVars[0].location);
 	pendingTex = NULL;
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	if (*texId == pendingTex) pendingTex = NULL;
	// TODO free memory ???
}

void Gfx_EnableMipmaps(void) { }  // TODO

void Gfx_DisableMipmaps(void) { } // TODO


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static float clearR, clearG, clearB;
static cc_bool depthWrite = true, depthTest = true;

static void UpdateDepthState(void) {
	GX2SetDepthOnlyControl(depthTest, depthWrite, GX2_COMPARE_FUNC_LEQUAL);
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, false, enabled);
}

void Gfx_SetFog(cc_bool enabled) {
	// TODO
}

void Gfx_SetFogCol(PackedCol color) {
	// TODO
}

void Gfx_SetFogDensity(float value) {
	// TODO
}

void Gfx_SetFogEnd(float value) {
	// TODO
}

void Gfx_SetFogMode(FogFunc func) {
	// TODO
}

static void SetAlphaTest(cc_bool enabled) {
	GX2SetAlphaTest(enabled, GX2_COMPARE_FUNC_GEQUAL, 0.5f);
}

static void SetAlphaBlend(cc_bool enabled) {
	GX2SetBlendControl(GX2_RENDER_TARGET_0,
		GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
		true,
		GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
	GX2SetColorControl(GX2_LOGIC_OP_COPY, enabled, FALSE, TRUE);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
}

void Gfx_ClearColor(PackedCol color) {
	clearR = PackedCol_R(color) / 255.0f;
	clearG = PackedCol_G(color) / 255.0f;
	clearB = PackedCol_B(color) / 255.0f;
}

void Gfx_SetDepthTest(cc_bool enabled) {
	depthTest = enabled;
	UpdateDepthState();
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	depthWrite = enabled;
	UpdateDepthState();
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	GX2ChannelMask mask = 0;
	if (r) mask |= GX2_CHANNEL_MASK_R;
	if (g) mask |= GX2_CHANNEL_MASK_G;
	if (b) mask |= GX2_CHANNEL_MASK_B;
	if (a) mask |= GX2_CHANNEL_MASK_A;
	
	// TODO: use GX2SetColorControl to disable all writing ???
	GX2SetTargetChannelMasks(mask, 0,0,0, 0,0,0,0);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
				  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1; // don't need index buffers.. ?
}

void Gfx_BindIb(GfxResourceID ib) {
}

void Gfx_DeleteIb(GfxResourceID* ib) {
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	GX2RBuffer* buf = Mem_TryAllocCleared(1, sizeof(GX2RBuffer));
	if (!buf) return NULL;
	
	buf->flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_READ | GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
	buf->elemSize  = strideSizes[fmt];
	buf->elemCount = count;
	
	if (GX2RCreateBuffer(buf)) return buf;
	// Something went wrong ?? TODO
	Mem_Free(buf);
	return NULL;
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	GX2RBuffer* buf = *vb;
	if (!buf) return;
	
	GX2RDestroyBufferEx(buf, 0);
	Mem_Free(buf);	
	*vb = NULL;
}

void Gfx_BindVb(GfxResourceID vb) {
	GX2RBuffer* buf = (GX2RBuffer*)vb;
	GX2RSetAttributeBuffer(buf, 0, buf->elemSize, 0);
	//GX2SetAttribBuffer(0, 
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	GX2RBuffer* buf = (GX2RBuffer*)vb;
	return GX2RLockBufferEx(buf, 0);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	GX2RBuffer* buf = (GX2RBuffer*)vb;
	GX2RUnlockBufferEx(buf, 0);
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Gfx_AllocStaticVb(fmt, maxVertices);
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }

void Gfx_BindDynamicVb(GfxResourceID vb)    { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return Gfx_LockVb(vb, fmt, count);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb)  { Gfx_UnlockVb(vb); Gfx_BindVb(vb); }


/*########################################################################################################################*
*-----------------------------------------------------Vertex rendering----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	
	group = fmt == VERTEX_FORMAT_TEXTURED ? &textureShader : &colorShader;
	GX2SetFetchShader(&group->fetchShader);
	GX2SetVertexShader(group->vertexShader);
	GX2SetPixelShader(group->pixelShader);
}

void Gfx_DrawVb_Lines(int verticesCount) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_LINES, verticesCount, 0, 1);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, 0, 1);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, startVertex, 1);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, startVertex, 1);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;
	
	// TODO dirty uniform
	struct Matrix mvp __attribute__((aligned(64)));	
	Matrix_Mul(&mvp, &_view, &_proj);
	if (!group) return;
	GX2SetVertexUniformReg(group->vertexShader->uniformVars[0].offset, 16, &mvp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO
}

void Gfx_DisableTextureOffset(void) {
	// TODO
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// TODO verify this
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z =  1.0f / (zNear - zFar);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = zNear / (zNear - zFar);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	// TODO verify this
	float zNear = 0.1f;
	float c = Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = zFar / (zNear - zFar);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear * zFar) / (zNear - zFar);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
	// TODO GX2SetSwapInterval(1);
}

void Gfx_BeginFrame(void) { 
	uint32_t swapCount, flipCount;
	OSTime lastFlip, lastVsync;
	
	for (int try = 0; try < 10; try++)
	{
		GX2GetSwapStatus(&swapCount, &flipCount, &lastFlip, &lastVsync);
		if (flipCount >= swapCount) break;
		GX2WaitForVsync(); // TODO vsync
	}
	
	GX2ContextState* state = WHBGfxGetTVContextState();
	GX2SetContextState(state);
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	GX2ColorBuffer* buf = WHBGfxGetTVColourBuffer();
	GX2DepthBuffer* dph = WHBGfxGetTVDepthBuffer();

	if (buffers & GFX_BUFFER_COLOR) {
		GX2ClearColor(buf, clearR, clearG, clearB, 1.0f);
	}
	if (buffers & GFX_BUFFER_DEPTH) {
		GX2ClearDepthStencilEx(dph, 1.0f, 0, GX2_CLEAR_FLAGS_DEPTH | GX2_CLEAR_FLAGS_STENCIL);
	}
}

static int drc_ticks;
void Gfx_EndFrame(void) {
	GX2ColorBuffer* buf;
	
	buf = WHBGfxGetTVColourBuffer();
	GX2CopyColorBufferToScanBuffer(buf, GX2_SCAN_TARGET_TV);	

	GX2ContextState* state = WHBGfxGetDRCContextState();
	GX2SetContextState(state);
	drc_ticks = (drc_ticks + 1) % 200;
	buf = WHBGfxGetDRCColourBuffer();
	GX2ClearColor(buf, drc_ticks / 200.0f, drc_ticks / 200.0f, drc_ticks / 200.0f, 1.0f);
	GX2CopyColorBufferToScanBuffer(buf, GX2_SCAN_TARGET_DRC);	
	
	GX2SwapScanBuffers();
	GX2Flush();
	GX2DrawDone();
	GX2SetTVEnable(TRUE);
	GX2SetDRCEnable(TRUE);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Wii U --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_OnWindowResize(void) {

}

void Gfx_SetViewport(int x, int y, int w, int h) {
	GX2SetViewport(x, y, w, h, 0.0f, 1.0f);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	GX2SetScissor( x, y, w, h);
}

void Gfx_3DS_SetRenderScreen1(enum Screen3DS screen) {
	GX2ContextState* tv_state  = WHBGfxGetTVContextState();
	GX2ContextState* drc_state = WHBGfxGetDRCContextState(); // TODO
	
	GX2SetContextState(screen == TOP_SCREEN ? tv_state : drc_state);
}
#endif
