#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"

#include <packet.h>
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

#define MAX_PALETTE_ENTRIES 2048

#define QWORD_ALIGNED __attribute__((aligned(16)))

typedef struct Matrix VU0_matrix QWORD_ALIGNED;
typedef struct Vec4   VU0_vector QWORD_ALIGNED;
typedef struct { int x, y, z, w; } VU0_IVector QWORD_ALIGNED;

static void* gfx_vertices;
extern framebuffer_t fb_colors[2];
extern zbuffer_t     fb_depth;
static VU0_vector vp_origin, vp_scale;
static cc_bool stateDirty, formatDirty;
static framebuffer_t* fb_draw;
static framebuffer_t* fb_display;

static VU0_matrix mvp;
extern void LoadMvpMatrix(VU0_matrix* matrix);
extern void LoadClipScaleFactors(VU0_vector* scale);
extern void LoadViewportOrigin(VU0_vector* origin);
extern void LoadViewportScale(VU0_vector* scale);

// double buffering
static packet_t* packets[2];
static packet_t* current;
static int context;

static qword_t* dma_beg;
static qword_t* Q;

static GfxResourceID white_square;
static int primitive_type;

void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 4x4 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[4 * 4];
	Mem_Set(pixels, 0xFF, sizeof(pixels));
	Bitmap_Init(bmp, 4, 4, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) {
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

// TODO: Find a better way than just increasing this hardcoded size
static void InitDMABuffers(void) {
	packets[0] = packet_init(50000, PACKET_NORMAL);
	packets[1] = packet_init(50000, PACKET_NORMAL);
}

static void UpdateContext(void) {
	fb_display = &fb_colors[context];
	fb_draw    = &fb_colors[context ^ 1];

	current = packets[context];
	
	dma_beg = current->data;
	// increment past the dmatag itself
	Q = dma_beg + 1;
}


static void SetTextureWrapping(int context) {
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;

	PACK_GIFTAG(Q, GS_SET_CLAMP(WRAP_REPEAT, WRAP_REPEAT, 0, 0, 0, 0), 
					GS_REG_CLAMP + context);
	Q++;
}

static void SetTextureSampling(int context) {
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;

	// TODO: should mipmapselect (first 0 after MIN_NEAREST) be 1?
	PACK_GIFTAG(Q, GS_SET_TEX1(LOD_USE_K, 0, LOD_MAG_NEAREST, LOD_MIN_NEAREST, 0, 0, 0), 
					GS_REG_TEX1 + context);
	Q++;
}

static void SetAlphaBlending(int context) {
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;

	// https://psi-rockin.github.io/ps2tek/#gsalphablending
	// Output = (((A - B) * C) >> 7) + D
	//        = (((src - dst) * alpha) >> 7) + dst
	//        =  (src * alpha - dst * alpha) / 128 + dst
	//        =  (src * alpha - dst * alpha) / 128 + dst * 128 / 128
	//        = ((src * alpha + dst * (128 - alpha)) / 128
	PACK_GIFTAG(Q, GS_SET_ALPHA(BLEND_COLOR_SOURCE, BLEND_COLOR_DEST, BLEND_ALPHA_SOURCE,
								BLEND_COLOR_DEST, 0x80), GS_REG_ALPHA + context);
	Q++;
}

static void InitDrawingEnv(void) {
	qword_t* beg = Q;
	Q = draw_setup_environment(Q, 0, fb_draw, &fb_depth);
	// GS can render from 0 to 4096, so set primitive origin to centre of that
	Q = draw_primitive_xyoffset(Q, 0, 2048 - Game.Width / 2, 2048 - Game.Height / 2);

	SetTextureWrapping(0);
	SetTextureSampling(0);
	SetAlphaBlending(0); // TODO has no effect ?
	Q = draw_finish(Q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, beg, Q - beg, 0, 0);
	dma_wait_fast();
	Q = dma_beg + 1;
}

static unsigned clut_offset, tex_offset;
void Gfx_Create(void) {
	primitive_type = 0; // PRIM_POINT, which isn't used here

	InitDMABuffers();
	context = 0;
	UpdateContext();
	
	stateDirty  = true;
	formatDirty = true;
	InitDrawingEnv();

	clut_offset = graph_vram_allocate(MAX_PALETTE_ENTRIES, 1, GS_PSM_32, GRAPH_ALIGN_BLOCK);
	tex_offset  = graph_vram_allocate(256, 256, GS_PSM_32, GRAPH_ALIGN_BLOCK);
	
// TODO maybe Min not actually needed?
	Gfx.MinTexWidth  = 4;
	Gfx.MinTexHeight = 4;	
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 256 * 256;
	Gfx.MaxLowResTexSize = 512 * 512; // TODO better fix, needed for onscreen keyboard
	Gfx.Created      = true;
	
	Gfx_RestoreState();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}

static CC_INLINE void DMAFlushBuffer(void) {
	if (Q == dma_beg) return;

	DMATAG_END(dma_beg, (Q - dma_beg) - 1, 0, 0, 0);
	dma_channel_send_chain(DMA_CHANNEL_GIF, dma_beg, Q - dma_beg, 0, 0);
}

static int CalcTransferWords(int width, int height, int psm) {
	switch (psm)
	{
	case GS_PSM_32:
	case GS_PSM_24:
	case GS_PSMZ_32:
	case GS_PSMZ_24:
		return (width * height);

	case GS_PSM_16:
	case GS_PSM_16S:
	case GS_PSMZ_16:
	case GS_PSMZ_16S:
		return (width * height) >> 1;

	case GS_PSM_8:
	case GS_PSM_8H:
		return (width * height) >> 2;

	case GS_PSM_4:
	case GS_PSM_4HL:
	case GS_PSM_4HH:
		return (width * height) >> 3;
	}
	return 0;
}

static qword_t* BuildTransfer(qword_t* q, u8* src, int width, int height, int psm, 
								int dst_base, int dst_width)
{
	int  words = CalcTransferWords(width, height, psm); // 4 words = 1 qword
	int qwords = (words + 3) / 4; // ceiling division by 4

	// Parameters for RAM -> GS transfer
	DMATAG_CNT(q,5,0,0,0);
	{
		q++;
		PACK_GIFTAG(q, GIF_SET_TAG(4,0,0,0,GIF_FLG_PACKED,1), GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_BITBLTBUF(0,0,0, dst_base >> 6, dst_width >> 6, psm), GS_REG_BITBLTBUF);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXPOS(0,0,0,0,0), GS_REG_TRXPOS);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXREG(width, height), GS_REG_TRXREG);
		q++;
		PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
		q++;
	}

	while (qwords)
	{
		int num_qwords = min(qwords, GIF_BLOCK_SIZE);

		DMATAG_CNT(q,1,0,0,0);
		{
			q++;
			PACK_GIFTAG(q, GIF_SET_TAG(num_qwords,0,0,0,GIF_FLG_IMAGE,0), 0);
			q++;
		}

		DMATAG_REF(q, num_qwords, (unsigned int)src, 0,0,0);
		{
			q++;
		}

		src    += num_qwords * 16;
		qwords -= num_qwords;
	}

	DMATAG_END(q,2,0,0,0);
	{
		q++;
		PACK_GIFTAG(q,GIF_SET_TAG(1,1,0,0,GIF_FLG_PACKED,1),GIF_REG_AD);
		q++;
		PACK_GIFTAG(q,1,GS_REG_TEXFLUSH);
		q++;
	}

	return q;
}

