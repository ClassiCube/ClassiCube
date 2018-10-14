#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "PackedCol.h"
#include "Vectors.h"
#include "GameStructs.h"

/* Abstracts a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;

enum CompareFunc {
	COMPARE_FUNC_ALWAYS, COMPARE_FUNC_NOTEQUAL,  COMPARE_FUNC_NEVER,
	COMPARE_FUNC_LESS,   COMPARE_FUNC_LESSEQUAL, COMPARE_FUNC_EQUAL,
	COMPARE_FUNC_GREATEREQUAL, COMPARE_FUNC_GREATER
};
enum BlendFunc {
	BLEND_FUNC_ZERO,          BLEND_FUNC_ONE,       BLEND_FUNC_SRC_ALPHA,
	BLEND_FUNC_INV_SRC_ALPHA, BLEND_FUNC_DST_ALPHA, BLEND_FUNC_INV_DST_ALPHA
};

enum VERTEX_FORMAT { VERTEX_FORMAT_P3FC4B, VERTEX_FORMAT_P3FT2FC4B };
enum FOG_FUNC      { FOG_LINEAR, FOG_EXP, FOG_EXP2 };
enum MATRIX_TYPE   { MATRIX_TYPE_PROJECTION, MATRIX_TYPE_VIEW, MATRIX_TYPE_TEXTURE };

void Gfx_Init(void);
void Gfx_Free(void);

int Gfx_MaxTexWidth, Gfx_MaxTexHeight;
float Gfx_MinZNear;
bool Gfx_LostContext;
bool Gfx_Mipmaps;
bool Gfx_CustomMipmapsLevels;
struct Matrix Gfx_View, Gfx_Projection;

#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)
#define GFX_MAX_INDICES (65536 / 4 * 6)
#define GFX_MAX_VERTICES 65536

/* Callback invoked when the context is lost. Repeatedly invoked until a context can be retrieved. */
ScheduledTaskCallback Gfx_LostContextFunction;

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps);
void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps);
void Gfx_BindTexture(GfxResourceID texId);
void Gfx_DeleteTexture(GfxResourceID* texId);
void Gfx_SetTexturing(bool enabled);
void Gfx_EnableMipmaps(void);
void Gfx_DisableMipmaps(void);

bool Gfx_GetFog(void);
void Gfx_SetFog(bool enabled);
void Gfx_SetFogCol(PackedCol col);
void Gfx_SetFogDensity(float value);
void Gfx_SetFogEnd(float value);
void Gfx_SetFogMode(int fogMode);

void Gfx_SetFaceCulling(bool enabled);
void Gfx_SetAlphaTest(bool enabled);
void Gfx_SetAlphaTestFunc(enum CompareFunc func, float refValue);
void Gfx_SetAlphaBlending(bool enabled);
void Gfx_SetAlphaBlendFunc(enum BlendFunc srcFunc, enum BlendFunc dstFunc);
/* Sets whether blending between the alpha components of the texture and vertex colour is performed. */
void Gfx_SetAlphaArgBlend(bool enabled);

void Gfx_Clear(void);
void Gfx_ClearCol(PackedCol col);
void Gfx_SetDepthTest(bool enabled);
void Gfx_SetDepthTestFunc(enum CompareFunc func);
void Gfx_SetColWriteMask(bool r, bool g, bool b, bool a);
void Gfx_SetDepthWrite(bool enabled);

GfxResourceID Gfx_CreateDynamicVb(int vertexFormat, int maxVertices);
GfxResourceID Gfx_CreateVb(void* vertices, int vertexFormat, int count);
GfxResourceID Gfx_CreateIb(void* indices, int indicesCount);
void Gfx_BindVb(GfxResourceID vb);
void Gfx_BindIb(GfxResourceID ib);
void Gfx_DeleteVb(GfxResourceID* vb);
void Gfx_DeleteIb(GfxResourceID* ib);

void Gfx_SetBatchFormat(int vertexFormat);
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount);
void Gfx_DrawVb_Lines(int verticesCount);
void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex);
void Gfx_DrawVb_IndexedTris(int verticesCount);
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex);

void Gfx_SetMatrixMode(int matrixType);
void Gfx_LoadMatrix(struct Matrix* matrix);
void Gfx_LoadIdentityMatrix(void);
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix);
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix);

/* Outputs a .png screenshot of the backbuffer */
ReturnCode Gfx_TakeScreenshot(struct Stream* output, int width, int height);
/* Warns if this graphics API has problems with the user's GPU. Returns whether legacy rendering mode is needed. */
bool Gfx_WarnIfNecessary(void);
void Gfx_BeginFrame(void);
void Gfx_EndFrame(void);
void Gfx_SetVSync(bool value);
void Gfx_OnWindowResize(void);
void Gfx_MakeApiInfo(void);
void Gfx_UpdateApiInfo(void);
#endif
