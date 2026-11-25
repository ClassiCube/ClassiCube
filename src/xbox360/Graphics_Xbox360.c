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

static XsdkShader vs_tex, vs_col, vs_oft;
static XsdkShader ps_tex, ps_col;

#define SAMPLER_0       0
#define MVP_CONST_INDEX 0 
#define MVP_CONST_COUNT 4 // each slot is 4 floats/1 vec4 
#define OFT_CONST_INDEX 4
#define OFT_CONST_COUNT 5

static const XsdkVertexElement textured_layout[] = {
	{ .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT3,  .offset_bytes =  0 },
	{ .usage = XSDK_USAGE_COLOR,    .format = XSDK_VERTEX_UBYTE4N, .offset_bytes = 12 },
	{ .usage = XSDK_USAGE_TEXCOORD, .format = XSDK_VERTEX_FLOAT2,  .offset_bytes = 16 },
};
static const XsdkVertexElement coloured_layout[] = {
	{ .usage = XSDK_USAGE_POSITION, .format = XSDK_VERTEX_FLOAT3,  .offset_bytes =  0 },
	{ .usage = XSDK_USAGE_COLOR,    .format = XSDK_VERTEX_UBYTE4N, .offset_bytes = 12 }
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

	CreateVertexShader(&vs_oft, &vs_offset_vert, SIZEOF_VERTEX_TEXTURED, 
						textured_layout, Array_Elems(textured_layout));
	CreateVertexShader(&vs_tex, &vs_textured_vert, SIZEOF_VERTEX_TEXTURED, 
						textured_layout, Array_Elems(textured_layout));
	CreateVertexShader(&vs_col, &vs_coloured_vert, SIZEOF_VERTEX_COLOURED, 
						coloured_layout, Array_Elems(coloured_layout));
}

static GfxResourceID white_square;
static XsdkSamplerState sampler_state;

