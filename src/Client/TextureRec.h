#ifndef CS_TEXTUREREC_H
#define CS_TEXTUREREC_H
#include "Typedefs.h"
/* Represents four coordinates of a 2D textured quad.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores the four texture coordinates that describe a textured quad.  */
typedef struct TextureRec_ {
	Real32 U1, V1, U2, V2;
} TextureRec;


/* Constructs a texture rectangle from origin and size. */
TextureRec TextureRec_FromRegion(Real32 u, Real32 v, Real32 uWidth, Real32 vHeight);

/* Constructs a texture rectangle from four points. */
TextureRec TextureRec_FromPoints(Real32 u1, Real32 v1, Real32 u2, Real32 v2);

/* Swaps U1 and U2 of a texture rectangle. */
void TextureRec_SwapU(TextureRec* rec);
#endif