#define CC_DYNAMIC_VBS_ARE_STATIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"
#include "../_BlockAlloc.h"

#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>
#include <malloc.h>
#include "gs_gpu.h"

#define QWORD_ALIGNED CC_ALIGNED(16)

#define GB_HALF  2048
#define GB_RANGE 4096

typedef struct Vec4   VU0_vector QWORD_ALIGNED;
typedef struct { int x, y, z, w; } VU0_IVector QWORD_ALIGNED;

static void* gfx_vertices;
extern framebuffer_t fb_colors[2];
extern zbuffer_t     fb_depth;
static VU0_vector vp_origin, vp_scale;
static cc_bool stateDirty, formatDirty;
static framebuffer_t* fb_draw;
static framebuffer_t* fb_display;

static struct Matrix mvp;
extern void LoadMvpMatrix(struct Matrix* matrix);
extern void LoadClipScaleFactors(VU0_vector* scale);
extern void LoadViewportOrigin(VU0_vector* origin);
extern void LoadViewportScale(VU0_vector* scale);

// double buffering
static qword_t* dma_bufs[2];
static qword_t* cur_buf;
static int cur_context;

static qword_t* dma_beg;
static qword_t* Q;
#define DRAWCTX_0 0

static GfxResourceID white_square;

static void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 4x4 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[4 * 4];
	Mem_Set(pixels, 0xFF, sizeof(pixels));
	Bitmap_Init(bmp, 4, 4, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}


static qword_t* AllocDMABuffer(int qwords) {
	// TODO ucab memory with Flush at start? need to adjust free too though
	return memalign(64, qwords * 16);
}

static void FreeDMABuffer(qword_t* buffer) {
	// TODO ucab memory with Flush at start? need to adjust free too though
	free(buffer);
}

// TODO: Find a better way than just increasing this hardcoded size
static void InitDMABuffers(void) {
	dma_bufs[0] = AllocDMABuffer(50000);
	dma_bufs[1] = AllocDMABuffer(50000);
}


static void UpdateContext(void) {
	fb_display = &fb_colors[cur_context];
	fb_draw    = &fb_colors[cur_context ^ 1];

	cur_buf = dma_bufs[cur_context];
	
	dma_beg = cur_buf;
	// increment past the dmatag itself
	Q = dma_beg + 1;
}


/*########################################################################################################################*
*-------------------------------------------------------Misc GIF tags-----------------------------------------------------*
*#########################################################################################################################*/
static qword_t* GS_SetDepthBuffer(qword_t* q, zbuffer_t* zb, unsigned skipMask) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_ZBUF(zb->address >> 11, zb->zsm, skipMask), GS_REG_ZBUF); q++;
	}
	return q;
}

static qword_t* GS_SetFrameBuffer(qword_t* q, framebuffer_t* fb, unsigned skipMask) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_FRAME(fb->address >> 11, fb->width >> 6, fb->psm, skipMask), GS_REG_FRAME); q++;
	}
	return q;
}

static qword_t* GS_SetScissor(qword_t* q, int x, int y, int w, int h) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_SCISSOR(x, x+w-1, y,y+h-1), GS_REG_SCISSOR); q++;
	}
	return q;
}

static qword_t* GS_DrawFinish(qword_t *q) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,1,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, 1, GS_REG_FINISH); q++;
	}
	return q;

}

static qword_t* GS_SetPrimXYOffset(qword_t *q, int x, int y) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_XYOFFSET((x << 4), (y << 4)), GS_REG_XYOFFSET); q++;
	}
	return q;
}

// NOTE still needed as PRE/PRIM field only work when gif tag is PACKED (see PCSX2 and DobieStation for reference)
static qword_t* GS_SetPrimMode(qword_t* q, int mode) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0, GIF_PRE_ENABLE,mode, GIF_FLG_PACKED, 1), GIF_REG_NOP); q++;
	{
		PACK_GIFTAG(q, 0,0); q++;
	}
	return q;
}

