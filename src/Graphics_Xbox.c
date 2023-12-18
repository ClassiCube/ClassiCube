#include "Core.h"
#if defined CC_BUILD_XBOX
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <pbkit/pbkit.h>

#define MAX_RAM_ADDR 0x03FFAFFF
#define MASK(mask, val) (((val) << (__builtin_ffs(mask)-1)) & (mask))

// https://github.com/XboxDev/nxdk/blob/master/samples/triangle/main.c
// https://xboxdevwiki.net/NV2A/Vertex_Shader#Output_registers
#define VERTEX_ATTR_INDEX  0
#define COLOUR_ATTR_INDEX  3
#define TEXTURE_ATTR_INDEX 9

// A lot of figuring out which GPU registers to use came from:
// - comparing against pbgl and pbkit

static void LoadVertexShader(uint32_t* program, int programSize) {
	uint32_t* p;
	
	// Set cursor for program upload
	p = pb_begin();
	p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
	pb_end(p);

	// Copy program instructions (16 bytes each)
	for (int i = 0; i < programSize / 16; i++) 
	{
		p = pb_begin();
		pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
		Mem_Copy(p, &program[i * 4], 4 * 4);
		p += 4;
		pb_end(p);
	}
}

static uint32_t vs_coloured_program[] = {
	#include "../misc/xbox/vs_coloured.inl"
};
static uint32_t vs_textured_program[] = {
	#include "../misc/xbox/vs_textured.inl"
};


static void LoadFragmentShader_Coloured(void) {
	uint32_t* p;

	p = pb_begin();
	#include "../misc/xbox/ps_coloured.inl"
	pb_end(p);
}

static void LoadFragmentShader_Textured(void) {
	uint32_t* p;

	p = pb_begin();
	#include "../misc/xbox/ps_textured.inl"
	pb_end(p);
}


static void SetupShaders(void) {
	uint32_t *p;

	p = pb_begin();
	// Set run address of shader
	p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);

	// Set execution mode
	p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
					MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM)
					| MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

	p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);

	
	// resets "z perspective" flag
	//p = pb_push1(p, NV097_SET_CONTROL0, 0);
	pb_end(p);
}
 
static void ResetState(void) {
	uint32_t* p = pb_begin();

	p = pb_push1(p, NV097_SET_ALPHA_FUNC, 0x04); // GL_GREATER & 0x0F
	p = pb_push1(p, NV097_SET_ALPHA_REF,  0x7F);
	p = pb_push1(p, NV097_SET_DEPTH_FUNC, 0x03); // GL_LEQUAL & 0x0F
	
	p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
	p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
	p = pb_push1(p, NV097_SET_BLEND_EQUATION,     NV097_SET_BLEND_EQUATION_V_FUNC_ADD); // TODO not needed?
	
	p = pb_push1(p, NV097_SET_CULL_FACE, NV097_SET_CULL_FACE_V_FRONT);
	// the order ClassiCube specifies quad vertices in are in the wrong order
	//  compared to what the GPU expects for front and back facing quads
	
	/*pb_push(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16); p++;
	for (int i = 0; i < 16; i++) 
	{
		*(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
	}*/
	pb_end(p);
}

