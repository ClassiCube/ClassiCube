#ifndef CC_PICKING_H
#define CC_PICKING_H
#include "Vectors.h"
CC_BEGIN_HEADER

/* 
Provides ray tracer functionality for calculating picking/selecting intersection
  e.g. calculating block selected in the world by the user, clipping the camera
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

/* Implements a voxel ray tracer
http://www.xnawiki.com/index.php/Voxel_traversal
https://web.archive.org/web/20120113051728/http://www.xnawiki.com/index.php?title=Voxel_traversal

Implementation based on: "A Fast Voxel Traversal Algorithm for Ray Tracing"
John Amanatides, Andrew Woo
http://www.cse.yorku.ca/~amana/research/grid.pdf
http://www.devmaster.net/articles/raytracing_series/A%20faster%20voxel%20traversal%20algorithm%20for%20ray%20tracing.pdf
*/
struct RayTracer {
	IVec3 pos;    /* Coordinates of block within world */
	Vec3 origin, dir;
	Vec3 Min, Max; /* Min/max coords of block's bounding box. */
	BlockID block;
	IVec3 step;
	Vec3 tMax, tDelta;
	/* Result only data */
	Vec3 intersect;      /* Coords at which the ray exactly intersected this block. */
	IVec3 translatedPos; /* Coords of the neighbouring block that is closest to the player */
	cc_bool valid;       /* Whether the ray tracer actually intersected with a block */
	Face closest;        /* Face of the intersected block that is closet to the player */
	Vec3 invDir;
};

/* Marks the given ray tracer as having no result. */
void RayTracer_SetInvalid(struct RayTracer* t);
/* Initialises the given ray tracer with the given origin and direction. */
void RayTracer_Init(struct RayTracer* t, const Vec3* origin, const Vec3* dir);
/* Moves to next grid cell position on the ray. */
void RayTracer_Step(struct RayTracer* t);

/* Determines the picked block based on the given origin and direction vector.
   Marks pickedPos as invalid if a block could not be found due to going outside map boundaries
   or not being able to find a suitable candiate within the given reach distance.*/
void Picking_CalcPickedBlock(const Vec3* origin, const Vec3* dir, float reach, struct RayTracer* t);
void Picking_ClipCameraPos(const Vec3* origin, const Vec3* dir, float reach, struct RayTracer* t);

CC_END_HEADER
#endif