static qword_t* GS_InitRegisters(qword_t* q, framebuffer_t* fb, zbuffer_t* zb) {
	PACK_GIFTAG(q,GIF_SET_TAG(12,0,0,0,GIF_FLG_PACKED,1),GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_FRAME(fb->address >> 11, fb->width >> 6, fb->psm, 0), GS_REG_FRAME); q++; // frame buffer
		PACK_GIFTAG(q, GS_SET_ZBUF(zb->address >> 11, zb->zsm, 0), GS_REG_ZBUF); q++; // depth buffer

		PACK_GIFTAG(q, GS_SET_PRMODECONT(PRIM_OVERRIDE_ENABLE), GS_REG_PRMODECONT); q++;
		PACK_GIFTAG(q, GS_SET_TEST(
								DRAW_ENABLE,  ATEST_METHOD_ALLPASS,
								0x00,         ATEST_KEEP_ALL,
								DRAW_DISABLE, 0,
								DRAW_ENABLE,  ZTEST_METHOD_ALLPASS), GS_REG_TEST); q++;
	
		// https://psi-rockin.github.io/ps2tek/#gsalphablending
		// Output = (((A - B) * C) >> 7) + D
		//        = (((src - dst) * alpha) >> 7) + dst
		//        =  (src * alpha - dst * alpha) / 128 + dst
		//        =  (src * alpha - dst * alpha) / 128 + dst * 128 / 128
		//        = ((src * alpha + dst * (128 - alpha)) / 128
		PACK_GIFTAG(q, GS_SET_ALPHA(BLEND_COLOR_SOURCE, BLEND_COLOR_DEST, BLEND_ALPHA_SOURCE,
									BLEND_COLOR_DEST, 0x80), GS_REG_ALPHA); q++; // TODO has no effect ??
	
		PACK_GIFTAG(q, GS_SET_DTHE(GS_DISABLE), GS_REG_DTHE); q++; // dithering off (no need to set matrix)
		PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP); q++; // colour clamping on
		PACK_GIFTAG(q, GS_SET_FBA(ALPHA_CORRECT_RGBA32), GS_REG_FBA); q++; // NOTE: change to RGBA16 for 16bpp framebuffer
		PACK_GIFTAG(q, GS_SET_PABE(DRAW_DISABLE), GS_REG_PABE); q++; // per pixel alpha blending
	
		// Texture settings
		PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_REPEAT, WRAP_REPEAT, 0,0,0,0), GS_REG_CLAMP); q++;
		PACK_GIFTAG(q, GS_SET_TEXA(0x80, ALPHA_EXPAND_NORMAL, 0x80), GS_REG_TEXA); q++;
		// TODO: should mipmapselect (first 0 after MIN_NEAREST) be 1?
		PACK_GIFTAG(q, GS_SET_TEX1(LOD_USE_K, 0, LOD_MAG_NEAREST, LOD_MIN_NEAREST, 0, 0, 0), GS_REG_TEX1); q++;
	}	
	return q;
}


/*########################################################################################################################*
*-----------------------------------------------------GPU initialisation--------------------------------------------------*
*#########################################################################################################################*/
static void InitGuardband(void) {
	// PS2 clipping guard band ranges from 0..GB_RANGE
	// - 0 < screen_x < GB_RANGE
	// - 0 < VIEWPORT(X/W) + WINDOW_OFFSET_X < GB_RANGE
	// - 0 < ((X/W) * vp_hwidth + vp_x + vp_hwidth) + (GB_HALF-vp_hwidth) < GB_RANGE
	// - 0 <  (X/W) * vp_hwidth + vp_x              +  GB_HALF            < GB_RANGE
	// Although accurately rescaling from viewport range to guard band range
	//  would involve vp_x and vp_hwidth, this complicates the calculation
	//  as e.g. a non-zero vp_x means viewport is not equally distant from the
	//  left and right guardband planes.
	// So to simplify calculation, just set viewport = screen size for clipping:
	// - 0 < (X/W) * SCR_HWIDTH + (GB_HALF) < GB_RANGE
	// - -GB_HALF < (X/W) * SCR_HWIDTH < GB_HALF 
	// - -GB_HALF/SCR_HWIDTH < (X/W) < GB_HALF/SCR_HWIDTH
	// - W * -GB_HALF/SCR_HWIDTH < X < W * GB_HALF/SCR_HWIDTH
	// - -W < X / (GB_HALF/SCR_HWIDTH) < W
	// - -W < X * (SCR_HWIDTH/GB_HALF) < W
	// Clipping against guardband instead of view frustum reduces the
	//   number of triangles that go through the slower 'clipping' codepath
	VU0_vector clip_scale;
	clip_scale.x = Game.Width  / 2047.0f;
	clip_scale.y = Game.Height / 2047.0f;
	clip_scale.z = 1.0f;
	clip_scale.w = 1.0f;

	LoadClipScaleFactors(&clip_scale);
}

static void InitDrawingEnv(void) {
	qword_t* beg = Q;
	Q = GS_InitRegisters(Q, fb_draw, &fb_depth);

	Q = GS_SetPrimMode(Q, PRIM_TRIANGLE);
	Q = GS_DrawFinish(Q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, beg, Q - beg, 0, 0);
	dma_wait_fast();
	Q = dma_beg + 1;
}

static void InitPalette(void);
static void InitTextureMem(void);

static void InitGPUState(void) {
	InitDMABuffers();
	InitPalette();
	InitTextureMem();
}

void Gfx_Create(void) {
	if (!Gfx.Created) InitGPUState();

	cur_context = 0;
	UpdateContext();
	
	stateDirty  = true;
	formatDirty = true;
	InitDrawingEnv();
	InitGuardband();
	Gfx_OnWindowResize();
	
// TODO maybe Min not actually needed?
	Gfx.MinTexWidth  = 4;
	Gfx.MinTexHeight = 4;	
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 256 * 256;
	Gfx.MaxLowResTexSize = 512 * 512; // TODO better fix, needed for onscreen keyboard
	Gfx.Created      = true;
	
	Gfx.NonPowTwoTexturesSupport = GFX_NONPOW2_UPLOAD;
	Gfx_RestoreState();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}

