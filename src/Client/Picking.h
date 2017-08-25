#ifndef CS_PICKING_H
#define CS_PICKING_H
#include "Typedefs.h"
#include "Constants.h"
#include "Vectors.h"
#include "BlockID.h"
/* Data for picking/selecting block by the user, and clipping the camera.
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

void Picking_CalculatePickedBlock(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos);

void Picking_ClipCameraPos(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos);
#endif