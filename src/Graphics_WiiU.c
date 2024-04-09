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
#include <gx2r/draw.h>
#include <gx2r/mem.h>
#include <gx2r/buffer.h>
#include <whb/gfx.h>
#include "../build-wiiu/coloured_gsh.h"
#include "../build-wiiu/textured_gsh.h"

static WHBGfxShaderGroup colorShader;
static WHBGfxShaderGroup textureShader;

static void InitGfx(void) {
	WHBGfxInit();
	
	WHBGfxLoadGFDShaderGroup(&colorShader, 0, coloured_gsh);
	WHBGfxInitShaderAttribute(&colorShader, "in_pos", 0,  0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
	WHBGfxInitShaderAttribute(&colorShader, "in_col", 0, 12, GX2_ATTRIB_FORMAT_UNORM_8_8_8_8);
	WHBGfxInitFetchShader(&colorShader);
	
	WHBGfxLoadGFDShaderGroup(&textureShader, 0, textured_gsh);
	WHBGfxInitShaderAttribute(&textureShader, "in_pos", 0,  0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
	WHBGfxInitShaderAttribute(&textureShader, "in_col", 0, 12, GX2_ATTRIB_FORMAT_UNORM_8_8_8_8);
	WHBGfxInitShaderAttribute(&textureShader, "in_pos", 0, 16, GX2_ATTRIB_FORMAT_FLOAT_32_32);
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
}

static void Gfx_RestoreState(void) {
	Gfx_SetFaceCulling(false);
	InitDefaultResources();
	gfx_format = -1;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return (void*)1;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
}

void Gfx_BindTexture(GfxResourceID texId) {
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
}

void Gfx_EnableMipmaps(void) { }  // TODO

void Gfx_DisableMipmaps(void) { } // TODO


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static float clearR, clearG, clearB;

void Gfx_SetFaceCulling(cc_bool enabled) {
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

void Gfx_SetAlphaTest(cc_bool enabled) {
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
	// TODO
}

void Gfx_ClearColor(PackedCol color) {
	clearR = PackedCol_R(color) / 255.0f;
	clearG = PackedCol_G(color) / 255.0f;
	clearB = PackedCol_B(color) / 255.0f;
}

void Gfx_SetDepthTest(cc_bool enabled) {
}

void Gfx_SetDepthWrite(cc_bool enabled) {
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
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
static WHBGfxShaderGroup* group;

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
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, 0, 1);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, startVertex, 1);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, startVertex, 1);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;
	struct Matrix mvp __attribute__((aligned(64)));	
	Matrix_Mul(&mvp, &_view, &_proj);
	if (!group) return;
	
	
static float transposed_mvp[4*4] __attribute__((aligned(64)));
float* m = &mvp;
	
	// Transpose matrix
	for (int i = 0; i < 4; i++)
	{
		transposed_mvp[i * 4 + 0] = m[0  + i];
		transposed_mvp[i * 4 + 1] = m[4  + i];
		transposed_mvp[i * 4 + 2] = m[8  + i];
		transposed_mvp[i * 4 + 3] = m[12 + i];
	}
	
	//Platform_LogConst("MVP");
        GX2SetVertexUniformReg(group->vertexShader->uniformVars[0].offset, 16, &mvp);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	// TODO
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

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	// TODO verify this
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);
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

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) { 
	WHBGfxBeginRender();
	WHBGfxBeginRenderTV();
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

void Gfx_EndFrame(void) {
	WHBGfxFinishRenderTV();
	WHBGfxFinishRender();

	if (gfx_minFrameMs) LimitFPS();
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Wii U --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_OnWindowResize(void) {

}
#endif