static void FlushMainDMABuffer(void) {
	dma_wait_fast();

	if (Q != dma_beg + 1) {
		DMATAG_END(dma_beg, (Q - dma_beg) - 1, 0, 0, 0);
		dma_channel_send_chain(DMA_CHANNEL_GIF, dma_beg, Q - dma_beg, 0, 0);
	}
	Q = dma_beg + 1;
}


/*########################################################################################################################*
*--------------------------------------------------VRAM transfer/memory---------------------------------------------------*
*#########################################################################################################################*/
static int vram_pointer;

// Returns size in words
static int VRAM_Size(int width, int height, int psm) {
	width = CC_ALIGNUP(width, 64);

	switch (psm)
	{
		case GS_PSM_4:
			return (width * height) >> 3;
		case GS_PSM_8:
			return (width * height) >> 2;
		case GS_PSM_16:
		case GS_PSM_16S:
		case GS_PSMZ_16:
		case GS_PSMZ_16S:
			return (width * height) >> 1;
		case GS_PSM_24:
		case GS_PSM_32:
		case GS_PSM_8H:
		case GS_PSM_4HL:
		case GS_PSM_4HH:
		case GS_PSMZ_24:
		case GS_PSMZ_32:	
			return width * height;
	}
	return 0;
}

int Gfx_VRAM_Alloc(int width, int height, int psm) {
	int size = VRAM_Size(width, height, psm);
	int addr = vram_pointer;

	vram_pointer += size;
	return addr;
}

int Gfx_VRAM_AllocPaged(int width, int height, int psm) {
	int size = VRAM_Size(width, height, psm);
	int addr = vram_pointer;

	// Align to 2048 words / 8192 bytes (VRAM page alignment)
	vram_pointer += CC_ALIGNUP(size, 2048);
	return addr;
}

static int CalcTransferBytes(int width, int height, int psm) {
	switch (psm)
	{
	case GS_PSM_32:
	case GS_PSM_24:
	case GS_PSMZ_32:
	case GS_PSMZ_24:
		return (width * height) << 2;

	case GS_PSM_16:
	case GS_PSM_16S:
	case GS_PSMZ_16:
	case GS_PSMZ_16S:
		return (width * height) << 1;

	case GS_PSM_8:
	case GS_PSM_8H:
		return (width * height);

	case GS_PSM_4:
	case GS_PSM_4HL:
	case GS_PSM_4HH:
		return (width * height) >> 1;
	}
	return 0;
}

static qword_t* PrepareTransfer(qword_t* q, u8* src, int width, int height, int psm, 
								int dst_base, int dst_width)
{
	int  bytes = CalcTransferBytes(width, height, psm);
	int qwords = (bytes + 15) / 16; // ceiling division by 16

	CPU_FlushDataCache(src, bytes);
	// TODO ucab memory? but this can run into obscure issues if memory range has ever been cached before..
	// https://www.psx-place.com/threads/newlib-porting-challenges.26821/page-2

	// Parameters for RAM -> GS transfer
	DMATAG_CNT(q, 5, 0,0,0); q++;
	{
		PACK_GIFTAG(q, GIF_SET_TAG(4,0,0,0,GIF_FLG_PACKED,1), GIF_REG_AD); q++;
		{
			PACK_GIFTAG(q, GS_SET_BITBLTBUF(0,0,0, dst_base >> 6, dst_width >> 6, psm), GS_REG_BITBLTBUF); q++;
			PACK_GIFTAG(q, GS_SET_TRXPOS(0,0,0,0,0),     GS_REG_TRXPOS); q++;
			PACK_GIFTAG(q, GS_SET_TRXREG(width, height), GS_REG_TRXREG); q++;
			PACK_GIFTAG(q, GS_SET_TRXDIR(0),             GS_REG_TRXDIR); q++;
		}
	}

	while (qwords)
	{
		int num_qwords = min(qwords, GIF_BLOCK_SIZE);

		DMATAG_CNT(q, 1, 0,0,0); q++;
		{
			PACK_GIFTAG(q, GIF_SET_TAG(num_qwords,0,0,0,GIF_FLG_IMAGE,0), 0); q++;
		}

		DMATAG_REF(q, num_qwords, (unsigned int)src, 0,0,0); q++;

		src    += num_qwords * 16;
		qwords -= num_qwords;
	}

	// TODO only call for window pixels ??
	DMATAG_END(q, 2, 0,0,0); q++;
	{
		PACK_GIFTAG(q, GIF_SET_TAG(1,1,0,0,GIF_FLG_PACKED,1), GIF_REG_AD); q++;
		{
			PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH); q++;
		}
	}

	return q;
}

void Gfx_TransferPixels(void* src, int width, int height, 
						int format, unsigned dst_base, unsigned dst_stride) {
	qword_t* buf = AllocDMABuffer(200);
	qword_t* q   = buf;

	q = PrepareTransfer(q, src, width, height, format, dst_base, dst_stride);
	dma_channel_send_chain(DMA_CHANNEL_GIF, buf, q - buf, 0,0);
	dma_wait_fast();

	FreeDMABuffer(buf);
}


