#define CC_DYNAMIC_VBS_ARE_STATIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Logger.h"
#include "../Window.h"
#include "../_BlockAlloc.h"

#include <malloc.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include "ge_gpu.h"

#define BUFFER_WIDTH  512
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define FB_SIZE (BUFFER_WIDTH * SCREEN_HEIGHT * 4)
#define ZB_SIZE (BUFFER_WIDTH * SCREEN_HEIGHT * 2)

static unsigned int CC_ALIGNED(16) list[262144];
static cc_uint8* gfx_vertices;
static int gfx_fields;


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static int formatFields[] = {
	GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
	GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D
};

static void guInit(void) {
	void* framebuffer0 = (void*)0;
	void* framebuffer1 = (void*)FB_SIZE;
	void* depthbuffer  = (void*)(FB_SIZE + FB_SIZE);
	
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, framebuffer0, BUFFER_WIDTH);
	sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, framebuffer1, BUFFER_WIDTH);
	sceGuDepthBuffer(depthbuffer, BUFFER_WIDTH);
	
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuDisable(GU_TEXTURE_2D);
	
	sceGuAlphaFunc(GU_GREATER, 0x7f, 0xff);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuDepthFunc(GU_LEQUAL); // sceGuDepthFunc(GU_GEQUAL);
	sceGuClearDepth(65535); // sceGuClearDepth(0);
	GE_set_viewport_z(0, 65535); // GE_set_viewport_z(65535, 0);

	GE_set_depth_range(0, 65535);
	GE_upload_world_matrix((const float*)&Matrix_Identity);
	sceGuColor(0xffffffff);
	
	sceGuEnable(GU_CLIP_PLANES); // TODO: swap near/far instead of this?
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);

	Gfx_OnWindowResize();
	sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0); // 32 bit CLUT entries
	
	sceGuFinish();
	sceGeDrawSync(GU_SYNC_WAIT); // waits until FINISH command is reached
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

