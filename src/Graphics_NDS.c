#include "Core.h"
#ifdef CC_BUILD_NDS
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <nds.h>

#define DS_MAT_PROJECTION 0
#define DS_MAT_POSITION   1
#define DS_MAT_MODELVIEW  2
#define DS_MAT_TEXTURE    3

static int matrix_modes[] = { DS_MAT_PROJECTION, DS_MAT_MODELVIEW };
static int lastMatrix;


/*########################################################################################################################*
*-------------------------------------------------------Memory alloc------------------------------------------------------*
*#########################################################################################################################*/
#define blockalloc_page(block) ((block) >> 3)
#define blockalloc_bit(block)  (1 << ((block) & 0x07))
#define BLOCKS_PER_PAGE 8

static CC_INLINE int blockalloc_can_alloc(cc_uint8* table, int beg, int blocks) {
	for (int i = beg; i < beg + blocks; i++)
	{
		cc_uint8 page = table[blockalloc_page(i)];
		if (page & blockalloc_bit(i)) return false;
	}
	return true;
}

static int blockalloc_alloc(cc_uint8* table, int maxBlocks, int blocks) {
	if (blocks > maxBlocks) return -1;

	for (int i = 0; i < maxBlocks - blocks;) 
	{
		cc_uint8 page = table[blockalloc_page(i)];

		// If entire page is used, skip entirely over it
		if ((i & 0x07) == 0 && page == 0xFF) { i += 8; continue; }

		// If block is used, move onto trying next block
 		if (page & blockalloc_bit(i)) { i++; continue; }
		
		// If can't be allocated starting at block, try next
		if (!blockalloc_can_alloc(table, i, blocks)) { i++; continue; }

		for (int j = i; j < i + blocks; j++) 
		{
			table[blockalloc_page(j)] |= blockalloc_bit(j);
        }
        return i;
    }
	return -1;
}

