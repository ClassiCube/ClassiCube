#define CC_DYNAMIC_VBS_ARE_STATIC
#define OVERRIDE_BEGEND2D_FUNCTIONS
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"
#include "../_BlockAlloc.h"

#include <stdint.h>
#include <yaul.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 224
#define CMDS_COUNT 450
#define HDR_CMDS     2

static struct {
	vdp1_cmdt_t hdrs[HDR_CMDS];
	vdp1_cmdt_t list[CMDS_COUNT];
	vdp1_cmdt_t extra; // extra room for 'end' command if needed
} cmds;
static uint16_t z_table[CMDS_COUNT];
static int cmds_3DCount;

static vdp1_cmdt_t* cmds_cur;
static uint16_t* z_cur;

static PackedCol clear_color;

static const vdp1_cmdt_draw_mode_t color_draw_mode = {
	.cc_mode    = VDP1_CMDT_CC_REPLACE,
	.color_mode = VDP1_CMDT_CM_RGB_32768
};
static const vdp1_cmdt_draw_mode_t shaded_draw_mode = {
	.cc_mode    = VDP1_CMDT_CC_GOURAUD,
	.color_mode = VDP1_CMDT_CM_RGB_32768
};

#define CMDT_REGION_COUNT    2048
#define TEXTURE_REGION_SIZE  442000
#define GOURAUD_REGION_COUNT 1024
#define CLUT_REGION_COUNT    256

static vdp1_gouraud_table_t* gouraud_base;
static vdp1_clut_t* clut_base;
static vdp1_cmdt_t* cmdt_base;
static void* texture_base;

static void InitVRAM(void) {
    vdp1_vram_t ptr = VDP1_VRAM(0);

    cmdt_base = (vdp1_cmdt_t*)ptr;
    ptr += CMDT_REGION_COUNT * sizeof(vdp1_cmdt_t);

    texture_base = (void*)ptr;
    ptr += TEXTURE_REGION_SIZE;

    gouraud_base = (vdp1_gouraud_table_t*)ptr;
    ptr += GOURAUD_REGION_COUNT * sizeof(vdp1_gouraud_table_t);

    clut_base = (vdp1_clut_t*)ptr;
    ptr += CLUT_REGION_COUNT * sizeof(vdp1_clut_t);

	int S = (int)(ptr - VDP1_VRAM(sizeof(vdp1_cmdt_t)));
	int T = VDP1_VRAM_SIZE;
	Platform_Log2("VRAM ALLOC: %i of %i", &S, &T);

	if (ptr <= VDP1_VRAM(VDP1_VRAM_SIZE)) return;
	Process_Abort("Invalid VRAM allocations");
}

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
		
		vdp1_gouraud_table_t* cur = &gouraud_base[i];
		cur->colors[0] = gouraud;
		cur->colors[1] = gouraud;
		cur->colors[2] = gouraud;
		cur->colors[3] = gouraud;
	}
}

static GfxResourceID white_square;
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	struct Bitmap bmp;
	BitmapCol pixels[8 * 8];
	Mem_Set(pixels, 0xFF, sizeof(pixels));

	Bitmap_Init(bmp, 8, 8, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

static void SetupHeaderCommands(void) {
	vdp1_cmdt_t* cmd;

	cmd = &cmds.hdrs[0];
	cmd->cmd_ctrl = VDP1_CMDT_SYSTEM_CLIP_COORD;
	cmd->cmd_xc   = SCREEN_WIDTH  - 1;
	cmd->cmd_yc   = SCREEN_HEIGHT - 1;

	cmd = &cmds.hdrs[1];
	cmd->cmd_ctrl = VDP1_CMDT_LOCAL_COORD;
	cmd->cmd_xa   = SCREEN_WIDTH  / 2;
	cmd->cmd_ya   = SCREEN_HEIGHT / 2;
}

void Gfx_Create(void) {
	if (!Gfx.Created) {
		// TODO less ram for gouraud base
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));
        vdp2_sprite_priority_set(0, 6);

		InitVRAM();
		UpdateVDP1Env();
		CalcGouraudColours();
	}

	Gfx.MinTexWidth  =  8;
	Gfx.MinTexHeight =  8;
	Gfx.MaxTexWidth  = 128;
	Gfx.MaxTexHeight = 16; // 128
	Gfx.Created      = true;
	Gfx.Limitations  = GFX_LIMIT_NO_UV_SUPPORT | GFX_LIMIT_MAX_VERTEX_SIZE;
	SetupHeaderCommands();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static uint16_t cur_char_size;
