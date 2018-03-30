#ifndef CC_ISOMETRICDRAWER_H
#define CC_ISOMETRICDRAWER_H
#include "VertexStructs.h"
#include "Block.h"
/* Draws 2D isometric blocks for the hotbar and inventory UIs.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Maximum number of vertices used to draw a block in isometric way. */
#define ISOMETRICDRAWER_MAXVERTICES 16
void IsometricDrawer_BeginBatch(VertexP3fT2fC4b* vertices, GfxResourceID vb);
void IsometricDrawer_DrawBatch(BlockID block, Real32 size, Real32 x, Real32 y);
void IsometricDrawer_EndBatch(void);
#endif