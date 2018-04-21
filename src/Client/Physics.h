#ifndef CC_PHYSICS_H
#define CC_PHYSICS_H
#include "Vectors.h"
/* Contains:
   - An axis aligned bounding box, and various methods related to them.
   - Various methods for intersecting geometry.
   - Calculates all possible blocks that a moving entity can intersect with.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct Entity_ Entity;

/* Descibes an axis aligned bounding box. */
typedef struct AABB_ { Vector3 Min, Max; } AABB;
void AABB_FromCoords6(AABB* result, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2);
void AABB_FromCoords(AABB* result, Vector3* min, Vector3* max);
void AABB_Make(AABB* result, Vector3* pos, Vector3* size);
void AABB_Offset(AABB* result, AABB* bb, Vector3* amount);
bool AABB_Intersects(AABB* bb, AABB* other);
bool AABB_Contains(AABB* parent, AABB* child);
bool AABB_ContainsPoint(AABB* parent, Vector3* P);

/* Calculates the intersection points of a ray and a rotated bounding box. */
bool Intersection_RayIntersectsRotatedBox(Vector3 origin, Vector3 dir, Entity* target, Real32* tMin, Real32* tMax);
/* Calculates the intersection points of a ray and a bounding box.
Source: http://www.cs.utah.edu/~awilliam/box/box.pdf */
bool Intersection_RayIntersectsBox(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, Real32* t0, Real32* t1);

typedef struct SearcherState_ { Int32 X, Y, Z; Real32 tSquared; } SearcherState;
extern SearcherState* Searcher_States;
Int32 Searcher_FindReachableBlocks(Entity* entity, AABB* entityBB, AABB* entityExtentBB);
void Searcher_CalcTime(Vector3* vel, AABB *entityBB, AABB* blockBB, Real32* tx, Real32* ty, Real32* tz);
void Searcher_Free(void);
#endif