#include "Core.h"

#if defined CC_BUILD_GCWII

#include "_GraphicsBase.h"

#include "Errors.h"

#include "Logger.h"

#include "Window.h"

#include <malloc.h>
#include <string.h>

#include <gccore.h>



static void* fifo_buffer;

#define FIFO_SIZE (256 * 1024)

extern void* Window_XFB;

static void* xfbs[2];

static  int curFB;





/*########################################################################################################################*

*---------------------------------------------------------General---------------------------------------------------------*

*#########################################################################################################################*/

static void InitGX(void) {

	GXRModeObj* mode = VIDEO_GetPreferredMode(NULL);

	fifo_buffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));

	memset(fifo_buffer, 0, FIFO_SIZE);



	GX_Init(fifo_buffer, FIFO_SIZE);

	GX_SetViewport(0, 0, mode->fbWidth, mode->efbHeight, 0, 1);

	GX_SetDispCopyYScale((f32)mode->xfbHeight / (f32)mode->efbHeight);

	GX_SetScissor(0, 0, mode->fbWidth, mode->efbHeight);

	GX_SetDispCopySrc(0, 0, mode->fbWidth, mode->efbHeight);

	GX_SetDispCopyDst(mode->fbWidth, mode->xfbHeight);

	GX_SetCopyFilter(mode->aa, mode->sample_pattern,

					 GX_TRUE, mode->vfilter);

	GX_SetFieldMode(mode->field_rendering,

					((mode->viHeight==2*mode->xfbHeight)?GX_ENABLE:GX_DISABLE));



	GX_SetCullMode(GX_CULL_NONE);

	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_InvVtxCache();

	

	xfbs[0] = Window_XFB;

	xfbs[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(mode));

}



static GfxResourceID white_square;

void Gfx_Create(void) {

	Gfx.MaxTexWidth  = 512;

	Gfx.MaxTexHeight = 512;

	Gfx.Created      = true;

	

	InitGX();

	InitDefaultResources();

	// INITDFAULTRESOURCES causes stack overflow due to gfx_indices

	//Thread_Sleep(20 * 1000);

}



void Gfx_Free(void) { 

	FreeDefaultResources(); 

}



cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_RestoreState(void) { }

void Gfx_FreeState(void) { }



/*########################################################################################################################*

*---------------------------------------------------------Textures--------------------------------------------------------*

*#########################################################################################################################*/

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {

	return 0;

}



void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {

}



void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {

}



void Gfx_DeleteTexture(GfxResourceID* texId) {

}



void Gfx_EnableMipmaps(void) { }

void Gfx_DisableMipmaps(void) { }



void Gfx_BindTexture(GfxResourceID texId) {

}





/*########################################################################################################################*

*-----------------------------------------------------State management----------------------------------------------------*

*#########################################################################################################################*/

static GXColor gfx_clearColor = { 0, 0, 0, 255 };

void Gfx_SetFaceCulling(cc_bool enabled)   { }

void Gfx_SetAlphaBlending(cc_bool enabled) { }

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }



void Gfx_ClearCol(PackedCol color) {

	gfx_clearColor.r = PackedCol_R(color);

	gfx_clearColor.g = PackedCol_G(color);

	gfx_clearColor.b = PackedCol_B(color);

	

	GX_SetCopyClear(gfx_clearColor, 0x00ffffff); // TODO: use GX_MAX_Z24 

}



void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {

}

static cc_bool depth_write = true, depth_test = true;
static void UpdateDepthState(void) {
	// match Desktop behaviour, where disabling depth testing also disables depth writing
	// TODO do we actually need to & here?
	GX_SetZMode(depth_test, GX_LEQUAL, depth_write & depth_test);
}


void Gfx_SetDepthWrite(cc_bool enabled) {
	depth_write = enabled;
	UpdateDepthState();

}


void Gfx_SetDepthTest(cc_bool enabled) {
	depth_test = enabled;
	UpdateDepthState();

}



/*########################################################################################################################*

*---------------------------------------------------------Matrices--------------------------------------------------------*

*#########################################################################################################################*/

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {

	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho

	//   The simplified calculation below uses: L = 0, R = width, T = 0, B = height

	// NOTE: Shared with OpenGL. might be wrong to do that though?

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



	// Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum

	// For a FOV based perspective matrix, left/right/top/bottom are calculated as:

	//   left = -c * aspect, right = c * aspect, bottom = -c, top = c

	// Calculations are simplified because of left/right and top/bottom symmetry

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



	String_Format1(info, "-- Using GC/WII (%i bit) --\n", &pointerSize);

	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);

}



