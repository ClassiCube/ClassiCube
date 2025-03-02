#include "Core.h"
#if defined CC_BUILD_PS1
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>
#include "../misc/ps1/ps1defs.h"
// Based off https://github.com/Lameguy64/PSn00bSDK/blob/master/examples/beginner/hello/main.c


// Length of the ordering table, i.e. the range Z coordinates can have, 0-15 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 512
#define OCT_LENGTH 128

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 32768

typedef struct {
	DISPENV disp_env;
	DRAWENV draw_env;

	uint32_t ot[OT_LENGTH];
	//uint32_t oct[OCT_LENGTH];
	uint8_t  buffer[BUFFER_LENGTH];
} RenderBuffer;

static RenderBuffer buffers[2];
static uint8_t*     next_packet;
static uint8_t*     next_packet_end;
static int          active_buffer;
static RenderBuffer* buffer;
static void* lastPoly;
static cc_bool cullingEnabled, noMemWarned;

static void OnBufferUpdated(void) {
	buffer          = &buffers[active_buffer];
	next_packet     = buffer->buffer;
    next_packet_end = next_packet + BUFFER_LENGTH;
	ClearOTagR(buffer->ot, OT_LENGTH);
	//ClearOTagR(buffer->oct, OCT_LENGTH);
}

static void SetupContexts(int w, int h, int r, int g, int b) {
	SetDefDrawEnv(&buffers[0].draw_env, 0, 0, w, h);
	SetDefDispEnv(&buffers[0].disp_env, 0, 0, w, h);
	SetDefDrawEnv(&buffers[1].draw_env, 0, h, w, h);
	SetDefDispEnv(&buffers[1].disp_env, 0, h, w, h);

	setRGB0(&buffers[0].draw_env, r, g, b);
	setRGB0(&buffers[1].draw_env, r, g, b);
	buffers[0].draw_env.isbg = 1;
	buffers[1].draw_env.isbg = 1;
	/*
	buffers[0].draw_env.tw.w = 16;
	buffers[0].draw_env.tw.h = 16;
	buffers[1].draw_env.tw.w = 16;
	buffers[1].draw_env.tw.h = 16;
	*/
	active_buffer = 0;
	OnBufferUpdated();
}

// NOINLINE to avoid polluting the hot path
static CC_NOINLINE void* new_primitive_nomem(void) {
	if (noMemWarned) return NULL;
	noMemWarned = true;
	
	Platform_LogConst("OUT OF VERTEX RAM");
	return NULL;
}

static void* new_primitive(int size) {
	uint8_t* prim  = next_packet;
	next_packet += size;

	if (next_packet <= next_packet_end)
		return (void*)prim;
	return new_primitive_nomem();
}

