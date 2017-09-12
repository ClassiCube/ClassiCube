#ifndef CS_TERRAINATLAS_H
#define CS_TERRAINATLAS_H
#include "Typedefs.h"
#include "Bitmap.h"
#include "2DStructs.h"
/* Represents the 2D texture atlas of terrain.png, and converted into an array of 1D textures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Number of elements in each row of 2D atlas. */
#define Atlas2D_ElementsPerRow 16
/* Number of rows in 2D atlas. */
#define Atlas2D_RowsCount 16

/* The actual bitmap of the 2D atlas. */
Bitmap Atlas2D_Bitmap;
/* Size in pixels of each element in the 2D atlas. */
Int32 Atlas2D_ElementSize;
/* Updates the underlying atlas bitmap, fields, and texture. */
void Atlas2D_UpdateState(Bitmap bmp);
/* Creates a native texture that contains the tile at the specified index. */
Int32 Atlas2D_LoadTextureElement(TextureLoc texLoc);
/* Disposes of the underlying atlas bitmap. */
void Atlas2D_Free(void);

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
/* Returns the count of used 1D atlases. (i.e. highest used 1D atlas index + 1) */
Int32 Atlas1D_UsedAtlasesCount(void);
void Atlas1D_Free(void);
#endif