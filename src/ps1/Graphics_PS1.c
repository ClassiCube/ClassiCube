#define OVERRIDE_BEGEND2D_FUNCTIONS
#define CC_DYNAMIC_VBS_ARE_STATIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>
#include "ps1defs.h"
// Based off https://github.com/Lameguy64/PSn00bSDK/blob/master/examples/beginner/hello/main.c

#define wait_while(cond) while (cond) { __asm__ volatile(""); }


/*########################################################################################################################*
*------------------------------------------------------GPU management-----------------------------------------------------*
*#########################################################################################################################*/
static volatile cc_uint32 vblank_count;

static void vblank_handler(void) { vblank_count++; }

#define DMA_IDX_GPU (DMA_GPU * 4)
#define DMA_IDX_OTC (DMA_OTC * 4)

void Gfx_ResetGPU(void) {
	int needs_exit = EnterCriticalSection();
	InterruptCallback(IRQ_VBLANK, &vblank_handler);
	if (needs_exit) ExitCriticalSection();

	GPU_GP1 = GP1_CMD_RESET_GPU;
	ChangeClearPAD(0);

	// Enable GPU and OTC DMA channels at priority 3
	DMA_DPCR |= (0xB << DMA_IDX_GPU) | (0xB << DMA_IDX_OTC);

	// Stop pending DMA
	DMA_CHCR(DMA_GPU) = CHRC_MODE_SLICE | CHRC_FROM_RAM;
	DMA_CHCR(DMA_OTC) = CHRC_MODE_SLICE;
}

void Gfx_VSync(void) {
	cc_uint32 counter = vblank_count;

	for (int i = 0; i < 0x100000; i++) 
	{
		if (counter != vblank_count) return;
	}

	// VSync IRQ may be swallowed by BIOS, try to undo that
	Platform_LogConst("VSync timed out");
	ChangeClearPAD(0);
}


/*########################################################################################################################*
*------------------------------------------------------Render buffers-----------------------------------------------------*
*#########################################################################################################################*/
// Length of the ordering table, i.e. the range Z coordinates can have, 0-15 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 512

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 32768

struct EnvPrimities {
	uint32_t tag;
	uint32_t tpage_code[1];
	uint32_t twin_code[1];
	uint32_t area_code[2];
	uint32_t ofst_code[1];
	uint32_t fill_code[3];
};

typedef struct {
	struct EnvPrimities env;
	int clipX, clipY, clipW, clipH;
	int ofstX, ofstY;
	uint32_t fb_pos;

	uint32_t ot[OT_LENGTH];
	uint8_t  buffer[BUFFER_LENGTH];
} RenderBuffer;

static RenderBuffer buffers[2];
static void*     next_packet;
static uint8_t*  next_packet_end;
static int       active_buffer;
static RenderBuffer* cur_buffer;
static void* lastPoly;

static void BuildContext(RenderBuffer* buf) {
	struct EnvPrimities* prim = &buf->env;
	setaddr(&prim->tag, &buf->ot[OT_LENGTH - 1]);
	setlen(&prim->tag, 8);

	// Texture page (reset active page and set dither/mask bits)
	prim->tpage_code[0] = GP0_CMD_TEXTURE_PAGE | (1 << 9);
	prim->twin_code[0]  = GP0_CMD_TEXTURE_WINDOW;
	prim->area_code[0]  = GP0_CMD_DRAW_MIN_PACK(buf->clipX, buf->clipY);
	prim->area_code[1]  = GP0_CMD_DRAW_MAX_PACK(buf->clipX + buf->clipW - 1, buf->clipY + buf->clipH - 1);
	prim->ofst_code[0]  = GP0_CMD_DRAW_OFFSET_PACK(buf->clipX + buf->ofstX, buf->clipY + buf->ofstY);
	// VRAM clear command
	prim->fill_code[0]  = PACK_RGBC(0, 0, 0, GP0_CMD_MEM_FILL);
	prim->fill_code[1]  = GP0_CMD_FILL_XY(buf->clipX, buf->clipY);
	prim->fill_code[2]  = GP0_CMD_FILL_WH(buf->clipW, buf->clipH);
}

static void InitContext(RenderBuffer* buf, int x, int y, int w, int h) {
	buf->clipX  = x;
	buf->clipY  = y;
	buf->clipW  = w;
	buf->clipH  = h;
	buf->fb_pos = GP1_CMD_DISPLAY_ADDRESS_XY(x, y);

	BuildContext(buf);
}

