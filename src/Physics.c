#include "Physics.h"
#include "ExtMath.h"
#include "Block.h"
#include "World.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Logger.h"
#include "Entity.h"


/*########################################################################################################################*
*----------------------------------------------------------AABB-----------------------------------------------------------*
*#########################################################################################################################*/
void AABB_Make(struct AABB* result, const Vec3* pos, const Vec3* size) {
	result->Min.x = pos->x - size->x * 0.5f;
	result->Min.y = pos->y;
	result->Min.z = pos->z - size->z * 0.5f;

	result->Max.x = pos->x + size->x * 0.5f;
	result->Max.y = pos->y + size->y;
	result->Max.z = pos->z + size->z * 0.5f;
}

void AABB_Offset(struct AABB* result, const struct AABB* bb, const Vec3* amount) {
	Vec3_Add(&result->Min, &bb->Min, amount);
	Vec3_Add(&result->Max, &bb->Max, amount);
}

cc_bool AABB_Intersects(const struct AABB* bb, const struct AABB* other) {
	return
		bb->Max.x >= other->Min.x && bb->Min.x <= other->Max.x &&
		bb->Max.y >= other->Min.y && bb->Min.y <= other->Max.y &&
		bb->Max.z >= other->Min.z && bb->Min.z <= other->Max.z;
}

cc_bool AABB_Contains(const struct AABB* parent, const struct AABB* child) {
	return
		child->Min.x >= parent->Min.x && child->Min.y >= parent->Min.y && child->Min.z >= parent->Min.z &&
		child->Max.x <= parent->Max.x && child->Max.y <= parent->Max.y && child->Max.z <= parent->Max.z;
}

cc_bool AABB_ContainsPoint(const struct AABB* parent, const Vec3* P) {
	return
		P->x >= parent->Min.x && P->y >= parent->Min.y && P->z >= parent->Min.z &&
		P->x <= parent->Max.x && P->y <= parent->Max.y && P->z <= parent->Max.z;
}


/*########################################################################################################################*
*------------------------------------------------------Intersection-------------------------------------------------------*
*#########################################################################################################################*/
static Vec3 Intersection_InverseRotate(Vec3 pos, struct Entity* target) {
	pos = Vec3_RotateY(pos, -target->RotY * MATH_DEG2RAD);
	pos = Vec3_RotateZ(pos, -target->RotZ * MATH_DEG2RAD);
	pos = Vec3_RotateX(pos, -target->RotX * MATH_DEG2RAD);
	return pos;
}

cc_bool Intersection_RayIntersectsRotatedBox(Vec3 origin, Vec3 dir, struct Entity* target, float* tMin, float* tMax) {
	Vec3 delta, invDir;
	struct AABB bb;

	/* This is the rotated AABB of the model we want to test for intersection
			  *
			 / \     we then perform a counter       *---*   and we can then do
	   ====>* x *    rotation to the rotated AABB    | x |   a standard AABB test
			 \ /     and ray origin and direction    *---*   with the rotated ray
			  *                                     /
												   /                           
	*/
	Vec3_Sub(&delta, &origin, &target->Position);   /* delta  = origin - target.Position */
	delta = Intersection_InverseRotate(delta, target); /* delta  = UndoRotation(delta) */
	Vec3_Add(&origin, &delta, &target->Position);   /* origin = delta + target.Position */

	dir = Intersection_InverseRotate(dir, target);
	Entity_GetPickingBounds(target, &bb);

	invDir.x = 1.0f / dir.x;
	invDir.y = 1.0f / dir.y;
	invDir.z = 1.0f / dir.z;
	return Intersection_RayIntersectsBox(origin, invDir, bb.Min, bb.Max, tMin, tMax);
}

cc_bool Intersection_RayIntersectsBox(Vec3 origin, Vec3 invDir, Vec3 min, Vec3 max, float* t0, float* t1) {
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	*t0 = 0; *t1 = 0;
	
	if (invDir.x >= 0) {
		tmin = (min.x - origin.x) * invDir.x;
		tmax = (max.x - origin.x) * invDir.x;
	} else {
		tmin = (max.x - origin.x) * invDir.x;
		tmax = (min.x - origin.x) * invDir.x;
	}

	if (invDir.y >= 0) {
		tymin = (min.y - origin.y) * invDir.y;
		tymax = (max.y - origin.y) * invDir.y;
	} else {
		tymin = (max.y - origin.y) * invDir.y;
		tymax = (min.y - origin.y) * invDir.y;
	}

	if (tmin > tymax || tymin > tmax) return false;
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	if (invDir.z >= 0) {
		tzmin = (min.z - origin.z) * invDir.z;
		tzmax = (max.z - origin.z) * invDir.z;
	} else {
		tzmin = (max.z - origin.z) * invDir.z;
		tzmax = (min.z - origin.z) * invDir.z;
	}

	if (tmin > tzmax || tzmin > tmax) return false;
	if (tzmin > tmin) tmin = tzmin;
	if (tzmax < tmax) tmax = tzmax;

	*t0 = tmin; *t1 = tmax;
	return tmin >= 0.0f;
}


/*########################################################################################################################*
*----------------------------------------------------Collisions finder----------------------------------------------------*
*#########################################################################################################################*/
#define SEARCHER_STATES_MIN 64
static struct SearcherState searcherDefaultStates[SEARCHER_STATES_MIN];
static cc_uint32 searcherCapacity = SEARCHER_STATES_MIN;
struct SearcherState* Searcher_States = searcherDefaultStates;

