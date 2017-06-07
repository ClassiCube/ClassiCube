#ifndef CS_ISOMETRICDRAWER_H
#define CS_ISOMETRICDRAWER_H
#include "Typedefs.h"
#include "GraphicsEnums.h"
#include "VertexStructs.h"
#include "BlockEnums.h"
/* Draws 2D isometric blocks for the hotbar and inventory UIs.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Sets up state for drawing a batch of isometric blocks. */
void IsometricDrawer_BeginBatch(VertexP3fT2fC4b* vertices, GfxResourceID vb);

/* Adds a block to the current batch. */
void IsometricDrawer_DrawBatch(BlockID block, Real32 size, Real32 x, Real32 y);

/* Finishes drawing a batch of isometric blocks. */
void IsometricDrawer_EndBatch(void);


/* Calculates cached variables used for batching. (rotation, transform matrices) */
static void IsometricDrawer_InitCache(void);

/* Gets texture of given face of block, flushing the batch to the GPU
if the texture is in a different 1D atlas than the previous items. */
static TextureLoc IsometricDrawer_GetTexLoc(BlockID block, Face face);

static void IsometricDrawer_SpriteZQuad(BlockID block, bool firstPart);

static void IsometricDrawer_SpriteXQuad(BlockID block, bool firstPart);

/* Flushes the current batch to the GPU, used when next item in the batch
uses a different 1D texture atlas than the previous items. */
static void IsometricDrawer_Flush(void);


/* Rotates the given 3D coordinates around the x axis. */
static void IsometricDrawer_RotateX(Real32 cosA, Real32 sinA);

/* Rotates the given 3D coordinates around the y axis. */
static void IsometricDrawer_RotateY(Real32 cosA, Real32 sinA);

#endif