/*########################################################################################################################*
*-------------------------------------------------Framebuffer allocation--------------------------------------------------*
*#########################################################################################################################*/
void Gfx_AllocFramebuffers(void) {
	fb_colors[0].width   = DisplayInfo.Width;
	fb_colors[0].height  = DisplayInfo.Height;
	fb_colors[0].mask    = 0;
	fb_colors[0].psm     = GS_PSM_24;
	fb_colors[0].address = Gfx_VRAM_AllocPaged(fb_colors[0].width, fb_colors[0].height, fb_colors[0].psm);

	fb_colors[1].width   = DisplayInfo.Width;
	fb_colors[1].height  = DisplayInfo.Height;
	fb_colors[1].mask    = 0;
	fb_colors[1].psm     = GS_PSM_24;
	fb_colors[1].address = Gfx_VRAM_AllocPaged(fb_colors[1].width, fb_colors[1].height, fb_colors[1].psm);

	fb_depth.enable      = 1;
	fb_depth.method      = ZTEST_METHOD_ALLPASS;
	fb_depth.mask        = 0;
	fb_depth.zsm         = GS_ZBUF_16S;
	fb_depth.address     = Gfx_VRAM_AllocPaged(fb_colors[0].width, fb_colors[0].height, fb_depth.zsm);
}


/*########################################################################################################################*
*---------------------------------------------------------Palettes--------------------------------------------------------*
*#########################################################################################################################*/
#define MAX_PAL_4BPP_ENTRIES 16
static unsigned clut_offset;

static void InitPalette(void) {
	clut_offset = Gfx_VRAM_AllocPaged(MAX_PAL_4BPP_ENTRIES, 1, GS_PSM_32);
}

static qword_t* UploadPalette(qword_t* q, BitmapCol* palette) {
	// psm8, w=16 h=16
	// psm4, w=8  h=2

	PACK_GIFTAG(q, GIF_SET_TAG(4,0,0,0,GIF_FLG_PACKED,1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_BITBLTBUF(0,0,0, clut_offset >> 6, 64 >> 6, GS_PSM_32), GS_REG_BITBLTBUF); q++;
		PACK_GIFTAG(q, GS_SET_TRXPOS(0,0,0,0,0), GS_REG_TRXPOS); q++;
		PACK_GIFTAG(q, GS_SET_TRXREG(8, 2),      GS_REG_TRXREG); q++;
		PACK_GIFTAG(q, GS_SET_TRXDIR(0),         GS_REG_TRXDIR); q++;
	}

	// (16 * 4) / 16 = 4 qwords
	PACK_GIFTAG(q, GIF_SET_TAG(4, 1,0,0,GIF_FLG_IMAGE,0), 0); q++;
	{
		Mem_Copy(q, palette, MAX_PAL_4BPP_ENTRIES * 4);
		q += 4;
	}
	return q;
}

static CC_INLINE int FindInPalette(BitmapCol* palette, int pal_count, BitmapCol color) {
	for (int i = 0; i < pal_count; i++) 
	{
		if (palette[i] == color) return i;
	}
	return -1;
}

static int CalcPalette(BitmapCol* palette, struct Bitmap* bmp, int rowWidth) {
	int width = bmp->width, height = bmp->height;

	BitmapCol* row = bmp->scan0;
	int pal_count  = 0;
	
	for (int y = 0; y < height; y++, row += rowWidth)
	{
		for (int x = 0; x < width; x++) 
		{
			BitmapCol color = row[x];
			int idx = FindInPalette(palette, pal_count, color);
			if (idx >= 0) continue;

			// Too many distinct colours
			if (pal_count >= MAX_PAL_4BPP_ENTRIES) return 0;

			palette[pal_count] = color;
			pal_count++;
		}
	}
	return pal_count;
}


/*########################################################################################################################*
*------------------------------------------------------Texture memory-----------------------------------------------------*
*#########################################################################################################################*/
#define VRAM_SIZE_WORDS (1024 * 1024)

// PS2 textures are always 64 word aligned minimum
#define TEXMEM_BLOCK_SIZE 64

#define TEXMEM_MAX_BLOCKS (VRAM_SIZE_WORDS / TEXMEM_BLOCK_SIZE)
static cc_uint8 tex_4HL_table[TEXMEM_MAX_BLOCKS / BLOCKS_PER_PAGE];
static cc_uint8 tex_4HH_table[TEXMEM_MAX_BLOCKS / BLOCKS_PER_PAGE];
static int texmem_4bpp_blocks;

static unsigned tex_offset;
#define TEXMEM_4BPP_TO_VRAM(block) ((block) * TEXMEM_BLOCK_SIZE)

