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
#define OT_LENGTH 1024

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 32768

typedef struct {
	DISPENV disp_env;
	DRAWENV draw_env;

	cc_uint32 ot[OT_LENGTH];
	cc_uint8  buffer[BUFFER_LENGTH];
} RenderBuffer;

static RenderBuffer buffers[2];
static cc_uint8*    next_packet;
static int          active_buffer;
static RenderBuffer* buffer;

static void OnBufferUpdated(void) {
	buffer      = &buffers[active_buffer];
	next_packet = buffer->buffer;
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

static void FlipBuffers(void) {
	DrawSync(0);
	VSync(0);

	RenderBuffer* draw_buffer = &buffers[active_buffer];
	RenderBuffer* disp_buffer = &buffers[active_buffer ^ 1];

	PutDispEnv(&disp_buffer->disp_env);
	DrawOTagEnv(&draw_buffer->ot[OT_LENGTH - 1], &draw_buffer->draw_env);

	active_buffer ^= 1;
	OnBufferUpdated();
}

static void* new_primitive(int size) {
	RenderBuffer* buffer = &buffers[active_buffer];
	uint8_t* prim        = next_packet;

	next_packet += size;

	assert(next_packet <= &buffer->buffer[BUFFER_LENGTH]);
	return (void*)prim;
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
	Gfx.MaxTexHeight = 128;
	Gfx.Created      = true;
	
	Gfx_RestoreState();
	ResetGraph(0);

	SetupContexts(Window_Main.Width, Window_Main.Height, 63, 0, 127);
	SetDispMask(1);

	InitGeom();
	//gte_SetGeomOffset(Window_Main.Width / 2, Window_Main.Height / 2);
	// Set screen depth (basically FOV control, W/2 works best)
	//gte_SetGeomScreen(Window_Main.Width / 2);
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
//   22 texture packs are usable, and are then divided into
//     - 4 pages for 256 wide textures, 8 for 128 wide, 10 for 64
#define TPAGE_START_HOR 5
#define TPAGES_PER_HALF 16

#define TPAGE_WIDTH   64
#define TPAGE_HEIGHT 256
#define MAX_TEX_PAGES 22
static cc_uint8 vram_used[(MAX_TEX_PAGES * TPAGE_HEIGHT) / 8];

#define VRAM_SetUsed(line) (vram_used[(line) / 8] |=  (1 << ((line) % 8)))
#define VRAM_UnUsed(line)  (vram_used[(line) / 8] &= ~(1 << ((line) % 8)))
#define VRAM_IsUsed(line)  (vram_used[(line) / 8] &   (1 << ((line) % 8)))

static void VRAM_GetBlockRange(int width, int* beg, int* end) {
	if (width >= 256) {
		*beg =  0;
		*end =  4 * TPAGE_HEIGHT;
	} else if (width >= 128) {
		*beg =  4 * TPAGE_HEIGHT;
		*end = 12 * TPAGE_HEIGHT;
	} else {
		*beg = 12 * TPAGE_HEIGHT;
		*end = 22 * TPAGE_HEIGHT;
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
	VRAM_GetBlockRange(width, &beg, &end);
	
	// TODO kinda inefficient
	for (int i = beg; i < end - height; i++) 
	{
		if (VRAM_IsUsed(i)) continue;
		
		if (VRAM_IsRangeFree(i, i + height)) return i;
	}
	return -1;
}

#define TEXTURES_MAX_COUNT 64
typedef struct GPUTexture {
	cc_uint16 width, height;
	cc_uint16 line, tpage;
} GPUTexture;
static GPUTexture textures[TEXTURES_MAX_COUNT];
static GPUTexture* active_tex;

#define BGRA8_to_PS1(src) \
	((src[2] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[0] & 0xF8) << 7) | ((src[3] & 0x80) << 8)

static void* AllocTextureAt(int i, struct Bitmap* bmp) {
	cc_uint16* tmp = Mem_TryAlloc(bmp->width * bmp->height, 2);
	if (!tmp) return NULL;

	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint32* src = bmp->scan0 + y * bmp->width;
		cc_uint16* dst = tmp        + y * bmp->width;
		
		for (int x = 0; x < bmp->width; x++) {
			cc_uint8* color = (cc_uint8*)&src[x];
			dst[x] = BGRA8_to_PS1(color);
		}
	}

	GPUTexture* tex = &textures[i];
	int line = VRAM_FindFreeBlock(bmp->width, bmp->height);
	if (line == -1) { Mem_Free(tmp); return NULL; }
	
	tex->width  = bmp->width;
	tex->height = bmp->height;
	tex->line   = line;
	
	int page = TPAGE_START_HOR + (line / TPAGE_HEIGHT);
	// In bottom half of VRAM? Need to offset horizontally again
	if (page >= TPAGES_PER_HALF) page += TPAGE_START_HOR;

	int pageX = (page % TPAGES_PER_HALF);
	int pageY = (page / TPAGES_PER_HALF);

	for (int i = tex->line; i < tex->line + tex->height; i++) 
	{
		VRAM_SetUsed(i);
	}
	tex->tpage = (2 << 7) | (pageY << 4) | pageX;
	Platform_Log3("%i x %i  = %i", &bmp->width, &bmp->height, &line);
	Platform_Log3("  at %i (%i, %i)", &page, &pageX, &pageY);
		
	RECT rect;
	rect.x = pageX * TPAGE_WIDTH;
	rect.y = pageY * TPAGE_HEIGHT + (line % TPAGE_HEIGHT);
	rect.w = bmp->width;
	rect.h = bmp->height;

	int RX = rect.x, RY = rect.y;
	Platform_Log2("  LOAD AT: %i, %i", &RX, &RY);
	LoadImage2(&rect, tmp);
	
	Mem_Free(tmp);
	return tex;
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	for (int i = 0; i < TEXTURES_MAX_COUNT; i++)
	{
		if (textures[i].width) continue;
		return AllocTextureAt(i, bmp);
	}

	Platform_LogConst("No room for more textures");
	return NULL;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	active_tex = (GPUTexture*)texId;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (!data) return;
	GPUTexture* tex = (GPUTexture*)data;

	for (int i = tex->line; i < tex->line + tex->height; i++) 
	{
		VRAM_UnUsed(i);
	}
	tex->width = 0; tex->height = 0;
	*texId = NULL;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
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
	// TODO
}

void Gfx_SetAlphaTest(cc_bool enabled) {
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
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
static void* gfx_vertices;

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
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, mvp;
#define ToFixed(v) (int)(v * (1 << 12))

static void LoadTransformMatrix(struct Matrix* src) {
	// https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati
	MATRIX mtx;

	mtx.t[0] = 0;
	mtx.t[1] = 0;
	mtx.t[2] = 0;

	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row1.x, &src->row1.y, &src->row1.z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row2.x, &src->row2.y, &src->row2.z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row3.x, &src->row3.y, &src->row3.z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row4.x, &src->row4.y, &src->row4.z);
	//Platform_LogConst("====");

	float len1 = Math_SqrtF(src->row1.x + src->row1.y + src->row1.z);
	float len2 = Math_SqrtF(src->row2.x + src->row2.y + src->row2.z);
	float len3 = Math_SqrtF(src->row3.x + src->row3.y + src->row3.z);

	mtx.m[0][0] = ToFixed(1);
	mtx.m[0][1] = 0;
	mtx.m[0][2] = 0;

	mtx.m[1][0] = 0;
	mtx.m[1][1] = ToFixed(1);
	mtx.m[1][2] = 0;

	mtx.m[2][0] = 0;
	mtx.m[2][1] = ToFixed(1);
	mtx.m[2][2] = 1;
	
	gte_SetRotMatrix(&mtx);
	gte_SetTransMatrix(&mtx);
}

/*static void LoadTransformMatrix(struct Matrix* src) {
	// https://math.stackexchange.com/questions/237369/given-this-transformation-matrix-how-do-i-decompose-it-into-translation-rotati
	MATRIX mtx;

	mtx.t[0] = ToFixed(src->row4.x);
	mtx.t[1] = ToFixed(src->row4.y);
	mtx.t[2] = ToFixed(src->row4.z);

	Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row1.x, &src->row1.y, &src->row1.z);
	Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row2.x, &src->row2.y, &src->row2.z);
	Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row3.x, &src->row3.y, &src->row3.z);
	Platform_Log3("X: %f3, Y: %f3, Z: %f3", &src->row4.x, &src->row4.y, &src->row4.z);
	Platform_LogConst("====");

	float len1 = Math_SqrtF(src->row1.x + src->row1.y + src->row1.z);
	float len2 = Math_SqrtF(src->row2.x + src->row2.y + src->row2.z);
	float len3 = Math_SqrtF(src->row3.x + src->row3.y + src->row3.z);

	mtx.m[0][0] = ToFixed(src->row1.x / len1);
	mtx.m[0][1] = ToFixed(src->row1.y / len1);
	mtx.m[0][2] = ToFixed(src->row1.z / len1);

	mtx.m[1][0] = ToFixed(src->row2.x / len2);
	mtx.m[1][1] = ToFixed(src->row2.y / len2);
	mtx.m[1][2] = ToFixed(src->row2.z / len2);

	mtx.m[2][0] = ToFixed(src->row3.x / len3);
	mtx.m[2][1] = ToFixed(src->row3.y / len3);
	mtx.m[2][2] = ToFixed(src->row3.z / len3);
	
	gte_SetRotMatrix(&mtx);
	gte_SetTransMatrix(&mtx);
}*/

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&mvp, &_view, &_proj);
	LoadTransformMatrix(&mvp);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
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

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.01f;
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

static void Transform(Vec3* result, struct VertexTextured* a, const struct Matrix* mat) {
	/* a could be pointing to result - therefore can't directly assign X/Y/Z */
	float x = a->x * mat->row1.x + a->y * mat->row2.x + a->z * mat->row3.x + mat->row4.x;
	float y = a->x * mat->row1.y + a->y * mat->row2.y + a->z * mat->row3.y + mat->row4.y;
	float z = a->x * mat->row1.z + a->y * mat->row2.z + a->z * mat->row3.z + mat->row4.z;
	float w = a->x * mat->row1.w + a->y * mat->row2.w + a->z * mat->row3.w + mat->row4.w;
	
	result->x = (x/w) *  (320/2) + (320/2); 
	result->y = (y/w) * -(240/2) + (240/2);
	result->z = (z/w) * OT_LENGTH;
}

cc_bool VERTEX_LOGGING;
static void DrawColouredQuads(int verticesCount, int startVertex) {
	return;
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex + i;
		
		POLY_F4* poly = new_primitive(sizeof(POLY_F4));
		setPolyF4(poly);

		Vec3 coords[4];
		Transform(&coords[0], &v[0], &mvp);
		Transform(&coords[1], &v[1], &mvp);
		Transform(&coords[2], &v[2], &mvp);
		Transform(&coords[3], &v[3], &mvp);

		poly->x0 = coords[1].x; poly->y0 = coords[1].y;
		poly->x1 = coords[0].x; poly->y1 = coords[0].y;
		poly->x2 = coords[2].x; poly->y2 = coords[2].y;
		poly->x3 = coords[3].x; poly->y3 = coords[3].y;

		int p = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) / 4;
		if (p < 0 || p >= OT_LENGTH) continue;

		int X = v[0].x, Y = v[0].y, Z = v[0].z;
		//if (VERTEX_LOGGING) Platform_Log3("IN: %i, %i, %i", &X, &Y, &Z);
		X = poly->x1; Y = poly->y1, Z = coords[0].z;

		poly->r0 = PackedCol_R(v->Col);
		poly->g0 = PackedCol_G(v->Col);
		poly->b0 = PackedCol_B(v->Col);
		//if (VERTEX_LOGGING) Platform_Log4("OUT: %i, %i, %i (%i)", &X, &Y, &Z, &p);

		// TODO: 2D shouldn't use AddPrim, draws in the wrong way
		addPrim(&buffer->ot[p >> 2], poly);
	}
}

