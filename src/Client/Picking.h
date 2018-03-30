#ifndef CC_PICKING_H
#define CC_PICKING_H
#include "Constants.h"
#include "Vectors.h"
/* Data for picking/selecting block by the user, and clipping the camera.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Describes the picked/selected block by the user and its position. */
typedef struct PickedPos_ {
	Vector3 Min;            /* Minimum coords of the block's bounding box.*/
	Vector3 Max;            /* Maximum coords of the block's bounding box. */
	Vector3 Intersect;      /* Coords at which the ray intersected this block. */
	Vector3I BlockPos;      /* Coords of the block */
	Vector3I TranslatedPos; /* Coords of the neighbouring block that is closest to the player */	
	bool Valid;             /* Whether this instance actually has a selected block currently */
	Face ClosestFace;       /* Face of the picked block that is closet to the player */
	BlockID Block;          /* Block ID of the picked block */
} PickedPos;

/* Implements a voxel ray tracer
http://www.xnawiki.com/index.php/Voxel_traversal
https://web.archive.org/web/20120113051728/http://www.xnawiki.com/index.php?title=Voxel_traversal

Implementation based on: "A Fast Voxel Traversal Algorithm for Ray Tracing"
John Amanatides, Andrew Woo
http://www.cse.yorku.ca/~amana/research/grid.pdf
http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
*/
typedef struct RayTracer_ {
	Int32 X, Y, Z;
	Vector3 Origin, Dir;
	Vector3 Min, Max; /* Block data */
	BlockID Block;    /* Block data */
	Vector3I step;
	Vector3 tMax, tDelta;
} RayTracer;

void PickedPos_SetAsValid(PickedPos* pos, RayTracer* t, Vector3 intersect);
void PickedPos_SetAsInvalid(PickedPos* pos);
void RayTracer_SetVectors(RayTracer* t, Vector3 origin, Vector3 dir);
void RayTracer_Step(RayTracer* t);

/* Determines the picked block based on the given origin and direction vector.
   Marks pickedPos as invalid if a block could not be found due to going outside map boundaries
   or not being able to find a suitable candiate within the given reach distance.*/
void Picking_CalculatePickedBlock(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos);
void Picking_ClipCameraPos(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos);
#endif