static void InitTextureMem(void) {
	tex_offset = Gfx_VRAM_Alloc(256, 256, GS_PSM_32);

	// Overlap 4bpp textures with the two 24bpp framebuffers
	texmem_4bpp_blocks = fb_depth.address / TEXMEM_BLOCK_SIZE;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
struct GPUTexture {
	cc_uint16 width, height;     // 4 bytes
	cc_uint16 base, blocks;      // 4 bytes
	cc_uint16 log2_w, log2_h;    // 4 bytes
	cc_uint16 format, pal_index; // 4 bytes
	BitmapCol palette[MAX_PAL_4BPP_ENTRIES];
	BitmapCol pixels[]; // must be aligned to 16 bytes
};
#define GS_TEXTURE_STRIDE(tex) CC_ALIGNUP((tex)->width, 64)

static void UploadToVRAM(struct GPUTexture* tex, int dst_addr) {
	// TODO terrible perf
	FlushMainDMABuffer();
	dma_wait_fast();

	// 4bpp has extra garbage pixels when odd rows
	int src_w = tex->width, src_h = tex->height;
	if (tex->format != GS_PSM_32 && (src_w & 1)) src_w++;

	int dst_stride = GS_TEXTURE_STRIDE(tex);
	Gfx_TransferPixels(tex->pixels, src_w, src_h, 
						tex->format, dst_addr, dst_stride);
}

static void ConvertTexture_Palette(cc_uint8* dst, struct Bitmap* bmp, int rowWidth, BitmapCol* palette, int pal_count) {
	int width  = (bmp->width + 1) >> 1;
	int height = bmp->height;
	
	for (int y = 0; y < height; y++)
	{
		BitmapCol* src = bmp->scan0 + y * rowWidth;
		
		for (int x = 0; x < width; x++, src += 2)
		{
			int pal_0 = FindInPalette(palette, pal_count, src[0]);
			int pal_1 = FindInPalette(palette, pal_count, src[1]);

			*dst++ = pal_0 | (pal_1 << 4);
		}
	}
}

static int Log2Dimension(int len) { return Math_ilog2(Math_NextPowOf2(len)); }

static void GPUTexture_Init(struct GPUTexture* tex, struct Bitmap* bmp) {
	tex->width  = bmp->width;
	tex->height = bmp->height;
	tex->log2_w = Log2Dimension(bmp->width);
	tex->log2_h = Log2Dimension(bmp->height);
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	BitmapCol palette[MAX_PAL_4BPP_ENTRIES] QWORD_ALIGNED;
	int pal_count = 0;

	if (!(flags & TEXTURE_FLAG_DYNAMIC)) {
		pal_count = CalcPalette(palette, bmp, rowWidth);
	}
	//Platform_Log4("%i, c%i (%i x %i)", &pal_index, &pal_count, &bmp->width, &bmp->height);
	
	if (pal_count > 0) {
		struct GPUTexture* tex = memalign(16, sizeof(struct GPUTexture) + bmp->width * bmp->height);
		if (!tex) return 0;

		GPUTexture_Init(tex, bmp);
		Mem_Copy(tex->palette, palette, sizeof(palette));

		tex->format = GS_PSM_4;
		ConvertTexture_Palette((cc_uint8*)tex->pixels, bmp, rowWidth, palette, pal_count);

		int size    = VRAM_Size(tex->width, CC_ALIGNUP(tex->height, 32), GS_PSM_4HH);		
		// TODO fix properly. alignup instead
		int blocks  = SIZE_TO_BLOCKS(size, TEXMEM_BLOCK_SIZE);
		tex->blocks = blocks;
		//Platform_Log4("ALLOC to 4: %i / %i (%i X %i)", &size, &blocks, &bmp->width, &bmp->height);

		// Try to store entirely in VRAM in upper bits of colour framebuffers
		int base = blockalloc_alloc(tex_4HL_table, texmem_4bpp_blocks, blocks);
		if (base >= 0) {
			tex->base   = base;
			tex->format = GS_PSM_4HL;

			//Platform_Log4("ALLOC to 4HL: %i @ %i (%i X %i)", &base, &blocks, &bmp->width, &bmp->height);
			UploadToVRAM(tex, TEXMEM_4BPP_TO_VRAM(base));
			return realloc(tex, sizeof(struct GPUTexture));
		}

		base = blockalloc_alloc(tex_4HH_table, texmem_4bpp_blocks, blocks);
		if (base >= 0) {
			tex->base   = base;
			tex->format = GS_PSM_4HH;

			//Platform_Log4("ALLOC to 4HH: %i @ %i (%i X %i)", &base, &blocks, &bmp->width, &bmp->height);
			UploadToVRAM(tex, TEXMEM_4BPP_TO_VRAM(base));
			return realloc(tex, sizeof(struct GPUTexture));
		}
		return tex;
	} else {
		struct GPUTexture* tex = memalign(16, sizeof(struct GPUTexture) + bmp->width * bmp->height * 4);
		if (!tex) return 0;
		GPUTexture_Init(tex, bmp);

		tex->format = GS_PSM_32;
		CopyPixels(tex->pixels, bmp->width * BITMAPCOLOR_SIZE, 
				   bmp->scan0,  rowWidth * BITMAPCOLOR_SIZE,
				   bmp->width,  bmp->height);

		//int size    = VRAM_Size(1 << tex->log2_w, max(32, 1 << tex->log2_h), GS_PSM_4HH);		
		// TODO fix properly. alignup instead
		//int blocks  = SIZE_TO_BLOCKS(size, TEXMEM_BLOCK_SIZE); size = blocks / (2048 / 64);
		//Platform_Log4("32BPP: b %i / p %i (%i X %i)", &size, &blocks, &bmp->width, &bmp->height);
		return tex;
	}
}

static unsigned clut_counter;
static qword_t* UpdateTextureBuffer(qword_t* q, struct GPUTexture* tex, unsigned buf_addr) {
	unsigned buf_stride = GS_TEXTURE_STRIDE(tex);

	PACK_GIFTAG(q, GIF_SET_TAG(2,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		unsigned clut_addr  = tex->format == GS_PSM_32 ? 0 : clut_offset >> 6;
		unsigned clut_entry = tex->format == GS_PSM_32 ? 0 : ((clut_counter++) & 0x0F);
		unsigned clut_mode  = tex->format == GS_PSM_32 ? CLUT_NO_LOAD : CLUT_LOAD;

		PACK_GIFTAG(q, 1, GS_REG_TEXFLUSH); q++;

		PACK_GIFTAG(q, GS_SET_TEX0(buf_addr >> 6, buf_stride >> 6, tex->format,
								   tex->log2_w, tex->log2_h, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE,
								   clut_addr, GS_PSM_32, CLUT_STORAGE_MODE1, clut_entry, clut_mode), GS_REG_TEX0); q++;
	}
	return q;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	struct GPUTexture* tex = (struct GPUTexture*)texId;
	
	int dst_addr = tex_offset;
	
	if (tex->format == GS_PSM_4HH || tex->format == GS_PSM_4HL) {
		// No need to upload whole texture to vram
		dst_addr = TEXMEM_4BPP_TO_VRAM(tex->base);

		Q = UploadPalette(Q, tex->palette);
	} else {
		UploadToVRAM(tex, dst_addr);
	}
	Q = UpdateTextureBuffer(Q, tex, dst_addr);
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	struct GPUTexture* tex = (struct GPUTexture*)(*texId);
	if (!tex) return;

	if (tex->format == GS_PSM_4HH) {
		blockalloc_dealloc(tex_4HH_table, tex->base, tex->blocks);
	}
	if (tex->format == GS_PSM_4HL) {
		blockalloc_dealloc(tex_4HL_table, tex->base, tex->blocks);
	}

	Mem_Free(tex);
	*texId = NULL;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	struct GPUTexture* tex = (struct GPUTexture*)texId;
	BitmapCol* dst = (tex->pixels + x) + y * tex->width;

	CopyPixels(dst,        tex->width * BITMAPCOLOR_SIZE, 
			  part->scan0, rowWidth   * BITMAPCOLOR_SIZE,
			  part->width, part->height);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
static int clearR, clearG, clearB;
static cc_bool gfx_depthTest;

void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { } // TODO PACK_GIFTAG(q, GS_SET_FOGCOL(0,0,0), GS_REG_FOGCOL);
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

static qword_t* UpdateState(qword_t* q) {
	// TODO: toggle Enable instead of method ?
	int aMethod = gfx_alphaTest ? ATEST_METHOD_GREATER_EQUAL : ATEST_METHOD_ALLPASS;
	int zMethod = gfx_depthTest ? ZTEST_METHOD_GREATER_EQUAL : ZTEST_METHOD_ALLPASS;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		// NOTE: Reference value is 0x40 instead of 0x80, since alpha values are halved compared to normal
		PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE,  aMethod, 0x40, ATEST_KEEP_ALL,
								   DRAW_DISABLE, DRAW_DISABLE,
								   DRAW_ENABLE,  zMethod), GS_REG_TEST); q++;
	}
	stateDirty = false;
	return q;
}

