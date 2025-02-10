#include "Core.h"
#if defined CC_BUILD_DREAMCAST
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <malloc.h>
#include <string.h>
#include <kos.h>
#include <dc/matrix.h>
#include <dc/pvr.h>
#include "../third_party/gldc/src/gldc.h"

static cc_bool renderingDisabled;
static cc_bool stateDirty;

#define VERTEX_BUFFER_SIZE 32 * 40000
#define PT_ALPHA_REF 0x011c
#define MAX_TEXTURE_COUNT 768


/*########################################################################################################################*
*------------------------------------------------------Commands list------------------------------------------------------*
*#########################################################################################################################*/
#define VERTEX_SIZE 32

struct CommandsList {
	uint32_t length;
	uint32_t capacity;
	uint32_t list_type;
	uint8_t* data;
} __attribute__((aligned(32)));

// e.g. 1 -> 256, 50 -> 256, 256 -> 256
#define ROUND_UP_256(v) (((v) + 0xFFu) & ~0xFFu)

static CC_INLINE void* CommandsList_Reserve(struct CommandsList* list, cc_uint32 count) {
    cc_uint32 cur_size = list->length * VERTEX_SIZE;

    if (count < list->capacity) {
        return list->data + cur_size;
    }
    /* We overallocate so that we don't make small allocations during push backs */
    count = ROUND_UP_256(count);

    cc_uint32 new_size = count * VERTEX_SIZE;
    cc_uint8* cur_data = list->data;

    cc_uint8* data = (cc_uint8*)memalign(0x20, new_size);
    if (!data) return NULL;

    memcpy(data, cur_data, cur_size);
    free(cur_data);

    list->data     = data;
    list->capacity = count;
    return data + cur_size;
}

static void CommandsList_Append(struct CommandsList* list, const void* cmd) {
	void* dst = CommandsList_Reserve(list, list->length + 1);
	if (!dst) Process_Abort("Out of memory");
	
	memcpy(dst, cmd, VERTEX_SIZE);
	list->length++;
}



/*########################################################################################################################*
*-----------------------------------------------------Texture memory------------------------------------------------------*
*#########################################################################################################################*/
// For PVR2 GPU, highly recommended that multiple textures don't cross the same 2048 byte page alignment
// So to avoid this, ensure that each texture is allocated at the start of a 2048 byte page
#define TEXMEM_PAGE_SIZE 2048
#define TEXMEM_PAGE_MASK (TEXMEM_PAGE_SIZE - 1)
// Round up to nearest page
#define TEXMEM_PAGE_ROUNDUP(size) (((size) + TEXMEM_PAGE_MASK) & ~TEXMEM_PAGE_MASK)
// Leave a little bit of memory available by KOS PVR code
#define TEXMEM_RESERVED (48 * 1024)
#define TEXMEM_TO_PAGE(addr) ((cc_uint32)((addr) - texmem_base) / TEXMEM_PAGE_SIZE)

TextureObject* TEXTURE_ACTIVE;
static TextureObject TEXTURE_LIST[MAX_TEXTURE_COUNT];

// Base address in VRAM for textures
static cc_uint8* texmem_base;
// Total number of pages in VRAM
static cc_uint32 texmem_pages;
// Stores which pages in VRAM are currently used
static cc_uint8* texmem_used;

void texmem_init(void) {
    size_t vram_free = pvr_mem_available();
    size_t tmem_size = vram_free - TEXMEM_RESERVED;

    void* base   = pvr_mem_malloc(tmem_size);
    texmem_base  = (void*)TEXMEM_PAGE_ROUNDUP((cc_uintptr)base);
	texmem_pages = tmem_size / TEXMEM_PAGE_SIZE;
	texmem_used  = Mem_AllocCleared(1, texmem_pages, "Page state");
}

static int texmem_move(cc_uint8* ptr, cc_uint32 size) {
	cc_uint32 pages = TEXMEM_PAGE_ROUNDUP(size) / TEXMEM_PAGE_SIZE;
	cc_uint32 page  = TEXMEM_TO_PAGE(ptr);
	int moved = 0;

	// Try to shift downwards towards prior allocated texture
	while (page > 0 && texmem_used[page - 1] == 0) {
		page--; moved++;
		texmem_used[page + pages] = 0;
	}
	
	// Mark previously empty pages as now used
	for (int i = 0; i < moved; i++) 
	{
		texmem_used[page + i] = 1;
	}
	return moved * TEXMEM_PAGE_SIZE;
}