static GfxResourceID white_square;
void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 2x2 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[4] = { BitmapColor_RGB(255, 0, 0), BITMAPCOLOR_WHITE, BITMAPCOLOR_WHITE, BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 2, 2, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 128;
	Gfx.MaxTexHeight = 256;
	Gfx.Created      = true;
	
	Gfx_RestoreState();

	SetupContexts(Window_Main.Width, Window_Main.Height, 63, 0, 127);
	SetDispMask(1);

	InitGeom();
	gte_SetGeomOffset(Window_Main.Width / 2, Window_Main.Height / 2);
	gte_SetGeomScreen(Window_Main.Height / 2);
	
	
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*------------------------------------------------------VRAM transfers-----------------------------------------------------*
*#########################################################################################################################*/
#define wait_while(cond) while (cond) { __asm__ volatile(""); }

static void WaitUntilFinished(void) {
	// Wait until DMA to GPU has finished
	wait_while((DMA_CHCR(DMA_GPU) & CHRC_STATUS_BUSY));
	// Wait until GPU is ready to receive DMA data again
	wait_while(!(GPU_GP1 & GPU_STATUS_DMA_RECV_READY));
	// Wait until GPU is ready to receive commands again
	wait_while(!(GPU_GP1 & GPU_STATUS_CMD_READY));
}

#define DMA_BLOCK_SIZE 16 // max block size per Nocash PSX docs
void Gfx_TransferToVRAM(int x, int y, int w, int h, void* pixels) {
	unsigned num_words  = (w * h) / 2; // number of uint16s -> uint32s
	unsigned num_blocks = Math_CeilDiv(num_words, DMA_BLOCK_SIZE);
	unsigned block_size = DMA_BLOCK_SIZE;

	// Special case for very small transfers
	if (num_words < DMA_BLOCK_SIZE) {
		num_blocks = 1;
		block_size = num_words;
	}

	// Wait until GPU is ready to receive a command
	wait_while(!(GPU_GP1 & GPU_STATUS_CMD_READY));

	GPU_GP1 = GP1_CMD_DMA_MODE | GP1_DMA_NONE;
	GPU_GP0 = GP0_CMD_CLEAR_VRAM_CACHE;

	// Write GPU command for transferring RAM to VRAM
	GPU_GP0 = GP0_CMD_TRANSFER_TO_VRAM;
	GPU_GP0 = x | (y << 16);
	GPU_GP0 = w | (h << 16);

	GPU_GP1 = GP1_CMD_DMA_MODE | GP1_DMA_CPU_TO_GP0;

	// Wait until any prior DMA to GPU has finished
	wait_while((DMA_CHCR(DMA_GPU) & CHRC_STATUS_BUSY));
	// Wait until GPU is ready to receive DMA data
	wait_while(!(GPU_GP1 & GPU_STATUS_DMA_RECV_READY));

	DMA_MADR(DMA_GPU) = (uint32_t)pixels;
	DMA_BCR(DMA_GPU)  = block_size | (num_blocks << 16);
	DMA_CHCR(DMA_GPU) = CHRC_BEGIN | CHRC_MODE_SLICE | CHRC_FROM_RAM;

	WaitUntilFinished();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
// VRAM can be divided into texture pages
//   32 texture pages total - each page is 64 x 256
//   10 texture pages are occupied by the doublebuffered display
//   22 texture pages are usable for textures
// These 22 pages are then divided into:
//     - 5 for 128+ wide horizontal, 6 otherwise
//     - 11 pages for vertical textures
#define TPAGE_WIDTH   64
#define TPAGE_HEIGHT 256
#define TPAGES_PER_HALF 16

// Horizontally oriented textures (first group of 16)
#define TPAGE_START_HOR 5
#define MAX_HOR_TEX_PAGES 11
#define MAX_HOR_TEX_LINES (MAX_HOR_TEX_PAGES * TPAGE_HEIGHT)

// Horizontally oriented textures (second group of 16)
#define TPAGE_START_VER (16 + 5)
#define MAX_VER_TEX_PAGES 11
#define MAX_VER_TEX_LINES (MAX_VER_TEX_PAGES * TPAGE_WIDTH)

static cc_uint8 vram_used[(MAX_HOR_TEX_LINES + MAX_VER_TEX_LINES) / 8];
#define VRAM_SetUsed(line) (vram_used[(line) / 8] |=  (1 << ((line) % 8)))
#define VRAM_UnUsed(line)  (vram_used[(line) / 8] &= ~(1 << ((line) % 8)))
#define VRAM_IsUsed(line)  (vram_used[(line) / 8] &   (1 << ((line) % 8)))

#define VRAM_BoundingAxis(width, height) height > width ? width : height

static void VRAM_GetBlockRange(int width, int height, int* beg, int* end) {
	if (height > width) {
		*beg = MAX_HOR_TEX_LINES;
		*end = MAX_HOR_TEX_LINES + MAX_VER_TEX_LINES;
	} else if (width >= 128) {
		*beg = 0;
		*end = 5 * TPAGE_HEIGHT;
	} else {
		*beg = 5 * TPAGE_HEIGHT;
		*end = MAX_HOR_TEX_LINES;
	}
}

static cc_bool VRAM_IsRangeFree(int beg, int end) {
	for (int i = beg; i < end; i++) 
	{
		if (VRAM_IsUsed(i)) return false;
	}
	return true;
}

static int VRAM_FindFreeBlock(int width, int height) {
	int beg, end;
	VRAM_GetBlockRange(width, height, &beg, &end);
	
	// TODO kinda inefficient
	for (int i = beg; i < end - height; i++) 
	{
		if (VRAM_IsUsed(i)) continue;
		
		if (VRAM_IsRangeFree(i, i + height)) return i;
	}
	return -1;
}

static void VRAM_AllocBlock(int line, int width, int height) {
	int lines = VRAM_BoundingAxis(width, height);
	for (int i = line; i < line + lines; i++) 
	{
		VRAM_SetUsed(i);
	}
}

static void VRAM_FreeBlock(int line, int width, int height) {
	int lines = VRAM_BoundingAxis(width, height);
	for (int i = line; i < line + lines; i++) 
	{
		VRAM_UnUsed(i);
	}
}

static int VRAM_CalcPage(int line) {
	if (line < MAX_HOR_TEX_LINES) {
		return TPAGE_START_HOR + (line / TPAGE_HEIGHT);
	} else {
		line -= MAX_HOR_TEX_LINES;
		return TPAGE_START_VER + (line / TPAGE_WIDTH);
	}
}


#define TEXTURES_MAX_COUNT 64
typedef struct GPUTexture {
	cc_uint16 width, height;
	cc_uint8  u_shift, v_shift;
	cc_uint16 line, tpage;
	cc_uint8  xOffset, yOffset;
} GPUTexture;
static GPUTexture textures[TEXTURES_MAX_COUNT];
static GPUTexture* curTex;

static void* AllocTextureAt(int i, struct Bitmap* bmp, int rowWidth) {
	cc_uint16* tmp = Mem_TryAlloc(bmp->width * bmp->height, 2);
	if (!tmp) return NULL;

	// TODO: Only copy when rowWidth != bmp->width
	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint16* src = bmp->scan0 + y * rowWidth;
		cc_uint16* dst = tmp        + y * bmp->width;
		
		for (int x = 0; x < bmp->width; x++) {
			dst[x] = src[x];
		}
	}

	GPUTexture* tex = &textures[i];
	int line = VRAM_FindFreeBlock(bmp->width, bmp->height);
	if (line == -1) { Mem_Free(tmp); return NULL; }
	
	tex->width  = bmp->width;  tex->u_shift = 10 - Math_ilog2(bmp->width);
	tex->height = bmp->height; tex->v_shift = 10 - Math_ilog2(bmp->height);
	tex->line   = line;
	
	int page   = VRAM_CalcPage(line);
	int pageX  = (page % TPAGES_PER_HALF);
	int pageY  = (page / TPAGES_PER_HALF);
	tex->tpage = (2 << 7) | (pageY << 4) | pageX;

	VRAM_AllocBlock(line, bmp->width, bmp->height);
	if (bmp->height > bmp->width) {
		tex->xOffset = line % TPAGE_WIDTH;
		tex->yOffset = 0;
	} else {
		tex->xOffset = 0;
		tex->yOffset = line % TPAGE_HEIGHT;
	}
	
	Platform_Log3("%i x %i  = %i", &bmp->width, &bmp->height, &line);
	Platform_Log3("  at %i (%i, %i)", &page, &pageX, &pageY);
		
	int x = pageX * TPAGE_WIDTH  + tex->xOffset;
	int y = pageY * TPAGE_HEIGHT + tex->yOffset;
	int w = bmp->width;
	int h = bmp->height;

	Platform_Log2("  LOAD AT: %i, %i", &x, &y);
	Gfx_TransferToVRAM(x, y, w, h, tmp);
	
	Mem_Free(tmp);
	return tex;
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	for (int i = 0; i < TEXTURES_MAX_COUNT; i++)
	{
		if (textures[i].width) continue;
		return AllocTextureAt(i, bmp, rowWidth);
	}

	Platform_LogConst("No room for more textures");
	return NULL;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	curTex = (GPUTexture*)texId;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (!data) return;
	GPUTexture* tex = (GPUTexture*)data;

	VRAM_FreeBlock(tex->line, tex->width, tex->height);
	tex->width = 0; tex->height = 0;
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
	cullingEnabled = enabled;
}

static void SetAlphaTest(cc_bool enabled) {
}

static uint32_t blend_mode;
static void SetAlphaBlend(cc_bool enabled) {
	// NOTE: Semitransparent polygons are simply configured
	//   to draw all pixels as final_color = B/2 + F/2
	// This works well enough for e.g. water and ice textures,
	//   but not for the UI - so it's only used in 3D mode
	blend_mode = enabled ? POLY_CMD_SEMITRNS : 0;
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearBuffers(GfxBuffers buffers) {
}

void Gfx_ClearColor(PackedCol color) {
	int r = PackedCol_R(color);
	int g = PackedCol_G(color);
	int b = PackedCol_B(color);

	setRGB0(&buffers[0].draw_env, r, g, b);
	setRGB0(&buffers[1].draw_env, r, g, b);
}

void Gfx_SetDepthTest(cc_bool enabled) {
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	// TODO
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

static cc_bool depth_only;
void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	depth_only = depthOnly;
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
// Preprocess vertex buffers into optimised layout for PS1
struct PS1VertexColoured { int x, y, z; unsigned rgbc; };
struct PS1VertexTextured { int x, y, z; unsigned rgbc; int u, v; };
static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

#define XYZInteger(value) ((value) >> 6)
#define XYZFixed(value) ((int)((value) * (1 << 6)))
#define UVFixed(value)  ((int)((value) * 1024.0f) & 0x3FF) // U/V wrapping not supported


#define POLY_CODE_F4 (GP0_CMD_POLYGON | POLY_CMD_QUAD)
#define POLY_LEN_F4  5
struct CC_POLY_F4 {
	uint32_t tag;
	uint32_t rgbc; // r0, g0, b0, code;
	int16_t	 x0, y0;
	int16_t	 x1, y1;
	int16_t	 x2, y2;
	int16_t	 x3, y3;
};

#define POLY_CODE_FT4 (GP0_CMD_POLYGON | POLY_CMD_QUAD | POLY_CMD_TEXTURED)
#define POLY_LEN_FT4  9
struct CC_POLY_FT4 {
	uint32_t tag;
	uint32_t rgbc; // r0, g0, b0, code;
	uint16_t x0, y0;
	uint8_t	 u0, v0;
	uint16_t clut;
	int16_t  x1, y1;
	uint8_t	 u1, v1;
	uint16_t tpage;
	int16_t	 x2, y2;
	uint8_t	 u2, v2;
	uint16_t pad0;
	int16_t	 x3, y3;
	uint8_t	 u3, v3;
	uint16_t pad1;
};

static void PreprocessTexturedVertices(void) {
	struct PS1VertexTextured* dst = gfx_vertices;
	struct VertexTextured* src    = gfx_vertices;
	float u, v;

	// PS1 need to use raw U/V coordinates
	//   i.e. U = (src->U * tex->width) % tex->width
	// To avoid expensive floating point conversions,
	//  convert the U coordinates to fixed point
	//  (using 10 bits for the fractional coordinate)
	// Converting from fixed point using 1024 as base
	//  to tex->width/height as base is relatively simple:
	//  value / 1024 = X / tex_size
	//  X = value * tex_size / 1024
	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = XYZFixed(src->x);
		dst->y = XYZFixed(src->y);
		dst->z = XYZFixed(src->z);
		
		u = src->U * 0.99f;
		if(src->V == 1.0f)
		{
			v = 0.99f;
		}
		else
		{
			v = src->V;
		}
		//u = src->U * 0.99f;
		//v = src->V == 1.0f ? 0.99f : src->V;

		dst->u = UVFixed(u);
		dst->v = UVFixed(v);

		// https://problemkaputt.de/psxspx-gpu-rendering-attributes.htm
		// "For untextured graphics, 8bit RGB values of FFh are brightest. However, for texture blending, 8bit values of 80h are brightest"
		int R = PackedCol_R(src->Col) >> 1;
		int G = PackedCol_G(src->Col) >> 1;
		int B = PackedCol_B(src->Col) >> 1;
		dst->rgbc = R | (G << 8) | (B << 16) | POLY_CODE_FT4;
	}
}

static void PreprocessColouredVertices(void) {
	struct PS1VertexColoured* dst = gfx_vertices;
	struct VertexColoured* src    = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = XYZFixed(src->x);
		dst->y = XYZFixed(src->y);
		dst->z = XYZFixed(src->z);

		int R = PackedCol_R(src->Col);
		int G = PackedCol_G(src->Col);
		int B = PackedCol_B(src->Col);
		dst->rgbc = R | (G << 8) | (B << 16) | POLY_CODE_F4;
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
MATRIX transform_matrix;
#define ToFixed(v) (int)(v * (1 << 12))
#define ToFixedTr(v) (int)(v * (1 << 6))

static void LoadTransformMatrix(struct Matrix* src) {
	// https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati	
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

	//Platform_Log4("X:  %f3, %f3, %f3, %f3", &src->row1.x, &src->row2.x, &src->row3.x, &src->row4.x);
	//Platform_Log4("Y:  %f3, %f3, %f3, %f3", &src->row1.y, &src->row2.y, &src->row3.y, &src->row4.y);
	//Platform_Log4("Z:  %f3, %f3, %f3, %f3", &src->row1.z, &src->row2.z, &src->row3.z, &src->row4.z);
	//Platform_Log4("W:  %f3, %f3, %f3, %f3", &src->row1.w, &src->row2.w, &src->row3.w, &src->row4.w);
	//Platform_LogConst("====");

	// Use w instead of z
	// (row123.z = row123.w, only difference is row4.z/w being different)
	transform_matrix.t[0] = ToFixedTr(src->row4.x)<<2;
	transform_matrix.t[1] = ToFixedTr(-src->row4.y)<<2;
	transform_matrix.t[2] = ToFixedTr(src->row4.w)<<2;


	transform_matrix.m[0][0] = ToFixed(src->row1.x);
	transform_matrix.m[0][1] = ToFixed(src->row2.x);
	transform_matrix.m[0][2] = ToFixed(src->row3.x);
	
	transform_matrix.m[1][0] = ToFixed(-src->row1.y);
	transform_matrix.m[1][1] = ToFixed(-src->row2.y);
	transform_matrix.m[1][2] = ToFixed(-src->row3.y);
	
	transform_matrix.m[2][0] = ToFixed(src->row1.w);
	transform_matrix.m[2][1] = ToFixed(src->row2.w);
	transform_matrix.m[2][2] = ToFixed(src->row3.w);
	
	gte_SetRotMatrix(&transform_matrix);
	gte_SetTransMatrix(&transform_matrix);
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
	float zNear = 0.001f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c;
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

static int Transform(IVec3* result, struct PS1VertexTextured* a) {
	
	int x = a->x * mvp_row1.x + a->y * mvp_row2.x + a->z * mvp_row3.x + mvp_trans.x;
	int y = a->x * mvp_row1.y + a->y * mvp_row2.y + a->z * mvp_row3.y + mvp_trans.y;
	int z = a->x * mvp_row1.z + a->y * mvp_row2.z + a->z * mvp_row3.z + mvp_trans.z;
	int w = a->x * mvp_row1.w + a->y * mvp_row2.w + a->z * mvp_row3.w + mvp_trans.w;
	if (w <= 0) return 1;
	
	result->x = (x *  160      / w) + 160; 
	result->y = (y * -120      / w) + 120;
	result->z = (z * OT_LENGTH / w);
	
	if(result->x > 640)
	{
		return 1;
	}
	else if(result->x < -320)
	{
		return 1;
	}
	
	
	if(result->y > 480)
	{
		return 1;
	}
	else if(result->y < -240)
	{
		return 1;
	}
	

	
	return z>w;
}

static void DrawColouredQuads2D(int verticesCount, int startVertex) {
	return;
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexColoured* v = (struct PS1VertexColoured*)gfx_vertices + startVertex + i;
		
		struct CC_POLY_FT4* poly = new_primitive(sizeof(struct CC_POLY_FT4));
        if (!poly) return;

		setlen(poly, POLY_LEN_F4);
		poly->rgbc = v->rgbc;

		poly->x0 = XYZInteger(v[1].x); poly->y0 = XYZInteger(v[1].y);
		poly->x1 = XYZInteger(v[0].x); poly->y1 = XYZInteger(v[0].y);
		poly->x2 = XYZInteger(v[2].x); poly->y2 = XYZInteger(v[2].y);
		poly->x3 = XYZInteger(v[3].x); poly->y3 = XYZInteger(v[3].y);

		if (lastPoly) { 
			setaddr(poly, getaddr(lastPoly)); setaddr(lastPoly, poly); 
		} else {
			addPrim(&buffer->ot[0], poly);
		}
		lastPoly = poly;
	}
}

static void DrawTexturedQuads2D(int verticesCount, int startVertex) {
	int uOffset = curTex->xOffset, vOffset = curTex->yOffset;
	int uShift  = curTex->u_shift, vShift  = curTex->v_shift;

	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexTextured* v = (struct PS1VertexTextured*)gfx_vertices + startVertex + i;
		
		struct CC_POLY_FT4* poly = new_primitive(sizeof(struct CC_POLY_FT4));
        if (!poly) return;

		setlen(poly, POLY_LEN_FT4);
		poly->rgbc  = v->rgbc;
		poly->tpage = curTex->tpage;
		poly->clut  = 0;

		poly->x0 = XYZInteger(v[1].x); poly->y0 = XYZInteger(v[1].y);
		poly->x1 = XYZInteger(v[0].x); poly->y1 = XYZInteger(v[0].y);
		poly->x2 = XYZInteger(v[2].x); poly->y2 = XYZInteger(v[2].y);
		poly->x3 = XYZInteger(v[3].x); poly->y3 = XYZInteger(v[3].y);
		
		poly->u0 = (v[1].u >> uShift) + uOffset;
		poly->v0 = (v[1].v >> vShift) + vOffset;
		poly->u1 = (v[0].u >> uShift) + uOffset;
		poly->v1 = (v[0].v >> vShift) + vOffset;
		poly->u2 = (v[2].u >> uShift) + uOffset;
		poly->v2 = (v[2].v >> vShift) + vOffset;
		poly->u3 = (v[3].u >> uShift) + uOffset;
		poly->v3 = (v[3].v >> vShift) + vOffset;

		if (lastPoly) { 
			setaddr(poly, getaddr(lastPoly)); setaddr(lastPoly, poly); 
		} else {
			addPrim(&buffer->ot[0], poly);
		}
		lastPoly = poly;
	}
}

static void DrawColouredQuads3D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexColoured* v = (struct PS1VertexColoured*)gfx_vertices + startVertex + i;

		IVec3 coords[4];
		int clipped = 0;
		clipped |= Transform(&coords[0], &v[0]);
		clipped |= Transform(&coords[1], &v[1]);
		clipped |= Transform(&coords[2], &v[2]);
		clipped |= Transform(&coords[3], &v[3]);
		if (clipped) continue;
		
		struct CC_POLY_F4* poly = new_primitive(sizeof(struct CC_POLY_F4));
        if (!poly) return;

		setlen(poly, POLY_LEN_F4);
		poly->rgbc = v->rgbc;

		poly->x0 = coords[1].x; poly->y0 = coords[1].y;
		poly->x1 = coords[0].x; poly->y1 = coords[0].y;
		poly->x2 = coords[2].x; poly->y2 = coords[2].y;
		poly->x3 = coords[3].x; poly->y3 = coords[3].y;

		int p = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) / 4;
		if (p < 0 || p >= OT_LENGTH) continue;

		addPrim(&buffer->ot[p >> 2], poly);
	}
}

