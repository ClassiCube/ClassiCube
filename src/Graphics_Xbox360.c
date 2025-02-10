#include "Core.h"
#ifdef CC_BUILD_XBOX360
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <xenos/xe.h>
#include <xenos/edram.h>

#include "../misc/xbox360/ps_coloured.h"
#include "../misc/xbox360/vs_coloured.h"
#include "../misc/xbox360/ps_textured.h"
#include "../misc/xbox360/vs_textured.h"
static struct XenosShader* shdr_tex_vs;
static struct XenosShader* shdr_tex_ps;
static struct XenosShader* shdr_col_vs;
static struct XenosShader* shdr_col_ps;

static struct XenosDevice device;
static struct XenosDevice* xe;

static const struct XenosVBFFormat textured_vbf = {
3, {
	{ XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4 },
	{ XE_USAGE_COLOR,    0, XE_TYPE_UBYTE4 },
	{ XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2 }
} };

static const struct XenosVBFFormat coloured_vbf = {
2, {
	{ XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4 },
	{ XE_USAGE_COLOR,    0, XE_TYPE_UBYTE4 }
} };

static void CreateState(void) {
	xe = &device;
	Xe_Init(xe);
	edram_init(xe);
	
	struct XenosSurface* fb = Xe_GetFramebufferSurface(xe);
	Xe_SetRenderTarget(xe, fb);
}

static void CreateShaders(void) {
	shdr_tex_vs = Xe_LoadShaderFromMemory(xe, (void*)vs_textured);
	Xe_InstantiateShader(xe, shdr_tex_vs, 0);	 
	Xe_ShaderApplyVFetchPatches(xe, shdr_tex_vs, 0, &textured_vbf);
	shdr_tex_ps = Xe_LoadShaderFromMemory(xe, (void*)ps_textured);
	Xe_InstantiateShader(xe, shdr_tex_ps, 0);
    	
	shdr_col_vs = Xe_LoadShaderFromMemory(xe, (void*)vs_coloured);
	Xe_InstantiateShader(xe, shdr_col_vs, 0);	 
	Xe_ShaderApplyVFetchPatches(xe, shdr_col_vs, 0, &coloured_vbf);
	shdr_col_ps = Xe_LoadShaderFromMemory(xe, (void*)ps_coloured);
	Xe_InstantiateShader(xe, shdr_col_ps, 0);
}

void Gfx_Create(void) {
	if (!Gfx.Created) {
		CreateState();
		CreateShaders();	
	}
	Gfx.Created = true;
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 512 * 512;
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
	InitDefaultResources();
	Gfx_SetFaceCulling(false);
	SetAlphaBlend(false);

	Xe_SetAlphaFunc(xe, XE_CMP_GREATER);
	Xe_SetAlphaRef(xe,  0.5f);
	Xe_SetZFunc(xe,     XE_CMP_LESSEQUAL);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void SetTextureData(struct XenosSurface* xtex, int x, int y, const struct Bitmap* bmp, int rowWidth, int lvl) {
	void* dst = Xe_Surface_LockRect(xe, xtex, x, y, bmp->width, bmp->height, XE_LOCK_WRITE);

	CopyTextureData(dst, bmp->width * BITMAPCOLOR_SIZE,
					bmp, rowWidth   * BITMAPCOLOR_SIZE);
	
	Xe_Surface_Unlock(xe, xtex);
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	struct XenosSurface* xtex = Xe_CreateTexture(xe, bmp->width, bmp->height, 1, XE_FMT_8888, 0);
	SetTextureData(xtex, 0, 0, bmp, rowWidth, 0);
	return xtex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	struct XenosSurface* xtex = (struct XenosSurface*)texId;
	SetTextureData(xtex, x, y, part, rowWidth, 0);
}

void Gfx_BindTexture(GfxResourceID texId) {
	struct XenosSurface* xtex = (struct XenosSurface*)texId;
	Xe_SetTexture(xe, 0, xtex);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	struct XenosSurface* xtex = (struct XenosSurface*)(*texId);
	if (xtex) Xe_DestroyTexture(xe, xtex);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }  // TODO

void Gfx_DisableMipmaps(void) { } // TODO


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFaceCulling(cc_bool enabled) {
	Xe_SetCullMode(xe, enabled ? XE_CULL_CW : XE_CULL_NONE);
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
	Xe_SetAlphaTestEnable(xe, enabled);
}

static void SetAlphaBlend(cc_bool enabled) {
	if (enabled) {
		Xe_SetBlendControl(xe,
			XE_BLEND_SRCALPHA, XE_BLENDOP_ADD, XE_BLEND_INVSRCALPHA,
			XE_BLEND_SRCALPHA, XE_BLENDOP_ADD, XE_BLEND_INVSRCALPHA);
	} else {
		Xe_SetBlendControl(xe,
			XE_BLEND_ONE, XE_BLENDOP_ADD, XE_BLEND_ZERO,
			XE_BLEND_ONE, XE_BLENDOP_ADD, XE_BLEND_ZERO);
	}
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
	// TODO
}

void Gfx_ClearColor(PackedCol color) { 
	Xe_SetClearColor(xe, color);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	Xe_SetZEnable(xe, enabled);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	Xe_SetZWrite(xe, enabled);
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
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) {
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	*ib = NULL;
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	int size = count * strideSizes[fmt];
	return Xe_CreateVertexBuffer(xe, size);
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)(*vb);
	if (xvb) Xe_DestroyVertexBuffer(xe, xvb);
	*vb = NULL;
}

