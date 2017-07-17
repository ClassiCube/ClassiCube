#ifndef CS_MODELBUILDER_H
#define CS_MODELBUILDER_H
#include "Typedefs.h"
#include "Vectors.h"
#include "IModel.h"
/* Contains various structs and methods that assist with building an entity model.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Describes data for a box being built. */
typedef struct BoxDesc_ {
	/* Texture coordinates and dimensions. */
	Int32 TexX, TexY, SidesW, BodyW, BodyH;

	/* Box corner coordinates. */
	Real32 X1, X2, Y1, Y2, Z1, Z2;

	/* Coordinate around which this box is rotated. */
	Real32 RotX, RotY, RotZ;
} BoxDesc;


/* Sets the texture origin for this part within the texture atlas. */
void BoxDesc_TexOrigin(BoxDesc* desc, Int32 x, Int32 y);

/* Sets the the two corners of this box, in pixel coordinates. */
void BoxDesc_SetBounds(BoxDesc* desc, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2);

/* Expands the corners of this box outwards by the given amount in pixel coordinates. */
void BoxDesc_Expand(BoxDesc* desc, Real32 amount);

/* Scales the corners of this box outwards by the given amounts. */
void BoxDesc_Scale(BoxDesc* desc, Real32 scale);

/* Sets the coordinate that this box is rotated around, in pixel coordinates. */
void BoxDesc_RotOrigin(BoxDesc* desc, Int8 x, Int8 y, Int8 z);

/* Swaps the min and max X around, resulting in the part being drawn mirrored. */
void BoxDesc_MirrorX(BoxDesc* desc);


/* Constructs a description of the given box, from two corners.
See BoxDesc_BuildBox for details on texture layout. */
BoxDesc BoxDesc_Box(Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2);

/* Constructs a description of the given rotated box, from two corners.
See BoxDesc_BuildRotatedBox for details on texture layout. */
BoxDesc BoxDesc_RotatedBox(Int32 x1, Int32 y1, Int32 z1, Int32 x2, Int32 y2, Int32 z2);


/* Builds a box model assuming the follow texture layout:
let SW = sides width, BW = body width, BH = body height
*********************************************************************************************
|----------SW----------|----------BW----------|----------BW----------|----------------------|
|S--------------------S|S--------top---------S|S-------bottom-------S|----------------------|
|W--------------------W|W--------tex---------W|W--------tex---------W|----------------------|
|----------SW----------|----------BW----------|----------BW----------|----------------------|
*********************************************************************************************
|----------SW----------|----------BW----------|----------SW----------|----------BW----------|
|B--------left--------B|B-------front--------B|B-------right--------B|B--------back--------B|
|H--------tex---------H|H--------tex---------H|H--------tex---------H|H--------tex---------H|
|----------SW----------|----------BW----------|----------SW----------|----------BW----------|
********************************************************************************************* */
ModelPart BoxDesc_BuildBox(IModel* m, BoxDesc* desc);

/* Builds a box model assuming the follow texture layout:
let SW = sides width, BW = body width, BH = body height
*********************************************************************************************
|----------SW----------|----------BW----------|----------BW----------|----------------------|
|S--------------------S|S-------front--------S|S--------back--------S|----------------------|
|W--------------------W|W--------tex---------W|W--------tex---------W|----------------------|
|----------SW----------|----------BW----------|----------BW----------|----------------------|
*********************************************************************************************
|----------SW----------|----------BW----------|----------BW----------|----------SW----------|
|B--------left--------B|B-------bottom-------B|B-------right--------B|B--------top---------B|
|H--------tex---------H|H--------tex---------H|H--------tex---------H|H--------tex---------H|
|----------SW----------|----------BW----------|----------BW----------|----------------------|
********************************************************************************************* */
ModelPart BoxDesc_BuildRotatedBox(IModel* m, BoxDesc* desc);


static void BoxDesc_XQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 z1, Real32 z2, Real32 y1, Real32 y2, Real32 x);

static void BoxDesc_YQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 z1, Real32 z2, Real32 y);

static void BoxDesc_ZQuad(IModel* m, Int32 texX, Int32 texY, Int32 texWidth, Int32 texHeight,
	Real32 x1, Real32 x2, Real32 y1, Real32 y2, Real32 z);
#endif