static void blockalloc_free(cc_uint8* table, int origin, int blocks) {
	// Mark the used blocks as free again
	for (int i = origin; i < origin + blocks; i++) 
	{
		table[blockalloc_page(i)] &= ~blockalloc_bit(i);
    }
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static CC_INLINE void CopyHWords(void* src, void* dst, int len) {
	// VRAM must be written to in 16 bit units
	u16* src_ = src;
	u16* dst_ = dst;

	for (int i = 0; i < len; i++) dst_[i] = src_[i];
}

void ResetGPU(void) {
    powerOn(POWER_3D_CORE | POWER_MATRIX); // Enable 3D core & geometry engine

    if (GFX_BUSY) {
        // Geometry engine sill busy from before, give it some time
        swiWaitForVBlank();
		swiWaitForVBlank();
		swiWaitForVBlank();

        if (GFX_BUSY) {
            // The geometry engine is still busy. This can happen due to a
            // partial vertex upload by the previous homebrew application (=>
            // ARM7->ARM9 forced reset).  So long as the buffer wasn't flushed,
            // this is recoverable, so we attempt to do so.
            for (int i = 0; i < 8; i++)
            {
                GFX_VERTEX16 = 0;

                // TODO: Do we need such a high arbitrary delay value?
                swiDelay(0x400);
                if (!GFX_BUSY) break;
            }
        }
    }

    // Clear the FIFO
    GFX_STATUS |= (1 << 29);

    // prime the vertex/polygon buffers
    GFX_FLUSH = 0;

	GFX_ALPHA_TEST = 7; // Alpha threshold ranges from 0 to 15
	
	GFX_CONTROL = GL_ANTIALIAS | GL_TEXTURE_2D | GL_FOG | GL_BLEND;

	GFX_CLEAR_DEPTH = GL_MAX_DEPTH;
    GFX_TEX_FORMAT  = 0;
    GFX_POLY_FORMAT = 0;

	// Reset texture matrix to identity
    MATRIX_CONTROL  = DS_MAT_TEXTURE;
	MATRIX_IDENTITY = 0;
	MATRIX_CONTROL  = matrix_modes[lastMatrix];
}

void Gfx_Create(void) {
	Gfx_RestoreState();

	Gfx.MinTexWidth  =   8;
	Gfx.MinTexHeight =   8;
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
	//Gfx.MaxTexSize   = 256 * 256;
	Gfx.Created	     = true;
	Gfx.Limitations  = GFX_LIMIT_VERTEX_ONLY_FOG;

	ResetGPU();
	Gfx_ClearColor(PackedCol_Make(0, 120, 80, 255));
	Gfx_SetViewport(0, 0, 256, 192);
	
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_TEXTURE);
	vramSetBankC(VRAM_C_TEXTURE);
	vramSetBankD(VRAM_D_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	
	Gfx_SetFaceCulling(false);
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();

	vramSetBankA(VRAM_A_LCD);
	vramSetBankB(VRAM_B_LCD);
	vramSetBankC(VRAM_C_LCD);
	vramSetBankD(VRAM_D_LCD);
	vramSetBankE(VRAM_E_LCD);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Nintendo DS --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) { 
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	int x2 = x + w - 1;
	int y2 = y + h - 1;
	GFX_VIEWPORT = x | (y << 8) | (x2 << 16) | (y2 << 24);
}

void Gfx_SetScissor (int x, int y, int w, int h) { }

void Gfx_BeginFrame(void) {
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO
} 

void Gfx_ClearColor(PackedCol color) {
	int R = PackedCol_R(color) >> 3;
	int G = PackedCol_G(color) >> 3;
	int B = PackedCol_B(color) >> 3;

	int rgb    = RGB15(R, G, B);
	int alpha  = 31;
	int polyID = 63; // polygon ID of clear plane

	GFX_CLEAR_COLOR = rgb | ((alpha & 0x1F) << 16) | ((polyID & 0x3F) << 24);
}

void Gfx_EndFrame(void) {
	// W buffering is used for fog
	GFX_FLUSH = GL_WBUFFERING;
	// TODO not needed?
	swiWaitForVBlank();
}


/*########################################################################################################################*
*------------------------------------------------------Texture memory-----------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture_ {
	cc_uint16 width, height;
	u32 texFormat, texBase, texBlocks;
	u16 palFormat, palBase, palBlocks;
} CCTexture;

// Texture VRAM banks - Banks A, B, C, D (128 kb each)
#define TEX_TOTAL_SIZE (128 * 1024 * 4)
// Use 256 bytes for size of each block
#define TEX_BLOCK_SIZE 256

#define TEX_TOTAL_BLOCKS (TEX_TOTAL_SIZE / TEX_BLOCK_SIZE)
static cc_uint8 tex_table[TEX_TOTAL_BLOCKS / BLOCKS_PER_PAGE];

static void texture_alloc(CCTexture* tex, int size) {
	int blocks = (size + (TEX_BLOCK_SIZE - 1)) / TEX_BLOCK_SIZE;
	int addr   = blockalloc_alloc(tex_table, TEX_TOTAL_BLOCKS, blocks);
	if (addr == -1) return;

	tex->texBase   = addr;
	tex->texBlocks = blocks;
}

static void texture_release(CCTexture* tex) {
	blockalloc_free(tex_table, tex->texBase, tex->texBlocks);
}

// Palette VRAM banks - Bank E (64 kb)
#define PAL_TOTAL_SIZE (64 * 1024)
// Use 16 hwords for size of each block
#define PAL_BLOCK_SIZE 32

#define PAL_TOTAL_BLOCKS (PAL_TOTAL_SIZE / PAL_BLOCK_SIZE)
static cc_uint8 pal_table[PAL_TOTAL_BLOCKS / BLOCKS_PER_PAGE];

static void palette_alloc(CCTexture* tex, int size) {
	int blocks = (size + (PAL_BLOCK_SIZE - 1)) / PAL_BLOCK_SIZE;
	int addr   = blockalloc_alloc(pal_table, PAL_TOTAL_BLOCKS, blocks);
	if (addr == -1) return;

	tex->palBase   = addr;
	tex->palBlocks = blocks;
}

static void palette_release(CCTexture* tex) {
	blockalloc_free(pal_table, tex->palBase, tex->palBlocks);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static CCTexture* activeTex;
static int texOffsetX, texOffsetY;

static void UpdateTextureMatrix(void) {
	int width  = activeTex ? activeTex->width  : 0;
	int height = activeTex ? activeTex->height : 0;
	
	// Scale uvm to fit into texture size
	MATRIX_CONTROL  = DS_MAT_TEXTURE;
	MATRIX_IDENTITY = 0;

	MATRIX_SCALE = width  << 8; // X scale
	MATRIX_SCALE = height << 8; // Y scale
	MATRIX_SCALE = 0;           // Z scale

	if (texOffsetX || texOffsetY) {
		MATRIX_TRANSLATE = (texOffsetX * width ); // X
		MATRIX_TRANSLATE = (texOffsetY * height); // Y
		MATRIX_TRANSLATE = 0;          // Z
	}

	MATRIX_CONTROL = matrix_modes[lastMatrix];
}

void Gfx_EnableTextureOffset(float x, float y) {
	// Looks bad due to low uvm precision
	// TODO: Right for negative x/y ?
	// TODO speed probably isn't quite right
	texOffsetX = (int)(x * 32768) & (32768 - 1);
	texOffsetY = (int)(y * 32768) & (32768 - 1);
	UpdateTextureMatrix();
}

void Gfx_DisableTextureOffset(void) {
	texOffsetX = 0;
	texOffsetY = 0;
	UpdateTextureMatrix();
}

static CC_INLINE int FindInPalette(cc_uint16* pal, int pal_size, cc_uint16 color) {
	if ((color >> 15) == 0) return 0;
	
	for (int i = 1; i < pal_size; i++) 
	{
		if (pal[i] == color) return i;
	}
	return -1;
}

static CC_INLINE int CalcPalette(cc_uint16* palette, struct Bitmap* bmp, int rowWidth) {
	int pal_count = 1;
	palette[0]    = 0; // entry 0 is transparent colour
	
	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint16* row = bmp->scan0 + y * rowWidth;
		
		for (int x = 0; x < bmp->width; x++) 
		{
			cc_uint16 color = row[x];
			int idx = FindInPalette(palette, pal_count, color);
			if (idx >= 0) continue;

			if (pal_count >= 256) return 256 + 1;

			palette[pal_count] = color;
			pal_count++;
		}
	}
	return pal_count;
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = Mem_TryAllocCleared(1, sizeof(CCTexture));
	if (!tex) return 0;

	tex->width  = bmp->width;
	tex->height = bmp->height;

	cc_uint16 palette[256];
	int pal_count = CalcPalette(palette, bmp, rowWidth);
	int tex_size, tex_fmt;

	if (pal_count <= 4) {
		tex_size = bmp->width * bmp->height / 4; // 2 bits per pixel
		tex_fmt  = GL_RGB4;
	} else if (pal_count <= 16) {
		tex_size = bmp->width * bmp->height / 2; // 4 bits per pixel
		tex_fmt  = GL_RGB16;
	} else if (pal_count <= 256) {
		tex_size = bmp->width * bmp->height;     // 8 bits per pixel
		tex_fmt  = GL_RGB256;
	} else {
		tex_size = bmp->width * bmp->height * 2; // 16 bits per pixel
		tex_fmt  = GL_RGBA;
	}

	texture_alloc(tex, tex_size);
	if (!tex->texBlocks) {
		Platform_Log2("No VRAM for %i x %i texture", &bmp->width, &bmp->height);
		return NULL;
	}

	int offset = tex->texBase * TEX_BLOCK_SIZE;
	u16* addr  = (u16*) ((u8*)VRAM_A + offset);
	u16* tmp_u16[128]; // 256 bytes
	char* tmp = (char*)tmp_u16;

	u32 banks = vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);

	int stride;

	if (tex_fmt == GL_RGB4) {
		char* buf = malloc(tex_size);
		int i = 0;
		if (!buf) return NULL;

		for (int y = 0; y < bmp->height; y++)
		{
			cc_uint16* row = bmp->scan0 + y * rowWidth;
		
			for (int x = 0; x < bmp->width; x++, i++) 
			{
				int idx = FindInPalette(palette, pal_count, row[x]);
			
				if ((i & 3) == 0) {
					buf[i >> 2] = idx;
				} else {
					buf[i >> 2] |= idx << (2 * (i & 3));
				}
			}
		}

		CopyHWords(buf, addr, tex_size >> 1);
		free(buf);
	} else if (tex_fmt == GL_RGB16) {
		stride = bmp->width >> 2;

		for (int y = 0; y < bmp->height; y++, addr += stride)
		{
			cc_uint16* row = bmp->scan0 + y * rowWidth;
		
			for (int x = 0; x < bmp->width; x++) 
			{
				int idx = FindInPalette(palette, pal_count, row[x]);
			
				if ((x & 1) == 0) {
					tmp[x >> 1] = idx;
				} else {
					tmp[x >> 1] |= idx << 4;
				}
			}
			CopyHWords(tmp, addr, stride);
		}
	} else if (tex_fmt == GL_RGB256) {
		stride = bmp->width >> 1;

		for (int y = 0; y < bmp->height; y++, addr += stride)
		{
			cc_uint16* row = bmp->scan0 + y * rowWidth;
		
			for (int x = 0; x < bmp->width; x++) 
			{
				tmp[x] = FindInPalette(palette, pal_count, row[x]);
			}
			CopyHWords(tmp, addr, stride);
		}
	} else {
		stride = bmp->width;

		for (int y = 0; y < bmp->height; y++, addr += stride) {
			cc_uint16* src = bmp->scan0 + y * rowWidth;
			CopyHWords(src, addr, stride);
		}
	}

	vramRestorePrimaryBanks(banks);

	int sSize  = (Math_ilog2(tex->width)  - 3) << 20;
	int tSize  = (Math_ilog2(tex->height) - 3) << 23;

	tex->texFormat = (offset >> 3) | sSize | tSize | (tex_fmt << 26) | 
						GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT;

	if (tex_fmt != GL_RGBA) {
		palette_alloc(tex, pal_count * 2);
		offset = tex->palBase * PAL_BLOCK_SIZE;

		vramSetBankE(VRAM_E_LCD);
    	CopyHWords(palette, (u8*)VRAM_E + offset, pal_count);
		vramSetBankE(VRAM_E_TEX_PALETTE);

		tex->palFormat = tex_fmt == GL_RGB4 ? (offset >> 3) : (offset >> 4);
	}
	return tex;
}

void Gfx_BindTexture(GfxResourceID texId) {
	activeTex = (CCTexture*)texId;
	GFX_TEX_FORMAT = activeTex ? activeTex->texFormat : 0;
	GFX_PAL_FORMAT = activeTex ? activeTex->palFormat : 0;	

	UpdateTextureMatrix();
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	
	int width = tex->width;
	return;
	// TODO doesn't work without VRAM bank changing to LCD and back maybe??
	// (see what glTeximage2D does ??)
	
	/*for (int yy = 0; yy < part->height; yy++)
	{
		cc_uint16* dst = vram_ptr + width * (y + yy) + x;
		cc_uint16* src = part->scan0 + rowWidth * yy;
		
		for (int xx = 0; xx < part->width; xx++)
		{
			*dst++ = *src++;
		}
	}*/
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = (CCTexture*)(*texId);

	if (tex) {
		texture_release(tex);
		palette_release(tex);
		Mem_Free(tex);
	}
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/

void Gfx_SetDepthWrite(cc_bool enabled) { }
void Gfx_SetDepthTest(cc_bool enabled)  { }

static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
}

