#define CC_DYNAMIC_VBS_ARE_STATIC
#define CC_SCRATCH_VBS_ARE_DYNAMIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Logger.h"
#include "../Window.h"

#include <pbkit/pbkit.h>
#include "nv2a_gpu.h"

#define MAX_RAM_ADDR 0x03FFAFFF
 
static void ResetState(void) {
	uint32_t* p = pb_begin();

	p = NV2A_set_alpha_test_func(p, 0x04); // GL_GREATER & 0x0F
	p = NV2A_set_alpha_test_ref(p,  0x7F);
	p = NV2A_set_depth_func(p,      0x03); // GL_LEQUAL & 0x0F
	
	p = NV2A_set_alpha_blend_src(p, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
	p = NV2A_set_alpha_blend_dst(p, NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
	p = NV2A_set_alpha_blend_eq( p, NV097_SET_BLEND_EQUATION_V_FUNC_ADD); // TODO not needed?
	
	p = NV2A_set_cull_face_mode(p, NV097_SET_CULL_FACE_V_FRONT);
	// the order ClassiCube specifies quad vertices in are in the wrong order
	//  compared to what the GPU expects for front and back facing quads

	int width  = pb_back_buffer_width();
	int height = pb_back_buffer_height();
	p = NV2A_set_clear_rect(p, 0, 0, width, height);

	pb_end(p);

	p = pb_begin();
	p = NV2A_reset_all_vertex_attribs(p);
	p = NV2A_set_texture0_matrix(p, false);
	pb_end(p);
}

static GfxResourceID white_square;

static void VS_Init(void);
void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 2048; // TODO: 4096?
	Gfx.MaxTexHeight = 2048;
	Gfx.MaxTexSize   = 512 * 512;
	Gfx.Created      = true;

	InitDefaultResources();
	pb_show_front_screen();

	VS_Init();
	ResetState();
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx.NonPowTwoTexturesSupport = GFX_NONPOW2_UPLOAD;
		
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_Free(void) { 
	FreeDefaultResources();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
static void Gfx_RestoreState(void) { }
static void Gfx_FreeState(void) { }


/*########################################################################################################################*
*-------------------------------------------------------Vertex shaders----------------------------------------------------*
*#########################################################################################################################*/
static uint32_t vs_coloured_program[] = {
	#include "../../build/xbox/vs_coloured.inl"
};
static uint32_t vs_textured_program[] = {
	#include "../../build/xbox/vs_textured.inl"
};
static uint32_t vs_offset_program[] = {
	#include "../../build/xbox/vs_offset.inl"
};

static uint32_t vs_coloured_fog_program[] = {
	#include "../../build/xbox/vs_coloured_fog.inl"
};
static uint32_t vs_textured_fog_program[] = {
	#include "../../build/xbox/vs_textured_fog.inl"
};
static uint32_t vs_offset_fog_program[] = {
	#include "../../build/xbox/vs_offset_fog.inl"
};

// https://github.com/XboxDev/nxdk/blob/master/samples/triangle/main.c
// https://xboxdevwiki.net/NV2A/Vertex_Shader#Output_registers
#define VERTEX_ATTR_INDEX  0
#define COLOUR_ATTR_INDEX  3
#define TEXTURE_ATTR_INDEX 9

// A lot of figuring out which GPU registers to use came from:
// - comparing against pbgl and pbkit
static int vs_coloured,     vs_textured,     vs_offset;
static int vs_coloured_fog, vs_textured_fog, vs_offset_fog;

#define VS_CONST_MVP  0
#define VS_CONST_OFST 4

static int VS_Load(const uint32_t* program, int programSize) {
	int offset;
	uint32_t* p = pb_begin();
	p = NV2A_VS_upload_program(p, &offset, program, programSize);
	pb_end(p);
	return offset;
}

static void SetupShaders(void) {
	uint32_t *p;

	p = pb_begin();
	p = NV2A_VS_set_active(p, 0);
	p = NV2A_set_transform_mode(p, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM);

	pb_end(p);
}

static void VS_Init(void) {
	SetupShaders();

	vs_coloured = VS_Load(vs_coloured_program, sizeof(vs_coloured_program));
	vs_textured = VS_Load(vs_textured_program, sizeof(vs_textured_program));
	vs_offset   = VS_Load(vs_offset_program,   sizeof(vs_offset_program));

	vs_coloured_fog = VS_Load(vs_coloured_fog_program, sizeof(vs_coloured_fog_program));
	vs_textured_fog = VS_Load(vs_textured_fog_program, sizeof(vs_textured_fog_program));
	vs_offset_fog   = VS_Load(vs_offset_fog_program,   sizeof(vs_offset_fog_program));
}

static int tex_offset;
static int VS_CalcActiveProgram(void) {
	if (tex_offset) 
		return gfx_fogEnabled ? vs_offset_fog : vs_offset;
	if (gfx_format == VERTEX_FORMAT_TEXTURED) 
		return gfx_fogEnabled ? vs_textured_fog : vs_textured;

	return gfx_fogEnabled ? vs_coloured_fog : vs_coloured;
}


/*########################################################################################################################*
*-------------------------------------------------------Pixel shaders-----------------------------------------------------*
*#########################################################################################################################*/
// https://xboxdevwiki.net/NV2A/Pixel_Combiner

// https://developer.download.nvidia.com/assets/gamedev/docs/DynamicTexturing.pdf
// https://developer.download.nvidia.com/assets/gamedev/docs/combiners.pdf
// https://developer.download.nvidia.com/assets/gamedev/docs/GDC01_TextureShaders.pdf
// https://developer.download.nvidia.com/assets/gamedev/docs/ProgrammableTextureBlending.pdf

static void PS_UpdateActive(void) {
	uint32_t* p;
	p = pb_begin();

	if (gfx_format == VERTEX_FORMAT_TEXTURED && gfx_fogEnabled) {
		#include "../../build/xbox/ps_textured_fog.inl"
	} else if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		#include "../../build/xbox/ps_textured.inl"
	} else if (gfx_fogEnabled) {
		#include "../../build/xbox/ps_coloured_fog.inl"
	} else {
		#include "../../build/xbox/ps_coloured.inl"
	}
	pb_end(p);
}


/*########################################################################################################################*
*---------------------------------------------------------Texturing-------------------------------------------------------*
*#########################################################################################################################*/
typedef struct {
	cc_uint8 log2_w, log2_h;
	cc_uint32* pixels;
} CCTexture;

// See Graphics_Dreamcast.c for twiddling explanation
// (only difference is dreamcast is XY while xbox is YX)
static CC_INLINE void TwiddleCalcFactors(unsigned w, unsigned h, 
										unsigned* maskX, unsigned* maskY) {
	*maskX = 0;
	*maskY = 0;
	int shift = 0;

	for (; w > 1 || h > 1; w >>= 1, h >>= 1)
	{
		if (w > 1 && h > 1) {
			// Add interleaved X and Y bits
			*maskY += 0x02 << shift;
			*maskX += 0x01 << shift;
			shift  += 2;
		} else if (w > 1) {
			// Add a linear X bit
			*maskX += 0x01 << shift;
			shift  += 1;		
		} else if (h > 1) {
			// Add a linear Y bit
			*maskY += 0x01 << shift;
			shift  += 1;		
		}
	}
}

static int Log2Dimension(int len) { return Math_ilog2(Math_NextPowOf2(len)); }

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int dst_w = Math_NextPowOf2(bmp->width);
	int dst_h = Math_NextPowOf2(bmp->height);
	int size  = dst_w * dst_h * 4;

	CCTexture* tex = Mem_Alloc(1, sizeof(CCTexture), "GPU texture");
	tex->pixels    = MmAllocateContiguousMemoryEx(size, 0, MAX_RAM_ADDR, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
	
	tex->log2_w = Math_ilog2(dst_w);
	tex->log2_h = Math_ilog2(dst_h);
	cc_uint32* dst = tex->pixels;

	int src_w = bmp->width, src_h = bmp->height;
	unsigned maskX, maskY;
	unsigned X = 0, Y = 0;
	TwiddleCalcFactors(dst_w, dst_h, &maskX, &maskY);
	
	for (int y = 0; y < src_h; y++)
	{
		cc_uint32* src = bmp->scan0 + y * rowWidth;
		X = 0;
		
		for (int x = 0; x < src_w; x++, src++)
		{
			dst[X | Y] = *src;
			X = (X - maskX) & maskX;
		}
		Y = (Y - maskY) & maskY;
	}
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int originX, int originY, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	cc_uint32* dst = tex->pixels;
	
	int width = part->width, height = part->height;
	unsigned maskX, maskY;
	unsigned X = 0, Y = 0;
	TwiddleCalcFactors(1 << tex->log2_w, 1 << tex->log2_h, &maskX, &maskY);

	// Calculate start twiddled X and Y values
	for (int x = 0; x < originX; x++) { X = (X - maskX) & maskX; }
	for (int y = 0; y < originY; y++) { Y = (Y - maskY) & maskY; }
	unsigned startX = X;
	
	for (int y = 0; y < height; y++)
	{
		cc_uint32* src = part->scan0 + rowWidth * y;
		X = startX;
		
		for (int x = 0; x < width; x++, src++)
		{
			dst[X | Y] = *src;
			X = (X - maskX) & maskX;
		}
		Y = (Y - maskY) & maskY;
	}
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = (CCTexture*)(*texId);
	if (!tex) return;

	MmFreeContiguousMemory(tex->pixels);
	Mem_Free(tex);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	CCTexture* tex = (CCTexture*)texId;
	if (!tex) tex  = white_square;
	
	unsigned log_u = tex->log2_w;
	unsigned log_v = tex->log2_h;
	uint32_t* p;

	p = pb_begin();
	p = NV2A_set_texture0_pointer(p, tex->pixels);
	p = NV2A_set_texture0_format(p, tex->log2_w, tex->log2_h);
	p = NV2A_set_texture0_control0(p, gfx_alphaTest);
	p = NV2A_set_texture0_wrapmode(p);
	p = NV2A_set_texture0_filter(p);
	pb_end(p);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_ClearColor(PackedCol color) {
	uint32_t* p = pb_begin();
	p = NV2A_set_clear_colour(p, color);
	pb_end(p);
}

void Gfx_SetFaceCulling(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	p = NV2A_set_cull_face(p, enabled);
	pb_end(p);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetAlphaBlend(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	p = NV2A_set_alpha_blend(p, enabled);
	pb_end(p);
}

static void SetAlphaTest(cc_bool enabled) {
	uint32_t* p = pb_begin();
	p = NV2A_set_alpha_test(p, enabled);
	p = NV2A_set_texture0_control0(p, gfx_alphaTest);
	pb_end(p);
}

static void SetDepthWrite(cc_bool enabled) {
	uint32_t* p = pb_begin();
	p = NV2A_set_depth_write(p, enabled);
	pb_end(p);
}

static void SetDepthTest(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	p = NV2A_set_depth_test(p, enabled);
	pb_end(p);
}


static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	uint32_t* p = pb_begin();
	p = NV2A_set_color_write_mask(p, r, g, b, a);
	pb_end(p);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
    DefaultDepthOnlyRendering(depthOnly);
}


/*########################################################################################################################*
*-----------------------------------------------------Frame management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BeginFrame(void) {
	pb_wait_for_vbl(); // TODO: only when vsync? move to end frame?
	pb_reset();
	pb_target_back_buffer();

	uint32_t* p = pb_begin();
	p = NV2A_reset_control0(p);
	pb_end(p);
}

void Gfx_EndFrame(void) {
	while (pb_busy())     { } // Wait for frame completion
	while (pb_finished()) { } // Swap when possible
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	uint32_t* p = pb_begin();
	p = NV2A_clear_buffers(p, buffers & GFX_BUFFER_COLOR, buffers & GFX_BUFFER_DEPTH);
	pb_end(p);
	
	while (pb_busy()) { } // Wait for completion TODO: necessary??
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint8* gfx_vertices;

static void* AllocBuffer(int count, int elemSize) {
	return MmAllocateContiguousMemoryEx(count * elemSize, 0, MAX_RAM_ADDR, 16, PAGE_WRITECOMBINE | PAGE_READWRITE);
}

static void FreeBuffer(GfxResourceID* buffer) {
	GfxResourceID ptr = *buffer;
	if (ptr) MmFreeContiguousMemory(ptr);
	*buffer = NULL;
}


GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)    { }

void Gfx_DeleteIb(GfxResourceID* ib) { }


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return AllocBuffer(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { 
	gfx_vertices = vb;
	uint32_t* p  = pb_begin();

	p = NV2A_set_vertex_attrib_pointer(p, VERTEX_ATTR_INDEX,  gfx_vertices +  0);
	p = NV2A_set_vertex_attrib_pointer(p, COLOUR_ATTR_INDEX,  gfx_vertices + 12);
	p = NV2A_set_vertex_attrib_pointer(p, TEXTURE_ATTR_INDEX, gfx_vertices + 16);
	// Harmless to set TEXTURE_ATTR_INDEX, even when vertex format is coloured only
	pb_end(p);
}

void Gfx_DeleteVb(GfxResourceID* vb) { FreeBuffer(vb); }

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) { return vb; }

void Gfx_UnlockVb(GfxResourceID vb) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;

	uint32_t* p = pb_begin();
	p = NV2A_VS_set_active(p, VS_CalcActiveProgram());
	p = NV2A_fog_set_enabled(p, enabled);
	pb_end(p);
	PS_UpdateActive();
}

static void SetFogColor(PackedCol color) {
	uint32_t* p = pb_begin();

	p = NV2A_fog_set_color(p,
					PackedCol_R(color),
					PackedCol_G(color),
					PackedCol_B(color),
					PackedCol_A(color));
	pb_end(p);
}

static void UpdateFog(void) {
	int mode;
	float bias, scale;

	// https://github.com/xemu-project/xemu/blob/478b4f496102379c7eaa7f3ec10e714a703c4300/hw/xbox/nv2a/pgraph/glsl/vsh.c#L334
	switch (gfx_fogMode) {
		case FOG_LINEAR:
			// FACTOR    = fogParam.x + DIST * fogParam.y - 1.0
			// GL_LINEAR = (end-DIST)/end
			// GL_LINEAR = 1.0 - DIST/end
			// GL_LINEAR = 2.0 - DIST/end - 1.0
			// GL_LINEAR = 2.0 + DIST * (-1/end) - 1.0

			// therefore fogParam.x = 2.0 and fogParam.y = (-1/end)
			mode  = NV097_SET_FOG_MODE_V_LINEAR;
			scale = -1.0f / gfx_fogEnd;
			bias  = 2.0f;
			break;

		case FOG_EXP:
			// FACTOR = fogParam.x + exp2(DIST * fogParam.y * 16.0) - 1.5
			// GL_EXP = exp(-density * DIST)
			// GL_EXP = 1.5 + exp(-density*DIST) - 1.5
			// GL_EXP = 1.5 + exp(DIST * -density/16.0 * 16.0) - 1.5
			// GL_EXP = 1.5 + exp2(log2(e) * DIST * -density/16.0 * 16.0) - 1.5
			// GL_EXP = 1.5 + exp2(DIST * (-density * log2(e)/16.0) * 16.0) - 1.5
			// GL_EXP = 1.5 + exp2(DIST * (-density/(16.0*loge(2)) * 16.0) - 1.5  (using rule logx(y) = 1/logy(x))

			// therefore fogParam.x = 1.5 and fogParam.y = -density/(16.0*loge(2))
			#define LOGE_2 0.693147f
			mode  = NV097_SET_FOG_MODE_V_EXP;
			bias  = 1.5f;
 			scale = -gfx_fogDensity / (16 * LOGE_2);
			break;

		case FOG_EXP2:
			// FACTOR  = fogParam.x + exp2(-DIST * DIST * fogParam.y * fogParam.y * 32.0) - 1.5
			// GL_EXP2 = exp(-(density * DIST)*(density*DIST))
			// GL_EXP2 = 1.5 + exp(-DIST * DIST * density * density) - 1.5
			// GL_EXP2 = 1.5 + exp(-DIST * DIST * (density/sqrt(32)) * (density/sqrt(32)) * 32.0) - 1.5
			// GL_EXP2 = 1.5 + exp2(log2(e) * -DIST * DIST * (density/sqrt(32)) * (density/sqrt(32)) * 32.0) - 1.5
			// GL_EXP2 = 1.5 + exp2(-DIST * DIST * (density*sqrt(log2(e))/sqrt(32)) * (density*sqrt(log2(e)/sqrt(32)) * 32.0) - 1.5
			// GL_EXP2 = 1.5 + exp2(-DIST * DIST * (density*sqrt(log2(e)/32)) * (density*sqrt(log2(e)/32)) * 32.0) - 1.5
			// GL_EXP2 = 1.5 + exp2(-DIST * DIST * (density*sqrt(1/(loge(2)*32))) * (density*sqrt(1/(loge(2)*32))) * 32.0) - 1.5
			// GL_EXP2 = 1.5 + exp2(-DIST * DIST * (density/sqrt(loge(2)*32)) * (density/sqrt(loge(2)*32)) * 32.0) - 1.5

			// therefore fogParam.x = 1.5 and fogParam.y = density/sqrt(loge(2)*32)
			#define SQRT_32LOGE_2 4.70964f
			mode  = NV097_SET_FOG_MODE_V_EXP2;
			bias  = 1.5f;
			scale = gfx_fogDensity / SQRT_32LOGE_2;
			break;
	}
	
	uint32_t* p = pb_begin();
	p = NV2A_fog_set_mode(p, mode);
	p = NV2A_fog_set_params(p, bias, scale);
	pb_end(p);
}

static void SetFogDensity(float value) { UpdateFog(); }

static void SetFogEnd(float value)     { UpdateFog(); }

static void SetFogMode(FogFunc func)   { UpdateFog(); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/

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
}

// https://github.com/XboxDev/nxdk/blob/master/samples/mesh/math3d.c#L292
static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
	// TODO: The above matrix breaks the held block
	// Below works but breaks map rendering
	
/*
	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -zFar / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear * zFar) / (zFar - zNear);
	matrix->row4.w =  0.0f;*/
}

void Gfx_OnWindowResize(int width, int height) {
	Gfx_SetScissor( 0, 0, width, height);
	Gfx_SetViewport(0, 0, width, height);
}

//static struct Vec4 vp_scale  = { 320, -240, 8388608, 1 };
//static struct Vec4 vp_offset = { 320,  240, 8388608, 1 };
static struct Vec4 vp_scale, vp_offset;
static struct Matrix _view, _proj, _mvp;
// TODO Upload constants too
// if necessary, look at vs.inl output for 'c[5]' etc..

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	struct Matrix* dst = type == MATRIX_PROJ ? &_proj : &_view;
	*dst = *matrix;
	Matrix_Mul(&_mvp, &_view, &_proj);

	struct Matrix final;
	struct Matrix vp = Matrix_Identity;
	vp.row1.x = vp_scale.x;
	vp.row2.y = vp_scale.y;
	vp.row3.z = 8388608; // 2^24 / 2
	vp.row4.x = vp_offset.x;
	vp.row4.y = vp_offset.y;
	vp.row4.z = 8388608; // 2^24 / 2

	Matrix_Mul(&final, &_mvp, &vp);

	uint32_t* p;
	p = pb_begin();
	p = NV2A_VS_upload_constants(p, VS_CONST_MVP, &final, 16);
	pb_end(p);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Mem_Copy(mvp, &_mvp, sizeof(struct Matrix));
}

void Gfx_EnableTextureOffset(float x, float y) {
	struct Vec4 offset = { x, y, 0, 0 };
	uint32_t* p = pb_begin();
	tex_offset  = true;

	p = NV2A_VS_upload_constants(p, VS_CONST_OFST, &offset, 4);
	p = NV2A_VS_set_active(p, VS_CalcActiveProgram());
	pb_end(p);
}

void Gfx_DisableTextureOffset(void) {
	uint32_t* p = pb_begin();
	tex_offset  = false;

	p = NV2A_VS_set_active(p, VS_CalcActiveProgram());
	pb_end(p);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	vp_scale.x  = w *  0.5f;
	vp_scale.y  = h * -0.5f;
	vp_offset.x = x + w * 0.5f;
	vp_offset.y = y + h * 0.5f;
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	uint32_t* p = pb_begin();
	p = NV2A_set_clip_rect(p, x, y, w, h);
	pb_end(p);
}


/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	uint32_t* p = pb_begin();

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		p = NV2A_set_vertex_attrib_format(p, VERTEX_ATTR_INDEX,  
					NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,      3, SIZEOF_VERTEX_TEXTURED);
		p = NV2A_set_vertex_attrib_format(p, COLOUR_ATTR_INDEX,  
					NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, SIZEOF_VERTEX_TEXTURED);
		p = NV2A_set_vertex_attrib_format(p, TEXTURE_ATTR_INDEX, 
					NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,      2, SIZEOF_VERTEX_TEXTURED);
	} else {
		p = NV2A_set_vertex_attrib_format(p, VERTEX_ATTR_INDEX, 
					NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,      3, SIZEOF_VERTEX_COLOURED);
		p = NV2A_set_vertex_attrib_format(p, COLOUR_ATTR_INDEX, 
					NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 4, SIZEOF_VERTEX_COLOURED);
		p = NV2A_set_vertex_attrib_format(p, TEXTURE_ATTR_INDEX, 
					NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,      0, 0);
	}

	p = NV2A_VS_set_active(p, VS_CalcActiveProgram());
	pb_end(p);
	
	PS_UpdateActive();
}

void Gfx_DrawVb_Lines(int verticesCount) {
	NV2A_DrawArrays(NV097_SET_BEGIN_END_OP_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	NV2A_DrawArrays(NV097_SET_BEGIN_END_OP_QUADS, startVertex, verticesCount);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	NV2A_DrawArrays(NV097_SET_BEGIN_END_OP_QUADS,           0, verticesCount);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	NV2A_DrawArrays(NV097_SET_BEGIN_END_OP_QUADS, startVertex, verticesCount);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using XBox --\n");
	PrintMaxTextureInfo(info);
}

