#ifndef CS_TERRAINATLAS1D_H
#define CS_TERRAINATLAS1D_H
#include "Typedefs.h"
#include "TextureRec.h"
#include "TerrainAtlas2D.h"
/* Represents 2D terrain.png converted into an array of 1D textures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* The number of elements each 1D atlas contains. */
Int32 TerrainAtlas1D_ElementsPerAtlas1D;

/* The number of elements in each adjusted 1D atlas. 
(in the case only 1 1D atlas used, and is resized vertically to be a power of two) */
Int32 TerrainAtlas1D_ElementsPerBitmap;

/* Size of a texture coord V for an element in a 1D atlas. */
Real32 TerrainAtlas1D_InvElementSize;

/* Native texture ID for each 1D atlas. */
Int32 TerrainAtlas1D_TexIds[TerrainAtlas2D_ElementsPerRow * TerrainAtlas2D_RowsCount];
Int32 TerrainAtlas1D_TexIdsCount;

/* Retrieves the 1D texture rectangle and 1D atlas index of the given texture. */
TextureRec TerrainAtlas1D_TexRec(Int32 texId, Int32 uCount, Int32* index);

/* Returns the index of the 1D texture within the array of 1D textures containing the given texture id.*/
#define TerrainAtlas1D_Index(texId) (texId) / elementsPerAtlas1D;

/* Returns the index of the given texture id within a 1D texture. */
#define TerrainAtlas1D_RowId(texId) (texId) % elementsPerAtlas1D

/* Updates variables and native textures for the 1D atlas array. */
void TerrainAtlas1D_UpdateState();

static void TerrainAtlas1D_Convert2DTo1D(Int32 atlasesCount, Int32 atlas1DHeight);

static void TerrainAtlas1D_Make1DTexture(Int32 i, Int32 atlas1DHeight, Int32* index);

/* Returns the count of used 1D atlases. (i.e. highest used 1D atlas index + 1) */
Int32 TerrainAtlas1D_UsedAtlasesCount();

void TerrainAtlas1D_Free();
#endif