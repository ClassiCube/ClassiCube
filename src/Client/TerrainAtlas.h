#ifndef CC_TERRAINATLAS_H
#define CC_TERRAINATLAS_H
#include "Bitmap.h"
#include "2DStructs.h"
/* Represents the 2D texture atlas of terrain.png, and converted into an array of 1D textures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define ATLAS2D_ELEMENTS_PER_ROW 16
#define ATLAS2D_ROWS_COUNT 16

Bitmap Atlas2D_Bitmap;
Int32 Atlas2D_ElementSize;
void Atlas2D_UpdateState(Bitmap* bmp);
/* Creates a native texture that contains the tile at the specified index. */
GfxResourceID Atlas2D_LoadTextureElement(TextureLoc texLoc);
void Atlas2D_Free(void);

/* The theoretical largest number of 1D atlases that a 2D atlas can be broken down into. */
#define ATLAS1D_MAX_ATLASES_COUNT (ATLAS2D_ELEMENTS_PER_ROW * ATLAS2D_ROWS_COUNT)
/* The number of elements each 1D atlas contains. */
Int32 Atlas1D_ElementsPerAtlas;
/* The number of elements in each adjusted 1D atlas.
(in the case only 1 1D atlas used, and is resized vertically to be a power of two) */
Int32 Atlas1D_ElementsPerBitmap;
/* Size of a texture coord V for an element in a 1D atlas. */
Real32 Atlas1D_InvElementSize;
/* Native texture ID for each 1D atlas. */
GfxResourceID Atlas1D_TexIds[ATLAS1D_MAX_ATLASES_COUNT];
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
/* Returns the count of used 1D atlases. (i.e. highest used 1D atlas index + 1) */
Int32 Atlas1D_UsedAtlasesCount(void);
void Atlas1D_Free(void);
#endif