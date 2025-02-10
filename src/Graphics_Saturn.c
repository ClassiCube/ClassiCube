#include "Core.h"
#if defined CC_BUILD_SATURN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <stdint.h>
#include <yaul.h>
#include <stdlib.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 224
#define CMDS_COUNT 400
#define HDR_CMDS 2

static struct {
	vdp1_cmdt_t hdrs[HDR_CMDS];
	vdp1_cmdt_t list[CMDS_COUNT];
	vdp1_cmdt_t extra[1]; // extra room for 'end' command if needed
} cmds;
static uint16_t z_table[CMDS_COUNT];
static int cmds_count, cmds_3DCount;

static PackedCol clear_color;
static vdp1_vram_partitions_t _vdp1_vram_partitions;
static void* tex_vram_addr;
static void* tex_vram_cur;
static int tex_width = 8, tex_height = 8;
static vdp1_gouraud_table_t* gourad_base;
static cc_bool noMemWarned;

// NOINLINE to avoid polluting the hot path
static CC_NOINLINE vdp1_cmdt_t* NextPrimitive_nomem(void) {
	if (noMemWarned) return NULL;
	noMemWarned = true;
	
	Platform_LogConst("OUT OF VERTEX RAM");
	return NULL;
}

static vdp1_cmdt_t* NextPrimitive(uint16_t z) {
	if (cmds_count >= CMDS_COUNT) return NextPrimitive_nomem();

	z_table[cmds_count] = UInt16_MaxValue - z;
	return &cmds.list[cmds_count++];
}

static const vdp1_cmdt_draw_mode_t color_draw_mode = {
	.cc_mode    = VDP1_CMDT_CC_REPLACE,
	.color_mode = VDP1_CMDT_CM_RGB_32768
};
static const vdp1_cmdt_draw_mode_t shaded_draw_mode = {
	.cc_mode    = VDP1_CMDT_CC_GOURAUD,
	.color_mode = VDP1_CMDT_CM_RGB_32768
};

static void UpdateVDP1Env(void) {
	vdp1_env_t env;
	vdp1_env_default_init(&env);

	int R = PackedCol_R(clear_color);
	int G = PackedCol_G(clear_color);
	int B = PackedCol_B(clear_color);
	env.erase_color = RGB1555(1, R >> 3, G >> 3, B >> 3);

	vdp1_env_set(&env);
}

static void CalcGouraudColours(void) {
	for (int i = 0; i < 1024; i++)
	{
		// 1024 = 10 bits, divided into RRR GGGG BBB
		int r_idx = (i & 0x007) >> 0, R = r_idx << (5 - 3);
		int g_idx = (i & 0x078) >> 3, G = g_idx << (5 - 4);
		int b_idx = (i & 0x380) >> 7, B = b_idx << (5 - 3);
		rgb1555_t gouraud = RGB1555(1, R, G, B);
		
		vdp1_gouraud_table_t* cur = &_vdp1_vram_partitions.gouraud_base[i];
		cur->colors[0] = gouraud;
		cur->colors[1] = gouraud;
		cur->colors[2] = gouraud;
		cur->colors[3] = gouraud;
	}
}

