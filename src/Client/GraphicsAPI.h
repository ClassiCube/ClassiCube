#ifndef CS_GFXAPI_H
#define CS_GFXAPI_H
#include "Typedefs.h"
#include "EventHandler.h"
#include "Bitmap.h"
#include "PackedCol.h"
#include "String.h"
#include "Matrix.h"
#include "Game.h"
#include "GraphicsEnums.h"

/* Abstracts a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define VCOUNT(indicesCount) ((indicesCount) / 6 * 4)
#define ICOUNT(verticesCount) ((verticesCount) / 4 * 6)

/* Initalises this graphics API. */
void Gfx_Init(void);

/* Frees memory allocated by this graphics API. */
void Gfx_Free(void);

/* Maximum supported length of a dimension (width and height) of a 2D texture. */
Int32 Gfx_MaxTextureDimensions;

/* Minimum near plane value supported by the graphics API. */
float Gfx_MinZNear;

/* Returns whether this graphics api had a valid context. */
bool Gfx_LostContext;

/* Maximum number of vertices that can be indexed. */
#define Gfx_MaxIndices (65536 / 4 * 6)
/* Maximum number of vertices that can be indexed. */
#define Gfx_MaxVertex 65536


/* Event raised when a context is destroyed after having been previously lost. */
Event_Void Gfx_ContextLost[EventHandler_Size];
Int32 Gfx_ContextLostCount;

/* Event raised when a context is recreated after having been previously lost. */
Event_Void Gfx_ContextRecreated[EventHandler_Size];
Int32 Gfx_ContextRecreatedCount;

/* TODO: IMPLEMENT THIS */
/*	/// <summary> Delegate that is invoked when the current context is lost,
	/// and is repeatedly invoked until the context can be retrieved. </summary>
	public Action<ScheduledTask> LostContextFunction;*/


/* Creates a new native texture from the given bitmap.
NOTE: only power of two dimension textures are supported. */
GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool);

/* Updates the sub-rectangle (texX, texY) -> (texX + part.Width, texY + part.Height)
of the native texture associated with the given ID, with the pixels encapsulated in the 'part' instance. */
void Gfx_UpdateTexturePart(GfxResourceID texId, Int32 texX, Int32 texY, Bitmap* part);

/* Binds the given texture id so that it can be used for rasterization. */
void Gfx_BindTexture(GfxResourceID texId);

/* Frees all native resources held for the given texture id. */
void Gfx_DeleteTexture(GfxResourceID* texId);

/* Sets whether texturing is applied when rasterizing primitives. */
void Gfx_SetTexturing(bool enabled);


/* Sets whether fog is currently enabled. */
void Gfx_SetFog(bool enabled);

/* Sets the fog colour that is blended with final primitive colours. */
void Gfx_SetFogColour(PackedCol col);

/* Sets the density of exp and exp^2 fog */
void Gfx_SetFogDensity(Real32 value);

/* Sets the start radius of fog for linear fog. */
void Gfx_SetFogStart(Real32 value);

/* Sets the end radius of fog for for linear fog. */
void Gfx_SetFogEnd(Real32 value);

/* Sets the current fog mode. (linear, exp, or exp^2) */
void Gfx_SetFogMode(Fog fogMode);


/* Whether back facing primitives should be culled by the 3D graphics api. */
void Gfx_SetFaceCulling(bool enabled);

/* Sets hether alpha testing is currently enabled. */
void Gfx_SetAlphaTest(bool enabled);

/* Sets the alpha test compare function that is used when alpha testing is enabled. */
void Gfx_SetAlphaTestFunc(CompareFunc compareFunc, Real32 refValue);

/* Whether alpha blending is currently enabled. */
void Gfx_SetAlphaBlending(bool enabled);

/* Sets the alpha blend function that is used when alpha blending is enabled. */
void Gfx_SetAlphaBlendFunc(BlendFunc srcBlendFunc, BlendFunc dstBlendFunc);

/* Whether blending between the alpha components of the texture and colour are performed. */
void Gfx_SetAlphaArgBlend(bool enabled);


/* Clears the underlying back and/or front buffer. */
void Gfx_Clear(void);

/* Sets the colour the screen is cleared to when Clear() is called. */
void Gfx_ClearColour(PackedCol col);


/* Whether depth testing is currently enabled. */
void Gfx_SetDepthTest(bool enabled);