static void AllocResources(void) {
	const XsdkSampler sampler = { 
		.mag_filter = XSDK_FILTER_POINT,
		.min_filter = XSDK_FILTER_POINT,
		.mip_filter = XSDK_FILTER_POINT,
		.address_u  = XSDK_ADDRESS_WRAP,
		.address_v  = XSDK_ADDRESS_WRAP 
	};
	xsdk_texture_sampler_state(&sampler_state, &sampler);

	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 2048;
	Gfx.MaxTexHeight = 2048;
	Gfx.MaxTexSize   = 1024 * 1024;

	if (!Gfx.Created) {
		LoadShaders();
		AllocResources();
	}

	Gfx.Created = true;
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	// TODO nonpow2 upload
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
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	XsdkTexture* tex = Mem_TryAlloc(1, sizeof(XsdkTexture));
	if (!tex) return NULL;

	uint32_t status = xsdk_texture_create(tex, XSDK_FORMAT_B8G8R8A8, bmp->width, bmp->height, 1, true);
	if (status == STATUS_NO_MEMORY) { Mem_Free(tex); return NULL; }
	if (!NT_SUCCESS(status)) Process_Abort2(status, "alloc texture");

	xsdk_texture_upload_rows(tex, 0, 0, bmp->height, bmp->scan0, rowWidth * 4);
	xsdk_gpu_invalidate(tex->address, tex->size_bytes);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO update textures
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	XsdkTexture* tex  = texId;

	xsdk_gpu_set_texture(SAMPLER_0, tex, &sampler_state);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	XsdkTexture* tex = *texId;
	if (tex) { xsdk_texture_free(tex); Mem_Free(tex); }

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

static uint32_t clear_color;
void Gfx_ClearColor(PackedCol color) {
	int r = PackedCol_R(color);
	int g = PackedCol_G(color);
	int b = PackedCol_B(color);
	int a = PackedCol_A(color);

	clear_color = b | (g << 8) | (r << 16) | (a << 24);
}

static void UpdateDepthState(void) {
	const XsdkDepthStencil depth = {
		.depth_test    = gfx_depthTest,
		.depth_write   = gfx_depthWrite,
		.depth_compare = XSDK_COMPARE_LESS_EQUAL,
	};
	XsdkDepthStencilState state;

	xsdk_gpu_depth_stencil_state(&state, &depth);
	xsdk_gpu_set_depth_stencil(&state);
}

static void SetDepthTest(cc_bool enabled)  { UpdateDepthState(); }
static void SetDepthWrite(cc_bool enabled) { UpdateDepthState(); }


static void UpdateBlendState(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	cc_bool no_blend = !gfx_alphaBlend;

	XsdkBlend blend = {
		.color_source = no_blend ? XSDK_BLEND_ONE  : XSDK_BLEND_SRC_ALPHA,
		.color_dest   = no_blend ? XSDK_BLEND_ZERO : XSDK_BLEND_ONE_MINUS_SRC_ALPHA,
		.color_op     = XSDK_BLEND_OP_ADD,
		.alpha_source = no_blend ? XSDK_BLEND_ONE  : XSDK_BLEND_SRC_ALPHA,
		.alpha_dest   = no_blend ? XSDK_BLEND_ZERO : XSDK_BLEND_ONE_MINUS_SRC_ALPHA,
		.alpha_op     = XSDK_BLEND_OP_ADD,
		.write_mask   = (r ? 0x1 : 0) | (g ? 0x2 : 0) | (b ? 0x4 : 0) | (a ? 0x8 : 0),
		.alpha_to_coverage = false
	};

	XsdkBlendState state;
	xsdk_gpu_blend_state(&state, &blend);
	xsdk_gpu_set_blend(&state);
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	UpdateBlendState(r, g, b, a);
}

static void SetAlphaTest(cc_bool enabled) {
	//Xe_SetAlphaTestEnable(xe, enabled);
}

static void SetAlphaBlend(cc_bool enabled) {
	UpdateBlendState(gfx_colorMask[0], gfx_colorMask[1], gfx_colorMask[2], gfx_colorMask[3]);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

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
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return xsdk_gpu_alloc(count * strideSizes[fmt]);
} // TODO gpu_alloc is always 64kb aligned, so this is wasteful

void Gfx_DeleteVb(GfxResourceID* vb) {
	void* ptr = *vb;
	if (ptr) xsdk_gpu_free(ptr);
	*vb = NULL;
}

static void* locked_vb;
static int locked_size;

static void FlushLockedVb(GfxResourceID vb) {
	// TODO: xsdk documentation says have to do this to avoid 64 bit write issues. verify this
	cc_uint32* src  = (cc_uint32*)locked_vb;
	cc_uint32* dst  = (cc_uint32*)vb;
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
#define PRIM_TYPE_LINELIST  2
#define PRIM_TYPE_QUADLIST 13
static cc_uint32 gpu_vb_addr; 

static void UpdateShader(void);
static void SetVertexFormat(VertexFormat fmt) { UpdateShader(); }

void Gfx_BindVb(GfxResourceID vb) {
	gpu_vb_addr = xsdk_gpu_address(vb);
}

void Gfx_DrawVb_Lines(int verticesCount) {
	xsdk_gpu_set_vertices(0, gpu_vb_addr, verticesCount * gfx_stride);
	xsdk_gpu_draw(PRIM_TYPE_LINELIST, verticesCount);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	xsdk_gpu_set_vertices(0, gpu_vb_addr, verticesCount * gfx_stride);
	xsdk_gpu_draw(PRIM_TYPE_QUADLIST, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	xsdk_gpu_set_vertices(0, gpu_vb_addr + startVertex * gfx_stride, verticesCount * gfx_stride);
	xsdk_gpu_draw(PRIM_TYPE_QUADLIST, verticesCount);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	xsdk_gpu_set_vertices(0, gpu_vb_addr + startVertex * SIZEOF_VERTEX_TEXTURED, verticesCount * SIZEOF_VERTEX_TEXTURED);
	xsdk_gpu_draw(PRIM_TYPE_QUADLIST, verticesCount);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, _mvp;
static struct Vec4 texOffset;

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
	texOffset.x = x;
	texOffset.y = y;

	// TODO not working??
	UpdateShader();
	xsdk_gpu_set_vertex_constants(OFT_CONST_INDEX, &texOffset.x, OFT_CONST_COUNT);
}

void Gfx_DisableTextureOffset(void) {
	texOffset.x = 0;
	texOffset.y = 0;

	UpdateShader();
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
static void UpdateShader(void) {
	int colored = gfx_format == VERTEX_FORMAT_COLOURED;
	XsdkShader* vs = colored ? &vs_col : ((texOffset.x || texOffset.y) ? &vs_oft : &vs_tex);
	XsdkShader* ps = colored ? &ps_col : &ps_tex;
	
	xsdk_gpu_set_shaders(vs, ps);
	xsdk_built_shader_set_literals(&vs_coloured_vert); // TODO prob wrong
	//xsdk_built_shader_set_literals(ps);
}

// TODO tidy up functions
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
	// TODO vsync
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

	if (gfx_format >= 0) SetVertexFormat(gfx_format);
	UpdateDepthState();
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	// Xe_Clear is just a Resolve anyways
}

void Gfx_EndFrame(void) {
	// NOTE: resolve also resets other state too (confusingly)
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

void Gfx_SetViewport(int x, int y, int w, int h) {
	xsdk_gpu_set_viewport(x, y, w, h, 0.0f, 1.0f);
}

void Gfx_SetScissor (int x, int y, int w, int h) {
	xsdk_gpu_set_scissor(x, y, w, h);
}
