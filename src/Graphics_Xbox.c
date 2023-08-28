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

// Current format and size of vertices
static int gfx_stride, gfx_format = -1;

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

static void LoadFragmentShader(void) {
	uint32_t* p;

	p = pb_begin();
	#include "../misc/xbox/ps_colored.inl"
	pb_end(p);
}

static uint32_t vs_program[] = {
	#include "../misc/xbox/vs_colored.inl"
};

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

	
	uint32_t control0 = 0;
	#define NV097_SET_CONTROL0_TEXTUREPERSPECTIVE 0x100000
	control0 |= NV097_SET_CONTROL0_TEXTUREPERSPECTIVE;
	control0 |= NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE;
	p = pb_push1(p, NV097_SET_CONTROL0, control0);
	pb_end(p);
}
 
static void ResetState(void) {
	uint32_t* p = pb_begin();

	p = pb_push1(p, NV097_SET_ALPHA_FUNC, 0x204); // GREATER
	p = pb_push1(p, NV097_SET_ALPHA_REF,  0x7F);
	p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
	p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA);
	p = pb_push1(p, NV097_SET_DEPTH_FUNC, 0x203); // LEQUAL
	
	/*pb_push(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16); p++;
	for (int i = 0; i < 16; i++) 
	{
		*(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
	}*/
	pb_end(p);
}


void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512; // TODO: 1024?
	Gfx.Created      = true;

	InitDefaultResources();	
	pb_init();
	pb_show_front_screen();

	SetupShaders();
	LoadVertexShader(vs_program, sizeof(vs_program));
	LoadFragmentShader();
	ResetState();
}

void Gfx_Free(void) { 
	FreeDefaultResources(); 
	pb_kill();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
void Gfx_RestoreState(void) { }
void Gfx_FreeState(void) { }

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	// TODO
	return 1;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	// TODO
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	// TODO
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	// TODO
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
	pb_push1(p, NV097_SET_CULL_FACE, enabled);
	pb_end(p);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_SetAlphaBlending(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	pb_push1(p, NV097_SET_BLEND_ENABLE, enabled);
	pb_end(p);
}

void Gfx_SetAlphaTest(cc_bool enabled) { 	
	uint32_t* p = pb_begin();
	pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, enabled);
	pb_end(p);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	uint32_t* p = pb_begin();
	pb_push1(p, NV097_SET_DEPTH_MASK, enabled);
	pb_end(p);
}

void Gfx_SetDepthTest(cc_bool enabled) { 
	uint32_t* p = pb_begin();
	pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, enabled);
	pb_end(p);
}


void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	// TODO
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
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
	
	pb_erase_depth_stencil_buffer(0, 0, width, height);
	pb_fill(0, 0, width, height, clearColor);
	//pb_erase_text_screen();
	
	while (pb_busy()) { } // Wait for completion TODO: necessary 
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
	void* ptr = MmAllocateContiguousMemoryEx(count * elemSize, 0, MAX_RAM_ADDR, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);
	
	if (!ptr) Logger_Abort("Failed to allocate memory for buffer");
	return ptr;
}

static void FreeBuffer(GfxResourceID* buffer) {
	GfxResourceID ptr = *buffer;
	if (ptr) MmFreeContiguousMemory(ptr);
	*buffer = NULL;
}


GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	void* ib = AllocBuffer(count, sizeof(cc_uint16));
	fillFunc(ib, count, obj);
	return ib;
}

void Gfx_BindIb(GfxResourceID ib)    { gfx_indices = ib; }

void Gfx_DeleteIb(GfxResourceID* ib) { FreeBuffer(ib); }


GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) { 
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


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices)  {
	return AllocBuffer(maxVertices, strideSizes[fmt]);
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) { 
	return vb;
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { }

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	Mem_Copy(vb, vertices, vCount * gfx_stride);
}


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

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z =  1.0f / (zNear - zFar);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = zNear / (zNear - zFar);
}

// https://github.com/XboxDev/nxdk/blob/master/samples/mesh/math3d.c#L292
static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = -zFar / (zFar - zNear);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = (zNear * zFar) / (zFar - zNear);
	matrix->row4.W =  0.0f;
}

void Gfx_OnWindowResize(void) { }

static struct Matrix _view, _proj, _mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	struct Matrix* dst = type == MATRIX_PROJECTION ? &_proj : &_view;
	*dst = *matrix;
	
	struct Matrix combined;
	Matrix_Mul(&combined, &_proj, &_view);
	//Matrix_Mul(&combined, &viewport, &combined);
	
	//Platform_LogConst("--transform--");
	//Platform_Log4("[%f3, %f3, %f3, %f3]", &combined.row1.X, &combined.row1.Y, &combined.row1.Z, &combined.row1.W);
	//Platform_Log4("[%f3, %f3, %f3, %f3]", &combined.row2.X, &combined.row2.Y, &combined.row2.Z, &combined.row2.W);
	//Platform_Log4("[%f3, %f3, %f3, %f3]", &combined.row3.X, &combined.row3.Y, &combined.row3.Z, &combined.row3.W);
	//Platform_Log4("[%f3, %f3, %f3, %f3]", &combined.row4.X, &combined.row4.Y, &combined.row4.Z, &combined.row4.W);
	uint32_t* p;
	p = pb_begin();
	
		
	uint32_t control0 = 0;
	#define NV097_SET_CONTROL0_TEXTUREPERSPECTIVE 0x100000
	control0 |= NV097_SET_CONTROL0_TEXTUREPERSPECTIVE;
	control0 |= NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE;
	p = pb_push1(p, NV097_SET_CONTROL0, control0);

	// set shader constants cursor to C0
	p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96);

	// upload transformation matrix
	pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, 4*4 + 4);
	Mem_Copy(p, &combined, 16 * 4); p += 16;
	// Upload viewport too
	struct Vec4 viewport = { 320, 240, 8388608, 1 };
	Mem_Copy(p, &viewport, 4 * 4); p += 4;

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
		
	uint32_t control0 = 0;
	#define NV097_SET_CONTROL0_TEXTUREPERSPECTIVE 0x100000
	control0 |= NV097_SET_CONTROL0_TEXTUREPERSPECTIVE;
	control0 |= NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE;
	p = pb_push1(p, NV097_SET_CONTROL0, control0);

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
}

static void DrawArrays(int mode, int start, int count) {
	uint32_t *p = pb_begin();
	p = pb_push1(p, NV097_SET_BEGIN_END, mode);

	p = pb_push1(p, 0x40000000 | NV097_DRAW_ARRAYS,
					MASK(NV097_DRAW_ARRAYS_COUNT, (count-1)) | 
					MASK(NV097_DRAW_ARRAYS_START_INDEX, start));

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