static int texmem_defragment(void) {
	int moved_any = false;
	for (int i = 0; i < MAX_TEXTURE_COUNT; i++)
	{
		TextureObject* tex = &TEXTURE_LIST[i];
		if (!tex->data) continue;

		cc_uint32 size = tex->width * tex->height * 2;
		int moved = texmem_move(tex->data, size);
		if (!moved) continue;

		moved_any = true;
		memmove(tex->data - moved, tex->data, size);
		tex->data -= moved;
	}
	return moved_any;
}

static CC_INLINE int texmem_can_alloc(cc_uint32 beg, cc_uint32 pages) {
	if (texmem_used[beg]) return false;

	for (cc_uint32 page = beg; page < beg + pages; page++)
	{
		if (texmem_used[page]) return false;
	}
	return true;
}

static void* texmem_alloc_pages(cc_uint32 size) {
	cc_uint32 pages = TEXMEM_PAGE_ROUNDUP(size) / TEXMEM_PAGE_SIZE;
	if (pages > texmem_pages) return NULL;

	for (cc_uint32 page = 0; page < texmem_pages - pages; page++) 
	{
		if (!texmem_can_alloc(page, pages)) continue;

		for (cc_uint32 i = 0; i < pages; i++) 
			texmem_used[page + i] = 1;

        return texmem_base + page * TEXMEM_PAGE_SIZE;
    }
	return NULL;
}

static void* texmem_alloc(cc_uint32 size) {
    void* ptr = texmem_alloc_pages(size);
    if (ptr) return ptr;

	Platform_LogConst("Out of VRAM! Defragmenting..");
	while (texmem_defragment()) { }
    return texmem_alloc_pages(size);
}

static void texmem_free(cc_uint8* ptr, cc_uint32 size) {
	cc_uint32 pages = TEXMEM_PAGE_ROUNDUP(size) / TEXMEM_PAGE_SIZE;
    cc_uint32 page  = TEXMEM_TO_PAGE(ptr);

	for (cc_uint32 i = 0; i < pages; i++)
		texmem_used[page + i] = 0;
}

static cc_uint32 texmem_total_free(void) {
	cc_uint32 free = 0;
    for (cc_uint32 page = 0; page < texmem_pages; page++) 
	{
		if (!texmem_base[page]) free += TEXMEM_PAGE_SIZE;
    }
	return free;
}

