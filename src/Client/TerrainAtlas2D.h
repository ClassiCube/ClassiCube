#ifndef CS_TERRAINATLAS2D_H
#define CS_TERRAINATLAS2D_H
#include "Typedefs.h"
#include "Bitmap.h"
/* Represents the 2D texture atlas of terrain.png
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Number of elements in each row of 2D atlas. */
#define TerrainAtlas2D_ElementsPerRow 16

/* Number of rows in 2D atlas. */
#define TerrainAtlas2D_RowsCount 16

/* The actual bitmap of the 2D atlas. */
Bitmap TerrainAtlas2D_Bitmap;

/* Size in pixels of each element in the 2D atlas. */
Int32 TerrainAtlas2D_ElementSize;

/* Updates the underlying atlas bitmap, fields, and texture. */
void TerrainAtlas2D_UpdateState(Bitmap bmp);

/* Creates a native texture that contains the tile at the specified index. */
Int32 TerrainAtlas2D_LoadTextureElement(Int32 index);

/* Disposes of the underlying atlas bitmap. */
void TerrainAtlas2D_Free();
#endif