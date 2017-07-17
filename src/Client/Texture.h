#ifndef CS_TEXTURE_H
#define CS_TEXTURE_H
#include "Typedefs.h"
#include "PackedCol.h"
#include "TextureRec.h"
#include "GraphicsEnums.h"

/* Represents a simple 2D textured quad.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Contains the information necessary to describe a 2D textured quad. */
typedef struct Texture_ {
	/* Native texture ID.*/
	GfxResourceID ID;

	/* Origin of quad. */
	Int16 X, Y;

	/* Dimensions of quad. */
	Int16 Width, Height;

	/* Texture coordinates of top left corner. */
	Real32 U1, V1;

	/* Textures coordinates of bottom right corner. */
	Real32 U2, V2;
} Texture;


/* Creates a texture, with U1 and V1 being set to 0. */
Texture Texture_FromOrigin(GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	Real32 u2, Real32 v2);

/* Creates a texture, with U1/V1/U2/V2 using the input rectangle. */
Texture Texture_FromRec(GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	TextureRec rec);

/* Creates a texture. */
Texture Texture_From(GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	Real32 u1, Real32 u2, Real32 v1, Real32 v2);


/* Returns whether the given texture has a valid texture ID. */
bool Texture_IsValid(Texture* tex);

/* Renders this texture to the screen. */
void Texture_Render(Texture* tex);

/* Renders this texture to the screen, with the given colour as a shade. */
void Texture_RenderShaded(Texture* tex, PackedCol shadeColour);
#endif