static GfxResourceID white_square;
void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	struct Bitmap bmp;
	BitmapCol pixels[8 * 8];
	Mem_Set(pixels, 0xFF, sizeof(pixels));

	Bitmap_Init(bmp, 8, 8, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
	if (!Gfx.Created) {
        vdp1_vram_partitions_get(&_vdp1_vram_partitions);
		// TODO less ram for gourad base
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));
        vdp2_sprite_priority_set(0, 6);

		tex_vram_addr = _vdp1_vram_partitions.texture_base;
		tex_vram_cur  = _vdp1_vram_partitions.texture_base;
		gourad_base   = _vdp1_vram_partitions.gouraud_base;

		UpdateVDP1Env();
		CalcGouraudColours();
	}

	Gfx.MinTexWidth  =  8;
	Gfx.MinTexHeight =  8;
	Gfx.MaxTexWidth  = 128;
	Gfx.MaxTexHeight = 16; // 128
	Gfx.Created      = true;
	Gfx.Limitations  = GFX_LIMIT_NO_UV_SUPPORT;
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture {
	int width, height;
	void* addr;
} CCTexture;

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = Mem_TryAlloc(1, sizeof(CCTexture));
	if (!tex) return NULL;

	// use malloc to ensure tmp is in HRAM (can't DMA from LRAM))
	cc_uint16* tmp = malloc(bmp->width * bmp->height * 2);
	if (!tmp) return NULL;

	tex->addr   = tex_vram_addr;
	tex->width  = bmp->width;
	tex->height = bmp->height;

	tex_vram_addr += tex->width * tex->height * 2;
	int avail = (char*)gourad_base - (char*)tex_vram_addr;
	if (avail <= 0) {
		Platform_LogConst("OUT OF VRAM");
		Mem_Free(tmp); 
		return NULL; 
	} else {
		Platform_Log1("VRAM: %i bytes left", &avail);	
	}

	// TODO: Only copy when rowWidth != bmp->width
	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint16* src = bmp->scan0 + y * rowWidth;
		cc_uint16* dst = tmp        + y * bmp->width;

		for (int x = 0; x < bmp->width; x++)
		{
			dst[x] = src[x];
		}
	}

	scu_dma_transfer(0, tex->addr, tmp, tex->width * tex->height * 2);
	scu_dma_transfer_wait(0);
	free(tmp);
	return tex;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = (CCTexture*)texId;

	tex_vram_cur = tex->addr;
	tex_width    = tex->width;
	tex_height   = tex->height;
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = *texId;
	// TODO properly free vram
	if (tex) {
		// This is mainly to avoid leak with text in top left
		int size = tex->width * tex->height * 2;
		if (tex_vram_addr == tex->addr + size)
			tex_vram_addr -= size;

		Mem_Free(tex);
	}
	*texId = NULL;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

void Gfx_SetFaceCulling(cc_bool enabled) {
	// TODO
}

static void SetAlphaTest(cc_bool enabled) {
}

static void SetAlphaBlend(cc_bool enabled) {
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearBuffers(GfxBuffers buffers) {
}

void Gfx_ClearColor(PackedCol color) {
	if (color == clear_color) return;

	clear_color = color;
	UpdateVDP1Env();
}

void Gfx_SetDepthTest(cc_bool enabled) {
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	// TODO
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

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
// Preprocess vertex buffers into optimised layout for Saturn
struct SATVertexColoured { int x, y, z; PackedCol Col; };
struct SATVertexTextured { int x, y, z; PackedCol Col; int flip, pad; };
static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

#define XYZInteger(value) ((value) >> 6)
#define XYZFixed(value)   ((int)((value) * (1 << 6)))

static void* gfx_vertices;

static void PreprocessTexturedVertices(void) {
	struct SATVertexTextured* dst = gfx_vertices;
	struct VertexTextured* src    = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = XYZFixed(src->x);
		dst->y = XYZFixed(src->y);
		dst->z = XYZFixed(src->z);

        int r = PackedCol_R(src->Col);
        int g = PackedCol_G(src->Col);
        int b = PackedCol_B(src->Col);
        dst->Col = ((b >> 5) << 7) | ((g >> 4) << 3) | (r >> 5);
    }

	dst = gfx_vertices;
	src = gfx_vertices;
	for (int i = 0; i < buf_count; i += 4, src += 4, dst += 4)
	{
        int flipped = src[0].V > src[2].V;
		dst->flip   = flipped ? VDP1_CMDT_CHAR_FLIP_V : VDP1_CMDT_CHAR_FLIP_NONE;
    }
}

static void PreprocessColouredVertices(void) {
	struct SATVertexColoured* dst = gfx_vertices;
	struct VertexColoured* src    = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = XYZFixed(src->x);
		dst->y = XYZFixed(src->y);
		dst->z = XYZFixed(src->z);

        int r = PackedCol_R(src->Col);
        int g = PackedCol_G(src->Col);
        int b = PackedCol_B(src->Col);
        dst->Col = RGB1555(1, r >> 3, g >> 3, b >> 3).raw;
    }
}


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return Mem_TryAlloc(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
    buf_fmt   = fmt;
    buf_count = count;
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
    gfx_vertices = vb;

    if (buf_fmt == VERTEX_FORMAT_TEXTURED) {
        PreprocessTexturedVertices();
    } else {
        PreprocessColouredVertices();
    }
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return Gfx_LockVb(vb, fmt, count);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_UnlockVb(vb); }

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;
struct MatrixRow { int x, y, z, w; };
static struct MatrixRow mvp_row1, mvp_row2, mvp_row3, mvp_trans;