void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {

	gfx_minFrameMs = minFrameMs;

	gfx_vsync      = vsync;

}



void Gfx_BeginFrame(void) {

}



void Gfx_Clear(void) {

}



void Gfx_EndFrame(void) {

	curFB ^= 1;

	GX_CopyDisp(xfbs[curFB], GX_TRUE);

	GX_DrawDone();

	

	VIDEO_SetNextFramebuffer(xfbs[curFB]);

	VIDEO_Flush();

	VIDEO_WaitVSync();

	if (gfx_minFrameMs) LimitFPS();

}



void Gfx_OnWindowResize(void) { }





static cc_uint8* gfx_vertices;

/* Current format and size of vertices */

static int gfx_stride, gfx_format = -1, gfx_fields;





/*########################################################################################################################*

*----------------------------------------------------------Buffers--------------------------------------------------------*

*#########################################################################################################################*/

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) { return 0; }

void Gfx_BindIb(GfxResourceID ib)    { }

void Gfx_DeleteIb(GfxResourceID* ib) { }

static int vb_size;





GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {

	void* data = memalign(16, count * strideSizes[fmt]);

	if (!data) Logger_Abort("Failed to allocate memory for GFX VB");

	return data;

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

}



void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {

	vb_size = count * strideSizes[fmt];

	return vb; 

}



void Gfx_UnlockDynamicVb(GfxResourceID vb) { 

	gfx_vertices = vb; 

	//sceKernelDcacheWritebackInvalidateRange(vb, vb_size);

}



void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {

	Mem_Copy(vb, vertices, vCount * gfx_stride);

	//sceKernelDcacheWritebackInvalidateRange(vertices, vCount * gfx_stride);

}





/*########################################################################################################################*

*-----------------------------------------------------State management----------------------------------------------------*

*#########################################################################################################################*/

static PackedCol gfx_fogColor;

static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;

static int gfx_fogMode  = -1;



void Gfx_SetFog(cc_bool enabled) {

	gfx_fogEnabled = enabled;

}



void Gfx_SetFogCol(PackedCol color) {

	if (color == gfx_fogColor) return;

	gfx_fogColor = color;

}



void Gfx_SetFogDensity(float value) {

}



void Gfx_SetFogEnd(float value) {

	if (value == gfx_fogEnd) return;

	gfx_fogEnd = value;

}



void Gfx_SetFogMode(FogFunc func) {

}



void Gfx_SetAlphaTest(cc_bool enabled) { 

}



void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	//GX_SetColorUpdate(!depthOnly);

}





/*########################################################################################################################*

*---------------------------------------------------------Matrices--------------------------------------------------------*

*#########################################################################################################################*/

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_PROJECTION) {
		GX_LoadProjectionMtx(matrix, GX_PERSPECTIVE);
	} else {
		GX_LoadPosMtxImm(matrix, GX_PNMTX0);
	}

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



void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	GX_ClearVtxDesc();
	if (fmt == VERTEX_FORMAT_TEXTURED) {
		GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,   0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST,   GX_F32,   0);
	} else {
		GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
		GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,   0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	}
	
	
	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

}



void Gfx_DrawVb_Lines(int verticesCount) {

}


// TODO totally wrong, since should be using indexed drawing...
static void Draw_ColouredTriangles(int verticesCount, int startVertex) {
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, verticesCount);
	for (int i = startVertex; i < startVertex + verticesCount; i++) 
	{
		struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + i;
		
		GX_Position3f32(v->X, v->Y, v->Z);
		GX_Color4u8(PackedCol_R(v->Col), PackedCol_G(v->Col), PackedCol_B(v->Col), PackedCol_A(v->Col));
	}
	GX_End();
}

static void Draw_TexturedTriangles(int verticesCount, int startVertex) {
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, verticesCount);
	for (int i = startVertex; i < startVertex + verticesCount; i++) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + i;
		
		GX_Position3f32(v->X, v->Y, v->Z);
		GX_Color4u8(PackedCol_R(v->Col), PackedCol_G(v->Col), PackedCol_B(v->Col), PackedCol_A(v->Col));
		GX_TexCoord2f32(v->U, v->V);
	}
	GX_End();
}



void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
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
	Draw_TexturedTriangles(verticesCount, startVertex);

}

#endif