static qword_t* UpdateFormat(qword_t* q) {
	cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_PRMODE(PRIM_SHADE_GOURAUD, texturing, DRAW_DISABLE,
								  gfx_alphaBlend, DRAW_DISABLE, PRIM_MAP_ST,
								  DRAWCTX_0, PRIM_UNFIXED), GS_REG_PRMODE);	q++;
	}
	formatDirty = false;
	return q;
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	// TODO
}

static void SetAlphaTest(cc_bool enabled) {
	stateDirty = true;
}

static void SetAlphaBlend(cc_bool enabled) {
	formatDirty = true;
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static qword_t* ClearBuffers(qword_t *q) {
	int w = fb_colors[0].width;
	int h = fb_colors[0].height;
	int x = GB_HALF - w / 2;
	int y = GB_HALF - h / 2;

	union IntAndFloat Q; Q.f = 1.0f;

	int x0 = ftoi4(x);
	int x1 = ftoi4(x + w);

	int y0 = ftoi4(y);
	int y1 = ftoi4(y + h); 

	PACK_GIFTAG(q, GIF_SET_TAG(3,0, GIF_PRE_ENABLE,PRIM_SPRITE, GIF_FLG_PACKED,1), GIF_REG_AD); q++;
	{
		PACK_GIFTAG(q, GS_SET_TEST(
								DRAW_ENABLE,  ATEST_METHOD_ALLPASS,
								0x00,         ATEST_KEEP_ALL,
								DRAW_DISABLE, 0,
								DRAW_ENABLE,  ZTEST_METHOD_ALLPASS), GS_REG_TEST); q++;
		PACK_GIFTAG(q, GS_SET_PRMODE(0,0,0,0,0,0,DRAWCTX_0,1), GS_REG_PRMODE); q++;

		PACK_GIFTAG(q, GS_SET_RGBAQ(clearR,clearG,clearB, 0x80, Q.u), GIF_REG_RGBAQ); q++;
	}

	// Write one horizontal GS page strip at a time
	#define STRIP_WIDTH 32
	int strips = (w + STRIP_WIDTH - 1) / STRIP_WIDTH;	
	PACK_GIFTAG(q, GIF_SET_TAG(strips, 0,0,0,GIF_FLG_REGLIST,2), DRAW_XYZ_REGLIST); q++;

	for (int i = 0; i < strips; i++, x0 += ftoi4(32))
	{
		q->dw[0] = GIF_SET_XYZ(x0,                 y0, 0);
		q->dw[1] = GIF_SET_XYZ(x0 + ftoi4(31.75f), y1, 0);
		q++;
	}
	return q;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	Q = ClearBuffers(Q);

	Q = GS_SetPrimMode(Q, PRIM_TRIANGLE);
	Q = UpdateState(Q);
	Q = UpdateFormat(Q);
}

void Gfx_ClearColor(PackedCol color) {
	clearR = PackedCol_R(color);
	clearG = PackedCol_G(color);
	clearB = PackedCol_B(color);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	gfx_depthTest = enabled;
	stateDirty    = true;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	unsigned mask = !enabled;
	Q = GS_SetDepthBuffer(Q, &fb_depth, mask);
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned mask = 0;
	if (!r) mask |= 0x000000FF;
	if (!g) mask |= 0x0000FF00;
	if (!b) mask |= 0x00FF0000;
	if (!a) mask |= 0xFF000000;

	Q = GS_SetFrameBuffer(Q, fb_draw, mask);
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
// Preprocess vertex buffers into optimised layout for PS2
static VertexFormat buf_fmt;
static int buf_count;

// Precalculate all the vertex data adjustment
static void PreprocessTexturedVertices(void* vertices) {
    struct VertexTextured* v = vertices;

    for (int i = 0; i < buf_count; i++, v++)
    {
		// See 'Colour Functions' https://psi-rockin.github.io/ps2tek/#gstextures
		// Essentially, colour blending is calculated as
		//   finalR = (vertexR * textureR) >> 7
		// However, this behaves contrary to standard expectations
		//  and results in final vertex colour being too bright
		//
		// For instance, if vertexR was white and textureR was grey:
		//   finalR = (255 * 127) / 128 = 255
		// White would be produced as the final colour instead of expected grey
		//
		// To counteract this, just divide all vertex colours by 2 first
		v->Col = (v->Col & 0xFEFEFEFE) >> 1;

		// Alpha blending divides by 128 instead of 256, so need to half alpha here
		//  so that alpha blending produces the expected results like normal GPUs
		int A = PackedCol_A(v->Col) >> 1;
		v->Col = (v->Col & ~PACKEDCOL_A_MASK) | (A << PACKEDCOL_A_SHIFT);
    }
}

static void PreprocessColouredVertices(void* vertices) {
    struct VertexColoured* v = vertices;

    for (int i = 0; i < buf_count; i++, v++)
    {
		// Alpha blending divides by 128 instead of 256, so need to half alpha here
		//  so that alpha blending produces the expected results like normal GPUs
		int A = PackedCol_A(v->Col) >> 1;
		v->Col = (v->Col & ~PACKEDCOL_A_MASK) | (A << PACKEDCOL_A_SHIFT);
    }
}

static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	//return Mem_TryAlloc(count, strideSizes[fmt]);
	return memalign(16, count * strideSizes[fmt]);
	// align to 16 bytes, so DrawTexturedQuad/DrawColouredQuad can
	//  load vertices using the "load quad (16 bytes)" instruction
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

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	Matrix_Mul(&mvp, &_view, &_proj);
	// TODO	
	LoadMvpMatrix(&mvp);
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
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -2.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -(zFar + zNear) / (zFar - zNear);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	// Swap zNear/zFar since PS2 only supports GREATER or GEQUAL depth comparison modes
	float zNear_ = zFar;
	float zFar_  = 0.1f;
	float c = Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For pos FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;
	// TODO: Check is Frustum culling needs changing for this

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar_ + zNear_) / (zFar_ - zNear_);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar_ * zNear_) / (zFar_ - zNear_);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
typedef struct {
	u32 rgba;
	float q;
	float u;
	float v;
	xyz_t xyz;
} __attribute__((packed,aligned(8))) TexturedVertex;

