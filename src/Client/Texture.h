#ifndef CC_TEXTURE_H
#define CC_TEXTURE_H
#include "PackedCol.h"
#include "2DStructs.h"

/* Represents a simple 2D textured quad.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Contains the information necessary to describe a 2D textured quad. */
struct Texture {	
	GfxResourceID ID;     /* Native texture ID.*/
	Int16 X, Y;           /* Origin of quad. */
	UInt16 Width, Height; /* Dimensions of quad. */	
	Real32 U1, V1;        /* Texture coordinates of top left corner. */
	Real32 U2, V2;        /* Texture coordinates of bottom right corner. */
};

void Texture_FromOrigin(struct Texture* tex, GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height, Real32 u2, Real32 v2);
void Texture_FromRec(struct Texture* tex, GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height, struct TextureRec rec);
void Texture_From(struct Texture* tex, GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height, Real32 u1, Real32 u2, Real32 v1, Real32 v2);
void Texture_MakeInvalid(struct Texture* tex) ;

void Texture_Render(struct Texture* tex);
void Texture_RenderShaded(struct Texture* tex, PackedCol shadeCol);
#endif