// Resets ordering table to reverse default order
// TODO move wait until later
static void ResetOTableR(RenderBuffer* buf) {
	DMA_MADR(DMA_OTC) = (uint32_t)&buf->ot[OT_LENGTH - 1];
	DMA_BCR(DMA_OTC)  = OT_LENGTH;
	DMA_CHCR(DMA_OTC) = CHRC_NO_DREQ_WAIT | CHRC_BEGIN_XFER | CHRC_DIR_DECREMENT;

	wait_while(DMA_CHCR(DMA_OTC) & CHRC_STATUS_BUSY);
}

static void OnBufferUpdated(void) {
	cur_buffer      = &buffers[active_buffer];
	next_packet     = cur_buffer->buffer;
    next_packet_end = next_packet + BUFFER_LENGTH;

	ResetOTableR(cur_buffer);
}

static void SetupContexts(int w, int h) {
	InitContext(&buffers[0], 0, 0, w, h);
	InitContext(&buffers[1], 0, h, w, h);

	active_buffer = 0;
	OnBufferUpdated();
}


/*########################################################################################################################*
*----------------------------------------------------------General--------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID white_square;
static cc_bool cullingEnabled;

static void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// dummy texture (grey works better in menus than white)
	struct Bitmap bmp;
	BitmapCol pixels[4] = { BitmapColor_RGB(130, 130, 130), BitmapColor_RGB(130, 130, 130), BitmapColor_RGB(130, 130, 130), BitmapColor_RGB(130, 130, 130) };
	Bitmap_Init(bmp, 2, 2, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
	Gfx.Created      = true;
	Gfx.Limitations  = GFX_LIMIT_MAX_VERTEX_SIZE;
	
	Gfx_RestoreState();
	SetupContexts(Window_Main.Width, Window_Main.Height);
	Gfx_ClearColor(PackedCol_Make(63, 0, 127, 255));

	InitGeom();
	GTE_Set_ScreenOffsetX((Window_Main.Width  / 2) << 16);
	GTE_Set_ScreenOffsetY((Window_Main.Height / 2) << 16);

	GTE_Set_ScreenDistance(Window_Main.Height / 2);
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*------------------------------------------------------VRAM transfers-----------------------------------------------------*
*#########################################################################################################################*/
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
	wait_while((DMA_CHCR(DMA_GPU) & CHRC_STATUS_BUSY))

	// Wait until GPU is ready to receive DMA data
	wait_while(!(GPU_GP1 & GPU_STATUS_DMA_RECV_READY));

	DMA_MADR(DMA_GPU) = (uint32_t)pixels;
	DMA_BCR(DMA_GPU)  = block_size | (num_blocks << 16);
	DMA_CHCR(DMA_GPU) = CHRC_BEGIN_XFER | CHRC_MODE_SLICE | CHRC_FROM_RAM;

	WaitUntilFinished();
}


/*########################################################################################################################*
*------------------------------------------------------VRAM management----------------------------------------------------*
*#########################################################################################################################*/
// VRAM (1024x512) can be divided into texture pages
//   32 texture pages total - each page is 64 x 256
//   10 texture pages are occupied by the doublebuffered display
//   22 texture pages are usable for textures
// These 22 pages are then divided into:
//     - 4,4,3 pages for horizontal textures
//     - 4,4,3 pages for vertical textures
#define TPAGE_WIDTH   64
#define TPAGE_HEIGHT 256
#define TPAGES_PER_HALF 16

#define MAX_TEX_GROUPS    3
#define TEX_GROUP_LINES 256
#define MAX_TEX_LINES   (MAX_TEX_GROUPS * TEX_GROUP_LINES)

static cc_uint8 vram_used[(MAX_TEX_LINES + MAX_TEX_LINES) / 8];
#define VRAM_SetUsed(line) (vram_used[(line) / 8] |=  (1 << ((line) % 8)))
#define VRAM_UnUsed(line)  (vram_used[(line) / 8] &= ~(1 << ((line) % 8)))
#define VRAM_IsUsed(line)  (vram_used[(line) / 8] &   (1 << ((line) % 8)))

#define VRAM_BoundingAxis(width, height) height > width ? width : height