typedef struct {
	u32 rgba;
	float q;
	xyz_t xyz;
} __attribute__((packed,aligned(8))) ColouredVertex;

void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format  = fmt;
	gfx_stride  = strideSizes[fmt];
	formatDirty = true;
}

extern u64* DrawTexturedQuad(void* src, u64* dst, VU0_vector* tmp);
extern u64* DrawColouredQuad(void* src, u64* dst, VU0_vector* tmp);

static void DrawTexturedTriangles(int verticesCount, int startVertex) {
	struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex;
	qword_t* base = Q;
	Q++; // skip over GIF tag (will be filled in later)

	u64* dw  = (u64*)Q;
	u64* beg = dw;
	VU0_vector tmp[6];

	for (int i = 0; i < verticesCount / 4; i++, v += 4)
	{
		dw = DrawTexturedQuad(v, dw, tmp);
	}

	unsigned numVerts = (unsigned)(dw - beg) / 3;
	if (numVerts == 0) {
		Q--; // No GIF tag was added
	} else {
		if (numVerts & 1) dw++; // one more to even out number of doublewords
		Q = (qword_t*)dw;

		// Fill GIF tag in now that know number of GIF "primitives" (aka vertices)
		// 3 registers per GIF "primitive" (colour, texture, position)
		PACK_GIFTAG(base, GIF_SET_TAG(numVerts, 1, 0,0, GIF_FLG_REGLIST, 3), DRAW_STQ_REGLIST);
	}
}

