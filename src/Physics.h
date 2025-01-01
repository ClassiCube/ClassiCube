#ifndef CC_PHYSICS_H
#define CC_PHYSICS_H
#include "Vectors.h"
CC_BEGIN_HEADER

/* 
Provides various physics related structs and methods such as:
   - An axis aligned bounding box, and various methods related to them.
   - Various methods for intersecting geometry.
   - Calculates all possible blocks that a moving entity can intersect with
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct Entity;

/* Descibes an axis aligned bounding box. */
struct AABB { Vec3 Min, Max; };

/* Makes an AABB centred at the base of the given position, that is: */
/* Min = [pos.x - size.x/2, pos.y         , pos.z - size.z/2] */
/* Max = [pos.x + size.x/2, pos.y + size.y, pos.z + size.z/2] */
CC_API void AABB_Make(struct AABB* result, const Vec3* pos, const Vec3* size);
/* Adds amount to Min and Max corners of the given AABB. */
void AABB_Offset(struct AABB* result, const struct AABB* bb, const Vec3* amount);
/* Whether the given AABB touches another AABB on any axis. */
cc_bool AABB_Intersects(const struct AABB* bb, const struct AABB* other);
/* Whether the given ABBB contains another AABB. */
cc_bool AABB_Contains(const struct AABB* parent, const struct AABB* child);
/* Whether the given point lies insides bounds of the given AABB. */
cc_bool AABB_ContainsPoint(const struct AABB* parent, const Vec3* P);

/* Calculates the intersection point of a ray and a rotated bounding box. */
cc_bool Intersection_RayIntersectsRotatedBox(Vec3 origin, Vec3 dir, struct Entity* target, float* tMin, float* tMax);
/* Calculates the intersection point of a ray and a bounding box.
Source: http://www.cs.utah.edu/~awilliam/box/box.pdf */
/* NOTE: invDir is inverse of ray's direction (i.e. 1.0f / dir) */
cc_bool Intersection_RayIntersectsBox(Vec3 origin, Vec3 invDir, Vec3 min, Vec3 max, float* t0, float* t1);

struct SearcherState { int x, y, z; float tSquared; };
extern struct SearcherState* Searcher_States;
int Searcher_FindReachableBlocks(struct Entity* entity, struct AABB* entityBB, struct AABB* entityExtentBB);
void Searcher_CalcTime(Vec3* vel, struct AABB *entityBB, struct AABB* blockBB, float* tx, float* ty, float* tz);
void Searcher_Free(void);

CC_END_HEADER
#endif