static cc_uint32 texmem_total_used(void) {
	cc_uint32 used = 0;
    for (cc_uint32 page = 0; page < texmem_pages; page++) 
	{
		if (texmem_base[page]) used += TEXMEM_PAGE_SIZE;
    }
	return used;
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static struct CommandsList listOP;
static struct CommandsList listPT;
static struct CommandsList listTR;
// For faster performance, instead of having all 3 lists buffer everything first
//  and then have all the buffered data submitted to the TA (i.e. drawn) at the end of the frame,
//  instead have the most common list have data directly submitted to the TA throughout the frame
static struct CommandsList* direct = &listPT;
static cc_bool exceeded_vram;

static CC_INLINE struct CommandsList* ActivePolyList(void) {
    if (gfx_alphaBlend) return &listTR;
    if (gfx_alphaTest)  return &listPT;

    return &listOP;
}

static void no_vram_handler(uint32 code, void *data) { exceeded_vram = true; }

static void InitGPU(void) {
	cc_bool autosort = false; // Turn off auto sorting to match traditional GPU behaviour
	cc_bool fsaa     = false;
	AUTOSORT_ENABLED = autosort;

	pvr_init_params_t params = {
		// Opaque, punch through, translucent polygons with largest bin sizes
		{ PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32 },
		VERTEX_BUFFER_SIZE,
		0, fsaa,
		(autosort) ? 0 : 1,
		3 // extra OPBs
	};
    pvr_init(&params);

	// This event will be raised when no room in VRAM for vertices anymore
    asic_evt_set_handler(ASIC_EVT_PVR_PARAM_OUTOFMEM, no_vram_handler, NULL);
    asic_evt_enable(ASIC_EVT_PVR_PARAM_OUTOFMEM,      ASIC_IRQ_DEFAULT);
}

static void InitGLState(void) {
	pvr_set_zclip(0.0f);
	PVR_SET(PT_ALPHA_REF, 127); // define missing from KOS    
	//PVR_SET(PVR_SPANSORT_CFG, 0x0);

	ALPHA_TEST_ENABLED = false;
	CULLING_ENABLED    = false;
	BLEND_ENABLED      = false;
	DEPTH_TEST_ENABLED = false;
	DEPTH_MASK_ENABLED = true;
	TEXTURES_ENABLED   = false;
	FOG_ENABLED        = false;
	
	stateDirty       = true;
	listOP.list_type = PVR_LIST_OP_POLY;
	listPT.list_type = PVR_LIST_PT_POLY;
	listTR.list_type = PVR_LIST_TR_POLY;

	CommandsList_Reserve(&listOP, 1024 * 3);
	CommandsList_Reserve(&listPT,  512 * 3);
	CommandsList_Reserve(&listTR, 1024 * 3);
}

void Gfx_Create(void) {
	if (!Gfx.Created) InitGPU();
	if (!Gfx.Created) texmem_init();

	InitGLState();
	
	Gfx.MinTexWidth  = 8;
	Gfx.MinTexHeight = 8;
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 512 * 512; // reasonable cap as Dreamcast only has 8MB VRAM
	Gfx.Created      = true;
	
	Gfx_RestoreState();
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;

void Gfx_SetFaceCulling(cc_bool enabled) { 
	CULLING_ENABLED = enabled;
	stateDirty      = true;
}

static void SetAlphaBlend(cc_bool enabled) { 
	BLEND_ENABLED = enabled;
	stateDirty    = true;
}
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearColor(PackedCol color) {
	if (color == gfx_clearColor) return;
	gfx_clearColor = color;
	
	float r = PackedCol_R(color) / 255.0f;
	float g = PackedCol_G(color) / 255.0f;
	float b = PackedCol_B(color) / 255.0f;
	pvr_set_bg_color(r, g, b); // TODO: not working ?
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO: Doesn't work
}

void Gfx_SetDepthWrite(cc_bool enabled) { 
	if (DEPTH_MASK_ENABLED == enabled) return;
	
	DEPTH_MASK_ENABLED = enabled;
	stateDirty         = true;
}

void Gfx_SetDepthTest(cc_bool enabled) { 
	if (DEPTH_TEST_ENABLED == enabled) return;
	
	DEPTH_TEST_ENABLED = enabled;
	stateDirty         = true;
}

static void SetAlphaTest(cc_bool enabled) {
	ALPHA_TEST_ENABLED = enabled;
	stateDirty         = true;
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	// don't need a fake second pass in this case
	renderingDisabled = depthOnly;
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
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
	float zNear = 0.1f;

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
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static void* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
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
	gfx_vertices = vb;
	//sceKernelDcacheWritebackInvalidateRange(vb, vb_size);
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return memalign(16, maxVertices * strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
	//dcache_flush_range(vb, vb_size);
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


static unsigned Interleave(unsigned x) {
	// Simplified "Interleave bits by Binary Magic Numbers" from
	// http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN

	x = (x | (x << 8)) & 0x00FF00FF;
	x = (x | (x << 4)) & 0x0F0F0F0F;
	x = (x | (x << 2)) & 0x33333333;
	x = (x | (x << 1)) & 0x55555555;
	return x;
}

/*static int CalcTwiddledIndex(int x, int y, int w, int h) {
	// Twiddled index looks like this (lowest numbered bits are leftmost):
	//   - w = h: yxyx yxyx
	//   - w > h: yxyx xxxx
	//   - h > w: yxyx yyyy
	// And can therefore be broken down into two components:
	//  1) interleaved lower bits
	//  2) masked and then shifted higher bits
	
	int min_dimension    = Math.Min(w, h);
	
	int interleave_mask  = min_dimension - 1;
	int interleaved_bits = Math_ilog2(min_dimension);
	
	int shifted_mask = (~0) & ~interleave_mask;
	// as lower interleaved bits contain both X and Y, need to adjust the
	//  higher bit shift by number of interleaved bits used by other axis
	int shift_bits   = interleaved_bits;
	
	// For example, in the case of W=4 and H=8
	//  the bit pattern is yx_yx_yx_Y
	//  - lower 3 Y bits are interleaved
	//  - upper 1 Y bit must be shifted right 3 bits
	
	int lo_Y = Interleave(y & interleave_mask);
	int hi_Y = (y & shifted_mask) << shift_bits;
	int Y    = lo_Y | hi_Y;
	
	int lo_X  = Interleave(x & interleave_mask) << 1;
	int hi_X  = (x & shifted_mask) << shift_bits;
	int X     = lo_X | hi_X;

	return X | Y;
}*/

#define Twiddle_CalcFactors(w, h) \
	min_dimension    = min(w, h); \
	interleave_mask  = min_dimension - 1; \
	interleaved_bits = Math_ilog2(min_dimension); \
	shifted_mask     = ~interleave_mask; \
	shift_bits       = interleaved_bits;
	
#define Twiddle_CalcY(y) \
	lo_Y = Interleave(y & interleave_mask); \
	hi_Y = (y & shifted_mask) << shift_bits; \
	Y    = lo_Y | hi_Y;
	
#define Twiddle_CalcX(x) \
	lo_X  = Interleave(x & interleave_mask) << 1; \
	hi_X  = (x & shifted_mask) << shift_bits; \
	X     = lo_X | hi_X;
	
	
// B8 G8 R8 A8 > B4 G4 R4 A4
#define BGRA8_to_BGRA4(src) \
	((src[0] & 0xF0) >> 4) | (src[1] & 0xF0) | ((src[2] & 0xF0) << 4) | ((src[3] & 0xF0) << 8);	

static void ConvertTexture(cc_uint16* dst, struct Bitmap* bmp, int rowWidth) {
	unsigned min_dimension;
	unsigned interleave_mask, interleaved_bits;
	unsigned shifted_mask, shift_bits;
	unsigned lo_Y, hi_Y, Y;
	unsigned lo_X, hi_X, X;	
	Twiddle_CalcFactors(bmp->width, bmp->height);
	
	for (int y = 0; y < bmp->height; y++)
	{
		Twiddle_CalcY(y);
		cc_uint8* src = (cc_uint8*)(bmp->scan0 + y * rowWidth);
		
		for (int x = 0; x < bmp->width; x++, src += 4)
		{
			Twiddle_CalcX(x);
			dst[X | Y] = BGRA8_to_BGRA4(src);
		}
	}
}

static TextureObject* FindFreeTexture(void) {
    for (int i = 0; i < MAX_TEXTURE_COUNT; i++) 
	{
		TextureObject* tex = &TEXTURE_LIST[i];
        if (!tex->data) return tex;
    }
    return NULL;
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	TextureObject* tex = FindFreeTexture();
	if (!tex) return NULL;

	tex->width  = bmp->width;
	tex->height = bmp->height;
	tex->color  = PVR_TXRFMT_ARGB4444;

	tex->data = texmem_alloc(bmp->width * bmp->height * 2);
	if (!tex->data) { Platform_LogConst("Out of PVR VRAM!"); return NULL; }

	ConvertTexture(tex->data, bmp, rowWidth);
	return tex;
}

// TODO: struct GPUTexture ??
static void ConvertSubTexture(cc_uint16* dst, int texWidth, int texHeight,
				int originX, int originY, 
				struct Bitmap* bmp, int rowWidth) {
	unsigned min_dimension;
	unsigned interleave_mask, interleaved_bits;
	unsigned shifted_mask, shift_bits;
	unsigned lo_Y, hi_Y, Y;
	unsigned lo_X, hi_X, X;
	Twiddle_CalcFactors(texWidth, texHeight);
	
	for (int y = 0; y < bmp->height; y++)
	{
		int dstY = y + originY;
		Twiddle_CalcY(dstY);
		cc_uint8* src = (cc_uint8*)(bmp->scan0 + rowWidth * y);
		
		for (int x = 0; x < bmp->width; x++, src += 4)
		{
			int dstX = x + originX;
			Twiddle_CalcX(dstX);
			dst[X | Y] = BGRA8_to_BGRA4(src);
		}
	}
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	TextureObject* tex = (TextureObject*)texId;
	
	ConvertSubTexture(tex->data, tex->width, tex->height,
				x, y, part, rowWidth);
	// TODO: Do we need to flush VRAM?
}

void Gfx_BindTexture(GfxResourceID texId) {
    TEXTURE_ACTIVE = (TextureObject*)texId;
	stateDirty     = true;
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	TextureObject* tex = (TextureObject*)(*texId);
	if (!tex) return;

	cc_uint32 size = tex->width * tex->height * 2;
	texmem_free(tex->data, size);

	tex->data = NULL;
	*texId    = 0;
}



/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = 16.0f, gfx_fogDensity = 1.0f;
static FogFunc gfx_fogMode = -1;

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	if (FOG_ENABLED == enabled) return;
	
	FOG_ENABLED = enabled;
	stateDirty  = true;
}

void Gfx_SetFogCol(PackedCol color) {
	if (color == gfx_fogColor) return;
	gfx_fogColor = color;

	float r = PackedCol_R(color) / 255.0f; 
	float g = PackedCol_G(color) / 255.0f;
	float b = PackedCol_B(color) / 255.0f; 
	float a = PackedCol_A(color) / 255.0f;

	pvr_fog_table_color(a, r, g, b);
}

static void UpdateFog(void) {
	if (gfx_fogMode == FOG_LINEAR) {
		pvr_fog_table_linear(0.0f, gfx_fogEnd);
	} else if (gfx_fogMode == FOG_EXP) {
		pvr_fog_table_exp(gfx_fogDensity);
	} else if (gfx_fogMode == FOG_EXP2) {
		pvr_fog_table_exp2(gfx_fogDensity);
	}
}

void Gfx_SetFogDensity(float value) {
	if (value == gfx_fogDensity) return;
	gfx_fogDensity = value;
	UpdateFog();
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	gfx_fogEnd = value;
	UpdateFog();
}

void Gfx_SetFogMode(FogFunc func) {
	if (func == gfx_fogMode) return;
	gfx_fogMode = func;
	UpdateFog();
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static matrix_t __attribute__((aligned(32))) _proj, _view;
static float textureOffsetX, textureOffsetY;
static int textureOffset;

static float vp_scaleX, vp_scaleY, vp_offsetX, vp_offsetY;
static matrix_t __attribute__((aligned(32))) mat_vp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_PROJ) memcpy(&_proj, matrix, sizeof(struct Matrix));
	if (type == MATRIX_VIEW) memcpy(&_view, matrix, sizeof(struct Matrix));

	memcpy(&mat_vp, &Matrix_Identity, sizeof(struct Matrix));
	mat_vp[0][0] = vp_scaleX;
	mat_vp[1][1] = vp_scaleY;
	mat_vp[3][0] = vp_offsetX;
	mat_vp[3][1] = vp_offsetY;

	mat_load(&mat_vp);
	mat_apply(&_proj);
	mat_apply(&_view);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}


void Gfx_EnableTextureOffset(float x, float y) {
	textureOffset  = true;
	textureOffsetX = x;
	textureOffsetY = y;
}

void Gfx_DisableTextureOffset(void) {
	textureOffset  = false;
}

static CC_NOINLINE void ShiftTextureCoords(int count) {
	for (int i = 0; i < count; i++) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + i;
		v->U += textureOffsetX;
		v->V += textureOffsetY;
	}
}

static CC_NOINLINE void UnshiftTextureCoords(int count) {
	for (int i = 0; i < count; i++) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + i;
		v->U -= textureOffsetX;
		v->V -= textureOffsetY;
	}
}