static uint16_t cur_char_base;

typedef struct CCTexture {
	short width, height;
	unsigned offset, blocks;
} CCTexture;
#define CCTexture_Addr(tex) (texture_base + (tex)->offset * TEX_BLOCK_SIZE)

#define TEX_BLOCK_SIZE 256
#define TEX_TOTAL_BLOCKS (TEXTURE_REGION_SIZE / TEX_BLOCK_SIZE)
static cc_uint8 tex_table[TEX_TOTAL_BLOCKS / BLOCKS_PER_PAGE];

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = Mem_TryAlloc(1, sizeof(CCTexture));
	if (!tex) return NULL;

	int width  = bmp->width, height = bmp->height;
	int blocks = SIZE_TO_BLOCKS(width * height * BITMAPCOLOR_SIZE, TEX_BLOCK_SIZE);
	int addr   = blockalloc_alloc(tex_table, TEX_TOTAL_BLOCKS, blocks);

	if (addr == -1) {
		Platform_LogConst("OUT OF VRAM");
		Mem_Free(tex);
		return NULL; 
	}

	tex->width  = width;
	tex->height = height;
	tex->offset = addr;
	tex->blocks = blocks;

	int vram_free, vram_used;
	blockalloc_calc_usage(tex_table, TEX_TOTAL_BLOCKS, TEX_BLOCK_SIZE,
							&vram_free, &vram_used);
	Platform_Log2("VRAM: %i bytes left (%i used)", &vram_free, &vram_used);

	cc_uint8*  tmp = CCTexture_Addr(tex);
	cc_uint16* src = bmp->scan0;
	cc_uint16* dst = (cc_uint16*)tmp;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			*dst++ = src[x];
		}
		src += rowWidth;
	}
	return tex;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = (CCTexture*)texId;
	cc_uint8* addr = CCTexture_Addr(tex);

	cur_char_size  = (((tex->width >> 3) << 8) | tex->height) & 0x3FFF;
	cur_char_base  = ((vdp1_vram_t)addr >> 3) & 0xFFFF;
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = *texId;
	if (!tex) return;

	blockalloc_dealloc(tex_table, tex->offset, tex->blocks);
	Mem_Free(tex);
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

static void PreprocessTexturedVertices(void* vertices) {
	struct SATVertexTextured* dst = vertices;
	struct VertexTextured* src    = vertices;

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

	dst = vertices;
	src = vertices;
	for (int i = 0; i < buf_count; i += 4, src += 4, dst += 4)
	{
        int flipped = src[0].V > src[2].V;
		dst->flip   = flipped ? VDP1_CMDT_CHAR_FLIP_V : VDP1_CMDT_CHAR_FLIP_NONE;
    }
}