cc_bool Gfx_WarnIfNecessary(void) { return true; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;
	width  /= 64.0f; 
	height /= 64.0f;

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

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
// Preprocess vertex buffers into optimised layout for DS
static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

struct DSTexturedVertex {
	vu32 command;
	vu32 rgb;
	vu32 uv;
	vu32 xy; vu32 z;
};

struct DSColouredVertex {
	vu32 command;
	vu32 rgb;
	vu32 xy; vu32 z;
};

// Precalculate all the expensive vertex data conversion,
//  so that actual drawing of them is as fast as possible
static void PreprocessTexturedVertices(void) {
	struct   VertexTextured* src = gfx_vertices;
	struct DSTexturedVertex* dst = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		struct VertexTextured v = *src;
		
		v16 x = floattov16(v.x / 64.0f);
		v16 y = floattov16(v.y / 64.0f);
		v16 z = floattov16(v.z / 64.0f);
	
		int uvX = ((int) (v.U * 256.0f));
		int uvY = ((int) (v.V * 256.0f));

		int r = PackedCol_R(v.Col);
		int g = PackedCol_G(v.Col);
		int b = PackedCol_B(v.Col);
		
		dst->command = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_TEX_COORD, FIFO_VERTEX16, FIFO_NOP);
		
		dst->rgb = ARGB16(1, r >> 3, g >> 3, b >> 3);
		
		dst->uv = TEXTURE_PACK(uvX, uvY);
		
		dst->xy = (y << 16) | (x & 0xFFFF);
		dst->z  = z;
	}
	
	DC_FlushRange(gfx_vertices, buf_count * sizeof(struct DSTexturedVertex));
}

