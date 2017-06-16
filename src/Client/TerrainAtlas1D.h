#ifndef CS_TERRAINATLAS1D_H
#define CS_TERRAINATLAS1D_H
#include "Typedefs.h"
#include "TextureRec.h"
#include "TerrainAtlas2D.h"
/* Represents 2D terrain.png converted into an array of 1D textures.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* The theoretical largest number of 1D atlases that a 2D atlas can be broken down into. */
#define Atlas1D_MaxAtlasesCount (Atlas2D_ElementsPerRow * Atlas2D_RowsCount)

/* The number of elements each 1D atlas contains. */
Int32 Atlas1D_ElementsPerAtlas;

/* The number of elements in each adjusted 1D atlas.
(in the case only 1 1D atlas used, and is resized vertically to be a power of two) */
Int32 Atlas1D_ElementsPerBitmap;

/* Size of a texture coord V for an element in a 1D atlas. */
Real32 Atlas1D_InvElementSize;

/* Native texture ID for each 1D atlas. */
Int32 Atlas1D_TexIds[Atlas1D_MaxAtlasesCount];

/* Number of 1D atlases that actually have textures / are used. */
Int32 Atlas1D_Count;

/* Retrieves the 1D texture rectangle and 1D atlas index of the given texture. */
TextureRec Atlas1D_TexRec(TextureLoc texLoc, Int32 uCount, Int32* index);

/* Returns the index of the 1D atlas within the array of 1D atlases that contains the given texture id.*/
#define Atlas1D_Index(texLoc) ((texLoc) / Atlas1D_ElementsPerAtlas)

/* Returns the index of the given texture id within a 1D atlas. */
#define Atlas1D_RowId(texLoc) ((texLoc) % Atlas1D_ElementsPerAtlas)

/* Updates variables and native textures for the 1D atlas array. */
void Atlas1D_UpdateState(void);

static void Atlas1D_Convert2DTo1D(Int32 atlasesCount, Int32 atlas1DHeight);

static void Atlas1D_Make1DTexture(Int32 i, Int32 atlas1DHeight, Int32* index);

/* Returns the count of used 1D atlases. (i.e. highest used 1D atlas index + 1) */
Int32 Atlas1D_UsedAtlasesCount(void);

void Atlas1D_Free(void);
#endif