static GfxResourceID white_square;
void Gfx_Create(void) {
	if (!Gfx.Created) guInit();
	
	Gfx.MinTexWidth  = 2;
	Gfx.MinTexHeight = 2;
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.NonPowTwoTexturesSupport = GFX_NONPOW2_UPLOAD;
	Gfx.Created      = true;
	gfx_vsync        = true;
	
	Gfx_RestoreState();
	last_base = -1;
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

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

#define GU_Toggle(cap) if (enabled) { sceGuEnable(cap); } else { sceGuDisable(cap); }


/*########################################################################################################################*
*------------------------------------------------------Texture memory-----------------------------------------------------*
*#########################################################################################################################*/
#define VRAM_SIZE (2 * 1024 * 1024)
#define ALL_BUFFERS_SIZE (FB_SIZE + FB_SIZE + ZB_SIZE)
#define TEXMEM_TOTAL_FREE (VRAM_SIZE - ALL_BUFFERS_SIZE)

#define TEXMEM_BLOCK_SIZE 1024
#define TEXMEM_MAX_BLOCKS (TEXMEM_TOTAL_FREE / TEXMEM_BLOCK_SIZE)
static cc_uint8 tex_table[TEXMEM_MAX_BLOCKS / BLOCKS_PER_PAGE];
static int CLIPPED, UNCLIPPED;


/*########################################################################################################################*
*---------------------------------------------------------Palettes--------------------------------------------------------*
*#########################################################################################################################*/
#define MAX_PAL_4BPP_ENTRIES 16

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
*---------------------------------------------------------Swizzling-------------------------------------------------------*
*#########################################################################################################################*/
// PSP swizzled textures are 16 bytes X 8 rows 
//  = 4 '32 bit' pixels X 8 rows
//  = 16 '8 bit' pixels X 8 rows
//  = 32 '4 bit' pixels X 8 rows
static CC_INLINE void TwiddleCalcFactors(unsigned w, unsigned h, unsigned* maskX, unsigned* maskY) {
	*maskX = 0b00000011; // 2 linear X bits
	*maskY = 0b00011100; // 3 linear Y bits

	// Adjust for lower 2 X and 3 Y linear bits
	w >>= (2 + 1);
	h >>= (3 + 1);
	int shift = 2 + 3;

	for (; w > 0; w >>= 1) {
		*maskX += 0x01 << shift;
		shift  += 1;
	}

	for (; h > 0; h >>= 1) {
		*maskY += 0x01 << shift;
		shift  += 1;
	}
}

// Since can only address at byte level, each "byte" has two "4 bit" pixels in it
// Therefore the calculation below is for "8 bit pixels", but is called with w divided by 2
static CC_INLINE void TwiddleCalcFactors_Paletted(unsigned w, unsigned h, unsigned* maskX, unsigned* maskY) {
	*maskX = 0b00001111; // 4 linear X bits
	*maskY = 0b01110000; // 3 linear Y bits

	// Adjust for lower 4 X and 3 Y linear bits
	w >>= (4 + 1);
	h >>= (3 + 1);
	int shift = 4 + 3;

	for (; w > 0; w >>= 1) {
		*maskX += 0x01 << shift;
		shift  += 1;
	}

	for (; h > 0; h >>= 1) {
		*maskY += 0x01 << shift;
		shift  += 1;
	}
}

static CC_INLINE void UploadFullTexture(struct Bitmap* bmp, int rowWidth, 
										cc_uint32* dst, int dst_w, int dst_h) {
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
}

static CC_INLINE void UploadPalettedTexture(struct Bitmap* bmp, int rowWidth, BitmapCol* palette, int pal_count,
											cc_uint8* dst, int dst_w, int dst_h) {
	int src_w = bmp->width, src_h = bmp->height;
	unsigned maskX, maskY;
	unsigned X = 0, Y = 0;
	TwiddleCalcFactors_Paletted(dst_w >> 1, dst_h, &maskX, &maskY);
	
	for (int y = 0; y < src_h; y++)
	{
		cc_uint32* src = bmp->scan0 + y * rowWidth;
		X = 0;
		
		for (int x = 0; x < src_w; x += 2, src += 2)
		{
			int pal0 = FindInPalette(palette, pal_count, src[0]);
			int pal1 = FindInPalette(palette, pal_count, src[1]);
			dst[X | Y] = pal0 | (pal1 << 4);

			X = (X - maskX) & maskX;
		}
		Y = (Y - maskY) & maskY;
	}
}

static CC_INLINE void UploadPartialTexture(struct Bitmap* part, int rowWidth, cc_uint32* dst, int dst_w, int dst_h,
						int originX, int originY) {
	int src_w = part->width, src_h = part->height;
	unsigned maskX, maskY;
	unsigned X = 0, Y = 0;
	TwiddleCalcFactors(dst_w, dst_h, &maskX, &maskY);

	// Calculate start twiddled X and Y values
	for (int x = 0; x < originX; x++) { X = (X - maskX) & maskX; }
	for (int y = 0; y < originY; y++) { Y = (Y - maskY) & maskY; }
	unsigned startX = X;
	
	for (int y = 0; y < src_h; y++)
	{
		cc_uint32* src = part->scan0 + y * rowWidth;
		X = startX;
		
		for (int x = 0; x < src_w; x++, src++)
		{
			dst[X | Y] = *src;
			X = (X - maskX) & maskX;
		}
		Y = (Y - maskY) & maskY;
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture_ {
	cc_uint32 width, height;
	cc_uint16 base, blocks; // VRAM block usage
	cc_uint32 paletted;
	BitmapCol palette[MAX_PAL_4BPP_ENTRIES]; // NOTE: palette must be aligned to 16 bytes
	BitmapCol pixels[];    // NOTE: pixels must be aligned to 16 bytes
} CCTexture;

static_assert(sizeof(struct CCTexture_) % 16 == 0, "Texture struct size must be 16 byte aligned");
#define ALIGNUP(val, alignment) (((val) + ((alignment) - 1)) & -(alignment))

static CC_INLINE void* Texture_PixelsAddr(CCTexture* tex) {
	if (!tex->blocks) return tex->pixels;

	char* texmem_start = (char*)sceGeEdramGetAddr() + ALL_BUFFERS_SIZE;
	return texmem_start + (tex->base * TEXMEM_BLOCK_SIZE);
}

static CC_INLINE int Texture_PixelsSize(int w, int h, int paletted) {
	return paletted ? (w * h / 2) : (w * h * BITMAPCOLOR_SIZE);
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int dst_w = Math_NextPowOf2(bmp->width);
	int dst_h = Math_NextPowOf2(bmp->height);

	BitmapCol palette[MAX_PAL_4BPP_ENTRIES];
	int pal_count = 0;

	if (!(flags & TEXTURE_FLAG_DYNAMIC)) {
		pal_count = CalcPalette(palette, bmp, rowWidth);
	}

	int size   = Texture_PixelsSize(dst_w, dst_h, pal_count > 0);
	int blocks = SIZE_TO_BLOCKS(size, TEXMEM_BLOCK_SIZE);
	int base   = blockalloc_alloc(tex_table, TEXMEM_MAX_BLOCKS, blocks);
	CCTexture* tex;

	// Check if no room in VRAM
	if (base == -1) {
		// Swizzled texture assumes at least 16 bytes x 8 rows
		size = ALIGNUP(size, 16 * 8);
		tex  = (CCTexture*)memalign(16, sizeof(CCTexture) + size);

		blocks = 0;
		if (!tex) return 0;
	} else {
		tex = (CCTexture*)Mem_TryAlloc(1, sizeof(CCTexture));
		if (!tex) { 
			blockalloc_dealloc(tex_table, base, blocks);
			return 0;
		}
	}
	
	tex->width  = dst_w;
	tex->height = dst_h;
	tex->base   = base;
	tex->blocks = blocks;
	tex->paletted = pal_count > 0;

	void* addr = Texture_PixelsAddr(tex);
	if (pal_count > 0) {
		Mem_Copy(tex->palette, palette, sizeof(palette));
		UploadPalettedTexture(bmp, rowWidth, palette, pal_count, addr, dst_w, dst_h);
		CPU_FlushDataCache(tex->palette, sizeof(palette));
	} else {
		UploadFullTexture(bmp, rowWidth, addr, dst_w, dst_h);
	}

	CPU_FlushDataCache(addr, size);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	void* addr = Texture_PixelsAddr(tex);

	UploadPartialTexture(part, rowWidth, addr, tex->width, tex->height, x, y);

	// TODO: Do line by line and only invalidate the actually changed parts of lines? harder with swizzling
	// TODO: Invalidate full tex->size in case of very small textures?
	int size = Texture_PixelsSize(tex->width, tex->height, false);
	CPU_FlushDataCache(addr, size);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = (CCTexture*)(*texId);
	if (tex) {
    	blockalloc_dealloc(tex_table, tex->base, tex->blocks);
		Mem_Free(tex);
	}

	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	CCTexture* tex = (CCTexture*)texId;
	if (!tex) tex  = white_square; 
	
	if (tex->paletted) {
		sceGuClutLoad(MAX_PAL_4BPP_ENTRIES/8, tex->palette); // "count" is in units of "8 entries"
		sceGuTexMode(GU_PSM_T4, 0, 0, 1);
	} else {
		sceGuTexMode(GU_PSM_8888, 0, 0, 1);
	}

	void* addr = Texture_PixelsAddr(tex);
	sceGuTexImage(0, tex->width, tex->height, tex->width, addr);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;
void Gfx_SetFaceCulling(cc_bool enabled)   { GU_Toggle(GU_CULL_FACE); }
static void SetAlphaBlend(cc_bool enabled) { GU_Toggle(GU_BLEND); }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearColor(PackedCol color) {
	if (color == gfx_clearColor) return;
	sceGuClearColor(color);
	gfx_clearColor = color;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned int mask = 0xffffffff;
	if (r) mask &= 0xffffff00;
	if (g) mask &= 0xffff00ff;
	if (b) mask &= 0xff00ffff;
	if (a) mask &= 0x00ffffff;
	
	sceGuPixelMask(mask);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	sceGuDepthMask(enabled ? 0 : 0xffffffff);
}
void Gfx_SetDepthTest(cc_bool enabled)  { GU_Toggle(GU_DEPTH_TEST); }

/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho
	//   The simplified calculation below uses: L = 0, R = width, T = 0, B = height
	// NOTE: Shared with OpenGL. might be wrong to do that though?
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
	float zNear = 0.1f;
	float c = Cotangent(0.5f * fov);

	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum
	// For a FOV based perspective matrix, left/right/top/bottom are calculated as:
	//   left = -c * aspect, right = c * aspect, bottom = -c, top = c
	// Calculations are simplified because of left/right and top/bottom symmetry
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
	// TODO: should direct3d9 one be used insted with clip range from 0,1 ?
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* PSP_GetRow(struct Bitmap* bmp, int y, void* ctx) {
	cc_uint8* fb = (cc_uint8*)ctx;
	return (BitmapCol*)(fb + y * BUFFER_WIDTH * 4);
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	int fbWidth, fbFormat;
	void* fb;

	int res = sceDisplayGetFrameBuf(&fb, &fbWidth, &fbFormat, PSP_DISPLAY_SETBUF_NEXTFRAME);
	if (res < 0) return res;
	if (!fb)     return ERR_NOT_SUPPORTED;

	struct Bitmap bmp;
	bmp.scan0  = NULL;
	bmp.width  = SCREEN_WIDTH; 
	bmp.height = SCREEN_HEIGHT;

	return Png_Encode(&bmp, output, PSP_GetRow, false, fb);
}

void Gfx_GetApiInfo(cc_string* info) {
	int freeMem, usedMem;
	blockalloc_calc_usage(tex_table, TEXMEM_MAX_BLOCKS, TEXMEM_BLOCK_SIZE, 
							&freeMem, &usedMem);
	
	float freeMemMB = freeMem / (1024.0f * 1024.0f);
	float usedMemMB = usedMem / (1024.0f * 1024.0f);

	String_AppendConst(info, "-- Using PSP--\n");
	PrintMaxTextureInfo(info);
	String_Format2(info,     "Texture memory: %f2 MB used, %f2 MB free\n", &usedMemMB, &freeMemMB);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_BeginFrame(void) {
	sceGuStart(GU_DIRECT, list); 
	// TODO should be called in EndFrame, GPU commands outside EndFrame/BegFrame are missed otherwise
	last_base = -1;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	int targets = GU_FAST_CLEAR_BIT;
	if (buffers & GFX_BUFFER_COLOR) targets |= GU_COLOR_BUFFER_BIT;
	if (buffers & GFX_BUFFER_DEPTH) targets |= GU_DEPTH_BUFFER_BIT;
	
	sceGuClear(targets);
	// Clear involves draw commands
	GE_set_vertex_format(gfx_fields | GU_INDEX_16BIT);
	last_base = -1;
}

void Gfx_EndFrame(void) {
	sceGuFinish();
	sceGeDrawSync(GU_SYNC_WAIT); // waits until FINISH command is reached

	if (gfx_vsync) sceDisplayWaitVblankStart();
	sceGuSwapBuffers();

	//Platform_Log2("%i / %i", &CLIPPED, &UNCLIPPED);
	CLIPPED = UNCLIPPED = 0;
}

void Gfx_OnWindowResize(void) {
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor( 0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	// PSP X/Y guard band ranges from 0..4096
	// To minimise need to clip, centre the viewport around (2048, 2048)
	GE_set_screen_offset(2048 - (w / 2), 2048 - (h / 2));
	GE_set_viewport_xy(2048 + x, 2048 + y, w, h);
}

void Gfx_SetScissor (int x, int y, int w, int h) {
	int no_scissor = x == 0 && y == 0 && w == SCREEN_WIDTH && h == SCREEN_HEIGHT;
	sceGuScissor(x, y, w, h);
	if (no_scissor) { sceGuDisable(GU_SCISSOR_TEST); } else { sceGuEnable(GU_SCISSOR_TEST); }
}


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint16 CC_ALIGNED(16) gfx_indices[GFX_MAX_INDICES];
static int vb_size;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	fillFunc(gfx_indices, count, obj);
	return gfx_indices;
}

void Gfx_BindIb(GfxResourceID ib) { 
	// TODO doesn't work properly
	//GE_set_index_buffer(ib);
}
void Gfx_DeleteIb(GfxResourceID* ib) { }


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return memalign(16, count * strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) {
	CPU_FlushDataCache(vb, vb_size);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode  = -1;

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	//GU_Toggle(GU_FOG);
}

void Gfx_SetFogCol(PackedCol color) {
	if (color == gfx_fogColor) return;
	gfx_fogColor = color;
	//sceGuFog(0.0f, gfx_fogEnd, gfx_fogColor);
}

void Gfx_SetFogDensity(float value) {
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	gfx_fogEnd = value;
	//sceGuFog(0.0f, gfx_fogEnd, gfx_fogColor);
}

void Gfx_SetFogMode(FogFunc func) {
	/* TODO: Implemen fake exp/exp2 fog */
}

static void SetAlphaTest(cc_bool enabled) { GU_Toggle(GU_ALPHA_TEST); }

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
				  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
extern void Clip_LoadView(const float* src);
extern void Clip_LoadProj(const float* src);
extern void Clip_RecalcMVP(void);
extern void Clip_StoreMVP(float* dst);

static cc_bool clipping_dirty;
struct Plane { float a, b, c, d; } CC_ALIGNED(16);
static struct Plane frustum[6];
extern void Frustum_CalcPlanes(struct Plane* planes);
extern void Clip_SetGuardbandScale(const float* x, const float* y);

static CC_INLINE void RecalcMVP(void) {
	Clip_RecalcMVP();
	clipping_dirty = true;
}

static CC_NOINLINE void RecalcClipping(void) {
	Frustum_CalcPlanes(frustum);
	clipping_dirty = false;

	// PSP guard band ranges from 0..4096
	// - 0 < screen_x < 4096
	// - 0 < (X/W) * vp_hwidth + (2048 + vp_x) < 4096
	// Although accurately rescaling from viewport range to guard band range
	//  would involve vp_x and vp_hwidth, this does complicate the calculation
	//  as e.g. a non-zero vp_x means viewport is not equally distant from the
	//  left and right guardband planes.
	// So to simplify calculation, just pretend viewport is same size as screen:
	// - 0 < (X/W) * SCR_WIDTH + (2048) < 4096
	// - -2048 < (X/W) * SCR_WIDTH < 2048 
	// - -2048/SCR_WIDTH < (X/W) < 2048/SCR_WIDTH
	// - W * -2048/SCR_WIDTH < X < W * 2048/SCR_WIDTH
	// - -W < X / (2048/SCR_WIDTH) < W
	// - -W < X * (SCR_WIDTH/2048) < W
	float scaleX =  SCREEN_WIDTH  / 2047.0f;
	float scaleY = -SCREEN_HEIGHT / 2047.0f;
	Clip_SetGuardbandScale(&scaleX, &scaleY);
}

static void LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	const float* m = (const float*)matrix;

	if (type == MATRIX_PROJ) {
		GE_upload_proj_matrix(m);
		Clip_LoadProj(m);
	} else {
		GE_upload_view_matrix(m);
		Clip_LoadView(m);
	}
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	LoadMatrix(type, matrix);
	Clip_RecalcMVP();
	clipping_dirty = true;
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	LoadMatrix(MATRIX_VIEW, view);
	LoadMatrix(MATRIX_PROJ, proj);

	Clip_RecalcMVP();
	Clip_StoreMVP((float*)mvp);
	clipping_dirty = true;
}

void Gfx_EnableTextureOffset(float x, float y) {
	sceGuTexOffset(x, y);
}

void Gfx_DisableTextureOffset(void) { 
	sceGuTexOffset(0.0f, 0.0f);
}


/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_fields = formatFields[fmt];
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		sceGuEnable(GU_TEXTURE_2D);
	} else {
		sceGuDisable(GU_TEXTURE_2D);
	}
	GE_set_vertex_format(gfx_fields | GU_INDEX_16BIT);
}

void Gfx_DrawVb_Lines(int verticesCount) {
	// More efficient to set "indexed 16 bit" as default in Gfx_SetVertexFormat,
	//  rather than in every single triangle draw command
	GE_set_vertex_format(gfx_fields);
	GE_set_vertices(gfx_vertices);

	sceGuDrawArray(GU_LINES, 0, verticesCount, NULL, NULL);
	GE_set_vertex_format(gfx_fields | GU_INDEX_16BIT);
}

struct ClipVertex {
	float x, y, z, w;
	float u, v;
	int c, flags;
} CC_ALIGNED(16);

extern void TransformTexturedQuad(struct VertexTextured* V, struct ClipVertex* C);
static struct Vec4 Transform(struct VertexTextured* a, const struct Matrix* mat) {
	struct Vec4 vec;
	vec.x = a->x * mat->row1.x + a->y * mat->row2.x + a->z * mat->row3.x + mat->row4.x;
	vec.y = a->x * mat->row1.y + a->y * mat->row2.y + a->z * mat->row3.y + mat->row4.y;
	vec.z = a->x * mat->row1.z + a->y * mat->row2.z + a->z * mat->row3.z + mat->row4.z;
	vec.w = a->x * mat->row1.w + a->y * mat->row2.w + a->z * mat->row3.w + mat->row4.w;
	return vec;
}

static void ProcessVertices(int startVertex, int verticesCount) {
		struct VertexTextured* V = (struct VertexTextured*)gfx_vertices + startVertex;
		struct Matrix mvp;
		Clip_StoreMVP((float*)&mvp);
		struct Vec4 dst CC_ALIGNED(16);

		for (int i = 0; i < verticesCount; i++, V++)
		{
			extern int TestVertex2(struct VertexTextured* v, struct Vec4* d);
			int B = TestVertex2(V, &dst);

			if (B) UNCLIPPED++; else CLIPPED++;
	
			/*if (A == B) continue;
			Platform_LogConst("????");
			
			struct Vec4 vec = Transform(V, &mvp);
			Platform_Log4("  A: %f3/%f3/%f3/%f3", &dst.x, &dst.y, &dst.z, &dst.w);
			Platform_Log4("  S: %f3/%f3/%f3/%f3", &vec.x, &vec.y, &vec.z, &vec.w); vec.x /= vec.w; vec.y /= vec.w; vec.z /= vec.w;
			Platform_Log4("  D: %f3/%f3/%f3/%f3", &vec.x, &vec.y, &vec.z, &vec.w);*/
		}
		/*struct Matrix mvp;
		Clip_StoreMVP((float*)&mvp);

		struct ClipVertex clipped[8];
		struct VertexTextured* src = (struct VertexTextured*)gfx_vertices;
		TransformTexturedQuad(src, clipped);

		Vec3 res;
		Vec3_Transform(&res, (Vec3*)&src->x, &mvp);
		Platform_Log3("S: %f3/%f3/%f3", &res.x, &res.y, &res.z);
		Platform_Log3("D: %f3/%f3/%f3", &clipped[0].x, &clipped[0].y, &clipped[0].z);*/
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	GE_set_vertices(gfx_vertices + startVertex * gfx_stride);
	GE_set_indices(gfx_indices);
	if (clipping_dirty) RecalcClipping();

	sceGuDrawArray(GU_TRIANGLES, 0, ICOUNT(verticesCount), 
			NULL, NULL);

	if (!gfx_rendering2D && gfx_format == VERTEX_FORMAT_TEXTURED) {
		//ProcessVertices(startVertex, verticesCount);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	GE_set_vertices(gfx_vertices);
	GE_set_indices(gfx_indices);
	if (clipping_dirty) RecalcClipping();

	sceGuDrawArray(GU_TRIANGLES, 0, ICOUNT(verticesCount),
			NULL, NULL);

	if (!gfx_rendering2D && gfx_format == VERTEX_FORMAT_TEXTURED) {
		//ProcessVertices(0, verticesCount);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	GE_set_vertices(gfx_vertices + startVertex * SIZEOF_VERTEX_TEXTURED);
	GE_set_indices(gfx_indices);
	if (clipping_dirty) RecalcClipping();

	sceGuDrawArray(GU_TRIANGLES, 0, ICOUNT(verticesCount), 
			NULL, NULL);

	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		//ProcessVertices(startVertex, verticesCount);
	}
}