static void DrawTexturedQuad(struct PS1VertexTextured* v0, struct PS1VertexTextured* v1,
							struct PS1VertexTextured* v2, struct PS1VertexTextured* v3, int level);

#define CalcMidpoint(mid, p1, p2) \
	mid.x = (p1->x + p2->x) >> 1; \
	mid.y = (p1->y + p2->y) >> 1; \
	mid.z = (p1->z + p2->z) >> 1; \
	mid.u = (p1->u + p2->u) >> 1; \
	mid.v = (p1->v + p2->v) >> 1; \
	mid.rgbc = p1->rgbc;

static CC_NOINLINE void SubdivideQuad(struct PS1VertexTextured* v0, struct PS1VertexTextured* v1,
						struct PS1VertexTextured* v2, struct PS1VertexTextured* v3, int level) {
	if (level > 5) return;
	int v1short = 0;
	int v3short = 0;
	int diff = v0->x - v1->x;
	int mask = diff>>31;
	int vertsize = (diff^mask)-mask;
	diff = v0->y - v1->y;
	mask = diff>>31;
	vertsize += (diff^mask)-mask;
	diff = v0->z - v1->z;
	mask = diff>>31;
	vertsize += (diff^mask)-mask;
	if(vertsize < 128) v1short = 1;
	diff = v0->x - v3->x;
	mask = diff>>31;
	vertsize = (diff^mask)-mask;
	diff = v0->y - v3->y;
	mask = diff>>31;
	vertsize += (diff^mask)-mask;
	diff = v0->z - v3->z;
	mask = diff>>31;
	vertsize += (diff^mask)-mask;
	if(vertsize < 128) v3short = 1;
	struct PS1VertexTextured m01, m02, m03, m12, m32;

	// v0 --- m01 --- v1
	//  |  \   | \    |
	//  |    \ |   \  |
	//m03 ----m02----m12
	//  |  \   | \    |
	//  |    \ |   \  |
	// v3 ----m32---- v2
	
	if(v1short)
	{
		if(v3short)
		{
			DrawTexturedQuad(  v0,   v1, v2, v3, 3);
			return;
		}
		
		
		CalcMidpoint(m03, v0, v3);
		CalcMidpoint(m12, v1, v2);
		
		DrawTexturedQuad(  v0,   v1, &m12, &m03, level);
		DrawTexturedQuad(&m03, &m12,   v2,   v3, level);
	}
	else
	{
		if(v3short)
		{
			CalcMidpoint(m01, v0, v1);
			CalcMidpoint(m32, v3, v2);
			
			DrawTexturedQuad(  v0, &m01, &m32,   v3, level);
			DrawTexturedQuad(&m01,   v1,   v2, &m32, level);
	
			return;
		}
		
		CalcMidpoint(m02, v0, v2);
		CalcMidpoint(m01, v0, v1);
		CalcMidpoint(m32, v3, v2);
		CalcMidpoint(m03, v0, v3);
		CalcMidpoint(m12, v1, v2);

		DrawTexturedQuad(  v0, &m01, &m02, &m03, level);
		DrawTexturedQuad(&m01,   v1, &m12, &m02, level);
		DrawTexturedQuad(&m02, &m12,   v2, &m32, level);
		DrawTexturedQuad(&m03, &m02, &m32,   v3, level);
	}
	
}