static void PreprocessColouredVertices(void) {
	struct   VertexColoured* src = gfx_vertices;
	struct DSColouredVertex* dst = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		struct VertexColoured v = *src;
		
		v16 x = floattov16(v.x / 64.0f);
		v16 y = floattov16(v.y / 64.0f);
		v16 z = floattov16(v.z / 64.0f);

		int r = PackedCol_R(v.Col);
		int g = PackedCol_G(v.Col);
		int b = PackedCol_B(v.Col);
		
		dst->command = FIFO_COMMAND_PACK(FIFO_COLOR, FIFO_VERTEX16, FIFO_NOP, FIFO_NOP);
		
		dst->rgb = ARGB16(1, r >> 3, g >> 3, b >> 3);
		
		dst->xy = (y << 16) | (x & 0xFFFF);
		dst->z  = z;
	}
	
	DC_FlushRange(gfx_vertices, buf_count * sizeof(struct DSColouredVertex));
}

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)	{ }
void Gfx_DeleteIb(GfxResourceID* ib) { }


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
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool skipRendering;
static cc_bool backfaceCull;
static cc_bool alphaBlend;

static cc_bool fogEnabled;
static FogFunc fogMode;
static float fogDensityEnd;

static void SetPolygonMode() {
	int blend = !gfx_rendering2D && alphaBlend;
	u32 fmt =
		POLY_ALPHA(blend ? 14 : 31) | 
		(backfaceCull ? POLY_CULL_BACK : POLY_CULL_NONE) | 
		(fogEnabled ? POLY_FOG : 0) | 
		POLY_RENDER_FAR_POLYS | 
		POLY_RENDER_1DOT_POLYS;

	GFX_POLY_FORMAT = fmt;
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	backfaceCull = enabled;
	SetPolygonMode();
}

