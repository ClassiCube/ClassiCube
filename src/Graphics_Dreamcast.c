#include "Core.h"
#if defined CC_BUILD_DREAMCAST
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include "GL/gl.h"
#include "GL/glkos.h"
#include <malloc.h>
#include <kos.h>
#include <dc/matrix.h>
#include <dc/pvr.h>

/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;
static cc_bool renderingDisabled;


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Create(void) {
	glKosInit();
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx.MaxTexWidth);
	Gfx.MaxTexHeight = Gfx.MaxTexWidth;
	Gfx.Created      = true;
	Gfx_RestoreState();
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;
void Gfx_SetFaceCulling(cc_bool enabled)   { gl_Toggle(GL_CULL_FACE); }
void Gfx_SetAlphaBlending(cc_bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearCol(PackedCol color) {
	if (color == gfx_clearColor) return;
	gfx_clearColor = color;
	
	float r = PackedCol_R(color) / 255.0f;
	float g = PackedCol_G(color) / 255.0f;
	float b = PackedCol_B(color) / 255.0f;
	pvr_set_bg_color(r, g, b);
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO: Doesn't work
}

void Gfx_SetDepthWrite(cc_bool enabled) { glDepthMask(enabled); }
void Gfx_SetDepthTest(cc_bool enabled) { gl_Toggle(GL_DEPTH_TEST); }

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_SetAlphaTest(cc_bool enabled) { gl_Toggle(GL_ALPHA_TEST); }

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	// don't need a fake second pass in this case
	renderingDisabled = depthOnly;
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z = -2.0f / (zFar - zNear);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = -(zFar + zNear) / (zFar - zNear);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.W =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;

	String_Format1(info, "-- Using OpenGL (%i bit) --\n", &pointerSize);
	String_Format1(info, "Vendor: %c\n",     glGetString(GL_VENDOR));
	String_Format1(info, "Renderer: %c\n",   glGetString(GL_RENDERER));
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) { }
void Gfx_Clear(void) {
	// no need to use glClear
}

void Gfx_EndFrame(void) {
	glKosSwapBuffers();
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) {
	glViewport(0, 0, Game.Width, Game.Height);
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static void* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return 1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	void* data = memalign(16, count * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
	//return Mem_Alloc(count, strideSizes[fmt], "gfx VB");
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


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	void* data = memalign(16, maxVertices * strideSizes[fmt]);
	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");
	return data;
	//return Mem_Alloc(maxVertices, strideSizes[fmt], "gfx VB");
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
	//dcache_flush_range(vb, vb_size);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gfx_vertices = vb;
	Mem_Copy(vb, vertices, vCount * gfx_stride);
	//dcache_flush_range(vertices, vCount * gfx_stride);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) {
	gldcBindTexture((GLuint)texId);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GLuint id = (GLuint)(*texId);
	if (!id) return;
	gldcDeleteTexture(id);
	*texId = 0;
}

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
	// Twiddled index looks like this (starting from lowest numbered bits):
	//   e.g. w > h: yx_yx_xx_xx
	//   e.g. h > w: yx_yx_yy_yy
	// And can therefore be broken down into two components:
	//  1) interleaved lower bits
	//  2) masked and then shifted higher bits
	
	int min_dimension    = Math.Min(w, h);
	
	int interleave_mask  = min_dimension - 1;
	int interleaved_bits = Math_Log2(min_dimension);
	
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
	interleaved_bits = Math_Log2(min_dimension); \
	shifted_mask     = 0xFFFFFFFFU & ~interleave_mask; \
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

static void ConvertTexture(cc_uint16* dst, struct Bitmap* bmp) {
	unsigned min_dimension;
	unsigned interleave_mask, interleaved_bits;
	unsigned shifted_mask, shift_bits;
	unsigned lo_Y, hi_Y, Y;
	unsigned lo_X, hi_X, X;	
	Twiddle_CalcFactors(bmp->width, bmp->height);
	
	cc_uint8* src = (cc_uint8*)bmp->scan0;	
	for (int y = 0; y < bmp->height; y++)
	{
		Twiddle_CalcY(y);
		for (int x = 0; x < bmp->width; x++, src += 4)
		{
			Twiddle_CalcX(x);
			dst[X | Y] = BGRA8_to_BGRA4(src);
		}
	}
}

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	GLuint texId = gldcGenTexture();
	gldcBindTexture(texId);
	if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}
	
	gldcAllocTexture(bmp->width, bmp->height, GL_RGBA,
				GL_UNSIGNED_SHORT_4_4_4_4_REV_TWID_KOS);
				
	void* pixels;
	GLsizei width, height;
	gldcGetTexture(&pixels, &width, &height);
	ConvertTexture(pixels, bmp);
	return texId;
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
	gldcBindTexture(texId);
				
	void* pixels;
	GLsizei width, height;
	gldcGetTexture(&pixels, &width, &height);
	
	ConvertSubTexture(pixels, width, height,
				x, y, part, rowWidth);
	// TODO: Do we need to flush VRAM?
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_fogColor;
static float gfx_fogEnd = 16.0f, gfx_fogDensity = 1.0f;
static FogFunc gfx_fogMode = -1;

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	if (enabled) { glEnable(GL_FOG); } else { glDisable(GL_FOG); }
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

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_PROJECTION) memcpy(&_proj, matrix, sizeof(struct Matrix));
	if (type == MATRIX_VIEW)       memcpy(&_view, matrix, sizeof(struct Matrix));
	
	mat_load( &_proj);
	mat_apply(&_view);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
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
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	gfx_format = -1;

	glAlphaFunc(GL_GREATER, 0.5f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
#define VB_PTR gfx_vertices

static void SetupVertices(int startVertex) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
		glVertexPointer(3, GL_FLOAT, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	} else {
		cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
		glVertexPointer(3, GL_FLOAT, SIZEOF_VERTEX_COLOURED, (void*)(VB_PTR + offset));
	}
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_TEXTURE_2D);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
	SetupVertices(0);
	//glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	SetupVertices(startVertex);
	glDrawArrays(GL_QUADS, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	SetupVertices(0);
	
	if (textureOffset) ShiftTextureCoords(verticesCount);
	glDrawArrays(GL_QUADS, 0, verticesCount);
	if (textureOffset) UnshiftTextureCoords(verticesCount);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (renderingDisabled) return;
	
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexPointer(3, GL_FLOAT, SIZEOF_VERTEX_TEXTURED, (void*)(VB_PTR + offset));
	glDrawArrays(GL_QUADS, 0, verticesCount);
}
#endif