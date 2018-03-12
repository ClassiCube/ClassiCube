#ifndef CC_GFXAPI_H
#define CC_GFXAPI_H
#include "Typedefs.h"
#include "Bitmap.h"
#include "PackedCol.h"
#include "String.h"
#include "Matrix.h"
#include "Game.h"
#include "GraphicsEnums.h"
#include "GameStructs.h"

/* Abstracts a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
#define ICOUNT(verticesCount) (((verticesCount) >> 2) * 6)

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

/* Callback invoked when the current context is lost, and is repeatedly invoked until the context can be retrieved. */
ScheduledTaskCallback LostContextFunction;

/* Creates a new native texture from the given bitmap.
NOTE: only power of two dimension textures are supported. */
GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps);
/* Updates the sub-rectangle (x, y) -> (x + part.Width, y + part.Height)
of the native texture associated with the given ID, with the pixels encapsulated in the 'part' instance. */
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

/* Creates a vertex buffer that can have its data dynamically updated. */
GfxResourceID Gfx_CreateDynamicVb(Int32 vertexFormat, Int32 maxVertices);
/* Creates a static vertex buffer that has its data set at creation,
but the vertex buffer's data cannot be updated after creation.*/
GfxResourceID Gfx_CreateVb(void* vertices, Int32 vertexFormat, Int32 count);
/* Creates a static index buffer that has its data set at creation,
but the index buffer's data cannot be updated after creation. */
GfxResourceID Gfx_CreateIb(void* indices, Int32 indicesCount);
void Gfx_BindVb(GfxResourceID vb);
void Gfx_BindIb(GfxResourceID ib);
void Gfx_DeleteVb(GfxResourceID* vb);
void Gfx_DeleteIb(GfxResourceID* ib);

/* Informs the graphics API that the format of the vertex data used in subsequent
draw calls will be in the given format. */
void Gfx_SetBatchFormat(Int32 vertexFormat);
/* Binds and updates the data of the current dynamic vertex buffer's data.
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, Int32 vCount);
/* Draws the specified subset of the vertices in the current vertex buffer as lines. */
void Gfx_DrawVb_Lines(Int32 verticesCount);
/* Draws the specified subset of the vertices in the current vertex buffer as triangles. */
void Gfx_DrawVb_IndexedTris_Range(Int32 verticesCount, Int32 startVertex);
/* Draws the specified subset of the vertices in the current vertex buffer as triangles. */
void Gfx_DrawVb_IndexedTris(Int32 verticesCount);
/* Optimised version of DrawIndexedVb for VertexFormat_Pos3fTex2fCol4b */
void Gfx_DrawIndexedVb_TrisT2fC4b(Int32 verticesCount, Int32 startVertex);
static Int32 Gfx_strideSizes[2] = { 16, 24 };

void Gfx_SetMatrixMode(Int32 matrixType);
void Gfx_LoadMatrix(Matrix* matrix);
void Gfx_LoadIdentityMatrix(void);
void Gfx_CalcOrthoMatrix(Real32 width, Real32 height, Matrix* matrix);
void Gfx_CalcPerspectiveMatrix(Real32 fov, Real32 aspect, Real32 zNear, Real32 zFar, Matrix* matrix);

/* Outputs a .png screenshot of the backbuffer to the specified file. */
void Gfx_TakeScreenshot(STRING_PURE String* output, Int32 width, Int32 height);
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