static GfxResourceID white_square;
void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512; // TODO: 1024?
	Gfx.Created      = true;

	InitDefaultResources();	
	pb_init();
	pb_show_front_screen();

	SetupShaders();
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	ResetState();
		
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_Free(void) { 
	FreeDefaultResources(); 
	pb_kill();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
void Gfx_RestoreState(void) { }
void Gfx_FreeState(void) { }


/*########################################################################################################################*
*---------------------------------------------------------Texturing-------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture_ {
	cc_uint32 width, height;
	cc_uint32 pitch, pad;
	cc_uint32 pixels[];
} CCTexture;

// See Graphics_Dreamcast.c for twiddling explanation
static unsigned Interleave(unsigned x) {
	// Simplified "Interleave bits by Binary Magic Numbers" from
	// http://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN

	x = (x | (x << 8)) & 0x00FF00FF;
	x = (x | (x << 4)) & 0x0F0F0F0F;
	x = (x | (x << 2)) & 0x33333333;
	x = (x | (x << 1)) & 0x55555555;
	return x;
}

#define Twiddle_CalcFactors(w, h) \
	min_dimension    = min(w, h); \
	interleave_mask  = min_dimension - 1; \
	interleaved_bits = Math_ilog2(min_dimension); \
	shifted_mask     = 0xFFFFFFFFU & ~interleave_mask; \
	shift_bits       = interleaved_bits;
	
#define Twiddle_CalcY(y) \
	lo_Y = Interleave(y & interleave_mask) << 1; \
	hi_Y = (y & shifted_mask) << shift_bits; \
	Y    = lo_Y | hi_Y;
	
#define Twiddle_CalcX(x) \
	lo_X  = Interleave(x & interleave_mask); \
	hi_X  = (x & shifted_mask) << shift_bits; \
	X     = lo_X | hi_X;

static void ConvertTexture(cc_uint32* dst, struct Bitmap* bmp) {
	unsigned min_dimension;
	unsigned interleave_mask, interleaved_bits;
	unsigned shifted_mask, shift_bits;
	unsigned lo_Y, hi_Y, Y;
	unsigned lo_X, hi_X, X;	
	Twiddle_CalcFactors(bmp->width, bmp->height);
	
	cc_uint32* src = bmp->scan0;	
	for (int y = 0; y < bmp->height; y++)
	{
		Twiddle_CalcY(y);
		for (int x = 0; x < bmp->width; x++, src++)
		{
			Twiddle_CalcX(x);
			dst[X | Y] = *src;
		}
	}
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	int size = 16 + bmp->width * bmp->height * 4;
	CCTexture* tex = MmAllocateContiguousMemoryEx(size, 0, MAX_RAM_ADDR, 16, PAGE_WRITECOMBINE | PAGE_READWRITE);
	
	tex->width  = bmp->width;
	tex->height = bmp->height;
	tex->pitch  = bmp->width * 4;
	ConvertTexture(tex->pixels, bmp);
	return tex;
}


void Gfx_UpdateTexture(GfxResourceID texId, int originX, int originY, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	cc_uint32* dst = tex->pixels;
	
	unsigned min_dimension;
	unsigned interleave_mask, interleaved_bits;
	unsigned shifted_mask, shift_bits;
	unsigned lo_Y, hi_Y, Y;
	unsigned lo_X, hi_X, X;
	Twiddle_CalcFactors(tex->width, tex->height);
	
	for (int y = 0; y < part->height; y++)
	{
		int dstY = y + originY;
		Twiddle_CalcY(dstY);
		cc_uint32* src = part->scan0 + y * rowWidth;
		
		for (int x = 0; x < part->width; x++)
		{
			int dstX = x + originX;
			Twiddle_CalcX(dstX);
			dst[X | Y] = *src++;
		}
	}
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	// TODO
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	CCTexture* tex = (CCTexture*)texId;
	if (!tex) tex  = white_square;
	
	unsigned log_u = Math_ilog2(tex->width);
	unsigned log_v = Math_ilog2(tex->height);
	uint32_t* p;

	p = pb_begin();
	// set texture stage 0 state
	p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, (DWORD)tex->pixels & 0x03ffffff);
	p = pb_push1(p, NV097_SET_TEXTURE_FORMAT,
					0xA |
					MASK(NV097_SET_TEXTURE_FORMAT_COLOR, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8) |
					MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2)  | // textures have U and V
					MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS,  1)  |
					MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, log_u) |
					MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, log_v) |
					MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 1)); // 1 slice
	p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0, 
					NV097_SET_TEXTURE_CONTROL0_ENABLE | NV097_SET_TEXTURE_CONTROL0_MAX_LOD_CLAMP);
	p = pb_push1(p, NV097_SET_TEXTURE_ADDRESS, 
					0x00010101); // modes (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp 4=border 5=clamp to edge)
	p = pb_push1(p, NV097_SET_TEXTURE_FILTER,
					0x2000 |
					MASK(NV097_SET_TEXTURE_FILTER_MIN, 1) |
					MASK(NV097_SET_TEXTURE_FILTER_MAG, 1)); // 1 = nearest filter
	
	// set texture matrix state
	p = pb_push1(p, NV097_SET_TEXTURE_MATRIX_ENABLE, 0);
	pb_end(p);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol clearColor;

void Gfx_ClearCol(PackedCol color) {
	clearColor = color;
}

void Gfx_SetFaceCulling(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, enabled);
	pb_end(p);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_SetAlphaBlending(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	p = pb_push1(p, NV097_SET_BLEND_ENABLE, enabled);
	pb_end(p);
}

void Gfx_SetAlphaTest(cc_bool enabled) { 	
	uint32_t* p = pb_begin();
	p = pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, enabled);
	pb_end(p);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	uint32_t* p = pb_begin();
	p = pb_push1(p, NV097_SET_DEPTH_MASK, enabled);
	pb_end(p);
}

void Gfx_SetDepthTest(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, enabled);
	pb_end(p);
}


void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned mask = 0;
	if (r) mask |= NV097_SET_COLOR_MASK_RED_WRITE_ENABLE;
	if (g) mask |= NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE;
	if (b) mask |= NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE;
	if (a) mask |= NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE;
	
	uint32_t* p = pb_begin();
	p = pb_push1(p, NV097_SET_COLOR_MASK, mask);
	pb_end(p);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using XBox --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) {
	pb_wait_for_vbl();
	pb_reset();
	pb_target_back_buffer();
}

void Gfx_Clear(void) {
	int width  = pb_back_buffer_width();
	int height = pb_back_buffer_height();
	
	// TODO do ourselves
	pb_erase_depth_stencil_buffer(0, 0, width, height);
	pb_fill(0, 0, width, height, clearColor);
	//pb_erase_text_screen();
	
	while (pb_busy()) { } // Wait for completion TODO: necessary??
}

static int frames;
void Gfx_EndFrame(void) {
	//pb_print("Frame #%d\n", frames++);
	//pb_draw_text_screen();

	while (pb_busy())     { } // Wait for frame completion
	while (pb_finished()) { } // Swap when possible
	
	if (gfx_minFrameMs) LimitFPS();
}


/*########################################################################################################################*
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint8* gfx_vertices;
static cc_uint16* gfx_indices;

static void* AllocBuffer(int count, int elemSize) {
	return MmAllocateContiguousMemoryEx(count * elemSize, 0, MAX_RAM_ADDR, 16, PAGE_WRITECOMBINE | PAGE_READWRITE);
}

static void FreeBuffer(GfxResourceID* buffer) {
	GfxResourceID ptr = *buffer;
	if (ptr) MmFreeContiguousMemory(ptr);
	*buffer = NULL;
}


GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	void* ib = AllocBuffer(count, sizeof(cc_uint16));
	if (!ib) Logger_Abort("Failed to allocate memory for index buffer");

	fillFunc(ib, count, obj);
	return ib;
}

void Gfx_BindIb(GfxResourceID ib)    { gfx_indices = ib; }

void Gfx_DeleteIb(GfxResourceID* ib) { FreeBuffer(ib); }


static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return AllocBuffer(count, strideSizes[fmt]);
}

static uint32_t* PushAttribOffset(uint32_t* p, int index, cc_uint8* data) {
	return pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index * 4,
						(uint32_t)data & 0x03ffffff);
}

void Gfx_BindVb(GfxResourceID vb) { 
	gfx_vertices = vb;
	uint32_t* p  = pb_begin();
	
	// TODO: Avoid the same code twice..
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		p = PushAttribOffset(p, VERTEX_ATTR_INDEX,  gfx_vertices +  0);
		p = PushAttribOffset(p, COLOUR_ATTR_INDEX,  gfx_vertices + 12);
		p = PushAttribOffset(p, TEXTURE_ATTR_INDEX, gfx_vertices + 16);
	} else {
		p = PushAttribOffset(p, VERTEX_ATTR_INDEX,  gfx_vertices +  0);
		p = PushAttribOffset(p, COLOUR_ATTR_INDEX,  gfx_vertices + 12);
	}
	pb_end(p);
}

void Gfx_DeleteVb(GfxResourceID* vb) { FreeBuffer(vb); }

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) { return vb; }

void Gfx_UnlockVb(GfxResourceID vb) { }


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return AllocBuffer(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) { 
	return vb;
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
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


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/

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

// https://github.com/XboxDev/nxdk/blob/master/samples/mesh/math3d.c#L292
static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
	// TODO: The above matrix breaks the held block
	// Below works but breaks map rendering
	
/*
	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -zFar / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear * zFar) / (zFar - zNear);
	matrix->row4.w =  0.0f;*/
}