static cc_bool VRAM_IsRangeFree(int beg, int end) {
	for (int i = beg; i < end; i++) 
	{
		if (VRAM_IsUsed(i)) return false;
	}
	return true;
}

static int VRAM_FindFreeLines(int group, int lines) {
	int beg = group * TEX_GROUP_LINES;
	int end = beg + TEX_GROUP_LINES;
 
	// TODO kinda inefficient
	for (int i = beg; i < end - lines; i++) 
	{
		if (VRAM_IsUsed(i)) continue;
		
		if (VRAM_IsRangeFree(i, i + lines)) return i;
	}
	return -1;
}

static int VRAM_FindFreeBlock(int width, int height) {
	int l;

	if (height > width) {
		// Vertically oriented texture
		if ((l = VRAM_FindFreeLines(3, width)) >= 0) return l;
		if ((l = VRAM_FindFreeLines(4, width)) >= 0) return l;
		if ((l = VRAM_FindFreeLines(5, width)) >= 0) return l;
	} else {
		// Horizontally oriented texture
		if ((l = VRAM_FindFreeLines(0, height)) >= 0) return l;
		if ((l = VRAM_FindFreeLines(1, height)) >= 0) return l;
		if ((l = VRAM_FindFreeLines(2, height)) >= 0) return l;
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

#define TPAGE_START_HOR  5
#define TPAGE_START_VER 21

static int VRAM_CalcPage(int line) {
	if (line < TEX_GROUP_LINES * MAX_TEX_GROUPS) {
		int group = line / TPAGE_HEIGHT;
		return TPAGE_START_HOR + (group * 4);
	} else {
		line -= TEX_GROUP_LINES * MAX_TEX_GROUPS;
		return TPAGE_START_VER + (line / TPAGE_WIDTH);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
#define MAX_PALETTE_ENTRIES 16

static CC_INLINE int FindInPalette(BitmapCol* palette, int pal_count, BitmapCol color) {
	for (int i = 0; i < pal_count; i++) 
	{
		if (palette[i] == color) return i;
	}
	return -1;
}

static CC_INLINE int CalcPalette(BitmapCol* palette, struct Bitmap* bmp, int rowWidth) {
	int width = bmp->width, height = bmp->height;
	if (width < 16 || height < 16) return 0;

	BitmapCol* row = bmp->scan0;
	int pal_count  = 0;
	
	for (int y = 0; y < height; y++, row += rowWidth)
	{
		for (int x = 0; x < width; x++) 
		{
			BitmapCol color = row[x];
			int idx = FindInPalette(palette, pal_count, color);
			if (idx >= 0) continue;

			if (pal_count >= MAX_PALETTE_ENTRIES) return -1;

			palette[pal_count] = color;
			pal_count++;
		}
	}
	return pal_count;
}


#define TEXTURES_MAX_COUNT 64
typedef struct GPUTexture {
	cc_uint16 width, height;
	cc_uint8  u_shift, v_shift;
	cc_uint8  xOffset, yOffset;
	cc_uint16 tpage, clut, line;
} GPUTexture;
static GPUTexture textures[TEXTURES_MAX_COUNT];
static GPUTexture* curTex;

static void CreateFullTexture(BitmapCol* tmp, struct Bitmap* bmp, int rowWidth) {
	// TODO: Only copy when rowWidth != bmp->width
	for (int y = 0; y < bmp->height; y++)
	{
		BitmapCol* src = bmp->scan0 + y * rowWidth;
		BitmapCol* dst = tmp        + y * bmp->width;
		
		for (int x = 0; x < bmp->width; x++) 
		{
			dst[x] = src[x];
		}
	}
}

static void CreatePalettedTexture(BitmapCol* tmp, struct Bitmap* bmp, int rowWidth, BitmapCol* palette, int pal_count) {
	cc_uint8* dst  = (cc_uint8*)tmp;
	BitmapCol* src = bmp->scan0;

	int stride = (bmp->width * 2) >> 2;
	int width  = bmp->width;
	int height = bmp->height;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++) 
		{
			int idx = FindInPalette(palette, pal_count, src[x]);
			
			if ((x & 1) == 0) {
				dst[x >> 1] = idx;
			} else {
				dst[x >> 1] |= idx << 4;
			}
		}

		src += rowWidth;
		dst += stride;
	}
}

static void* AllocTextureAt(int i, struct Bitmap* bmp, int rowWidth) {
	// Store palette in scratchpad memory, as it's faster to read from
	// https://jsgroth.dev/blog/posts/ps1-cpu/#timings
	BitmapCol* palette = (BitmapCol*)SCRATCHPAD_MEM;
	int pal_count = CalcPalette(palette, bmp, rowWidth);

	cc_uint16* tmp;
	int tmp_size;
	
	if (pal_count > 0) {
		tmp_size = (bmp->width * bmp->height) >> 2; // each halfword has 4 4bpp pixels
	} else {
		tmp_size = bmp->width * bmp->height; // each halfword has 1 16bpp pixel
	}
	tmp = Mem_TryAlloc(tmp_size, 2);
	if (!tmp) return NULL;

	GPUTexture* tex = &textures[i];
	int line = VRAM_FindFreeBlock(bmp->width, bmp->height);
	if (line == -1) { Mem_Free(tmp); return NULL; }
	
	tex->width  = bmp->width;  tex->u_shift = 8 - Math_ilog2(bmp->width);
	tex->height = bmp->height; tex->v_shift = 8 - Math_ilog2(bmp->height);
	tex->line   = line;
	
	int page   = VRAM_CalcPage(line);
	int pageX  = (page % TPAGES_PER_HALF);
	int pageY  = (page / TPAGES_PER_HALF);
	int mode   = pal_count > 0 ? 0 : 2;
	tex->tpage = (mode << 7) | (pageY << 4) | pageX;
	tex->clut  = 0;

	VRAM_AllocBlock(line, bmp->width, bmp->height);
	if (bmp->height > bmp->width) {
		tex->xOffset = line % TPAGE_WIDTH;
		tex->yOffset = 0;
	} else {
		tex->xOffset = 0;
		tex->yOffset = line % TPAGE_HEIGHT;
	}
	
	Platform_Log4("%i x %i  = %i (%i)", &bmp->width, &bmp->height, &line, &pal_count);
	Platform_Log3("  at %i (%i, %i)", &page, &pageX, &pageY);

	if (pal_count > 0) {
		// 320 wide / 16 clut entries = 20 cluts per row
		int clutX = i % 20;
		int clutY = i / 20 + 490;
		tex->clut = GP0_CMD_CLUT_XY(clutX, clutY);

		// DMA cannot copy data from scratchpad memory, so copy palette onto stack
		BitmapCol pal[16]; 
		Mem_Copy(pal, palette, sizeof(pal));
		Gfx_TransferToVRAM(clutX * 16, clutY, 16, 1, pal);

		CreatePalettedTexture(tmp, bmp, rowWidth, palette, pal_count);
	} else {
		CreateFullTexture(tmp, bmp, rowWidth);
	}
		
	int x = pageX * TPAGE_WIDTH  + tex->xOffset;
	int y = pageY * TPAGE_HEIGHT + tex->yOffset;
	int w = pal_count > 0 ? (bmp->width >> 2) : bmp->width;
	int h = bmp->height;

	Platform_Log4("  LOAD AT: %i, %i (%i x %i)", &x, &y, &w, &h);
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

	uint32_t packed = PACK_RGBC(r, g, b, GP0_CMD_MEM_FILL);
	buffers[0].env.fill_code[0] = packed;
	buffers[1].env.fill_code[0] = packed;
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
// XYZ is stored in optimised form for 3D rendering and 2D rendering
struct PS1VertexColoured { 
	short vx, vy, vz, pad;
	short xx, yy;
	unsigned rgbc;
};
struct PS1VertexTextured { 
	short vx, vy, vz, pad;
	short xx, yy;
	unsigned rgbc;
	short u, v; 
};

static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

#define XYZInteger(value) ((value) >> 8)
#define XYZFixed(value) ((int)((value) * (1 << 8)))
#define UVFixed(value)  ((int)((value) * 256.0f) & 0xFF) // U/V wrapping not supported

static void PreprocessTexturedVertices(void* vertices) {
	struct PS1VertexTextured* dst = vertices;
	struct VertexTextured* src    = vertices;
	float u, v;

	// PS1 need to use raw U/V coordinates
	//   i.e. U = (src->U * tex->width) % tex->width
	// To avoid expensive floating point conversions,
	//  convert the U coordinates to fixed point
	//  (using 8 bits for the fractional coordinate)
	// Converting from fixed point using 256 as base
	//  to tex->width/height as base is relatively simple:
	//  value / 256 = X / tex_size
	//  X = value * tex_size / 256
	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		int x = XYZFixed(src->x);
		int y = XYZFixed(src->y);
		int z = XYZFixed(src->z);

		dst->vx = x;
		dst->vy = y;
		dst->vz = z;
		dst->xx = x >> 8;
		dst->yy = y >> 8;
		
		u = src->U * 0.999f;
		v = src->V == 1.0f ? 0.999f : src->V;

		dst->u = UVFixed(u);
		dst->v = UVFixed(v);

		// https://problemkaputt.de/psxspx-gpu-rendering-attributes.htm
		// "For untextured graphics, 8bit RGB values of FFh are brightest. However, for texture blending, 8bit values of 80h are brightest"
		int R = PackedCol_R(src->Col) >> 1;
		int G = PackedCol_G(src->Col) >> 1;
		int B = PackedCol_B(src->Col) >> 1;
		dst->rgbc = PACK_RGBC(R, G, B, POLY_CODE_FT4);
	}
}

static void PreprocessColouredVertices(void* vertices) {
	struct PS1VertexColoured* dst = vertices;
	struct VertexColoured* src    = vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		int x = XYZFixed(src->x);
		int y = XYZFixed(src->y);
		int z = XYZFixed(src->z);

		dst->vx = x;
		dst->vy = y;
		dst->vz = z;
		dst->xx = x >> 8;
		dst->yy = y >> 8;

		int R = PackedCol_R(src->Col);
		int G = PackedCol_G(src->Col);
		int B = PackedCol_B(src->Col);
		dst->rgbc = PACK_RGBC(R, G, B, POLY_CODE_F4);
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

#define ToFixed(v)   (int)(v * (1 << 12))
#define ToFixedTr(v) (int)(v * (1 << 8))

static void LoadTransformMatrix(struct Matrix* src) {
	MATRIX transform_matrix;
	// Use w instead of z
	// (row123.z = row123.w, only difference is row4.z/w being different)
	GTE_Set_TransX(ToFixedTr( src->row4.x));
	GTE_Set_TransY(ToFixedTr(-src->row4.y));
	GTE_Set_TransZ(ToFixedTr( src->row4.w));

	transform_matrix.m[0][0] = ToFixed(src->row1.x);
	transform_matrix.m[0][1] = ToFixed(src->row2.x);
	transform_matrix.m[0][2] = ToFixed(src->row3.x);
	
	transform_matrix.m[1][0] = ToFixed(-src->row1.y);
	transform_matrix.m[1][1] = ToFixed(-src->row2.y);
	transform_matrix.m[1][2] = ToFixed(-src->row3.y);
	
	transform_matrix.m[2][0] = ToFixed(src->row1.w);
	transform_matrix.m[2][1] = ToFixed(src->row2.w);
	transform_matrix.m[2][2] = ToFixed(src->row3.w);
	
	GTE_Load_RotMatrix(&transform_matrix);
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
#define VERTEX_TEX_SIZE sizeof(struct PS1VertexTextured)
#define VERTEX_COL_SIZE sizeof(struct PS1VertexColoured)

void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {

}

static void DrawColouredQuads2D(int verticesCount, int startVertex) {
	struct PS1VertexColoured* v = (struct PS1VertexColoured*)gfx_vertices + startVertex;
	psx_poly_F4* poly = next_packet;
	cc_uint8* max = next_packet_end - sizeof(*poly);

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{	
        if ((cc_uint8*)poly > max) break;

		setlen(poly, POLY_LEN_F4);
		poly->rgbc = v->rgbc | POLY_CMD_SEMITRNS;

		poly->x0 = v[1].xx; poly->y0 = v[1].yy;
		poly->x1 = v[0].xx; poly->y1 = v[0].yy;
		poly->x2 = v[2].xx; poly->y2 = v[2].yy;
		poly->x3 = v[3].xx; poly->y3 = v[3].yy;

		if (lastPoly) { 
			setaddr(poly, getaddr(lastPoly)); setaddr(lastPoly, poly); 
		} else {
			addPrim(&cur_buffer->ot[0], poly);
		}
		lastPoly = poly;
		poly++;
	}
	next_packet = poly;
}

static void DrawTexturedQuads2D(int verticesCount, int startVertex) {
	struct PS1VertexTextured* v = (struct PS1VertexTextured*)gfx_vertices + startVertex;	
	int uOffset = curTex->xOffset, vOffset = curTex->yOffset;
	int uShift  = curTex->u_shift, vShift  = curTex->v_shift;
	int tpage   = curTex->tpage,   clut    = curTex->clut;

	psx_poly_FT4* poly = next_packet;
	cc_uint8* max = next_packet_end - sizeof(*poly);

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{
        if ((cc_uint8*)poly > max) break;

		setlen(poly, POLY_LEN_FT4);
		poly->rgbc  = v->rgbc;
		poly->tpage = tpage;
		poly->clut  = clut;

		poly->x0 = v[1].xx; poly->y0 = v[1].yy;
		poly->x1 = v[0].xx; poly->y1 = v[0].yy;
		poly->x2 = v[2].xx; poly->y2 = v[2].yy;
		poly->x3 = v[3].xx; poly->y3 = v[3].yy;
		
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
			addPrim(&cur_buffer->ot[0], poly);
		}

		lastPoly = poly;
		poly++;
	}
	next_packet = poly;
}

static void DrawColouredQuads3D(int verticesCount, int startVertex) {
	struct PS1VertexColoured* v = (struct PS1VertexColoured*)gfx_vertices + startVertex;
	uint32_t* ot = cur_buffer->ot;

	psx_poly_F4* poly = next_packet;
	cc_uint8* max = next_packet_end - sizeof(*poly);

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{
		struct PS1VertexColoured* v0 = &v[0];
		struct PS1VertexColoured* v1 = &v[1];
		struct PS1VertexColoured* v2 = &v[2];
		struct PS1VertexColoured* v3 = &v[3];
		if ((cc_uint8*)poly > max) break;

		GTE_Load_XY0(v, 0 * VERTEX_COL_SIZE); // GTE_XY0 = v0->xy
		GTE_Load__Z0(v, 0 * VERTEX_COL_SIZE); // GTE__Z0 = v0->z
		GTE_Load_XY1(v, 1 * VERTEX_COL_SIZE); // GTE_XY1 = v1->xy
		GTE_Load__Z1(v, 1 * VERTEX_COL_SIZE); // GTE__Z1 = v1->z
		GTE_Load_XY2(v, 3 * VERTEX_COL_SIZE); // GTE_XY2 = v3->xy
		GTE_Load__Z2(v, 3 * VERTEX_COL_SIZE); // GTE__Z2 = v3->z

		GTE_Exec_RTPT(); // 23 cycles
		setlen(poly, POLY_LEN_F4);
		poly->rgbc = v0->rgbc;
	
		// Calculate Z depth
		GTE_Exec_AVSZ3(); // 5 cycles
		int p; GTE_Get_OTZ(p);
		if (p == 0 || (p >> 2) > OT_LENGTH) continue;

		GTE_Store_XY0(poly, offsetof(psx_poly_F4, x0));
		GTE_Store_XY1(poly, offsetof(psx_poly_F4, x1));
		GTE_Store_XY2(poly, offsetof(psx_poly_F4, x2));

		GTE_Load_XY0(v, 2 * VERTEX_COL_SIZE); // GTE_XY2 = v2->xy
		GTE_Load__Z0(v, 2 * VERTEX_COL_SIZE); // GTE__Z2 = v2->z

		GTE_Exec_RTPS(); // 15 cycles
		addPrim(&ot[p >> 2], poly);
		GTE_Store_XY2(poly, offsetof(psx_poly_F4, x3));
		
		poly++;
	}
	next_packet = poly;
}

static void DrawTexturedQuads3D(int verticesCount, int startVertex) {
	struct PS1VertexTextured* v = (struct PS1VertexTextured*)gfx_vertices + startVertex;
	int uOffset = curTex->xOffset;
	int vOffset = curTex->yOffset;
	int uShift  = curTex->u_shift;
	int vShift  = curTex->v_shift;

	int tpage = curTex->tpage, clut = curTex->clut;
	int bmode = blend_mode;
	uint32_t* ot = cur_buffer->ot;

	psx_poly_FT4* poly = next_packet;
	cc_uint8* max = next_packet_end - sizeof(*poly);

	for (int i = 0; i < verticesCount; i += 4, v += 4) 
	{
		struct PS1VertexTextured* v0 = &v[0];
		struct PS1VertexTextured* v1 = &v[1];
		struct PS1VertexTextured* v2 = &v[2];
		struct PS1VertexTextured* v3 = &v[3];
		if ((cc_uint8*)poly > max) break;
	
		GTE_Load_XY0(v, 0 * VERTEX_TEX_SIZE); // GTE_XY0 = v0->xy
		GTE_Load__Z0(v, 0 * VERTEX_TEX_SIZE); // GTE__Z0 = v0->z
		GTE_Load_XY1(v, 1 * VERTEX_TEX_SIZE); // GTE_XY1 = v1->xy
		GTE_Load__Z1(v, 1 * VERTEX_TEX_SIZE); // GTE__Z1 = v1->z
		GTE_Load_XY2(v, 3 * VERTEX_TEX_SIZE); // GTE_XY2 = v3->xy
		GTE_Load__Z2(v, 3 * VERTEX_TEX_SIZE); // GTE__Z2 = v3->z

		GTE_Exec_RTPT(); // 23 cycles
		setlen(poly, POLY_LEN_FT4);
		poly->rgbc = v0->rgbc | bmode;
		poly->u0   = (v1->u >> uShift) + uOffset;

		// Check for backface culling
		GTE_Exec_NCLIP(); // 8 cycles
		poly->tpage = tpage;
		poly->clut  = clut;
		int clip; GTE_Get_MAC0(clip);
		if (clip > 0) continue;
	
		// Calculate Z depth
		GTE_Exec_AVSZ3(); // 5 cycles
		int p; GTE_Get_OTZ(p);
		if (p == 0 || (p >> 2) > OT_LENGTH) continue;

		GTE_Store_XY0(poly, offsetof(psx_poly_FT4, x0));
		GTE_Store_XY1(poly, offsetof(psx_poly_FT4, x1));
		GTE_Store_XY2(poly, offsetof(psx_poly_FT4, x2));

		GTE_Load_XY0(v, 2 * VERTEX_TEX_SIZE); // GTE_XY2 = v2->xy
		GTE_Load__Z0(v, 2 * VERTEX_TEX_SIZE); // GTE__Z2 = v2->z

		GTE_Exec_RTPS(); // 15 cycles
		addPrim(&ot[p >> 2], poly);
		GTE_Store_XY2(poly, offsetof(psx_poly_FT4, x3));
	
		poly->v0 = (v1->v >> vShift) + vOffset;
		poly->u1 = (v0->u >> uShift) + uOffset;
		poly->v1 = (v0->v >> vShift) + vOffset;
		poly->u2 = (v2->u >> uShift) + uOffset;
		poly->v2 = (v2->v >> vShift) + vOffset;
		poly->u3 = (v3->u >> uShift) + uOffset;
		poly->v3 = (v3->v >> vShift) + vOffset;
	
		poly++;
	}
	next_packet = poly;
}

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
	lastPoly = NULL;
}

static void SendDrawCommands(RenderBuffer* buf) {
	wait_while(DMA_CHCR(DMA_GPU) & CHRC_STATUS_BUSY);

	DMA_MADR(DMA_GPU) = (uint32_t)&buf->env.tag;
	DMA_BCR(DMA_GPU)  = 0;
	DMA_CHCR(DMA_GPU) = CHRC_BEGIN_XFER | CHRC_MODE_CHAIN | CHRC_FROM_RAM;
}

void Gfx_EndFrame(void) {
	if ((cc_uint8*)next_packet >= next_packet_end - sizeof(psx_poly_FT4)) {
		Platform_LogConst("OUT OF VERTEX RAM");
	}
	WaitUntilFinished();
	Gfx_VSync();

	RenderBuffer* draw_buffer = &buffers[active_buffer];
	RenderBuffer* disp_buffer = &buffers[active_buffer ^ 1];

	// Use previous finished frame as display framebuffer
	GPU_GP1 = GP1_CMD_DISPLAY_ADDRESS | disp_buffer->fb_pos;

	// Start sending commands to GPU to draw this frame
	SendDrawCommands(cur_buffer);

	active_buffer ^= 1;
	OnBufferUpdated();
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_SetViewport(int x, int y, int w, int h) { } // TODO
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