static void SetAlphaBlend(cc_bool enabled) {
	alphaBlend = enabled;
	SetPolygonMode();
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

static void RecalculateFog() {
	if (fogMode == FOG_LINEAR) {
		int fogEnd = floattof32(fogDensityEnd);
		
		// Find shift value so that our fog end is
		//  inside maximum distance covered by fog table
		int shift = 10;
		while (shift > 0) {
			// why * 512? I dont know
			if (32 * (0x400 >> shift) * 512 >= fogEnd) break;
			shift--;
		}
		
		glFogShift(shift);
		GFX_FOG_OFFSET = 0;
		
		for (int i = 0; i < 32; i++) {
			int distance  = (i * 512 + 256) * (0x400 >> shift);
			int intensity = distance * 127 / fogEnd;
			if(intensity > 127) intensity = 127;
			
			GFX_FOG_TABLE[i] = intensity;
		}
		
		GFX_FOG_TABLE[31] = 127;
	} else {
		// TODO?
	}
}

void Gfx_SetFog(cc_bool enabled) {
	fogEnabled = enabled;
	SetPolygonMode();
}

void Gfx_SetFogCol(PackedCol color) {
	int r = PackedCol_R(color) >> 3;
	int g = PackedCol_G(color) >> 3;
	int b = PackedCol_B(color) >> 3;
	int a = 31;

	GFX_FOG_COLOR = RGB15(r, g, b) | (a << 16);
}

void Gfx_SetFogDensity(float value) {
	fogDensityEnd = value;
	RecalculateFog();
}

void Gfx_SetFogEnd(float value) {
	fogDensityEnd = value;
	RecalculateFog();
}

void Gfx_SetFogMode(FogFunc func) {
	fogMode = func;
	RecalculateFog();
}

static void SetAlphaTest(cc_bool enabled) {
	if (enabled) {
		//glEnable(GL_ALPHA_TEST);
	} else {
		//glDisable(GL_ALPHA_TEST);
	}
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	skipRendering = depthOnly;
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { 
		lastMatrix	   = type; 
		MATRIX_CONTROL = matrix_modes[type]; 
	}
	
	// loads 4x4 identity matrix
	if (matrix == &Matrix_Identity) {
		MATRIX_IDENTITY = 0;
		return;
		// TODO still scale?
	}

	// loads 4x4 matrix from memory
	const float* src = (const float*)matrix;
	for (int i = 0; i < 4 * 4; i++)
	{
		MATRIX_LOAD4x4 = floattof32(src[i]);
	}

	// Vertex commands are signed 16 bit values, with 12 bits fractional
	//  aka only from -8.0 to 8.0
	// That's way too small to be useful, so counteract that by scaling down
	//  vertices and then scaling up the matrix multiplication
	if (type == MATRIX_VIEW) {
		MATRIX_SCALE = floattof32(64.0f); // X scale
		MATRIX_SCALE = floattof32(64.0f); // Y scale
		MATRIX_SCALE = floattof32(64.0f); // Z scale
	}
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}


/*########################################################################################################################*
*--------------------------------------------------------Rendering--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {
}

static void CallDrawList(void* list, u32 listSize) {
	// Based on libnds glCallList
	while (dmaBusy(0) || dmaBusy(1) || dmaBusy(2) || dmaBusy(3));
	dmaSetParams(0, list, (void*) &GFX_FIFO, DMA_FIFO | listSize);
	while (dmaBusy(0));
}

static void Draw_ColouredTriangles(int verticesCount, int startVertex) {
	GFX_TEX_FORMAT = 0; // Disable texture

	GFX_BEGIN = GL_QUADS;
	CallDrawList(&((struct DSColouredVertex*) gfx_vertices)[startVertex], verticesCount * 4);
	GFX_END = 0;
}

static void Draw_TexturedTriangles(int verticesCount, int startVertex) {
	GFX_BEGIN = GL_QUADS;
	CallDrawList(&((struct DSTexturedVertex*) gfx_vertices)[startVertex], verticesCount * 5);
	GFX_END = 0;
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, startVertex);
	} else {
		Draw_ColouredTriangles(verticesCount, startVertex);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, 0);
	} else {
		Draw_ColouredTriangles(verticesCount, 0);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (skipRendering) return;
	Draw_TexturedTriangles(verticesCount, startVertex);
}
#endif