void Gfx_OnWindowResize(void) { }

static struct Matrix _view, _proj, _mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	struct Matrix* dst = type == MATRIX_PROJECTION ? &_proj : &_view;
	*dst = *matrix;
	
	struct Matrix combined;
	Matrix_Mul(&combined, &_view, &_proj);

	uint32_t* p;
	p = pb_begin();
	
	// resets "z perspective" flag
	p = pb_push1(p, NV097_SET_CONTROL0, 0);

	// set shader constants cursor to C0
	p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96);

	// upload transformation matrix
	pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, 4*4 + 4);
	Mem_Copy(p, &combined, 16 * 4); p += 16;
	// Upload viewport too
	struct Vec4 viewport = { 320, 240, 8388608, 1 };
	Mem_Copy(p, &viewport, 4 * 4); p += 4;
	// Upload constants too
	//struct Vec4 v = { 1, 1, 1, 1 };
	//Mem_Copy(p, &v, 4 * 4); p += 4;
	// if necessary, look at vs.inl output for 'c[5]' etc..

	pb_end(p);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {	
	Gfx_LoadMatrix(type, &Matrix_Identity);
}

void Gfx_EnableTextureOffset(float x, float y) {
}

void Gfx_DisableTextureOffset(void) {
}



/*########################################################################################################################*
*---------------------------------------------------------Drawing---------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Gfx_WarnIfNecessary(void) { return false; }

static uint32_t* PushAttrib(uint32_t* p, int index, int format, int size, int stride) {
	return pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
						MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
						MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size)   |
						MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	uint32_t* p = pb_begin();
	// Clear all attributes TODO optimise
	pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);
	for (int i = 0; i < 16; i++) 
	{
		*(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
	}
		
	// resets "z perspective" flag
	//p = pb_push1(p, NV097_SET_CONTROL0, 0);

	// TODO cache these..
	if (fmt == VERTEX_FORMAT_TEXTURED) {
		p = PushAttrib(p, VERTEX_ATTR_INDEX,  NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
						3, SIZEOF_VERTEX_TEXTURED);
		p = PushAttrib(p, COLOUR_ATTR_INDEX,  NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D,
						4, SIZEOF_VERTEX_TEXTURED);
		p = PushAttrib(p, TEXTURE_ATTR_INDEX, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
						2, SIZEOF_VERTEX_TEXTURED);
	} else {
		p = PushAttrib(p, VERTEX_ATTR_INDEX, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 
						3, SIZEOF_VERTEX_COLOURED);
		p = PushAttrib(p, COLOUR_ATTR_INDEX, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_D3D, 
						4, SIZEOF_VERTEX_COLOURED);
	}
	pb_end(p);
	
	if (fmt == VERTEX_FORMAT_TEXTURED) {
		LoadVertexShader(vs_textured_program, sizeof(vs_textured_program));
		LoadFragmentShader_Textured();
	} else {		
		LoadVertexShader(vs_coloured_program, sizeof(vs_coloured_program));
		LoadFragmentShader_Coloured();
	}
}

static void DrawArrays(int mode, int start, int count) {
	uint32_t *p = pb_begin();
	p = pb_push1(p, NV097_SET_BEGIN_END, mode);

	// NV097_DRAW_ARRAYS_COUNT is an 8 bit mask, so must be < 256
	while (count > 0)
	{
		int batch_count = min(count, 64); // TODO increase?
		
		p = pb_push1(p, 0x40000000 | NV097_DRAW_ARRAYS,
						MASK(NV097_DRAW_ARRAYS_COUNT, (batch_count-1)) | 
						MASK(NV097_DRAW_ARRAYS_START_INDEX, start));
		
		start += batch_count;				
		count -= batch_count;
	}

	p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
	pb_end(p);
}

void Gfx_DrawVb_Lines(int verticesCount) {
	DrawArrays(NV097_SET_BEGIN_END_OP_LINES, 0, verticesCount);
}

#define MAX_BATCH 120
static void DrawIndexedVertices(int verticesCount, int startVertex) {
	// TODO switch to indexed rendering
	DrawArrays(NV097_SET_BEGIN_END_OP_QUADS, startVertex, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	DrawIndexedVertices(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawIndexedVertices(verticesCount, 0);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawIndexedVertices(verticesCount, startVertex);
}
#endif
