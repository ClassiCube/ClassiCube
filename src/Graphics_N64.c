#include "Core.h"
#ifdef CC_BUILD_N64
#define CC_DYNAMIC_VBS_ARE_STATIC
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <libdragon.h>
#include <malloc.h>
#include <rspq_profile.h>
#include "../misc/n64/gpu.c"

/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static surface_t zbuffer;
static GfxResourceID white_square;

void Gfx_Create(void) {
	rspq_init();
	//rspq_profile_start();
    rdpq_init();
	//rdpq_debug_start(); // TODO debug
	//rdpq_debug_log(true);

	rdpq_set_mode_standard();
	__rdpq_mode_change_som(SOM_TEXTURE_PERSP, SOM_TEXTURE_PERSP);
	__rdpq_mode_change_som(SOM_ZMODE_MASK,    SOM_ZMODE_OPAQUE);
	rdpq_mode_dithering(DITHER_SQUARE_SQUARE);

    gpu_init();

	// Set alpha compare threshold
	gpu_push_rdp(RDP_CMD_SYNC_PIPE, 0);
	gpu_push_rdp(RDP_CMD_SET_BLEND_COLOR, (0 << 24) | (0 << 16) | (0 << 8) | 127);

    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
	Gfx.Created      = true;
	
	// TMEM only has 4 KB in it, which can be interpreted as
	// - 1024 32bpp pixels
	// - 2048 16bpp pixels 
	Gfx.MaxTexSize       = 1024;
	Gfx.MaxLowResTexSize = 2048;

	Gfx.NonPowTwoTexturesSupport = GFX_NONPOW2_FULL;
	Gfx_RestoreState();

	Gfx_SetFaceCulling(false);
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
	gpu_close();
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static color_t gfx_clearColor;

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Nintendo 64 --\n");
	String_AppendConst(info, "GPU: Nintendo 64 RDP (LibDragon OpenGL)\n");
	String_AppendConst(info, "T&L: Nintendo 64 RSP (LibDragon OpenGL)\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync; // TODO update vsync
}

void Gfx_OnWindowResize(void) { }

void Gfx_SetViewport(int x, int y, int w, int h) {
	gpuViewport(x, y, w, h);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	rdpq_set_scissor(x, y, x + w, y + h);
}

void Gfx_BeginFrame(void) {
	surface_t* disp = display_get();
    rdpq_attach(disp, &zbuffer);
    
	//Platform_LogConst("== BEGIN frame");
}

extern void __rdpq_autosync_change(int mode);
static color_t gfx_clearColor;

void Gfx_ClearBuffers(GfxBuffers buffers) {
	__rdpq_autosync_change(AUTOSYNC_PIPE);
	
	if (buffers & GFX_BUFFER_COLOR)
		rdpq_clear(gfx_clearColor);
	if (buffers & GFX_BUFFER_DEPTH)
		rdpq_clear_z(0xFFFC);
} 

void Gfx_ClearColor(PackedCol color) {
	gfx_clearColor.r = PackedCol_R(color);
	gfx_clearColor.g = PackedCol_G(color);
	gfx_clearColor.b = PackedCol_B(color);
	gfx_clearColor.a = PackedCol_A(color);
	//memcpy(&gfx_clearColor, &color, sizeof(color_t)); // TODO maybe use proper conversion from graphics.h ??
}

void Gfx_EndFrame(void) {
    rdpq_detach_show();

	//rspq_profile_dump();
	//rspq_profile_next_frame();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture {
	surface_t surface;
	rspq_block_t* upload_block;
} CCTexture;

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = (CCTexture*)texId;

	rspq_block_run(tex->upload_block);
	gpuSetTexSize(tex->surface.width, tex->surface.height);
}

#define ALIGNUP8(size) (((size) + 7) & ~0x07)

// A8 B8 G8 R8 > A1 B5 G5 B5
#define To16BitPixel(src) \
	((src & 0x80) >> 7) | ((src & 0xF800) >> 10) | ((src & 0xF80000) >> 13) | ((src & 0xF8000000) >> 16);