static void DrawTexturedQuads(int verticesCount, int startVertex) {
	int pageOffset = active_tex->line % TPAGE_HEIGHT;

	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;
		
		POLY_FT4* poly = new_primitive(sizeof(POLY_FT4));
		setPolyFT4(poly);
		poly->tpage = active_tex->tpage;
		poly->clut  = 0;

		Vec3 coords[4];
		Transform(&coords[0], &v[0], &mvp);
		Transform(&coords[1], &v[1], &mvp);
		Transform(&coords[2], &v[2], &mvp);
		Transform(&coords[3], &v[3], &mvp);

		// TODO & instead of % 
		poly->x0 = coords[1].x; poly->y0 = coords[1].y; poly->u0 = (int)(v[1].U * active_tex->width) % active_tex->width; poly->v0 = ((int)(v[1].V * active_tex->height) % active_tex->height) + pageOffset;
		poly->x1 = coords[0].x; poly->y1 = coords[0].y; poly->u1 = (int)(v[0].U * active_tex->width) % active_tex->width; poly->v1 = ((int)(v[0].V * active_tex->height) % active_tex->height) + pageOffset;
		poly->x2 = coords[2].x; poly->y2 = coords[2].y; poly->u2 = (int)(v[2].U * active_tex->width) % active_tex->width; poly->v2 = ((int)(v[2].V * active_tex->height) % active_tex->height) + pageOffset;
		poly->x3 = coords[3].x; poly->y3 = coords[3].y; poly->u3 = (int)(v[3].U * active_tex->width) % active_tex->width; poly->v3 = ((int)(v[3].V * active_tex->height) % active_tex->height) + pageOffset;

		//int P = active_tex->height, page = poly->tpage & 0xFF, ll = active_tex->line % TPAGE_HEIGHT;
		//Platform_Log4("XYZ: %f3 x %i, %i, %i", &v[0].V, &P, &page, &ll);
		int p = (coords[0].z + coords[1].z + coords[2].z + coords[3].z) / 4;
		if (p < 0 || p >= OT_LENGTH) continue;

		int X = v[0].x, Y = v[0].y, Z = v[0].z;
		//if (VERTEX_LOGGING) Platform_Log3("IN: %i, %i, %i", &X, &Y, &Z);
		X = poly->x1; Y = poly->y1, Z = coords[0].z;

		poly->r0 = PackedCol_R(v->Col);
		poly->g0 = PackedCol_G(v->Col);
		poly->b0 = PackedCol_B(v->Col);
		//if (VERTEX_LOGGING) Platform_Log4("OUT: %i, %i, %i (%i)", &X, &Y, &Z, &p);

		// TODO: 2D shouldn't use AddPrim, draws in the wrong way
		addPrim(&buffer->ot[p >> 2], poly);
	}
}

/*static void DrawQuads(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;
		
		POLY_F4* poly = new_primitive(sizeof(POLY_F4));
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

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		DrawTexturedQuads(verticesCount, startVertex);
	} else {
		DrawColouredQuads(verticesCount, startVertex);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		DrawTexturedQuads(verticesCount, 0);
	} else {
		DrawColouredQuads(verticesCount, 0);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawTexturedQuads(verticesCount, startVertex);
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}

void Gfx_BeginFrame(void) {
}

void Gfx_EndFrame(void) {
	FlipBuffers();
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS1 --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
