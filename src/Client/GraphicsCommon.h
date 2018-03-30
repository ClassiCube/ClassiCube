#ifndef CC_GFXCOMMON_H
#define CC_GFXCOMMON_H
#include "String.h"
#include "Texture.h"
#include "VertexStructs.h"

/* Provides common/shared methods for a 3D graphics rendering API.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

void GfxCommon_Init(void);
void GfxCommon_Free(void);
void GfxCommon_LoseContext(STRING_PURE String* reason);
void GfxCommon_RecreateContext(void);

/* Binds and draws the specified subset of the vertices in the current dynamic vertex buffer
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void GfxCommon_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, Int32 vCount);
/*Binds and draws the specified subset of the vertices in the current dynamic vertex buffer
This method also replaces the dynamic vertex buffer's data first with the given vertices before drawing. */
void GfxCommon_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, Int32 vCount);

GfxResourceID GfxCommon_quadVb;
void GfxCommon_Draw2DFlat(Int32 x, Int32 y, Int32 width, Int32 height, PackedCol col);
void GfxCommon_Draw2DGradient(Int32 x, Int32 y, Int32 width, Int32 height, PackedCol top, PackedCol bottom);
GfxResourceID GfxCommon_texVb;
void GfxCommon_Draw2DTexture(Texture* tex, PackedCol col);
void GfxCommon_Make2DQuad(Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices);
void GfxCommon_Mode2D(Int32 width, Int32 height);
void GfxCommon_Mode3D(void);

GfxResourceID GfxCommon_MakeDefaultIb(void);
void GfxCommon_MakeIndices(UInt16* indices, Int32 iCount);

void GfxCommon_SetupAlphaState(UInt8 draw);
void GfxCommon_RestoreAlphaState(UInt8 draw);

void GfxCommon_GenMipmaps(Int32 width, Int32 height, UInt8* lvlScan0, UInt8* scan0);
Int32 GfxCommon_MipmapsLevels(Int32 width, Int32 height);
#endif