#ifndef CC_PICKING_H
#define CC_PICKING_H
#include "Constants.h"
#include "Vectors.h"
/* Data for picking/selecting block by the user, and clipping the camera.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

/* Describes the picked/selected block by the user and its position. */
struct PickedPos {
	Vec3 Min;            /* Minimum coords of the block's bounding box.*/
	Vec3 Max;            /* Maximum coords of the block's bounding box. */
	Vec3 Intersect;      /* Coords at which the ray intersected this block. */
	IVec3 BlockPos;      /* Coords of the block */
	IVec3 TranslatedPos; /* Coords of the neighbouring block that is closest to the player */	
	cc_bool Valid;       /* Whether this instance actually has a selected block currently */
	Face Closest;        /* Face of the picked block that is closet to the player */
	BlockID Block;       /* Block ID of the picked block */
};

/* Implements a voxel ray tracer
http://www.xnawiki.com/index.php/Voxel_traversal
https://web.archive.org/web/20120113051728/http://www.xnawiki.com/index.php?title=Voxel_traversal

Implementation based on: "A Fast Voxel Traversal Algorithm for Ray Tracing"
John Amanatides, Andrew Woo
http://www.cse.yorku.ca/~amana/research/grid.pdf
http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
*/
struct RayTracer {
	int X, Y, Z;
	Vec3 Origin, Dir;
	Vec3 Min, Max; /* Block data */
	BlockID Block; /* Block data */
	IVec3 step;
	Vec3 tMax, tDelta;
};

void PickedPos_SetAsValid(struct PickedPos* pos, struct RayTracer* t, const Vec3* intersect);
void PickedPos_SetAsInvalid(struct PickedPos* pos);
void RayTracer_Init(struct RayTracer* t, const Vec3* origin, const Vec3* dir);
void RayTracer_Step(struct RayTracer* t);

/* Determines the picked block based on the given origin and direction vector.
   Marks pickedPos as invalid if a block could not be found due to going outside map boundaries
   or not being able to find a suitable candiate within the given reach distance.*/
void Picking_CalcPickedBlock(const Vec3* origin, const Vec3* dir, float reach, struct PickedPos* pos);
void Picking_ClipCameraPos(const Vec3* origin, const Vec3* dir, float reach, struct PickedPos* pos);
#endif