/* Sets the depth test compare function that is used when depth testing is enabled. */
void Gfx_SetDepthTestFunc(CompareFunc compareFunc);

/* Whether writing to the colour buffer is enabled. */
void Gfx_SetColourWrite(bool enabled);

/* Whether writing to the depth buffer is enabled. */
void Gfx_SetDepthWrite(bool enabled);


/* Creates a vertex buffer that can have its data dynamically updated. */
GfxResourceID Gfx_CreateDynamicVb(VertexFormat vertexFormat, Int32 maxVertices);

/* Creates a static vertex buffer that has its data set at creation,
but the vertex buffer's data cannot be updated after creation.*/
GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat vertexFormat, Int32 count);

/* Creates a static index buffer that has its data set at creation,
but the index buffer's data cannot be updated after creation. */
GfxResourceID Gfx_CreateIb(void* indices, Int32 indicesCount);

/* Sets the currently active vertex buffer to the given id. */
void Gfx_BindVb(GfxResourceID vb);

/* Sets the currently active index buffer to the given id. */
void Gfx_BindIb(GfxResourceID ib);

/* Frees all native resources held for the vertex buffer associated with the given id. */
void Gfx_DeleteVb(GfxResourceID* vb);

/* Frees all native resources held for the index buffer associated with the given id. */
void Gfx_DeleteIb(GfxResourceID* ib);

/* Informs the graphics API that the format of the vertex data used in subsequent
draw calls will be in the given format. */
void Gfx_SetBatchFormat(VertexFormat vertexFormat);

/* Binds and updates the data of the current dynamic vertex buffer's data.
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, Int32 vCount);

/* Draws the specified subset of the vertices in the current vertex buffer as lines. */
void Gfx_DrawVb_Lines(Int32 verticesCount);

/* Draws the specified subset of the vertices in the current vertex buffer as triangles. */
void Gfx_DrawVb_IndexedTris_Range(Int32 indicesCount, Int32 startIndex);

/* Draws the specified subset of the vertices in the current vertex buffer as triangles. */
void Gfx_DrawVb_IndexedTris(Int32 indicesCount);

/* Optimised version of DrawIndexedVb for VertexFormat_Pos3fTex2fCol4b */
void Gfx_DrawIndexedVb_TrisT2fC4b(Int32 indicesCount, Int32 startIndex);

static Int32 Gfx_strideSizes[2] = { 16, 24 };


/* Sets the matrix type that load/push/pop operations should be applied to. */
void Gfx_SetMatrixMode(MatrixType matrixType);

/* Sets the current matrix to the given matrix.*/
void Gfx_LoadMatrix(Matrix* matrix);

/* Sets the current matrix to the identity matrix. */
void Gfx_LoadIdentityMatrix(void);

/* Multiplies the current matrix by the given matrix, then
sets the current matrix to the result of the multiplication. */
void Gfx_MultiplyMatrix(Matrix* matrix);

/* Gets the top matrix the current matrix stack and pushes it to the stack. */
void Gfx_PushMatrix(void);

/* Removes the top matrix from the current matrix stack, then
sets the current matrix to the new top matrix of the stack. */
void Gfx_PopMatrix(void);

/* Loads an orthographic projection matrix for the given height.*/
void Gfx_LoadOrthoMatrix(Real32 width, Real32 height);


/*Outputs a .png screenshot of the backbuffer to the specified file. */
void Gfx_TakeScreenshot(String output, Int32 width, Int32 height);

/* Adds a warning to game's chat if this graphics API has problems with the current user's GPU. 
Returns boolean of whether legacy rendering mode is needed. */
bool Gfx_WarnIfNecessary(void);

/* Informs the graphic api to update its state in preparation for a new frame. */
void Gfx_BeginFrame(void);

/* Informs the graphic api to update its state in preparation for the end of a frame,
and to prepare that frame for display on the monitor. */
void Gfx_EndFrame(void);

/* Sets whether the graphics api should tie frame rendering to the refresh rate of the monitor. */
void Gfx_SetVSync(bool value);

/* Raised when the dimensions of the game's window have changed. */
void Gfx_OnWindowResize(void);


/* Makes an array of strings of information about the graphics API. */
void Gfx_MakeApiInfo(void);

#define Gfx_ApiInfo_Count 32
/* Array of strings for information about the graphics API.
Max of Gfx_ApiInfo_Count strings, check if string is included by checking length > 0*/
String Gfx_ApiInfo[Gfx_ApiInfo_Count];
#endif