static void UploadTexture(CCTexture* tex, rdpq_texparms_t* params) {
	rspq_block_begin();

	rdpq_tex_upload(TILE0, &tex->surface, params);

	tex->upload_block = rspq_block_end();
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	cc_bool bit16  = flags & TEXTURE_FLAG_LOWRES;
	// rows are actually 8 byte aligned in TMEM https://github.com/DragonMinded/libdragon/blob/f360fa1bb1fb3ff3d98f4ab58692d40c828636c9/src/rdpq/rdpq_tex.c#L132
	// so even though width * height * pixel size may fit within 4096 bytes, after adjusting for 8 byte alignment, row pitch * height may exceed 4096 bytes
	int pitch = bit16 ? ALIGNUP8(bmp->width * 2) : ALIGNUP8(bmp->width * 4);
	if (pitch * bmp->height > 4096) return 0;
	
	CCTexture* tex = Mem_Alloc(1, sizeof(CCTexture), "texture");
	tex->surface   = surface_alloc(bit16 ? FMT_RGBA16 : FMT_RGBA32, bmp->width, bmp->height);
	surface_t* fb  = &tex->surface;
		
	if (bit16) {
		cc_uint32* src = (cc_uint32*)bmp->scan0;
		cc_uint8*  dst = (cc_uint8*)fb->buffer;
		
		// 16 bpp requires reducing A8R8G8B8 to A1R5G5B5
		for (int y = 0; y < bmp->height; y++) 
		{	
			cc_uint32* src_row = src + y * rowWidth;
			cc_uint16* dst_row = (cc_uint16*)(dst + y * fb->stride);
			
			for (int x = 0; x < bmp->width; x++) 
			{
				dst_row[x] = To16BitPixel(src_row[x]);
			}
		}
	} else {
		// 32 bpp can just be copied straight across
		CopyPixels(fb->buffer, fb->stride, 
				   bmp->scan0, rowWidth * BITMAPCOLOR_SIZE,
				   bmp->width, bmp->height);
	}
	
	rdpq_texparms_t params =
	{
        .s.repeats = (flags & TEXTURE_FLAG_NONPOW2) ? 1 : REPEAT_INFINITE,
        .t.repeats = (flags & TEXTURE_FLAG_NONPOW2) ? 1 : REPEAT_INFINITE,
    };
	UploadTexture(tex, &params);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	surface_t* fb  = &tex->surface;

	cc_uint32* src = (cc_uint32*)part->scan0 + x;
	cc_uint8*  dst = (cc_uint8*)fb->buffer  + (x * 4) + (y * fb->stride);

	for (int srcY = 0; srcY < part->height; srcY++) 
	{
		Mem_Copy(dst + srcY * fb->stride,
				 src + srcY * rowWidth,
				 part->width * 4);
	}
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	CCTexture* tex = (CCTexture*)(*texId);
	if (!tex) return;
	
	if (tex->upload_block) rdpq_call_deferred((void (*)(void*))rspq_block_free, tex->upload_block);
	surface_free(&tex->surface);

	Mem_Free(tex);
	*texId = NULL;
}

static cc_bool bilinear_mode;
static void SetFilterMode(cc_bool bilinear) {
	if (bilinear_mode == bilinear) return;
	bilinear_mode = bilinear;

	uint64_t mode = bilinear ? FILTER_BILINEAR : FILTER_POINT;
	__rdpq_mode_change_som(SOM_SAMPLE_MASK, mode << SOM_SAMPLE_SHIFT);

	int offset = bilinear ? -((1 << TEX_SHIFT) / 2) : 0;
	gpuSetTexOffset(offset, offset);
}

void Gfx_EnableMipmaps(void) {
	// TODO move back to texture instead?
	SetFilterMode(Gfx.Mipmaps);
}