#define ToFixed(v) (int)(v * (1 << 12))

static void LoadTransformMatrix(struct Matrix* src) {
	mvp_trans.x = XYZFixed(1) * ToFixed(src->row4.x);
	mvp_trans.y = XYZFixed(1) * ToFixed(src->row4.y);
	mvp_trans.z = XYZFixed(1) * ToFixed(src->row4.z);
	mvp_trans.w = XYZFixed(1) * ToFixed(src->row4.w);
	
	mvp_row1.x = ToFixed(src->row1.x);
	mvp_row1.y = ToFixed(src->row1.y);
	mvp_row1.z = ToFixed(src->row1.z);
	mvp_row1.w = ToFixed(src->row1.w);
	
	mvp_row2.x = ToFixed(src->row2.x);
	mvp_row2.y = ToFixed(src->row2.y);
	mvp_row2.z = ToFixed(src->row2.z);
	mvp_row2.w = ToFixed(src->row2.w);
	
	mvp_row3.x = ToFixed(src->row3.x);
	mvp_row3.y = ToFixed(src->row3.y);
	mvp_row3.z = ToFixed(src->row3.z);
	mvp_row3.w = ToFixed(src->row3.w);
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	struct Matrix mvp;
	if (matrix == &Matrix_Identity && type == MATRIX_VIEW) {
		mvp = _proj; // 2D mode uses identity view matrix
	} else {
		Matrix_Mul(&mvp, &_view, &_proj);
	}
	
	LoadTransformMatrix(&mvp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	_view = *view;
	_proj = *proj;

	Matrix_Mul(mvp, view, proj);
	LoadTransformMatrix(mvp);
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO
}

void Gfx_DisableTextureOffset(void) {
	// TODO
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
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.01f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
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
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {

}

static int Transform(IVec3* result, struct SATVertexTextured* a) {
	int w = a->x * mvp_row1.w + a->y * mvp_row2.w + a->z * mvp_row3.w + mvp_trans.w;
	if (w <= 0) return 1;

	int x = a->x * mvp_row1.x + a->y * mvp_row2.x + a->z * mvp_row3.x + mvp_trans.x;
	cpu_divu_32_32_set(x * (SCREEN_WIDTH/2), w);

	int y = a->x * mvp_row1.y + a->y * mvp_row2.y + a->z * mvp_row3.y + mvp_trans.y;
	result->x = cpu_divu_quotient_get();
	cpu_divu_32_32_set(y * -(SCREEN_HEIGHT/2), w);

	int z = a->x * mvp_row1.z + a->y * mvp_row2.z + a->z * mvp_row3.z + mvp_trans.z;
	result->y = cpu_divu_quotient_get();
	cpu_divu_32_32_set(z * 512, w);

	if (result->x < -2048 || result->x > 2048 || result->y < -2048 || result->y > 2048) return 1;

	result->z = cpu_divu_quotient_get();
	return result->z < 0 || result->z > 512;
}

#define Coloured2D_X(value) XYZInteger(value) - SCREEN_WIDTH  / 2
#define Coloured2D_Y(value) XYZInteger(value) - SCREEN_HEIGHT / 2

static void DrawColouredQuads2D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct SATVertexColoured* v = (struct SATVertexColoured*)gfx_vertices + startVertex + i;

		int16_vec2_t points[4];
		points[0].x = Coloured2D_X(v[0].x); points[0].y = Coloured2D_Y(v[0].y);
		points[1].x = Coloured2D_X(v[1].x); points[1].y = Coloured2D_Y(v[1].y);
		points[2].x = Coloured2D_X(v[2].x); points[2].y = Coloured2D_Y(v[2].y);
		points[3].x = Coloured2D_X(v[3].x); points[3].y = Coloured2D_Y(v[3].y);

		rgb1555_t color; color.raw = v->Col;
		vdp1_cmdt_t* cmd = NextPrimitive(0);
		if (!cmd) return;

		vdp1_cmdt_polygon_set(cmd);
		vdp1_cmdt_color_set(cmd,     color);
		vdp1_cmdt_draw_mode_set(cmd, color_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);
	}
}