/*########################################################################################################################*
*-------------------------------------------------------State setup-------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	gfx_format = -1;
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
extern void apply_poly_header(pvr_poly_hdr_t* header, int list_type);
static cc_bool loggedNoVRAM;

extern Vertex* DrawColouredQuads(const void* src, Vertex* dst, int numQuads);
extern Vertex* DrawTexturedQuads(const void* src, Vertex* dst, int numQuads);

static Vertex* ReserveOutput(struct CommandsList* list, cc_uint32 elems) {
	Vertex* beg;
	for (;;)
	{
		if ((beg = CommandsList_Reserve(list, elems))) return beg;
		// Try to reduce view distance to save on RAM
		if (Game_ReduceVRAM()) continue;

		if (!loggedNoVRAM) {
			loggedNoVRAM = true;
			Logger_SysWarn(ERR_OUT_OF_MEMORY, "allocating temp memory");
		}
		return NULL;
	}
}

void DrawQuads(int count, void* src) {
	if (!count) return;
	struct CommandsList* list = ActivePolyList();

	cc_uint32 header_required = (list->length == 0) || stateDirty;
	// Reserve room for the vertices and header
	Vertex* beg = ReserveOutput(list, list->length + (header_required) + count);
	if (!beg) return;

	if (header_required) {
		apply_poly_header((pvr_poly_hdr_t*)beg, list->list_type);
		stateDirty = false;
		list->length++;
		beg++;
	}
	Vertex* end;

	if (TEXTURES_ENABLED) {
		end = DrawTexturedQuads(src, beg, count >> 2);
	} else {
		end = DrawColouredQuads(src, beg, count >> 2);
	}
	list->length += (end - beg);

	if (list != direct) return;
	SceneListSubmit((Vertex*)list->data, list->length);
	list->length = 0;
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	TEXTURES_ENABLED = fmt == VERTEX_FORMAT_TEXTURED;
	stateDirty       = true;
}

void Gfx_DrawVb_Lines(int verticesCount) {
	//SetupVertices(0);
	//glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	void* src;
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		src = gfx_vertices + startVertex * SIZEOF_VERTEX_TEXTURED;
	} else {
		src = gfx_vertices + startVertex * SIZEOF_VERTEX_COLOURED;
	}

	DrawQuads(verticesCount, src);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (textureOffset) ShiftTextureCoords(verticesCount);
	DrawQuads(verticesCount, gfx_vertices);
	if (textureOffset) UnshiftTextureCoords(verticesCount);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (renderingDisabled) return;
	
	void* src = gfx_vertices + startVertex * SIZEOF_VERTEX_TEXTURED;
	DrawQuads(verticesCount, src);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	int freeMem = texmem_total_free();
	int usedMem = texmem_total_used();
	
	float freeMemMB = freeMem / (1024.0 * 1024.0);
	float usedMemMB = usedMem / (1024.0 * 1024.0);
	
	String_AppendConst(info, "-- Using Dreamcast --\n");
	String_AppendConst(info, "GPU: PowerVR2 CLX2 100mHz\n");
	String_AppendConst(info, "T&L: GLdc library (KallistiOS / Kazade)\n");
	String_Format2(info,     "Texture memory: %f2 MB used, %f2 MB free\n", &usedMemMB, &freeMemMB);
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	// no need to use glClear
}

static pvr_dr_state_t dr_state;
static void BeginList(int list_type) {
	pvr_list_begin(list_type);
	pvr_dr_init(&dr_state);
}

static void FinishList(void) {
	sq_wait();
	pvr_list_finish();
}

void Gfx_BeginFrame(void) {
	pvr_scene_begin();
	// Directly render PT list, buffer other lists first
	BeginList(PVR_LIST_PT_POLY);
	if (!exceeded_vram) return;

	exceeded_vram = false;
	Game_ReduceVRAM();
}

static void SubmitList(struct CommandsList* list) {
	if (!list->length || list == direct) return;

	BeginList(list->list_type);
	SceneListSubmit((Vertex*)list->data, list->length);
	FinishList();
	list->length = 0;
}

void Gfx_EndFrame(void) {
	FinishList();

	SubmitList(&listOP);
	SubmitList(&listPT);
	SubmitList(&listTR);
	pvr_scene_finish();
	pvr_wait_ready();
}

void Gfx_OnWindowResize(void) {
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	vp_scaleX  = w *  0.5f; // hwidth
	vp_scaleY  = h * -0.5f; // hheight
	vp_offsetX = x + w * 0.5f; // x_plus_hwidth
	vp_offsetY = y + h * 0.5f; // y_plus_hheight
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	SCISSOR_TEST_ENABLED = x != 0 || y != 0 || w != Game.Width || h != Game.Height;
	stateDirty = true;

	pvr_poly_hdr_t c;
	c.cmd = PVR_CMD_USERCLIP;
	c.mode1 = c.mode2 = c.mode3 = 0;

	c.d1 = x >> 5;
	c.d2 = y >> 5;
	c.d3 = ((x + w) >> 5) - 1;
	c.d4 = ((y + h) >> 5) - 1;

	CommandsList_Append(&listOP, &c);
	CommandsList_Append(&listPT, &c);
	CommandsList_Append(&listTR, &c);
}
#endif
