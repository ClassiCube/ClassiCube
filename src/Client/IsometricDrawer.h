#ifndef CS_ISOMETRICDRAWER_H
#define CS_ISOMETRICDRAWER_H
#include "Typedefs.h"
#include "GraphicsEnums.h"
#include "VertexStructs.h"
#include "Block.h"
/* Draws 2D isometric blocks for the hotbar and inventory UIs.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Sets up state for drawing a batch of isometric blocks. */
void IsometricDrawer_BeginBatch(VertexP3fT2fC4b* vertices, GfxResourceID vb);
/* Adds a block to the current batch. */
void IsometricDrawer_DrawBatch(BlockID block, Real32 size, Real32 x, Real32 y);
/* Finishes drawing a batch of isometric blocks. */
void IsometricDrawer_EndBatch(void);
#endif