#define Textured2D_X(value) XYZInteger(value) - SCREEN_WIDTH  / 2
#define Textured2D_Y(value) XYZInteger(value) - SCREEN_HEIGHT / 2

static void DrawTexturedQuads2D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct SATVertexTextured* v = (struct SATVertexTextured*)gfx_vertices + startVertex + i;

		int16_vec2_t points[4];
		points[0].x = Textured2D_X(v[0].x); points[0].y = Textured2D_Y(v[0].y);
		points[1].x = Textured2D_X(v[1].x); points[1].y = Textured2D_Y(v[1].y);
		points[2].x = Textured2D_X(v[2].x); points[2].y = Textured2D_Y(v[2].y);
		points[3].x = Textured2D_X(v[3].x); points[3].y = Textured2D_Y(v[3].y);

		vdp1_cmdt_t* cmd = NextPrimitive(0);
		if (!cmd) return;

		vdp1_cmdt_distorted_sprite_set(cmd);
		vdp1_cmdt_char_size_set(cmd, tex_width, tex_height);
		vdp1_cmdt_char_base_set(cmd, (vdp1_vram_t)tex_vram_cur);
		vdp1_cmdt_char_flip_set(cmd, v->flip);
		vdp1_cmdt_draw_mode_set(cmd, v->Col == 1023 ? color_draw_mode : shaded_draw_mode);
		vdp1_cmdt_gouraud_base_set(cmd, (vdp1_vram_t)&gourad_base[v->Col]);
		vdp1_cmdt_vtx_set(cmd, 		 points);
	}
}

static void DrawColouredQuads3D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct SATVertexColoured* v = (struct SATVertexColoured*)gfx_vertices + startVertex + i;

		IVec3 coords[4];
		int clipped = 0;
		clipped |= Transform(&coords[0], &v[0]);
		clipped |= Transform(&coords[1], &v[1]);
		clipped |= Transform(&coords[2], &v[2]);
		clipped |= Transform(&coords[3], &v[3]);
		if (clipped) continue;

		int16_vec2_t points[4];
		points[0].x = coords[0].x; points[0].y = coords[0].y;
		points[1].x = coords[1].x; points[1].y = coords[1].y;
		points[2].x = coords[2].x; points[2].y = coords[2].y;
		points[3].x = coords[3].x; points[3].y = coords[3].y;

		int z = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) >> 2;
		rgb1555_t color; color.raw = v->Col;
		vdp1_cmdt_t* cmd = NextPrimitive(z);
		if (!cmd) return;

		vdp1_cmdt_polygon_set(cmd);
		vdp1_cmdt_color_set(cmd,     color);
		vdp1_cmdt_draw_mode_set(cmd, color_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);
	}
}

