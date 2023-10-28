#include "Core.h"
#ifdef CC_BUILD_N64
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <rspq_profile.h>
#include <GL/gl.h>


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void GL_UpdateVsync(void) {
	//GLContext_SetFpsLimit(gfx_vsync, gfx_minFrameMs);
}
static surface_t zbuffer;

void Gfx_Create(void) {
    rdpq_init(); // todo not needed
    gl_init();
    zbuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());

    rspq_profile_start();
    
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glEnd();
    
	Gfx.MaxTexWidth  = 32;
	Gfx.MaxTexHeight = 32;
	Gfx.Created      = true;
	/* necessary for android which "loses" context when window is closed */
	Gfx.LostContext  = false;

	Gfx_RestoreState();
	GL_UpdateVsync();
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
	gl_close();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
	if (Gfx.Created) GL_UpdateVsync();
}

void Gfx_BeginFrame(void) { 
	surface_t* disp = display_get();
    rdpq_attach(disp, &zbuffer);
	gl_context_begin();
}

void Gfx_Clear(void) { }

void Gfx_EndFrame(void) {
	gl_context_end();
    rdpq_detach_show();
    
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) {
	//  TODO don't call
	//glViewport(0, 0, Game.Width, Game.Height);
}

/* Current format and size of vertices */
static int gfx_stride;


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return 0;
}

void Gfx_BindTexture(GfxResourceID texId) {
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GLuint id = (GLuint)(*texId);
	if (!id) return;
	glDeleteTextures(1, &id);
	*texId = 0;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;
void Gfx_SetFaceCulling(cc_bool enabled)   {  }
void Gfx_SetAlphaBlending(cc_bool enabled) { }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void GL_ClearColor(PackedCol color) {
	glClearColor(PackedCol_R(color) / 255.0f, PackedCol_G(color) / 255.0f,
				 PackedCol_B(color) / 255.0f, PackedCol_A(color) / 255.0f);
}
void Gfx_ClearCol(PackedCol color) {
	if (color == gfx_clearColor) return;
	GL_ClearColor(color);
	gfx_clearColor = color;
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	//glColorMask(r, g, b, a); TODO
}

void Gfx_SetDepthWrite(cc_bool enabled) { }
void Gfx_SetDepthTest(cc_bool enabled) { }


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
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint16 __attribute__((aligned(16))) gfx_indices[GFX_MAX_INDICES];
static void* gfx_vertices;
static int vb_size;

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	fillFunc(gfx_indices, count, obj);
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)    { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	return Mem_Alloc(count, strideSizes[fmt], "GFX VB");
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
}


GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_Alloc(maxVertices, strideSizes[fmt], "GFX VB");
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	vb_size = count * strideSizes[fmt];
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gfx_vertices = vb;
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

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_SetAlphaTest(cc_bool enabled) {
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
}

void Gfx_EnableTextureOffset(float x, float y) {
}

void Gfx_DisableTextureOffset(void) { }


/*########################################################################################################################*
*-------------------------------------------------------State setup-------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
}

void Gfx_DrawVb_Lines(int verticesCount) {
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
}
#endif
