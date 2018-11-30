#ifndef CC_TERRAINATLAS_H
#define CC_TERRAINATLAS_H
#include "Bitmap.h"
/* Represents the 2D texture atlas of terrain.png, and converted into an array of 1D textures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Number of tiles in each row */
#define ATLAS2D_TILES_PER_ROW 16
#define ATLAS2D_MASK 15
#define ATLAS2D_SHIFT 4
/* Maximum supported number of rows in the atlas. */
#ifdef EXTENDED_TEXTURES
#define ATLAS2D_MAX_ROWS_COUNT 32
#else
#define ATLAS2D_MAX_ROWS_COUNT 16
#endif
/* Maximum possible number of 1D terrain atlases. (worst case, each 1D atlas only has 1 tile) */
#define ATLAS1D_MAX_ATLASES (ATLAS2D_TILES_PER_ROW * ATLAS2D_MAX_ROWS_COUNT)

/* Bitmap that contains the textures of all tiles. */
/* Tiles are indexed left to right, top to bottom. */
extern Bitmap Atlas_Bitmap;
/* Size of each tile in pixels. (default 16x16) */
extern int Atlas_TileSize;
/* Number of rows in the atlas. (default 16, can be 32) */
extern int Atlas_RowsCount;
/* Number of 1D atlases the atlas was split into. */
extern int Atlas1D_Count;
/* Number of tiles in each 1D atlas. */
extern int Atlas1D_TilesPerAtlas;
/* Converts a tile id into 1D atlas index, and index within that atlas. */
extern int Atlas1D_Mask, Atlas1D_Shift;
/* Texture V coord that equals the size of one tile. (i.e. 1/Atlas1D_TilesPerAtlas) */
/* NOTE: The texture U coord that equals the size of one tile is 1. */
extern float Atlas1D_InvTileSize;
/* Textures for each 1D atlas. Only Atlas1D_Count of these are valid. */
extern GfxResourceID Atlas1D_TexIds[ATLAS1D_MAX_ATLASES];

#define Atlas2D_TileX(texLoc) ((texLoc) &  ATLAS2D_MASK)  /* texLoc % ATLAS2D_TILES_PER_ROW */
#define Atlas2D_TileY(texLoc) ((texLoc) >> ATLAS2D_SHIFT) /* texLoc / ATLAS2D_TILES_PER_ROW */
/* Returns the index of the given tile id within a 1D atlas */
#define Atlas1D_RowId(texLoc) ((texLoc)  & Atlas1D_Mask)  /* texLoc % Atlas1D_TilesPerAtlas */
/* Returns the index of the 1D atlas within the array of 1D atlases that contains the given tile id */
#define Atlas1D_Index(texLoc) ((texLoc) >> Atlas1D_Shift) /* texLoc / Atlas1D_TilesPerAtlas */

/* Loads the given atlas and converts it into an array of 1D atlases. */
/* NOTE: Use Game_ChangeTerrainAtlas to change atlas, because that raises TextureEvents_AtlasChanged */
void Atlas_Update(Bitmap* bmp);
/* Loads the given tile into a new separate texture. */
GfxResourceID Atlas_LoadTile(TextureLoc texLoc);
/* Frees the atlas and 1D atlas textures. */
void Atlas_Free(void);
/* Returns the UV rectangle of the given tile id in the 1D atlases. */
/* That is, returns U1/U2/V1/V2 coords that make up the tile in a 1D atlas. */
/* index is set to the index of the 1D atlas that the tile is in. */
TextureRec Atlas1D_TexRec(TextureLoc texLoc, int uCount, int* index);
#endif