static void DrawTexturedQuads3D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct SATVertexTextured* v = (struct SATVertexTextured*)gfx_vertices + startVertex + i;

		IVec3 coords[4];
		int clipped = 0;
		clipped |= Transform(&coords[0], &v[0]);
		clipped |= Transform(&coords[1], &v[1]);
		clipped |= Transform(&coords[2], &v[2]);
		clipped |= Transform(&coords[3], &v[3]);
		if (clipped) continue;

		int16_vec2_t points[4];
		points[0].x = coords[0].x; points[0].y = coords[0].y;
		points[1].x = coords[1].x; points[1].y = coords[1].y;
		points[2].x = coords[2].x; points[2].y = coords[2].y;
		points[3].x = coords[3].x; points[3].y = coords[3].y;

		int z = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) >> 2;
		vdp1_cmdt_t* cmd = NextPrimitive(z);
		if (!cmd) return;

		vdp1_cmdt_distorted_sprite_set(cmd);
		vdp1_cmdt_char_size_set(cmd, tex_width, tex_height);
		vdp1_cmdt_char_base_set(cmd, (vdp1_vram_t)tex_vram_cur);
		vdp1_cmdt_char_flip_set(cmd, v->flip);
		vdp1_cmdt_draw_mode_set(cmd, v->Col == 1023 ? color_draw_mode : shaded_draw_mode);
		vdp1_cmdt_gouraud_base_set(cmd, (vdp1_vram_t)&gourad_base[v->Col]);
		vdp1_cmdt_vtx_set(cmd, 		 points);
	}
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	if (gfx_rendering2D) {
		if (gfx_format == VERTEX_FORMAT_TEXTURED) {
			DrawTexturedQuads2D(verticesCount, startVertex);
		} else {
			DrawColouredQuads2D(verticesCount, startVertex);
		}
	} else {
		if (gfx_format == VERTEX_FORMAT_TEXTURED) {
			DrawTexturedQuads3D(verticesCount, startVertex);
		} else {
			DrawColouredQuads3D(verticesCount, startVertex);
		}
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	Gfx_DrawVb_IndexedTris_Range(verticesCount, 0, DRAW_HINT_NONE);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawTexturedQuads3D(verticesCount, startVertex);
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_BeginFrame(void) {
	//Platform_LogConst("FRAME BEG");
	cmds_count   = 0;
	cmds_3DCount = 0;

	static const int16_vec2_t system_clip_coord  = { SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
	static const int16_vec2_t local_coord_center = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
	vdp1_cmdt_t* cmd;

	cmd = &cmds.hdrs[0];
	vdp1_cmdt_system_clip_coord_set(cmd);
	vdp1_cmdt_vtx_system_clip_coord_set(cmd, system_clip_coord);

	cmd = &cmds.hdrs[1];
	vdp1_cmdt_local_coord_set(cmd);
	vdp1_cmdt_vtx_local_coord_set(cmd, local_coord_center);
}

static void SortCommands(int left, int right) {
	vdp1_cmdt_t* values = cmds.list, value;
	uint16_t* keys = z_table, key;

	while (left < right) {
		int i = left, j = right;
		int pivot = keys[(i + j) >> 1];

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(SortCommands)
	}
}

void Gfx_EndFrame(void) {
	//Platform_LogConst("FRAME END");
	vdp1_cmdt_t* cmd;

	// TODO optimise Z sorting for 3D polygons
	if (cmds_3DCount) SortCommands(0, cmds_3DCount - 1);

	cmd = NextPrimitive(UInt16_MaxValue);
	if (!cmd) { cmd = &cmds.extra[0]; cmds_count++; }
	vdp1_cmdt_end_set(cmd);

	vdp1_cmdt_list_t cmdt_list;
	cmdt_list.cmdts = cmds.hdrs;
    cmdt_list.count = HDR_CMDS + cmds_count;
	vdp1_sync_cmdt_list_put(&cmdt_list, 0);

	vdp1_sync_render();
	vdp1_sync();
	vdp2_sync();
	vdp2_sync_wait();
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_SetViewport(int x, int y, int w, int h) { }
void Gfx_SetScissor (int x, int y, int w, int h) { }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Saturn --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_Begin2D(int width, int height) {
	Gfx_SetAlphaBlending(true);
	gfx_rendering2D = true;
	cmds_3DCount = cmds_count;
}

void Gfx_End2D(void) {
	Gfx_SetAlphaBlending(false);
	gfx_rendering2D = false;
}
#endif