void Gfx_DisableMipmaps(void) {
	SetFilterMode(FILTER_POINT);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFaceCulling(cc_bool enabled) { 
	gpuSetCullFace(enabled);
}

static void SetAlphaBlend(cc_bool enabled) { 
	rdpq_mode_blender(enabled ? RDPQ_BLENDER_MULTIPLY : 0);
	__rdpq_mode_change_som(SOM_ZMODE_MASK, enabled ? SOM_ZMODE_TRANSPARENT : SOM_ZMODE_OPAQUE);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetAlphaTest(cc_bool enabled) {
	__rdpq_mode_change_som(SOM_ALPHACOMPARE_MASK, enabled ? SOM_ALPHACOMPARE_THRESHOLD : 0);
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	//gpuColorMask(r, g, b, a); TODO
}

#define FLAG_Z_WRITE 0x02
void Gfx_SetDepthWrite(cc_bool enabled) { 
	__rdpq_mode_change_som(SOM_Z_WRITE, enabled ? SOM_Z_WRITE : 0);

	gpu_attr_z &= ~FLAG_Z_WRITE;
	gpu_attr_z |= enabled ? FLAG_Z_WRITE : 0;
	gpuUpdateFormat();
}

#define FLAG_Z_READ 0x01
void Gfx_SetDepthTest(cc_bool enabled) { 
	__rdpq_mode_change_som(SOM_Z_COMPARE, enabled ? SOM_Z_COMPARE : 0);

	gpu_attr_z &= ~FLAG_Z_READ;
	gpu_attr_z |= enabled ? FLAG_Z_READ : 0;
	gpuUpdateFormat();
}

static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
	gfx_format = -1;
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
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
#define MAX_CACHED_CALLS 8

struct VertexBufferCache {
	rspq_block_t* blocks[MAX_CACHED_CALLS];
	int offset[MAX_CACHED_CALLS];
	int counts[MAX_CACHED_CALLS];
};
struct VertexBuffer {
	struct VertexBufferCache cache;
	char vertices[];
};

static struct VertexBuffer* gfx_vb;
static int vb_count, vb_fmt;

static void VB_ClearCache(struct VertexBuffer* vb) {
	for (int i = 0; i < MAX_CACHED_CALLS; i++)
	{
		rspq_block_t* block = vb->cache.blocks[i];
		if (!block) continue;

		// TODO is this really safe ??
		rdpq_call_deferred((void (*)(void*))rspq_block_free, block);
	}
	 Mem_Set(&vb->cache, 0, sizeof(vb->cache));
}

static rspq_block_t* VB_GetCached(struct VertexBuffer* vb, int offset, int count) {
	if (!count) return NULL;

	// Lookup previously cached draw commands
	for (int i = 0; i < MAX_CACHED_CALLS; i++)
	{
		if (offset == vb->cache.offset[i] && count == vb->cache.counts[i])
			return vb->cache.blocks[i];
	}

	// Add a new cached draw command
	for (int i = 0; i < MAX_CACHED_CALLS; i++)
	{
		if (vb->cache.blocks[i]) continue;

		rspq_block_begin();
		gpu_pointer = gfx_vb->vertices;
		gpuDrawArrays(offset, count);
		rspq_block_t* block = rspq_block_end();

		vb->cache.blocks[i] = block;
		vb->cache.offset[i] = offset;
		vb->cache.counts[i] = count;
		return block;
	}

	// .. nope, no room for it
	return NULL;
}


GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)    { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	struct VertexBuffer* vb = memalign(16, sizeof(struct VertexBufferCache) + count * strideSizes[fmt]);
	if (vb) Mem_Set(&vb->cache, 0, sizeof(vb->cache));
	return vb;
}

void Gfx_BindVb(GfxResourceID vb) { 
	gfx_vb = vb; 
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (!data) return;

	VB_ClearCache(data);
	rspq_wait(); // for deferred deletes TODO needed?
	Mem_Free(data);
	*vb = NULL;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_count = count;
	vb_fmt   = fmt;
	return ((struct VertexBuffer*)vb)->vertices;
}

void Gfx_UnlockVb(GfxResourceID vb) {
	VB_ClearCache(vb); // data may have changed

	void* ptr = ((struct VertexBuffer*)vb)->vertices;
	if (vb_fmt == VERTEX_FORMAT_COLOURED) {
		convert_coloured_vertices(ptr, vb_count);
		CPU_FlushDataCache(ptr, vb_count * sizeof(struct rsp_vertex));
	} else {
		convert_textured_vertices(ptr, vb_count);
		CPU_FlushDataCache(ptr, vb_count * sizeof(struct rsp_vertex));
	}
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool depthOnlyRendering;

void Gfx_SetFog(cc_bool enabled) {
}

void Gfx_SetFogCol(PackedCol color) {
}

void Gfx_SetFogDensity(float value) {
}

void Gfx_SetFogEnd(float value) {
}

void Gfx_SetFogMode(FogFunc func) {
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	depthOnlyRendering = depthOnly; // TODO: Better approach? maybe using glBlendFunc instead?
	cc_bool enabled    = !depthOnly;

	//SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
	//			  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
	gpu_attr_tex = enabled;
	gpuUpdateFormat();
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	struct Matrix mvp CC_ALIGNED(64);	
	Matrix_Mul(&mvp, &_view, &_proj);
	gpuLoadMatrix((const float*)&mvp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	_proj = *proj;
	_view = *view;

	Matrix_Mul(mvp, view, proj);
	gpuLoadMatrix((const float*)mvp);
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO
}

void Gfx_DisableTextureOffset(void) { }


/*########################################################################################################################*
*--------------------------------------------------------Rendering--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	gpu_stride = gfx_stride;

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		rdpq_mode_combiner(RDPQ_COMBINER_TEX_SHADE);
	} else {
		rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
	}

	gpu_texturing = fmt == VERTEX_FORMAT_TEXTURED;
	gpu_attr_tex = gpu_texturing;
	gpuUpdateFormat();
}

void Gfx_DrawVb_Lines(int verticesCount) {
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	rspq_block_t* block = VB_GetCached(gfx_vb, startVertex, verticesCount);

	if (block) {
		rspq_block_run(block);
	} else {
		gpu_pointer = gfx_vb->vertices;
		gpuDrawArrays(startVertex, verticesCount);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	rspq_block_t* block = VB_GetCached(gfx_vb, 0, verticesCount);

	if (block) {
		rspq_block_run(block);
	} else {
		gpu_pointer = gfx_vb->vertices;
		gpuDrawArrays(0, verticesCount);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (depthOnlyRendering) return;
	rspq_block_t* block = VB_GetCached(gfx_vb, startVertex, verticesCount);

	if (block) {
		rspq_block_run(block);
	} else {
		gpu_pointer = gfx_vb->vertices;
		gpuDrawArrays(startVertex, verticesCount);
	}
}
#endif