void Gfx_TransferPixels(void* src, int width, int height, 
						int format, unsigned dst_base, unsigned dst_stride) {
	packet_t* packet = packet_init(200, PACKET_NORMAL);
	qword_t* q = packet->data;

	q = BuildTransfer(q, src, width, height, format, dst_base, dst_stride);
	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0,0);
	dma_wait_fast();

	packet_free(packet);
}


/*########################################################################################################################*
*---------------------------------------------------------Palettees--------------------------------------------------------*
*#########################################################################################################################*/
#define MAX_PALETTES (MAX_PALETTE_ENTRIES / 64)
#define MAX_PAL_ENTRIES 8 // Could be 16, but 8 for balance between texture load speed and VRAM usage

static cc_uint8 palettes_used[MAX_PALETTES];

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
			if (pal_count >= MAX_PAL_ENTRIES) return 0;

			palette[pal_count] = color;
			pal_count++;
		}
	}
	return pal_count;
}

#define PaletteAddr(index) (clut_offset + (index) * 64)

static void ApplyPalette(BitmapCol* palette, int pal_count, int pal_index) {
	palettes_used[pal_index] = true;

	dma_wait_fast();
// psm8, w=16 h=16
	Gfx_TransferPixels(palette, 8, 2, GS_PSM_32, PaletteAddr(pal_index), 64);
}