void CrossProduct(IVec3* a, IVec3* b, IVec3* out)
{
	out->x = (a->y*b->z-a->z*b->y) >> 9;
	out->y = (a->z*b->x-a->x*b->z) >> 9;
	out->z = (a->x*b->y-a->y*b->x) >> 9;
}

static CC_INLINE void DrawTexturedQuad(struct PS1VertexTextured* v0, struct PS1VertexTextured* v1,
							struct PS1VertexTextured* v2, struct PS1VertexTextured* v3, int level) {
	int clipped = 0;
	SVECTOR coord[4];
	struct CC_POLY_FT4* poly = new_primitive(sizeof(struct CC_POLY_FT4));
	if (!poly) return;

	setlen(poly, POLY_LEN_FT4);
	poly->rgbc  = v0->rgbc | blend_mode;
	
	coord[0].vx = v0->x<<2; coord[0].vy = v0->y<<2; coord[0].vz = v0->z<<2;
	coord[1].vx = v1->x<<2; coord[1].vy = v1->y<<2; coord[1].vz = v1->z<<2;
	coord[2].vx = v2->x<<2; coord[2].vy = v2->y<<2; coord[2].vz = v2->z<<2;
	coord[3].vx = v3->x<<2; coord[3].vy = v3->y<<2; coord[3].vz = v3->z<<2;
	gte_ldv3(&coord[0],&coord[1],&coord[3]);
	gte_rtpt();
	int p = 0;
	gte_nclip();
	gte_stopz( &p );
	
	if( p > 0 ) return;
	
	
	gte_avsz3();
	gte_stotz( &p );
	if(p == 0 || (p>>2) > OT_LENGTH) return;
	gte_stsxy0( &poly->x0 );
	gte_stsxy1( &poly->x1 );
	gte_stsxy2( &poly->x2 );
	gte_ldv0( &coord[2] );
	gte_rtps();
	gte_stsxy( &poly->x3 );
	int uOffset = curTex->xOffset;
	int vOffset = curTex->yOffset;
	int uShift  = curTex->u_shift;
	int vShift  = curTex->v_shift;
		
	
	poly->u0 = (v1->u >> uShift) + uOffset;
	poly->v0 = (v1->v >> vShift) + vOffset;
	poly->u1 = (v0->u >> uShift) + uOffset;
	poly->v1 = (v0->v >> vShift) + vOffset;
	poly->u2 = (v2->u >> uShift) + uOffset;
	poly->v2 = (v2->v >> vShift) + vOffset;
	poly->u3 = (v3->u >> uShift) + uOffset;
	poly->v3 = (v3->v >> vShift) + vOffset;
	
	poly->tpage = curTex->tpage;
	poly->clut  = 0;
	addPrim(&buffer->ot[p >> 2], poly);
	
	
}

