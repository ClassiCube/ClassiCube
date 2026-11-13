#define CC_DYNAMIC_VBS_ARE_STATIC
#define CC_SCRATCH_VBS_ARE_DYNAMIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"

#include <xsdk/gpu.h>
#include <xsdk/present.h>
//#include <xenos/xe.h>
//#include <xenos/edram.h>
#include "../../build/xbox360/shaders.h"

static XsdkShader vs_tex, vs_col;
static XsdkShader ps_tex, ps_col;

static const XsdkVertexElement textured_layout[] = {
	{ .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT3, .offset_bytes =  0 },
	{ .usage = XSDK_USAGE_COLOR,    .format = XSDK_VERTEX_UBYTE4, .offset_bytes = 12 },
	{ .usage = XSDK_USAGE_TEXCOORD, .format = XSDK_VERTEX_FLOAT2, .offset_bytes = 16 },
};
static const XsdkVertexElement coloured_layout[] = {
	{ .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT3, .offset_bytes =  0 },
	{ .usage = XSDK_USAGE_COLOR,    .format = XSDK_VERTEX_UBYTE4, .offset_bytes = 12 }
};

static void CreatePixelShader(XsdkShader* shdr, const XsdkBuiltShader* src) {
	uint32_t status = xsdk_shader_load(shdr, src->container, src->container_bytes);
	if (!NT_SUCCESS(status)) Process_Abort2(status, "loading pixel shader");
}

static void CreateVertexShader(XsdkShader* shdr, const XsdkBuiltShader* src, int stride, 
								const XsdkVertexElement* attribs, uint32_t num_attribs) {
	uint32_t status = xsdk_shader_load(shdr, src->container, src->container_bytes);
	if (!NT_SUCCESS(status)) Process_Abort2(status, "loading vertex shader");

	uint32_t stream_strides[] = { stride };
	status = xsdk_shader_set_vertex_layout(shdr, attribs, num_attribs, stream_strides);
	if (!NT_SUCCESS(status)) Process_Abort2(status, "building vertex shader");
}

static void LoadShaders(void) {
	CreatePixelShader(&ps_tex, &ps_textured_frag);
	CreatePixelShader(&ps_col, &ps_coloured_frag);

	CreateVertexShader(&vs_tex, &vs_textured_vert, SIZEOF_VERTEX_TEXTURED, 
						textured_layout, Array_Elems(textured_layout));
	CreateVertexShader(&vs_col, &vs_coloured_vert, SIZEOF_VERTEX_COLOURED, 
						coloured_layout, Array_Elems(coloured_layout));
}

void Gfx_Create(void) {
	if (!Gfx.Created) {
		LoadShaders();
	}
	Gfx.Created = true;
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	
	Gfx.MaxTexWidth  = 2048;
	Gfx.MaxTexHeight = 2048;
	Gfx.MaxTexSize   = 1024 * 1024;
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
	//Gfx_SetFaceCulling(false);
	//SetAlphaBlend(false);

	//Xe_SetAlphaFunc(xe, XE_CMP_GREATER);
	//Xe_SetAlphaRef(xe,  0.5f);
	//Xe_SetZFunc(xe,     XE_CMP_LESSEQUAL);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void SetTextureData(struct XenosSurface* xtex, int x, int y, const struct Bitmap* bmp, int rowWidth, int lvl) {
	//void* dst = Xe_Surface_LockRect(xe, xtex, x, y, bmp->width, bmp->height, XE_LOCK_WRITE);

	//CopyPixels(dst,        bmp->width * BITMAPCOLOR_SIZE,
	//		   bmp->scan0, rowWidth   * BITMAPCOLOR_SIZE,
	//		   bmp->width, bmp->height);
	
	//Xe_Surface_Unlock(xe, xtex);
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	//struct XenosSurface* xtex = Xe_CreateTexture(xe, bmp->width, bmp->height, 1, XE_FMT_8888, 0);
	//SetTextureData(xtex, 0, 0, bmp, rowWidth, 0);
	//return xtex;
	return (void*)1;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	//struct XenosSurface* xtex = (struct XenosSurface*)texId;
	//SetTextureData(xtex, x, y, part, rowWidth, 0);
}

void Gfx_BindTexture(GfxResourceID texId) {
	//struct XenosSurface* xtex = (struct XenosSurface*)texId;
	//Xe_SetTexture(xe, 0, xtex);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	//struct XenosSurface* xtex = (struct XenosSurface*)(*texId);
	//if (xtex) Xe_DestroyTexture(xe, xtex);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }  // TODO

void Gfx_DisableMipmaps(void) { } // TODO


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)         { } // TODO: implement
static void SetFogColor(PackedCol color) { } // TODO: implement
static void SetFogDensity(float value)   { } // TODO: implement
static void SetFogEnd(float value)       { } // TODO: implement
static void SetFogMode(FogFunc func)     { } // TODO: implement

void Gfx_SetFaceCulling(cc_bool enabled) {
	//Xe_SetCullMode(xe, enabled ? XE_CULL_CW : XE_CULL_NONE);
}

static void SetAlphaTest(cc_bool enabled) {
	//Xe_SetAlphaTestEnable(xe, enabled);
}

