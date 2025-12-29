#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Logger.h"
#include "../Window.h"

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
static unsigned int CC_ALIGNED(16) list[262144];

static cc_uint8* gfx_vertices;
static int gfx_fields;


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static int formatFields[] = {
	GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
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
	
	sceGuDepthRange(65535,0);
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuDisable(GU_TEXTURE_2D);
	
	sceGuAlphaFunc(GU_GREATER, 0x7f, 0xff);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuDepthFunc(GU_LEQUAL); // sceGuDepthFunc(GU_GEQUAL);
	sceGuClearDepth(65535); // sceGuClearDepth(0);
	sceGuDepthRange(0, 65535); // sceGuDepthRange(65535, 0);
	
	GE_upload_world_matrix((const float*)&Matrix_Identity);
	sceGuColor(0xffffffff);
	
	sceGuEnable(GU_CLIP_PLANES); // TODO: swap near/far instead of this?
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);

	Gfx_OnWindowResize();
	
	sceGuFinish();
	sceGeDrawSync(GU_SYNC_WAIT); // waits until FINISH command is reached
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

static GfxResourceID white_square;
void Gfx_Create(void) {
	if (!Gfx.Created) guInit();
	
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
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

#define GU_Toggle(cap) if (enabled) { sceGuEnable(cap); } else { sceGuDisable(cap); }



/*########################################################################################################################*
*---------------------------------------------------------Swizzling-------------------------------------------------------*
*#########################################################################################################################*/
// PSP swizzled textures are 16 bytes X 8 rows = 4 '32 bit' pixels X 8 rows
static CC_INLINE void TwiddleCalcFactors(unsigned w, unsigned h, unsigned* maskX, unsigned* maskY) {
	*maskX = 0b00011; // 2 linear X bits
	*maskY = 0b11100; // 3 linear Y bits

	// Adjust for lower 2 X and 2 Y linear bits
	w >>= 3;
	h >>= 4;
	int shift = 5;

	for (; w > 0; w >>= 1) {
		*maskX += 0x01 << shift;
		shift  += 1;
	}

	for (; h > 0; h >>= 1) {
		*maskY += 0x01 << shift;
		shift  += 1;
	}
}

static CC_INLINE void UploadFullTexture(struct Bitmap* bmp, int rowWidth, cc_uint32* dst, int dst_w, int dst_h) {
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
	cc_uint32 pad1, pad2; // data must be aligned to 16 bytes
	BitmapCol pixels[];
} CCTexture;

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int dst_w = Math_NextPowOf2(bmp->width);
	int dst_h = Math_NextPowOf2(bmp->height);

	// Swizzled texture assumes at least 16 bytes x 8 rows
	int size = max(16 * 8, dst_w * dst_h * 4);
	CCTexture* tex = (CCTexture*)memalign(16, 16 + size);
	
	tex->width  = dst_w;
	tex->height = dst_h;

	UploadFullTexture(bmp, rowWidth, tex->pixels, dst_w, dst_h);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	UploadPartialTexture(part, rowWidth, tex->pixels, tex->width, tex->height, x, y);

	// TODO: Do line by line and only invalidate the actually changed parts of lines?
	// TODO: Invalidate full tex->size in case of very small textures?
	sceKernelDcacheWritebackInvalidateRange(tex->pixels, (tex->width * tex->height) * 4);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	CCTexture* tex = (CCTexture*)texId;
	if (!tex) tex  = white_square; 
	
	sceGuTexMode(GU_PSM_8888, 0, 0, 1);
	sceGuTexImage(0, tex->width, tex->height, tex->width, tex->pixels);
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
	String_AppendConst(info, "-- Using PSP--\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_BeginFrame(void) {
	sceGuStart(GU_DIRECT, list);
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
}

void Gfx_OnWindowResize(void) {
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor( 0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	// PSP X/Y guard band ranges from 0..4096
	// To minimise need to clip, centre the viewport around (2048, 2048)
	sceGuOffset(2048 - (w / 2), 2048 - (h / 2));
	sceGuViewport(2048 + x, 2048 + y, w, h);
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
	gfx_vertices = vb; 
	sceKernelDcacheWritebackInvalidateRange(vb, vb_size);
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
	sceKernelDcacheWritebackInvalidateRange(vb, vb_size);
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


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
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	const float* m = (const float*)matrix;

	if (type == MATRIX_PROJ) {
		GE_upload_proj_matrix(m);
	} else {
		GE_upload_view_matrix(m);
	}
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
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

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	GE_set_vertices(gfx_vertices + startVertex * gfx_stride);
	GE_set_indices(gfx_indices);

	sceGuDrawArray(GU_TRIANGLES, 0, ICOUNT(verticesCount), 
			NULL, NULL);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	GE_set_vertices(gfx_vertices);
	GE_set_indices(gfx_indices);

	sceGuDrawArray(GU_TRIANGLES, 0, ICOUNT(verticesCount),
			NULL, NULL);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	GE_set_vertices(gfx_vertices + startVertex * SIZEOF_VERTEX_TEXTURED);
	GE_set_indices(gfx_indices);

	sceGuDrawArray(GU_TRIANGLES, 0, ICOUNT(verticesCount), 
			NULL, NULL);
}
