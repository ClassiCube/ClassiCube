#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "Bitmap.h"
#include "PackedCol.h"
#include "Vectors.h"
#include "GameStructs.h"

/* Abstracts a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct Stream_ Stream;

#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)
#define VERTEX_FORMAT_P3FC4B 0
#define VERTEX_FORMAT_P3FT2FC4B 1

enum COMPARE_FUNC {
	COMPARE_FUNC_ALWAYS, COMPARE_FUNC_NOTEQUAL,  COMPARE_FUNC_NEVER,
	COMPARE_FUNC_LESS,   COMPARE_FUNC_LESSEQUAL, COMPARE_FUNC_EQUAL,
	COMPARE_FUNC_GREATEREQUAL, COMPARE_FUNC_GREATER,
};
enum BLEND_FUNC {
	BLEND_FUNC_ZERO,          BLEND_FUNC_ONE,       BLEND_FUNC_SRC_ALPHA,
	BLEND_FUNC_INV_SRC_ALPHA, BLEND_FUNC_DST_ALPHA, BLEND_FUNC_INV_DST_ALPHA,
};

enum FOG_FUNC { FOG_LINEAR, FOG_EXP, FOG_EXP2, };
enum MATRIX_TYPE { MATRIX_TYPE_PROJECTION, MATRIX_TYPE_VIEW, MATRIX_TYPE_TEXTURE };

void Gfx_Init(void);
void Gfx_Free(void);

Int32 Gfx_MaxTextureDimensions;
Real32 Gfx_MinZNear;
bool Gfx_LostContext;
bool Gfx_Mipmaps;
bool Gfx_CustomMipmapsLevels;
Matrix Gfx_View, Gfx_Projection;

#define GFX_MAX_INDICES (65536 / 4 * 6)
#define GFX_MAX_VERTICES 65536
#define GFX_STRIDE_SIZES { 16, 24 }

/* Callback invoked when the current context is lost, and is repeatedly invoked until the context can be retrieved. */
ScheduledTaskCallback Gfx_LostContextFunction;

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps);
void Gfx_UpdateTexturePart(GfxResourceID texId, Int32 x, Int32 y, Bitmap* part, bool mipmaps);
void Gfx_BindTexture(GfxResourceID texId);
void Gfx_DeleteTexture(GfxResourceID* texId);
void Gfx_SetTexturing(bool enabled);
void Gfx_EnableMipmaps(void);
void Gfx_DisableMipmaps(void);

bool Gfx_GetFog(void);
void Gfx_SetFog(bool enabled);
void Gfx_SetFogColour(PackedCol col);
void Gfx_SetFogDensity(Real32 value);
void Gfx_SetFogStart(Real32 value);
void Gfx_SetFogEnd(Real32 value);
void Gfx_SetFogMode(Int32 fogMode);

void Gfx_SetFaceCulling(bool enabled);
void Gfx_SetAlphaTest(bool enabled);
void Gfx_SetAlphaTestFunc(Int32 compareFunc, Real32 refValue);
void Gfx_SetAlphaBlending(bool enabled);
void Gfx_SetAlphaBlendFunc(Int32 srcBlendFunc, Int32 dstBlendFunc);
/* Whether blending between the alpha components of the texture and colour are performed. */
void Gfx_SetAlphaArgBlend(bool enabled);

void Gfx_Clear(void);
void Gfx_ClearColour(PackedCol col);
void Gfx_SetDepthTest(bool enabled);
void Gfx_SetDepthTestFunc(Int32 compareFunc);
void Gfx_SetColourWriteMask(bool r, bool g, bool b, bool a);
void Gfx_SetDepthWrite(bool enabled);

GfxResourceID Gfx_CreateDynamicVb(Int32 vertexFormat, Int32 maxVertices);
GfxResourceID Gfx_CreateVb(void* vertices, Int32 vertexFormat, Int32 count);
GfxResourceID Gfx_CreateIb(void* indices, Int32 indicesCount);
void Gfx_BindVb(GfxResourceID vb);
void Gfx_BindIb(GfxResourceID ib);
void Gfx_DeleteVb(GfxResourceID* vb);
void Gfx_DeleteIb(GfxResourceID* ib);

void Gfx_SetBatchFormat(Int32 vertexFormat);
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, Int32 vCount);
void Gfx_DrawVb_Lines(Int32 verticesCount);
void Gfx_DrawVb_IndexedTris_Range(Int32 verticesCount, Int32 startVertex);
void Gfx_DrawVb_IndexedTris(Int32 verticesCount);
void Gfx_DrawIndexedVb_TrisT2fC4b(Int32 verticesCount, Int32 startVertex);

void Gfx_SetMatrixMode(Int32 matrixType);
void Gfx_LoadMatrix(Matrix* matrix);
void Gfx_LoadIdentityMatrix(void);
void Gfx_CalcOrthoMatrix(Real32 width, Real32 height, Matrix* matrix);
void Gfx_CalcPerspectiveMatrix(Real32 fov, Real32 aspect, Real32 zNear, Real32 zFar, Matrix* matrix);

/* Outputs a .png screenshot of the backbuffer */
void Gfx_TakeScreenshot(Stream* output, Int32 width, Int32 height);
/* Adds a warning to game's chat if this graphics API has problems with the current user's GPU. 
Returns boolean of whether legacy rendering mode is needed. */
bool Gfx_WarnIfNecessary(void);
void Gfx_BeginFrame(void);
void Gfx_EndFrame(void);
void Gfx_SetVSync(bool value);
void Gfx_OnWindowResize(void);
void Gfx_MakeApiInfo(void);

String Gfx_ApiInfo[8];
#endif