static void PreprocessColouredVertices(void* vertices) {
	struct SATVertexColoured* dst = vertices;
	struct VertexColoured* src    = vertices;

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
    if (buf_fmt == VERTEX_FORMAT_TEXTURED) {
        PreprocessTexturedVertices(vb);
    } else {
        PreprocessColouredVertices(vb);
    }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;
struct MatrixCol { int trans, col1, col2, col3; };
struct MatrixMVP { struct MatrixCol w, x, y; };
static struct MatrixMVP mvp_;

#define ToFixed(v) (int)(v * (1 << 12))

static void LoadTransformMatrix(struct Matrix* src) {
	mvp_.x.trans = XYZFixed(1) * ToFixed(src->row4.x);
	mvp_.y.trans = XYZFixed(1) * ToFixed(src->row4.y);
	mvp_.w.trans = XYZFixed(1) * ToFixed(src->row4.w);
	
	mvp_.x.col1 = ToFixed(src->row1.x);
	mvp_.y.col1 = ToFixed(src->row1.y);
	mvp_.w.col1 = ToFixed(src->row1.w);
	
	mvp_.x.col2 = ToFixed(src->row2.x);
	mvp_.y.col2 = ToFixed(src->row2.y);
	mvp_.w.col2 = ToFixed(src->row2.w);
	
	mvp_.x.col3 = ToFixed(src->row3.x);
	mvp_.y.col3 = ToFixed(src->row3.y);
	mvp_.w.col3 = ToFixed(src->row3.w);
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

// Calculates v->x * col1 + v->y * col2 + v->z * col3 + trans
static inline int __attribute__((always_inline)) TransformVector(void* v, struct MatrixCol* col) {
    int res;

    __asm__("clrmac                   \n"
			"lds.l @%[col]+, MACL     \n"
			"mac.l @%[col]+, @%[vec]+ \n"
			"mac.l @%[col]+, @%[vec]+ \n"
			"mac.l @%[col]+, @%[vec]+ \n"
 			"sts       macl,  %[res]  \n"
 			: [col] "+r" (col), [vec] "+r" (v), [res] "=r" (res)
 			: "m" (*v), "m" (*col)
 			: "mach", "macl", "memory");
    return res;
}

#define Coloured2D_X(value) XYZInteger(value) - SCREEN_WIDTH  / 2
#define Coloured2D_Y(value) XYZInteger(value) - SCREEN_HEIGHT / 2

static void DrawColouredQuads2D(int verticesCount, int startVertex) {
	struct SATVertexColoured* v = (struct SATVertexColoured*)gfx_vertices + startVertex;
	return; // TODO menus invisible otherwise

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{
		if (cmds_cur >= &cmds.extra) return;
		vdp1_cmdt_t* cmd = cmds_cur++;

		cmd->cmd_ctrl = VDP1_CMDT_POLYGON;
		cmd->cmd_colr = v->Col;
		cmd->cmd_pmod = 0xC0 | color_draw_mode.raw;

		cmd->cmd_xa = Coloured2D_X(v[0].x); cmd->cmd_ya = Coloured2D_Y(v[0].y);
		cmd->cmd_xb = Coloured2D_X(v[1].x); cmd->cmd_yb = Coloured2D_Y(v[1].y);
		cmd->cmd_xc = Coloured2D_X(v[2].x); cmd->cmd_yc = Coloured2D_Y(v[2].y);
		cmd->cmd_xd = Coloured2D_X(v[3].x); cmd->cmd_yd = Coloured2D_Y(v[3].y);
	}
}

#define Textured2D_X(value) XYZInteger(value) - SCREEN_WIDTH  / 2
#define Textured2D_Y(value) XYZInteger(value) - SCREEN_HEIGHT / 2

static void DrawTexturedQuads2D(int verticesCount, int startVertex) {
	uint16_t char_size = cur_char_size;
	uint16_t char_base = cur_char_base;
	uint16_t gour_base = ((vdp1_vram_t)gouraud_base >> 3);
	struct SATVertexTextured* v = (struct SATVertexTextured*)gfx_vertices + startVertex;

	for (int i = 0; i < verticesCount; i += 4, v += 4)
	{
		if (cmds_cur >= &cmds.extra) return;
		vdp1_cmdt_t* cmd = cmds_cur++;

		cmd->cmd_ctrl = VDP1_CMDT_DISTORTED_SPRITE | v->flip;
		cmd->cmd_size = char_size;
		cmd->cmd_srca = char_base;
		cmd->cmd_pmod = (v->Col == 1023 ? color_draw_mode : shaded_draw_mode).raw;
		cmd->cmd_grda = (gour_base + v->Col) & 0xFFFF;

		cmd->cmd_xa = Textured2D_X(v[0].x); cmd->cmd_ya = Textured2D_Y(v[0].y);
		cmd->cmd_xb = Textured2D_X(v[1].x); cmd->cmd_yb = Textured2D_Y(v[1].y);
		cmd->cmd_xc = Textured2D_X(v[2].x); cmd->cmd_yc = Textured2D_Y(v[2].y);
		cmd->cmd_xd = Textured2D_X(v[3].x); cmd->cmd_yd = Textured2D_Y(v[3].y);
	}
}

static int TransformColoured(struct SATVertexColoured* a, vdp1_cmdt_t* cmd) {
	short* dst = &cmd->cmd_xa;
	int aveZ = 0;

	for (int i = 0; i < 4; i++, a++)
	{
		int w = TransformVector(a, &mvp_.w);
		if (w <= 0) return -1;

		int x = TransformVector(a, &mvp_.x);
		cpu_divu_32_32_set(x * (SCREEN_WIDTH/2), w);

		int y = TransformVector(a, &mvp_.y);
		x = cpu_divu_quotient_get();
		cpu_divu_32_32_set(y * -(SCREEN_HEIGHT/2), w);

		*dst++ = x;
		if (x < -2048 || x > 2048) return -1;

		int z = (unsigned)w >> 6;
		if (z < 0 || z > 50000) return -1;
		aveZ += z;

		y = cpu_divu_quotient_get();
		if (y < -2048 || y > 2048) return -1;
		*dst++ = y;
	}
	return aveZ >> 2;
}

static void DrawColouredQuads3D(int verticesCount, int startVertex) {
	struct SATVertexColoured* v = (struct SATVertexColoured*)gfx_vertices + startVertex;

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{
		vdp1_cmdt_t* cmd = cmds_cur;
		if (cmd >= &cmds.extra) return;

		int z = TransformColoured(v, cmd);
		if (z < 0) continue;
	
		cmds_cur++;
		*z_cur++ = UInt16_MaxValue - z;

		cmd->cmd_ctrl = VDP1_CMDT_POLYGON;
		cmd->cmd_colr = v->Col;
		cmd->cmd_pmod = 0xC0 | color_draw_mode.raw;
	}
}

static int TransformTextured(struct SATVertexTextured* a, vdp1_cmdt_t* cmd) {
	short* dst = &cmd->cmd_xa;
	int aveZ = 0;

	for (int i = 0; i < 4; i++, a++)
	{
		int w = TransformVector(a, &mvp_.w);
		if (w <= 0) return -1;

		int x = TransformVector(a, &mvp_.x);
		cpu_divu_32_32_set(x * (SCREEN_WIDTH/2), w);

		int y = TransformVector(a, &mvp_.y);
		x = cpu_divu_quotient_get();
		cpu_divu_32_32_set(y * -(SCREEN_HEIGHT/2), w);

		*dst++ = x;
		if (x < -2048 || x > 2048) return -1;

		int z = (unsigned)w >> 6;
		if (z < 0 || z > 50000) return -1;
		aveZ += z;

		y = cpu_divu_quotient_get();
		if (y < -2048 || y > 2048) return -1;
		*dst++ = y;
	}
	return aveZ >> 2;
}

static void DrawTexturedQuads3D(int verticesCount, int startVertex) {
	struct SATVertexTextured* v = (struct SATVertexTextured*)gfx_vertices + startVertex;
	uint16_t char_size = cur_char_size;
	uint16_t char_base = cur_char_base;
	uint16_t gour_base = ((vdp1_vram_t)gouraud_base  >> 3);

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{
		vdp1_cmdt_t* cmd = cmds_cur;
		if (cmd >= &cmds.extra) return;

		int z = TransformTextured(v, cmd);
		if (z < 0) continue;
	
		cmds_cur++;
		*z_cur++ = UInt16_MaxValue - z;

		cmd->cmd_ctrl = VDP1_CMDT_DISTORTED_SPRITE | v->flip;
		cmd->cmd_size = char_size;
		cmd->cmd_srca = char_base;
		cmd->cmd_pmod = (v->Col == 1023 ? color_draw_mode : shaded_draw_mode).raw;
		cmd->cmd_grda = (gour_base + v->Col) & 0xFFFF;
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
	cmds_cur = cmds.list;
	z_cur    = z_table;
	cmds_3DCount = 0;
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
	if (cmds_cur >= &cmds.extra) Platform_LogConst("OUT OF VERTEX RAM");
	//Platform_LogConst("FRAME END");
	vdp1_cmdt_t* cmd;

	// TODO optimise Z sorting for 3D polygons
	if (cmds_3DCount) SortCommands(0, cmds_3DCount - 1);

	// cmds.extra is 1 past end of main command array
	cmd = cmds_cur < &cmds.extra ? cmds_cur : &cmds.extra;
	cmd->cmd_ctrl  = 0x8000; // end command bit

	int poly_cmds  = (int)(cmds_cur - cmds.list);
	int cmds_count = HDR_CMDS + poly_cmds + 1; // +1 for end command

	//vdp1_sync_wait();
	vdp1_sync_cmdt_put(cmds.hdrs, cmds_count, 0);
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
	cmds_3DCount = (int)(cmds_cur - cmds.list);
}

void Gfx_End2D(void) {
	Gfx_SetAlphaBlending(false);
	gfx_rendering2D = false;
}