static void SetAlphaBlend(cc_bool enabled) {
/*	if (enabled) {
		Xe_SetBlendControl(xe,
			XE_BLEND_SRCALPHA, XE_BLENDOP_ADD, XE_BLEND_INVSRCALPHA,
			XE_BLEND_SRCALPHA, XE_BLENDOP_ADD, XE_BLEND_INVSRCALPHA);
	} else {
		Xe_SetBlendControl(xe,
			XE_BLEND_ONE, XE_BLENDOP_ADD, XE_BLEND_ZERO,
			XE_BLEND_ONE, XE_BLENDOP_ADD, XE_BLEND_ZERO);
	}*/
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
	// TODO
}

static uint32_t clear_color;
void Gfx_ClearColor(PackedCol color) {
	clear_color = color;
}

static void SetDepthTest(cc_bool enabled) {
	//Xe_SetZEnable(xe, enabled);
}

static void SetDepthWrite(cc_bool enabled) {
	//Xe_SetZWrite(xe, enabled);
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
    DefaultDepthOnlyRendering(depthOnly);
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
#define VB_DATA_OFFSET 64

static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	void* ptr = xsdk_gpu_alloc(VB_DATA_OFFSET + count * strideSizes[fmt]);
	if (ptr) { *((cc_uint32*)ptr) = count * strideSizes[fmt]; }
	return ptr;
} // TODO gpu_alloc is always 64kb aligned, so this is wasteful

void Gfx_DeleteVb(GfxResourceID* vb) {
	void* ptr = *vb;
	if (ptr) xsdk_gpu_free(ptr);
	*vb = NULL;
}

void Gfx_BindVb(GfxResourceID vb) {
	cc_uint32 size = *((cc_uint32*)vb);
	xsdk_gpu_set_vertices(0, xsdk_gpu_address(vb + VB_DATA_OFFSET), size);
}

static void* locked_vb;
static int locked_size;

static void FlushLockedVb(GfxResourceID vb) {
	// TODO: xsdk documentation says have to do this to avoid 64 bit write issues. verify this
	cc_uint32* src  = (cc_uint32*)locked_vb;
	cc_uint32* dst  = (cc_uint32*)(vb + VB_DATA_OFFSET);
	int count_words = locked_size / 4;

	for (int i = 0; i < count_words; i++) 
	{
		uint32_t word;
		__builtin_memcpy(&word, &src[i], 4);
		dst[i] = word;
	}
	xsdk_gpu_invalidate(xsdk_gpu_address(dst), locked_size);
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	locked_vb   = Mem_Alloc(count, strideSizes[fmt], "VB temp");
	locked_size = count * strideSizes[fmt];

	return locked_vb;
}

void Gfx_UnlockVb(GfxResourceID vb) {
	FlushLockedVb(vb);
	Mem_Free(locked_vb);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex rendering----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	int colored = fmt == VERTEX_FORMAT_COLOURED;
	XsdkShader* vs = colored ? &vs_col : &vs_tex;
	//XsdkShader* ps = colored ? &ps_col : &ps_tex; TODO uncomment
	XsdkShader* ps = &ps_col;
	
	xsdk_gpu_set_shaders(vs, ps);
	xsdk_built_shader_set_literals(&vs_coloured_vert); // TODO prob wrong
	//xsdk_built_shader_set_literals(ps);
}

void Gfx_DrawVb_Lines(int verticesCount) {
	Platform_Log1("DRAW_LINES: %i", &verticesCount);
	//Xe_DrawPrimitive(xe, XE_PRIMTYPE_LINELIST, 0, verticesCount >> 1); TODO
}

#define PRIM_TYPE_QUADLIST 13 
void Gfx_DrawVb_IndexedTris(int verticesCount) {
	Platform_Log1("DRAW_TRIS: %i", &verticesCount);
	xsdk_gpu_draw(PRIM_TYPE_QUADLIST, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	Platform_Log1("DRAW_TRIS_RANGE: %i", &verticesCount);
	if (startVertex) return;
	xsdk_gpu_draw(PRIM_TYPE_QUADLIST, verticesCount);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	Platform_Log1("DRAW_TRIS_MAP: %i", &verticesCount);
	if (startVertex) return;
	xsdk_gpu_draw(PRIM_TYPE_QUADLIST, verticesCount);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, _mvp;

#define MVP_CONST_INDEX 0 
#define MVP_CONST_COUNT 4 // each slot is 4 floats/1 vec4 

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	struct Matrix* dst = type == MATRIX_PROJ ? &_proj : &_view;
	*dst = *matrix;
	
	Matrix_Mul(&_mvp, &_view, &_proj);
	xsdk_gpu_set_vertex_constants(MVP_CONST_INDEX, &_mvp.row1.x, MVP_CONST_COUNT);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj); // TODO not set vs constants twice

	Mem_Copy(mvp, &_mvp, sizeof(struct Matrix));
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

#define PRESENT_TIMEOUT_MS 5000
static uint32_t fb_address;

void Gfx_BeginFrame(void) {
	uint32_t status = xsdk_present_begin(&fb_address, PRESENT_TIMEOUT_MS);

	if (!NT_SUCCESS(status)) {
		Platform_Log1("xsdk_present_begin failed: %h",&status);
		return;
	}

	xsdk_gpu_set_target(Window_Main.Width, Window_Main.Height);
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	// Xe_Clear is just a Resolve anyways
}

void Gfx_EndFrame(void) {
	xsdk_gpu_resolve(fb_address, Window_Main.Width, Window_Main.Height, clear_color);
	xsdk_present_end();
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using XBox 360 --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_OnWindowResize(int width, int height) {

}

void Gfx_SetViewport(int x, int y, int w, int h) { }
void Gfx_SetScissor (int x, int y, int w, int h) { }