static void Searcher_QuickSort(int left, int right) {
	struct SearcherState* keys = Searcher_States; struct SearcherState key;

	while (left < right) {
		int i = left, j = right;
		float pivot = keys[(i + j) >> 1].tSquared;

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i].tSquared) i++;
			while (pivot < keys[j].tSquared) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(Searcher_QuickSort);
	}
}

int Searcher_FindReachableBlocks(struct Entity* entity, struct AABB* entityBB, struct AABB* entityExtentBB) {
	Vec3 vel = entity->Velocity;
	IVec3 min, max;
	cc_uint32 elements;
	struct SearcherState* curState;
	int count;

	BlockID block;
	struct AABB blockBB;
	float xx, yy, zz, tx, ty, tz;
	int x, y, z;

	Entity_GetBounds(entity, entityBB);
	/* Exact maximum extent the entity can reach, and the equivalent map coordinates. */
	entityExtentBB->Min.x = entityBB->Min.x + (vel.x < 0.0f ? vel.x : 0.0f);
	entityExtentBB->Min.y = entityBB->Min.y + (vel.y < 0.0f ? vel.y : 0.0f);
	entityExtentBB->Min.z = entityBB->Min.z + (vel.z < 0.0f ? vel.z : 0.0f);

	entityExtentBB->Max.x = entityBB->Max.x + (vel.x > 0.0f ? vel.x : 0.0f);
	entityExtentBB->Max.y = entityBB->Max.y + (vel.y > 0.0f ? vel.y : 0.0f);
	entityExtentBB->Max.z = entityBB->Max.z + (vel.z > 0.0f ? vel.z : 0.0f);

	IVec3_Floor(&min, &entityExtentBB->Min);
	IVec3_Floor(&max, &entityExtentBB->Max);
	elements = (max.x - min.x + 1) * (max.y - min.y + 1) * (max.z - min.z + 1);

	if (elements > searcherCapacity) {
		Searcher_Free();
		searcherCapacity = elements;
		Searcher_States  = (struct SearcherState*)Mem_Alloc(elements, sizeof(struct SearcherState), "collision search states");
	}
	curState = Searcher_States;

	/* Order loops so that we minimise cache misses */
	for (y = min.y; y <= max.y; y++) {
		for (z = min.z; z <= max.z; z++) {
			for (x = min.x; x <= max.x; x++) {
				block = World_GetPhysicsBlock(x, y, z);
				if (Blocks.Collide[block] != COLLIDE_SOLID) continue;

				xx = (float)x; yy = (float)y; zz = (float)z;
				blockBB.Min = Blocks.MinBB[block];
				blockBB.Min.x += xx; blockBB.Min.y += yy; blockBB.Min.z += zz;
				blockBB.Max = Blocks.MaxBB[block];
				blockBB.Max.x += xx; blockBB.Max.y += yy; blockBB.Max.z += zz;

				if (!AABB_Intersects(entityExtentBB, &blockBB)) continue; /* necessary for non whole blocks. (slabs) */
				Searcher_CalcTime(&vel, entityBB, &blockBB, &tx, &ty, &tz);
				if (tx > 1.0f || ty > 1.0f || tz > 1.0f) continue;

				curState->x = (x << 3) | (block  & 0x007);
				curState->y = (y << 4) | ((block & 0x078) >> 3);
				curState->z = (z << 3) | ((block & 0x380) >> 7);
				curState->tSquared = tx * tx + ty * ty + tz * tz;
				curState++;
			}
		}
	}

	count = (int)(curState - Searcher_States);
	if (count) Searcher_QuickSort(0, count - 1);
	return count;
}

void Searcher_CalcTime(Vec3* vel, struct AABB *entityBB, struct AABB* blockBB, float* tx, float* ty, float* tz) {
	float dx = vel->x > 0.0f ? blockBB->Min.x - entityBB->Max.x : entityBB->Min.x - blockBB->Max.x;
	float dy = vel->y > 0.0f ? blockBB->Min.y - entityBB->Max.y : entityBB->Min.y - blockBB->Max.y;
	float dz = vel->z > 0.0f ? blockBB->Min.z - entityBB->Max.z : entityBB->Min.z - blockBB->Max.z;

	*tx = vel->x == 0.0f ? MATH_POS_INF : Math_AbsF(dx / vel->x);
	*ty = vel->y == 0.0f ? MATH_POS_INF : Math_AbsF(dy / vel->y);
	*tz = vel->z == 0.0f ? MATH_POS_INF : Math_AbsF(dz / vel->z);

	if (entityBB->Max.x >= blockBB->Min.x && entityBB->Min.x <= blockBB->Max.x) *tx = 0.0f; /* Inlined XIntersects */
	if (entityBB->Max.y >= blockBB->Min.y && entityBB->Min.y <= blockBB->Max.y) *ty = 0.0f; /* Inlined YIntersects */
	if (entityBB->Max.z >= blockBB->Min.z && entityBB->Min.z <= blockBB->Max.z) *tz = 0.0f; /* Inlined ZIntersects */
}

void Searcher_Free(void) {
	if (Searcher_States != searcherDefaultStates) Mem_Free(Searcher_States);
	Searcher_States  = searcherDefaultStates;
	searcherCapacity = SEARCHER_STATES_MIN;
}
