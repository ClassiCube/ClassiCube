#include "Core.h"
#if defined CC_BUILD_PSP
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>

#define BUFFER_WIDTH  512
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

#define FB_SIZE (BUFFER_WIDTH * SCREEN_HEIGHT * 4)
static unsigned int __attribute__((aligned(16))) list[262144];


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint16 __attribute__((aligned(16))) gfx_indices[GFX_MAX_INDICES];
static int formatFields[] = {
	GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
	GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D
};
static ScePspFMatrix4 identity;

static void guInit(void) {
	void* framebuffer0 = (void*)0;
	void* framebuffer1 = (void*)FB_SIZE;
	void* depthbuffer  = (void*)(FB_SIZE + FB_SIZE);
	gumLoadIdentity(&identity);
	
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, framebuffer0, BUFFER_WIDTH);
	sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, framebuffer1, BUFFER_WIDTH);
	sceGuDepthBuffer(depthbuffer, BUFFER_WIDTH);
	
	sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuDisable(GU_TEXTURE_2D);
	
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuClearDepth(0);
	sceGuDepthRange(65535,0);
	
	
	sceGuSetMatrix(GU_MODEL, &identity);
	sceGuColor(0xffffffff);
	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuDisable(GU_SCISSOR_TEST);
	
	
    //sceGuEnable(GU_CLIP_PLANES);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

static void GLBackend_Init(void);
void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.Created      = true;
	MakeIndices(gfx_indices, GFX_MAX_INDICES);
	guInit();
	InitDefaultResources();
}

cc_bool Gfx_TryRestoreContext(void) { return true; }

void Gfx_Free(void) { FreeDefaultResources(); }
void Gfx_RestoreState(void) { }
void Gfx_FreeState(void) { }
#define GU_Toggle(cap) if (enabled) { sceGuEnable(cap); } else { sceGuDisable(cap); }

/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	cc_uint8* data = memalign(16, 16 + size);
	
	((cc_uint32*)data)[0] = bmp->width;
	((cc_uint32*)data)[1] = bmp->height;
	Mem_Copy(data + 16, bmp->scan0, size);
	return data;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }

void Gfx_BindTexture(GfxResourceID texId) {
	cc_uint8* data = (cc_uint8*)texId;
	int width = ((cc_uint32*)data)[0];
	int height = ((cc_uint32*)data)[1];
	sceGuTexMode(GU_PSM_8888,0,0,0);
	sceGuTexImage(0, width, height, width, data + 16);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;
void Gfx_SetFaceCulling(cc_bool enabled)   { GU_Toggle(GU_CULL_FACE); }
void Gfx_SetAlphaBlending(cc_bool enabled) { GU_Toggle(GU_BLEND); }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearCol(PackedCol color) {
	if (color == gfx_clearColor) return;
	sceGuClearColor(color);
	gfx_clearColor = color;
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned int mask = 0xffffffff;
	if (r) mask &= 0xffffff00;
	if (g) mask &= 0xffff00ff;
	if (b) mask &= 0xff00ffff;
	if (a) mask &= 0x00ffffff;
	
	//sceGuPixelMask(mask); TODO implement
}

void Gfx_SetDepthWrite(cc_bool enabled) { sceGuDepthMask(enabled); }
void Gfx_SetDepthTest(cc_bool enabled)  { GU_Toggle(GU_DEPTH_TEST); }
//void Gfx_SetDepthTest(cc_bool enabled)  { sceGuDisable(GU_DEPTH_TEST); }

/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	//*matrix = Matrix_Identity;
	//gumOrtho(matrix, 0.0f, width, height, 0.0f, ORTHO_NEAR, ORTHO_FAR);
	Matrix_Orthographic(matrix, 0.0f, width, 0.0f, height, ORTHO_NEAR, ORTHO_FAR);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zFar, struct Matrix* matrix) {
	float zNear = 0.1f;
	*matrix = Matrix_Identity;
	gumPerspective(matrix, 70.0f, aspect, 0.1f, zFar);
	//Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;

	String_Format1(info, "-- Using PSP (%i bit) --\n", &pointerSize);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) {
	sceGuStart(GU_DIRECT, list);
}
void Gfx_Clear(void) { sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT); }

void Gfx_EndFrame(void) {
	sceGuFinish();
	sceGuSync(0, 0);

	if (gfx_vsync) sceDisplayWaitVblankStart();
	sceGuSwapBuffers();
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
	sceKernelDcacheWritebackInvalidateRange(vb, vb_size);
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
	sceKernelDcacheWritebackInvalidateRange(vb, vb_size);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	Mem_Copy(vb, vertices, vCount * gfx_stride);
	sceKernelDcacheWritebackInvalidateRange(vertices, vCount * gfx_stride);
}


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

void Gfx_SetAlphaTest(cc_bool enabled) { 
	GU_Toggle(GU_ALPHA_TEST);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static int matrix_modes[] = { GU_PROJECTION, GU_VIEW };
static ScePspFMatrix4 tmp_matrix;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	gumLoadMatrix(&tmp_matrix, matrix);
	sceGuSetMatrix(matrix_modes[type], &tmp_matrix);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	sceGuSetMatrix(matrix_modes[type], &identity);
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
}

void Gfx_DrawVb_Lines(int verticesCount) {
	sceGuDrawArray(GU_LINES, gfx_fields, verticesCount, NULL, gfx_vertices);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	sceGuDrawArray(GU_TRIANGLES, gfx_fields | GU_INDEX_16BIT, ICOUNT(verticesCount), 
			gfx_indices, gfx_vertices + startVertex * gfx_stride);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	sceGuDrawArray(GU_TRIANGLES, gfx_fields | GU_INDEX_16BIT, ICOUNT(verticesCount),
			gfx_indices, gfx_vertices);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	sceGuDrawArray(GU_TRIANGLES, gfx_fields | GU_INDEX_16BIT, ICOUNT(verticesCount), 
			gfx_indices, gfx_vertices + startVertex * SIZEOF_VERTEX_TEXTURED);
}
#endif