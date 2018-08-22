#ifndef CC_PHYSICS_H
#define CC_PHYSICS_H
#include "Vectors.h"
/* Contains:
   - An axis aligned bounding box, and various methods related to them.
   - Various methods for intersecting geometry.
   - Calculates all possible blocks that a moving entity can intersect with.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Entity;

/* Descibes an axis aligned bounding box. */
struct AABB { Vector3 Min, Max; };
void AABB_Make(struct AABB* result, Vector3* pos, Vector3* size);
void AABB_Offset(struct AABB* result, struct AABB* bb, Vector3* amount);
bool AABB_Intersects(struct AABB* bb, struct AABB* other);
bool AABB_Contains(struct AABB* parent, struct AABB* child);
bool AABB_ContainsPoint(struct AABB* parent, Vector3* P);

/* Calculates the intersection points of a ray and a rotated bounding box. */
bool Intersection_RayIntersectsRotatedBox(Vector3 origin, Vector3 dir, struct Entity* target, Real32* tMin, Real32* tMax);
/* Calculates the intersection points of a ray and a bounding box.
Source: http://www.cs.utah.edu/~awilliam/box/box.pdf */
bool Intersection_RayIntersectsBox(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, Real32* t0, Real32* t1);

struct SearcherState { Int32 X, Y, Z; Real32 tSquared; };
extern struct SearcherState* Searcher_States;
Int32 Searcher_FindReachableBlocks(struct Entity* entity, struct AABB* entityBB, struct AABB* entityExtentBB);
void Searcher_CalcTime(Vector3* vel, struct AABB *entityBB, struct AABB* blockBB, Real32* tx, Real32* ty, Real32* tz);
void Searcher_Free(void);
#endif
