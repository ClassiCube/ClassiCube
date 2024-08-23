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
// Based off https://github.com/Lameguy64/PSn00bSDK/blob/master/examples/beginner/hello/main.c


// Length of the ordering table, i.e. the range Z coordinates can have, 0-15 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 512

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 32768

typedef struct {
	DISPENV disp_env;
	DRAWENV draw_env;

	uint32_t ot[OT_LENGTH];
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

	active_buffer = 0;
	OnBufferUpdated();
}

// NOINLINE to avoid polluting the hot path
static CC_NOINLINE new_primitive_nomem(void) {
	if (noMemWarned) return NULL;
	noMemWarned = true;
	
	Platform_LogConst("OUT OF VERTEX RAM");
	return NULL;
}

static void* new_primitive(int size) {
	uint8_t* prim  = next_packet;
	next_packet += size;

	if (next_packet <= next_packet_end);
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
	// Set screen depth (basically FOV control, W/2 works best)
	gte_SetGeomScreen(Window_Main.Width / 2);
}

void Gfx_Free(void) { 
	Gfx_FreeState();
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
	cc_uint8 width_shift, height_shift;
	cc_uint16 line, tpage;
	cc_uint8 xOffset, yOffset;
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
	
	tex->width  = bmp->width;  tex->width_shift  = Math_ilog2(bmp->width);
	tex->height = bmp->height; tex->height_shift = Math_ilog2(bmp->height);
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
		
	RECT rect;
	rect.x = pageX * TPAGE_WIDTH  + tex->xOffset;
	rect.y = pageY * TPAGE_HEIGHT + tex->yOffset;
	rect.w = bmp->width;
	rect.h = bmp->height;

	int RX = rect.x, RY = rect.y;
	Platform_Log2("  LOAD AT: %i, %i", &RX, &RY);
	LoadImage2(&rect, tmp);
	
	Mem_Free(tmp);
	return tex;
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
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

static void SetAlphaBlend(cc_bool enabled) {
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
// Preprocess vertex buffers into optimised layout for PS1
struct PS1VertexColoured { int x, y, z; PackedCol Col; };
struct PS1VertexTextured { int x, y, z; PackedCol Col; int u, v; };
static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

#define XYZInteger(value) ((value) >> 6)
#define XYZFixed(value) ((int)((value) * (1 << 6)))
#define UVFixed(value)  ((int)((value) * 1024.0f) & 0x3FF) // U/V wrapping not supported

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
		v = src->V == 1.0f ? 0.99f : src->V;

		dst->u = UVFixed(u);
		dst->v = UVFixed(v);
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

#define ToFixed(v) (int)(v * (1 << 12))

static void LoadTransformMatrix(struct Matrix* src) {
	// https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati
	MATRIX mtx;

	mtx.t[0] = ToFixed(src->row4.x);
	mtx.t[1] = ToFixed(src->row4.y);
	mtx.t[2] = ToFixed(src->row4.z);
	
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

	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row1.x, &src->row1.y, &src->row1.z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row2.x, &src->row2.y, &src->row2.z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row3.x, &src->row3.y, &src->row3.z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row4.x, &src->row4.y, &src->row4.z);
	//Platform_LogConst("====");

	mtx.m[0][0] = ToFixed(src->row1.x);
	mtx.m[0][1] = ToFixed(src->row1.y);
	mtx.m[0][2] = ToFixed(src->row1.z);

	mtx.m[1][0] = ToFixed(src->row2.x);
	mtx.m[1][1] = ToFixed(src->row2.y);
	mtx.m[1][2] = ToFixed(src->row2.z);

	mtx.m[2][0] = ToFixed(src->row3.x);
	mtx.m[2][1] = ToFixed(src->row3.y);
	mtx.m[2][2] = ToFixed(src->row3.z);
	
	gte_SetRotMatrix(&mtx);
	gte_SetTransMatrix(&mtx);
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
	float zNear = 0.05f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	float c = (float)Cotangent(0.5f * fov);
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

static int Transform(IVec3* result, struct PS1VertexTextured* a) {
	int x = a->x * mvp_row1.x + a->y * mvp_row2.x + a->z * mvp_row3.x + mvp_trans.x;
	int y = a->x * mvp_row1.y + a->y * mvp_row2.y + a->z * mvp_row3.y + mvp_trans.y;
	int z = a->x * mvp_row1.z + a->y * mvp_row2.z + a->z * mvp_row3.z + mvp_trans.z;
	int w = a->x * mvp_row1.w + a->y * mvp_row2.w + a->z * mvp_row3.w + mvp_trans.w;
	if (w <= 0) return 1;
	
	result->x = (x *  160      / w) + 160; 
	result->y = (y * -120      / w) + 120;
	result->z = (z * OT_LENGTH / w);
	return z > w;
}

static void DrawColouredQuads2D(int verticesCount, int startVertex) {
	return;
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexColoured* v = (struct PS1VertexColoured*)gfx_vertices + startVertex + i;
		
		POLY_F4* poly = new_primitive(sizeof(POLY_F4));
        if (!poly) return;
		setPolyF4(poly);

		poly->x0 = XYZInteger(v[1].x); poly->y0 = XYZInteger(v[1].y);
		poly->x1 = XYZInteger(v[0].x); poly->y1 = XYZInteger(v[0].y);
		poly->x2 = XYZInteger(v[2].x); poly->y2 = XYZInteger(v[2].y);
		poly->x3 = XYZInteger(v[3].x); poly->y3 = XYZInteger(v[3].y);

		poly->r0 = PackedCol_R(v->Col);
		poly->g0 = PackedCol_G(v->Col);
		poly->b0 = PackedCol_B(v->Col);

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
	int uShift  = 10 - curTex->width_shift;
	int vShift  = 10 - curTex->height_shift;

	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexTextured* v = (struct PS1VertexTextured*)gfx_vertices + startVertex + i;
		
		POLY_FT4* poly = new_primitive(sizeof(POLY_FT4));
        if (!poly) return;
		setPolyFT4(poly);
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

		// https://problemkaputt.de/psxspx-gpu-rendering-attributes.htm
		// "For untextured graphics, 8bit RGB values of FFh are brightest. However, for texture blending, 8bit values of 80h are brightest"
		poly->r0 = PackedCol_R(v->Col) >> 1;
		poly->g0 = PackedCol_G(v->Col) >> 1;
		poly->b0 = PackedCol_B(v->Col) >> 1;

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
		struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex + i;
		
		POLY_F4* poly = new_primitive(sizeof(POLY_F4));
        if (!poly) return;
		setPolyF4(poly);

		IVec3 coords[4];
		int clipped = 0;
		clipped |= Transform(&coords[0], &v[0]);
		clipped |= Transform(&coords[1], &v[1]);
		clipped |= Transform(&coords[2], &v[2]);
		clipped |= Transform(&coords[3], &v[3]);
		if (clipped) continue;

		poly->x0 = coords[1].x; poly->y0 = coords[1].y;
		poly->x1 = coords[0].x; poly->y1 = coords[0].y;
		poly->x2 = coords[2].x; poly->y2 = coords[2].y;
		poly->x3 = coords[3].x; poly->y3 = coords[3].y;

		int p = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) / 4;
		if (p < 0 || p >= OT_LENGTH) continue;

		poly->r0 = PackedCol_R(v->Col);
		poly->g0 = PackedCol_G(v->Col);
		poly->b0 = PackedCol_B(v->Col);

		addPrim(&buffer->ot[p >> 2], poly);
	}
}