static void DrawColouredTriangles(int verticesCount, int startVertex) {
	struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex;
	qword_t* base = Q;
	Q++; // skip over GIF tag (will be filled in later)

	u64* dw  = (u64*)Q;
	u64* beg = dw;
	VU0_vector tmp[6];

	for (int i = 0; i < verticesCount / 4; i++, v += 4)
	{
		dw = DrawColouredQuad(v, dw, tmp);
	}

	unsigned numVerts = (unsigned)(dw - beg) / 2;
	if (numVerts == 0) {
		Q--; // No GIF tag was added
	} else {
		Q = (qword_t*)dw;

		// Fill GIF tag in now that know number of GIF "primitives" (aka vertices)
		// 2 registers per GIF "primitive" (colour, position)
		PACK_GIFTAG(base, GIF_SET_TAG(numVerts, 1, 0,0, GIF_FLG_REGLIST, 2), DRAW_RGBAQ_REGLIST);
	}
}

static void DrawTriangles(int verticesCount, int startVertex) {
	if (stateDirty)  Q = UpdateState(Q);
	if (formatDirty) Q = UpdateFormat(Q);

	if ((Q - cur_buf) > 40000) {
		FlushMainDMABuffer();
		dma_wait_fast();
		Platform_LogConst("Too much geometry!!");
	}

	while (verticesCount)
	{
		int count = min(32000, verticesCount);

		if (gfx_format == VERTEX_FORMAT_COLOURED) {
			DrawColouredTriangles(count, startVertex);
		} else {
			DrawTexturedTriangles(count, startVertex);
		}
		verticesCount -= count; startVertex += count;
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
} /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	DrawTriangles(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawTriangles(verticesCount, 0);
	// TODO
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	DrawTriangles(verticesCount, startVertex);
	// TODO
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
	//Platform_LogConst("--- Frame ---");
}

void Gfx_EndFrame(void) {
	//Platform_LogConst("--- EF1 ---");
	Q = GS_DrawFinish(Q);
	
	FlushMainDMABuffer();
	//Platform_LogConst("--- EF2 ---");
		
	GS_wait_draw_finish();
	//Platform_LogConst("--- EF3 ---");
	if (gfx_vsync) GS_wait_vsync();
	
	cur_context ^= 1;
	UpdateContext();

	// Double buffering
	Q = GS_SetFrameBuffer(Q, fb_draw, 0);
	graph_set_framebuffer_filtered(fb_display->address,
                                   fb_display->width,
                                   fb_display->psm, 0, 0);
	//Platform_LogConst("--- EF4 ---");
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor( 0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	// PS2 X/Y guard band ranges from 0..GB_RANGE
	// To minimise need to clip, centre the viewport around (GB_RANGE/2, GB_RANGE/2)
	unsigned int maxZ = 0xFFFF;

	vp_origin.x =  ftoi4(GB_HALF + x);
	vp_origin.y = -ftoi4(GB_HALF + y);
	vp_origin.z =  maxZ / 2.0f;
	LoadViewportOrigin(&vp_origin);

	vp_scale.x =  ftoi4(w / 2);
	vp_scale.y = -ftoi4(h / 2);
	vp_scale.z =  maxZ / 2.0f;
	LoadViewportScale(&vp_scale);

	// Afterwards, subtract the viewport centre so coordinates end up in 0..SCR_WIDTH/HEIGHT
	int ox = GB_HALF - (w / 2);
	int oy = GB_HALF - (h / 2);
	Q = GS_SetPrimXYOffset(Q, ox, oy);

	// So e.g. for X coordinates:
	// - [-1, 1] (visible coordinate range)
	// - [-1, 1] * w/2 + (GB_HALF + x) (viewport transform)
	// - [-1, 1] * w/2 + (GB_HALF + x) - (GB_HALF - w/2) (viewport transform then screen offset)
	// - [-1, 1] * w/2 + GB_HALF + x - GB_HALF + w/2 (simplification #1)
	// - [-1, 1] * w/2 + x + w/2 (simplification #2)
	// - [-w/2, w/2] + x + w/2 (simplification #3)
	// - [-w/2+x+w/2, w/2+x+w/2] (simplification #4)
	// - [x, x+w] (simplification #5)
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	Q = GS_SetScissor(Q, x, y, w, h);
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS2 --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

