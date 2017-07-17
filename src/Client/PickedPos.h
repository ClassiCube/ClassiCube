#ifndef CS_PICKEDPOS_H
#define CS_PICKEDPOS_H
#include "Typedefs.h"
#include "Vector3I.h"
#include "Vectors.h"
#include "BlockEnums.h"
/* Data for picked/selected block by the user, and related methods.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Describes the picked/selected block by the user and its position. */
typedef struct PickedPos_ {

	/* Minimum world coordinates of the block's bounding box.*/
	Vector3 Min;

	/* Maximum world coordinates of the block's bounding box. */
	Vector3 Max;

	/* Exact world coordinates at which the ray intersected this block. */
	Vector3 Intersect;

	/* Integer world coordinates of the block. */
	Vector3I BlockPos;

	/* Integer world coordinates of the neighbouring block that is closest to the player. */
	Vector3I TranslatedPos;

	/* Whether this instance actually has a selected block currently. */
	bool Valid;

	/* Face of the picked block that is closet to the player. */
	Face ClosestFace;

	/* Block ID of the picked block. */
	BlockID Block;
} PickedPos;

/*  Mark as having a selected block, and calculates the closest face of the selected block's position. */
void PickedPos_SetAsValid(PickedPos* pos, Int32 x, Int32 y, Int32 z, Vector3 min, Vector3 max);

/* Marks as not having a selected block. */
void PickedPos_SetAsInvalid(PickedPos* pos);

static void PickedPos_TestAxis(Real32 dAxis, Real32* dist, Face face);
#endif