void Gfx_BindVb(GfxResourceID vb) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)vb;
	Xe_SetStreamSource(xe, 0, xvb, 0, gfx_stride);
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)vb;
	int size = count * strideSizes[fmt];
	return Xe_VB_Lock(xe, xvb, 0, size, XE_LOCK_WRITE);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)vb;
	Xe_VB_Unlock(xe, xvb);
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
	Platform_LogConst("CHANGE FORMAT");
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	
	if (fmt == VERTEX_FORMAT_COLOURED) {
		Xe_SetTexture(xe, 0, NULL);
		Xe_SetShader(xe,  SHADER_TYPE_PIXEL,  shdr_col_ps, 0);
		Xe_SetShader(xe,  SHADER_TYPE_VERTEX, shdr_col_vs, 0);
	} else {
		Xe_SetShader(xe,  SHADER_TYPE_PIXEL,  shdr_tex_ps, 0);
		Xe_SetShader(xe,  SHADER_TYPE_VERTEX, shdr_tex_vs, 0);
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
	Platform_Log1("DRAW_LINES: %i", &verticesCount);
	Xe_DrawPrimitive(xe, XE_PRIMTYPE_LINELIST, 0, verticesCount >> 1);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	Platform_Log1("DRAW_TRIS: %i", &verticesCount);
	Xe_DrawPrimitive(xe, XE_PRIMTYPE_QUADLIST, 0, verticesCount >> 2);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	Platform_Log1("DRAW_TRIS_RANGE: %i", &verticesCount);
	Xe_DrawPrimitive(xe, XE_PRIMTYPE_QUADLIST, startVertex, verticesCount >> 2);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	Platform_Log1("DRAW_TRIS_MAP: %i", &verticesCount);
	Xe_DrawPrimitive(xe, XE_PRIMTYPE_QUADLIST, startVertex, verticesCount >> 2);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, _mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	struct Matrix* dst = type == MATRIX_PROJ ? &_proj : &_view;
	*dst = *matrix;
	Platform_LogConst("LOAD MATRIX");
	
	Matrix_Mul(&_mvp, &_view, &_proj);
	// TODO: Is this a global uniform, or does it need to be reloaded on shader change?
	Xe_SetVertexShaderConstantF(xe, 0, (float*)&_mvp, 4);
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
}

void Gfx_BeginFrame(void) { 
	Platform_LogConst("BEGIN FRAME");
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	// Xe_Clear is just a Resolve anyways
}

void Gfx_EndFrame(void) {
	Platform_LogConst("END FRAME A");
	Xe_Resolve(xe);
	Xe_Sync(xe);
	Platform_LogConst("END FRAME B");
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using XBox 360 --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_OnWindowResize(void) {

}

void Gfx_SetViewport(int x, int y, int w, int h) { }
void Gfx_SetScissor (int x, int y, int w, int h) { }
#endif