static void DrawTexturedQuads3D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexTextured* v = (struct PS1VertexTextured*)gfx_vertices + startVertex + i;

		DrawTexturedQuad(&v[0], &v[1], &v[2], &v[3], 0);
	}
}

/*static void DrawQuads(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;
		
		POLY_F4* poly = new_primitive(sizeof(POLY_F4));
        if (!poly) return;
		setPolyF4(poly);

		SVECTOR coords[4];
		coords[0].vx = v[0].x; coords[0].vy = v[0].y; coords[0].vz = v[0].z;
		coords[1].vx = v[1].x; coords[1].vy = v[1].y; coords[1].vz = v[1].z;
		coords[2].vx = v[2].x; coords[2].vy = v[2].y; coords[2].vz = v[1].z;
		coords[3].vx = v[3].x; coords[3].vy = v[3].y; coords[3].vz = v[3].z;

		int X = coords[0].vx, Y = coords[0].vy, Z = coords[0].vz;
		//Platform_Log3("IN: %i, %i, %i", &X, &Y, &Z);
		gte_ldv3(&coords[0], &coords[1], &coords[2]);
		gte_rtpt();
		gte_stsxy0(&poly->x0);

		int p;
		gte_avsz3();
		gte_stotz( &p );

		X = poly->x0; Y = poly->y0, Z = p;
		//Platform_Log3("OUT: %i, %i, %i", &X, &Y, &Z);
		if (((p >> 2) >= OT_LENGTH) || ((p >> 2) < 0))
			continue;

		gte_ldv0(&coords[3]);
		gte_rtps();
		gte_stsxy3(&poly->x1, &poly->x2, &poly->x3);

		//poly->x0 = v[1].x; poly->y0 = v[1].y;
		//poly->x1 = v[0].x; poly->y1 = v[0].y;
		//poly->x2 = v[2].x; poly->y2 = v[2].y;
		//poly->x3 = v[3].x; poly->y3 = v[3].y;

		poly->r0 = PackedCol_R(v->Col);
		poly->g0 = PackedCol_G(v->Col);
		poly->b0 = PackedCol_B(v->Col);

		addPrim(&buffer->ot[p >> 2], poly);
	}
}*/

