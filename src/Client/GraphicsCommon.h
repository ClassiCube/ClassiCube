#ifndef CS_GFXCOMMON_H
#define CS_GFXCOMMON_H
#include "Typedefs.h"
#include "PackedCol.h"
#include "String.h"
#include "Texture.h"
#include "VertexStructs.h"
#include "Compiler.h"

/* Provides common/shared methods for a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Initalises common resources. */
void GfxCommon_Init(void);
/* Frees common resources. */
void GfxCommon_Free(void);

/* Handles a context being lost. */
void GfxCommon_LoseContext(STRING_TRANSIENT String* reason);
/* Handles a context being recreated. */
void GfxCommon_RecreateContext(void);


/* Binds and draws the specified subset of the vertices in the current dynamic vertex buffer
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void GfxCommon_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, Int32 vCount);
/*Binds and draws the specified subset of the vertices in the current dynamic vertex buffer
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void GfxCommon_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, Int32 vCount);

GfxResourceID GfxCommon_quadVb;
/* Draws a 2D flat coloured quad to the screen.*/
void GfxCommon_Draw2DFlat(Real32 x, Real32 y, Real32 width, Real32 height, PackedCol col);
/* Draws a 2D gradient coloured quad to the screen.*/
void GfxCommon_Draw2DGradient(Real32 x, Real32 y, Real32 width, Real32 height, 
	PackedCol topCol, PackedCol bottomCol);

GfxResourceID GfxCommon_texVb;
/* Draws a 2D texture to the screen. */
void GfxCommon_Draw2DTexture(Texture* tex, PackedCol col);
/* Makes the 2D vertices that compose a texture quad.*/
void GfxCommon_Make2DQuad(Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices);

/* Updates the various matrix stacks and properties so that the graphics API state
is suitable for rendering 2D quads and other 2D graphics to. */
void GfxCommon_Mode2D(Real32 width, Real32 height);
/* Updates the various matrix stacks and properties so that the graphics API state
 is suitable for rendering 3D vertices. */
void GfxCommon_Mode3D(void);

/* Makes the default index buffer used for drawing quads. */
GfxResourceID GfxCommon_MakeDefaultIb(void);
/* Fills the given index buffer with up to iCount indices. */
void GfxCommon_MakeIndices(UInt16* indices, Int32 iCount);

/* Sets the appropriate alpha testing/blending states necessary to render the given block. */
void GfxCommon_SetupAlphaState(UInt8 draw);
/* Resets the appropriate alpha testing/blending states necessary to render the given block. */
void GfxCommon_RestoreAlphaState(UInt8 draw);

void GfxCommon_GenMipmaps(Int32 width, Int32 height, UInt8* lvlScan0, UInt8* scan0);
Int32 GfxCommon_MipmapsLevels(Int32 width, Int32 height);
#endif