static void DrawTexturedQuads3D(int verticesCount, int startVertex) {
	int uOffset = curTex->xOffset, vOffset = curTex->yOffset;
	int uShift  = 10 - curTex->width_shift;
	int vShift  = 10 - curTex->height_shift;

	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct PS1VertexTextured* v = (struct PS1VertexTextured*)gfx_vertices + startVertex + i;
		
		POLY_FT4* poly = new_primitive(sizeof(POLY_FT4));
        if (!poly) return;
		setPolyFT4(poly);
		poly->tpage = curTex->tpage;
		poly->clut  = 0;

		IVec3 coords[4];
		int clipped = 0;
		clipped |= Transform(&coords[0], &v[0]);
		clipped |= Transform(&coords[1], &v[1]);
		clipped |= Transform(&coords[2], &v[2]);
		clipped |= Transform(&coords[3], &v[3]);
		if (clipped) continue;

		// TODO & instead of % 
		poly->x0 = coords[1].x; poly->y0 = coords[1].y;
		poly->x1 = coords[0].x; poly->y1 = coords[0].y;
		poly->x2 = coords[2].x; poly->y2 = coords[2].y;
		poly->x3 = coords[3].x; poly->y3 = coords[3].y;
		
		if (cullingEnabled) {
			// https://gamedev.stackexchange.com/questions/203694/how-to-make-backface-culling-work-correctly-in-both-orthographic-and-perspective
			int signA = (coords[1].x - coords[0].x) * (coords[2].y - coords[0].y);
			int signB = (coords[2].x - coords[0].x) * (coords[1].y - coords[0].y);
			if (signA > signB) continue;
		}
		
		poly->u0 = (v[1].u >> uShift) + uOffset;
		poly->v0 = (v[1].v >> vShift) + vOffset;
		poly->u1 = (v[0].u >> uShift) + uOffset;
		poly->v1 = (v[0].v >> vShift) + vOffset;
		poly->u2 = (v[2].u >> uShift) + uOffset;
		poly->v2 = (v[2].v >> vShift) + vOffset;
		poly->u3 = (v[3].u >> uShift) + uOffset;
		poly->v3 = (v[3].v >> vShift) + vOffset;
		
		int p = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) / 4;
		if (p < 0 || p >= OT_LENGTH) continue;

		poly->r0 = PackedCol_R(v->Col) >> 1;
		poly->g0 = PackedCol_G(v->Col) >> 1;
		poly->b0 = PackedCol_B(v->Col) >> 1;

		addPrim(&buffer->ot[p >> 2], poly);
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


void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	DrawQuads(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawQuads(verticesCount, 0);
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

	active_buffer ^= 1;
	OnBufferUpdated();
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