/*static void DrawQuads(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;
		
		POLY_F4* poly = new_primitive(sizeof(POLY_F4));
        if (!poly) return;
		setPolyF4(poly);

		poly->x0 = v[1].x; poly->y0 = v[1].y;
		poly->x1 = v[0].x; poly->y1 = v[0].y;
		poly->x2 = v[2].x; poly->y2 = v[2].y;
		poly->x3 = v[3].x; poly->y3 = v[3].y;

		poly->r0 = PackedCol_R(v->Col);
		poly->g0 = PackedCol_G(v->Col);
		poly->b0 = PackedCol_B(v->Col);

		int p = 0;
		addPrim(&buffer->ot[p >> 2], poly);
	}
}*/

static void DrawQuads(int verticesCount, int startVertex) {
	if (gfx_rendering2D && gfx_format == VERTEX_FORMAT_TEXTURED) {
		DrawTexturedQuads2D(verticesCount, startVertex);
	} else if (gfx_rendering2D) {
		DrawColouredQuads2D(verticesCount, startVertex);
	} else if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		DrawTexturedQuads3D(verticesCount, startVertex);
	} else {
		DrawColouredQuads3D(verticesCount, startVertex);
	}
}


void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	DrawQuads(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawQuads(verticesCount, 0);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (depth_only) return;
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
	lastPoly    = NULL;
	noMemWarned = false;
}