static int FindFreePalette(cc_uint8 flags) {
	if (flags & TEXTURE_FLAG_DYNAMIC) return -1;

	for (int i = 0; i < MAX_PALETTES; i++)
	{
		if (!palettes_used[i]) return i;
	}
	return -1;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture_ {
	cc_uint32 width, height;           // 8 bytes
	cc_uint16 log2_width, log2_height; // 4 bytes
	cc_uint16 format, pal_index;       // 4 bytes
	BitmapCol pixels[]; // must be aligned to 16 bytes
} CCTexture;

static void ConvertTexture_Palette(cc_uint8* dst, struct Bitmap* bmp, int rowWidth, BitmapCol* palette, int pal_count) {
	int width = bmp->width >> 1, height = bmp->height;
	
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

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	CCTexture* tex = (CCTexture*)memalign(16, 64 + size);
	
	tex->width       = bmp->width;
	tex->height      = bmp->height;
	tex->log2_width  = draw_log2(bmp->width);
	tex->log2_height = draw_log2(bmp->height);
	tex->pal_index   = 0;

	BitmapCol palette[MAX_PAL_ENTRIES] QWORD_ALIGNED;
	int pal_count = 0;
	int pal_index = FindFreePalette(flags);

	if (pal_index >= 0) {
		pal_count = CalcPalette(palette, bmp, rowWidth);
		if (pal_count > 0) ApplyPalette(palette, pal_count, pal_index);
	}
	//Platform_Log2("%i, %i", &pal_index, &pal_count);
	
	if (pal_count > 0) {
		tex->format    = GS_PSM_4;
		tex->pal_index = pal_index;
		ConvertTexture_Palette((cc_uint8*)tex->pixels, bmp, rowWidth, palette, pal_count);
	} else {
		tex->format = GS_PSM_32;
		CopyPixels(tex->pixels, bmp->width * BITMAPCOLOR_SIZE, 
				   bmp->scan0,  rowWidth * BITMAPCOLOR_SIZE,
				   bmp->width,  bmp->height);
	}
	return tex;
}

static void UpdateTextureBuffer(int context, CCTexture* tex, unsigned buf_addr, unsigned buf_stride) {
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;

	unsigned clut_addr  = tex->format == GS_PSM_32 ? 0 : PaletteAddr(tex->pal_index) >> 6;
	unsigned clut_entry = 0;
	//unsigned clut_addr  = tex->format == GS_PSM_32 ? 0 : clut_addr >> 6;
	//unsigned clut_entry = tex->format == GS_PSM_32 ? 0 : tex->pal_index;
	unsigned clut_mode  = tex->format == GS_PSM_32 ? CLUT_NO_LOAD : CLUT_LOAD;

	PACK_GIFTAG(Q, GS_SET_TEX0(buf_addr >> 6, buf_stride >> 6, tex->format,
							   tex->log2_width, tex->log2_height, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE,
							   clut_addr, GS_PSM_32, CLUT_STORAGE_MODE1, clut_entry, clut_mode), GS_REG_TEX0 + context);
	Q++;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = (CCTexture*)texId;
	
	unsigned dst_addr   = tex_offset;
	// GS stores stride in 64 block units
	// (i.e. gs_stride = real_stride >> 6, so min stride is 64)
	unsigned dst_stride = max(64, tex->width);
	
	// TODO terrible perf
	DMAFlushBuffer();
	dma_wait_fast();
	
	Gfx_TransferPixels(tex->pixels, tex->width, tex->height, 
						tex->format, dst_addr, dst_stride);
	
	// TODO terrible perf
	Q = dma_beg + 1;
	UpdateTextureBuffer(0, tex, dst_addr, dst_stride);
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = (CCTexture*)(*texId);
	if (!tex) return;

	if (tex->format != GS_PSM_32) {
		palettes_used[tex->pal_index] = false;
	}

	Mem_Free(tex);
	*texId = NULL;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	BitmapCol* dst = (tex->pixels + x) + y * tex->width;

	CopyPixels(dst,        tex->width * BITMAPCOLOR_SIZE, 
			  part->scan0, rowWidth  * BITMAPCOLOR_SIZE,
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
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

static void UpdateState(int context) {
	// TODO: toggle Enable instead of method ?
	int aMethod = gfx_alphaTest ? ATEST_METHOD_GREATER_EQUAL : ATEST_METHOD_ALLPASS;
	int zMethod = gfx_depthTest ? ZTEST_METHOD_GREATER_EQUAL : ZTEST_METHOD_ALLPASS;
	
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;
	// NOTE: Reference value is 0x40 instead of 0x80, since alpha values are halved compared to normal
	PACK_GIFTAG(Q, GS_SET_TEST(DRAW_ENABLE,  aMethod, 0x40, ATEST_KEEP_ALL,
							   DRAW_DISABLE, DRAW_DISABLE,
							   DRAW_ENABLE,  zMethod), GS_REG_TEST + context);
	Q++;
	
	stateDirty = false;
}

static void UpdateFormat(int context) {
	cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;
	
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;
	PACK_GIFTAG(Q, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_GOURAUD, texturing, DRAW_DISABLE,
							  gfx_alphaBlend, DRAW_DISABLE, PRIM_MAP_ST,
							  context, PRIM_UNFIXED), GS_REG_PRIM);
	Q++;
	
	formatDirty = false;
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

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	Q = draw_disable_tests(Q, 0, &fb_depth);
	Q = draw_clear(Q, 0, 2048.0f - fb_colors[0].width / 2.0f, 2048.0f - fb_colors[0].height / 2.0f,
					fb_colors[0].width, fb_colors[0].height, clearR, clearG, clearB);

	UpdateState(0);
	UpdateFormat(0);
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
	fb_depth.mask = !enabled;
	Q = draw_zbuffer(Q, 0, &fb_depth);
	fb_depth.mask = 0;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned mask = 0;
	if (!r) mask |= 0x000000FF;
	if (!g) mask |= 0x0000FF00;
	if (!b) mask |= 0x00FF0000;
	if (!a) mask |= 0xFF000000;

	fb_draw->mask = mask;
	Q = draw_framebuffer(Q, 0, fb_draw);
	fb_draw->mask = 0;
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
static void PreprocessTexturedVertices(void) {
    struct VertexTextured* v = gfx_vertices;

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

static void PreprocessColouredVertices(void) {
    struct VertexColoured* v = gfx_vertices;

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
	return memalign(16,count * strideSizes[fmt]);
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

extern void ViewportTransform(VU0_vector* src, VU0_IVector* dst);
static xyz_t FinishVertex(VU0_vector* src, float invW) {
	src->w = invW;
	VU0_IVector tmp;
	ViewportTransform(src, &tmp);

	xyz_t xyz;
	xyz.x = (short)tmp.x;
	xyz.y = (short)tmp.y;
	xyz.z = tmp.z;
	return xyz;
}

static u64* DrawTexturedTriangle(u64* dw, VU0_vector* coords, 
								struct VertexTextured* V0, struct VertexTextured* V1, struct VertexTextured* V2) {
	TexturedVertex* dst = (TexturedVertex*)dw;
	float q;

	// TODO optimise
	// Add the "primitives" to the GIF packet
	q   = 1.0f / coords[0].w;
	dst[0].rgba  = V0->Col;
	dst[0].q     = q;
	dst[0].u     = V0->U * q;
	dst[0].v     = V0->V * q;
	dst[0].xyz   = FinishVertex(&coords[0], q);

	q   = 1.0f / coords[1].w;
	dst[1].rgba  = V1->Col;
	dst[1].q     = q;
	dst[1].u     = V1->U * q;
	dst[1].v     = V1->V * q;
	dst[1].xyz   = FinishVertex(&coords[1], q);

	q   = 1.0f / coords[2].w;
	dst[2].rgba  = V2->Col;
	dst[2].q     = q;
	dst[2].u     = V2->U * q;
	dst[2].v     = V2->V * q;
	dst[2].xyz   = FinishVertex(&coords[2], q);

	return dw + 9;
}

extern void TransformTexturedQuad(void* src, VU0_vector* dst, VU0_vector* tmp, int* clip_flags);
extern void TransformColouredQuad(void* src, VU0_vector* dst, VU0_vector* tmp, int* clip_flags);
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
		// 2 registers per GIF "primitive" (colour, position)
		PACK_GIFTAG(base, GIF_SET_TAG(numVerts, 1,0,0, GIF_FLG_REGLIST, 3), DRAW_STQ_REGLIST);
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
		// 2 registers per GIF "primitive" (colour, texture, position)
		PACK_GIFTAG(base, GIF_SET_TAG(numVerts, 1,0,0, GIF_FLG_REGLIST, 2), DRAW_RGBAQ_REGLIST);
	}
}

static void DrawTriangles(int verticesCount, int startVertex) {
	if (stateDirty)  UpdateState(0);
	if (formatDirty) UpdateFormat(0);

	if ((Q - current->data) > 45000) {
		DMAFlushBuffer();
		dma_wait_fast();
		Q = dma_beg + 1;
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

// TODO: Can this be used? need to understand EOP more
static void SetPrimitiveType(int type) {
	if (primitive_type == type) return;
	primitive_type = type;
	
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;
	PACK_GIFTAG(Q, GS_SET_PRIM(type, PRIM_SHADE_GOURAUD, DRAW_DISABLE, DRAW_DISABLE,
							  DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_ST,
							  0, PRIM_UNFIXED), GS_REG_PRIM);
	Q++;
}

void Gfx_DrawVb_Lines(int verticesCount) {
	//SetPrimitiveType(PRIM_LINE);
} /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	//SetPrimitiveType(PRIM_TRIANGLE);
	DrawTriangles(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	//SetPrimitiveType(PRIM_TRIANGLE);
	DrawTriangles(verticesCount, 0);
	// TODO
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	//SetPrimitiveType(PRIM_TRIANGLE);
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

	Q = draw_finish(Q);
	
	dma_wait_fast();
	DMAFlushBuffer();
	//Platform_LogConst("--- EF2 ---");
		
	draw_wait_finish();
	//Platform_LogConst("--- EF3 ---");
	if (gfx_vsync) graph_wait_vsync();
	
	context ^= 1;
	UpdateContext();

	// Double buffering
	Q = draw_framebuffer(Q, 0, fb_draw);
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
	VU0_vector clip_scale;
	unsigned int maxZ = 1 << (24 - 1); // TODO: half this? or << 24 instead?

	vp_origin.x =  ftoi4(2048 - (x / 2));
	vp_origin.y = -ftoi4(2048 - (y / 2));
	vp_origin.z =  maxZ / 2.0f;
	LoadViewportOrigin(&vp_origin);

	vp_scale.x =  16 * (w / 2);
	vp_scale.y = -16 * (h / 2);
	vp_scale.z =  maxZ / 2.0f;
	LoadViewportScale(&vp_scale);

	float hwidth  = w / 2;
	float hheight = h / 2;
	// The code below clips to the viewport clip planes
	//  For e.g. X this is [2048 - vp_width / 2, 2048 + vp_width / 2]
	//  However the guard band itself ranges from 0 to 4096
	// To reduce need to clip, clip against guard band on X/Y axes instead
	/*return
		xAdj  >= -pos.w && xAdj  <= pos.w &&
		yAdj  >= -pos.w && yAdj  <= pos.w &&
		pos.z >= -pos.w && pos.z <= pos.w;*/	
		
	// Rescale clip planes to guard band extent:
	//  X/W * vp_hwidth <= vp_hwidth -- clipping against viewport
	//              X/W <= 1
	//              X   <= W
	//  X/W * vp_hwidth <= 2048      -- clipping against guard band
	//              X/W <= 2048 / vp_hwidth
	//              X * vp_hwidth / 2048 <= W
	
	clip_scale.x = hwidth  / 2048.0f;
	clip_scale.y = hheight / 2048.0f;
	clip_scale.z = 1.0f;
	clip_scale.w = 1.0f;
	
	LoadClipScaleFactors(&clip_scale);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	int context = 0;
	
	PACK_GIFTAG(Q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	Q++;
	PACK_GIFTAG(Q, GS_SET_SCISSOR(x, x+w-1, y,y+h-1), GS_REG_SCISSOR + context);
	Q++;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS2 --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

