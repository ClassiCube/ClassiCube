#ifndef CC_INTERSECTION_H
#define CC_INTERSECTION_H
#include "Typedefs.h"
#include "Vectors.h"
#include "Entity.h"
/* Contains various methods for intersecting geometry.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Calculates the intersection points of a ray and a rotated bounding box. */
bool Intersection_RayIntersectsRotatedBox(Vector3 origin, Vector3 dir, Entity* target, Real32* tMin, Real32* tMax);
/* Calculates the intersection points of a ray and a bounding box.
Source: http://www.cs.utah.edu/~awilliam/box/box.pdf */
bool Intersection_RayIntersectsBox(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, Real32* t0, Real32* t1);
#endif