void Gfx_EndFrame(void) {
	DrawSync(0);
	VSync(0);

	RenderBuffer* draw_buffer = &buffers[active_buffer];
	RenderBuffer* disp_buffer = &buffers[active_buffer ^ 1];

	PutDispEnv(&disp_buffer->disp_env);
	DrawOTagEnv(&draw_buffer->ot[OT_LENGTH - 1], &draw_buffer->draw_env);
	//DrawOTagEnv(&draw_buffer->oct[OCT_LENGTH - 1], &draw_buffer->draw_env);

	active_buffer ^= 1;
	OnBufferUpdated();
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_SetViewport(int x, int y, int w, int h)
{ 
	//DR_ENV *prim = &(buffers[0].draw_env.dr_env);
	//setDrawAreaXY(&(prim->offset),x,y,x+w,y+h);
	//prim = &(buffers[1].draw_env.dr_env);
	//setDrawAreaXY(&(prim->offset),x,y,x+w,y+h);
	buffers[0].draw_env.clip.x = x;
	buffers[0].draw_env.clip.y = y;
	buffers[0].draw_env.clip.w = w;
	buffers[0].draw_env.clip.h = h;
	
	buffers[1].draw_env.clip.x = x;
	buffers[1].draw_env.clip.y = y;
	buffers[1].draw_env.clip.w = w;
	buffers[1].draw_env.clip.h = h;
	
	buffers[0].disp_env.disp.x = x;
	buffers[0].disp_env.disp.y = y;
	buffers[0].disp_env.disp.w = w;
	buffers[0].disp_env.disp.h = h;
	
	buffers[1].disp_env.disp.x = x;
	buffers[1].disp_env.disp.y = y;
	buffers[1].disp_env.disp.w = w;
	buffers[1].disp_env.disp.h = h;
	//SetDefDrawEnv(&buffers[0].draw_env, x, y, w, h);
	//SetDefDispEnv(&buffers[0].disp_env, x, y, w, h);
	//SetDefDrawEnv(&buffers[1].draw_env, x, y+h, w, h);
	//SetDefDispEnv(&buffers[1].disp_env, x, y+h, w, h);
}
void Gfx_SetScissor (int x, int y, int w, int h) { }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS1 --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_Begin2D(int width, int height) {
	gfx_rendering2D = true;
	Gfx_SetAlphaBlending(true);
}

void Gfx_End2D(void) {
	gfx_rendering2D = false;
	Gfx_SetAlphaBlending(false);
}
#endif
