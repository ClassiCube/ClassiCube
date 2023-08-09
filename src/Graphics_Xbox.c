#include "Core.h"
#if defined CC_BUILD_XBOX
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <pbkit/pbkit.h>

#define MAX_RAM_ADDR 0x03FFAFFF // TODO: builtin for ffs ??
#define MASK(mask, val) (((val) << (ffs(mask)-1)) & (mask))

#define VERTEX_ATTR_INDEX  0
#define COLOUR_ATTR_INDEX  3
#define TEXTURE_ATTR_INDEX 9

// Current format and size of vertices
static int gfx_stride, gfx_format = -1;

// https://github.com/XboxDev/nxdk/blob/e955705ad7e5f59cdad456d0bd6680083d03758f/lib/pbkit/pbkit.c#L3611
// funcs: never(0x200), less(0x201), equal(0x202), less or equal(0x203)
// greater(0x204), not equal(0x205), greater or equal(0x206), always(0x207)

static void ResetState(void) {
	uint32_t* p = pb_begin();

	p = pb_push1(p, NV097_SET_ALPHA_FUNC, 0x204); // GREATER
	p = pb_push1(p, NV097_SET_ALPHA_REF,  0x7F);
	p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR,  NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
	p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR,  NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA);
	p = pb_push1(p, NV097_SET_DEPTH_FUNC, 0x206); // GEQUAL
	
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                   MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_FIXED)
                 | MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
	pb_end(p);
}

void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512; // TODO: 1024?
	Gfx.Created      = true;

	pb_init();
	pb_show_front_screen();
	InitDefaultResources();
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
	int pointerSize = sizeof(void*) * 8;

	String_Format1(info, "-- Using Xbox (%i bit) --\n", &pointerSize);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) {
	pb_wait_for_vbl(); // TODO: at end??
	pb_reset();
	pb_target_back_buffer();
}

void Gfx_Clear(void) {
	int width  = WindowInfo.Width;
	int height = WindowInfo.Height;
	
	pb_erase_depth_stencil_buffer(0, 0, width, height);
	pb_fill(0, 0, width, height, clearColor);
}

void Gfx_EndFrame(void) {
	while (pb_busy())     { } // Wait for frame completion
	while (pb_finished()) { } // Swap when possible
	
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) { }


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
						(uint32_t)data & 0x03fffff);
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
static int matrix_modes[] = { NV097_SET_PROJECTION_MATRIX, NV097_SET_MODEL_VIEW_MATRIX };

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

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	/* Deliberately swap zNear/zFar in projection matrix calculation to produce */
	/*  a projection matrix that results in a reversed depth buffer */
	/* https://developer.nvidia.com/content/depth-precision-visualized */
	float zNear_ = zFar;
	float zFar_  = Reversed_CalcZNear(fov, 24);

	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = zFar_ / (zNear_ - zFar_);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = (zNear_ * zFar_) / (zNear_ - zFar_);
	matrix->row4.W =  0.0f;
}

static void LoadMatrix(int reg, const struct Matrix* matrix) {
	float* m    = (float*)matrix;
	uint32_t* p = pb_begin();
	pb_push(p, reg, 4*4); p++;
	
	for (int i = 0; i < 4 * 4; i++)
	{
		*(p++) = *(uint32_t*)&m[i];
	}
    pb_end(p);
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	LoadMatrix(matrix_modes[type], matrix);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {	
	LoadMatrix(matrix_modes[type], &Matrix_Identity);
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

void Gfx_DrawVb_Lines(int verticesCount) {
	/* TODO */
}

#define MAX_BATCH 120
static void DrawIndexedVertices(int verticesCount, int startVertex) {
	cc_uint16* indices = gfx_indices + ICOUNT(startVertex);
	int indicesCount   = ICOUNT(verticesCount);
	return;
	
    uint32_t* p = pb_begin();
    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_TRIANGLES);
    
    
    for (int i = 0; i < indicesCount; ) {
        p = pb_begin();
        

        int batch_count = min(indicesCount - i, MAX_BATCH);
        p = pb_push1(p, 0x40000000 | NV097_ARRAY_ELEMENT16, batch_count);
        Mem_Copy(p, indices, batch_count * sizeof(uint16_t));
        p += batch_count / 2; // push buffer is 32 bit elements

		pb_end(p);
        i += batch_count;
        indices += batch_count;
    }

    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    pb_end(p);
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
