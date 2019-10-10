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
	result->Min.X = pos->X - size->X * 0.5f;
	result->Min.Y = pos->Y;
	result->Min.Z = pos->Z - size->Z * 0.5f;

	result->Max.X = pos->X + size->X * 0.5f;
	result->Max.Y = pos->Y + size->Y;
	result->Max.Z = pos->Z + size->Z * 0.5f;
}

void AABB_Offset(struct AABB* result, const struct AABB* bb, const Vec3* amount) {
	Vec3_Add(&result->Min, &bb->Min, amount);
	Vec3_Add(&result->Max, &bb->Max, amount);
}

cc_bool AABB_Intersects(const struct AABB* bb, const struct AABB* other) {
	return
		bb->Max.X >= other->Min.X && bb->Min.X <= other->Max.X &&
		bb->Max.Y >= other->Min.Y && bb->Min.Y <= other->Max.Y &&
		bb->Max.Z >= other->Min.Z && bb->Min.Z <= other->Max.Z;
}

cc_bool AABB_Contains(const struct AABB* parent, const struct AABB* child) {
	return
		child->Min.X >= parent->Min.X && child->Min.Y >= parent->Min.Y && child->Min.Z >= parent->Min.Z &&
		child->Max.X <= parent->Max.X && child->Max.Y <= parent->Max.Y && child->Max.Z <= parent->Max.Z;
}

cc_bool AABB_ContainsPoint(const struct AABB* parent, const Vec3* P) {
	return
		P->X >= parent->Min.X && P->Y >= parent->Min.Y && P->Z >= parent->Min.Z &&
		P->X <= parent->Max.X && P->Y <= parent->Max.Y && P->Z <= parent->Max.Z;
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
	Vec3 delta;
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
	return Intersection_RayIntersectsBox(origin, dir, bb.Min, bb.Max, tMin, tMax);
}

cc_bool Intersection_RayIntersectsBox(Vec3 origin, Vec3 dir, Vec3 min, Vec3 max, float* t0, float* t1) {
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	float invDirX, invDirY, invDirZ;
	*t0 = 0; *t1 = 0;
	
	invDirX = 1.0f / dir.X;
	if (invDirX >= 0) {
		tmin = (min.X - origin.X) * invDirX;
		tmax = (max.X - origin.X) * invDirX;
	} else {
		tmin = (max.X - origin.X) * invDirX;
		tmax = (min.X - origin.X) * invDirX;
	}

	invDirY = 1.0f / dir.Y;
	if (invDirY >= 0) {
		tymin = (min.Y - origin.Y) * invDirY;
		tymax = (max.Y - origin.Y) * invDirY;
	} else {
		tymin = (max.Y - origin.Y) * invDirY;
		tymax = (min.Y - origin.Y) * invDirY;
	}

	if (tmin > tymax || tymin > tmax) return false;
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	invDirZ = 1.0f / dir.Z;
	if (invDirZ >= 0) {
		tzmin = (min.Z - origin.Z) * invDirZ;
		tzmax = (max.Z - origin.Z) * invDirZ;
	} else {
		tzmin = (max.Z - origin.Z) * invDirZ;
		tzmax = (min.Z - origin.Z) * invDirZ;
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
	entityExtentBB->Min.X = entityBB->Min.X + (vel.X < 0.0f ? vel.X : 0.0f);
	entityExtentBB->Min.Y = entityBB->Min.Y + (vel.Y < 0.0f ? vel.Y : 0.0f);
	entityExtentBB->Min.Z = entityBB->Min.Z + (vel.Z < 0.0f ? vel.Z : 0.0f);

	entityExtentBB->Max.X = entityBB->Max.X + (vel.X > 0.0f ? vel.X : 0.0f);
	entityExtentBB->Max.Y = entityBB->Max.Y + (vel.Y > 0.0f ? vel.Y : 0.0f);
	entityExtentBB->Max.Z = entityBB->Max.Z + (vel.Z > 0.0f ? vel.Z : 0.0f);

	IVec3_Floor(&min, &entityExtentBB->Min);
	IVec3_Floor(&max, &entityExtentBB->Max);
	elements = (max.X - min.X + 1) * (max.Y - min.Y + 1) * (max.Z - min.Z + 1);

	if (elements > searcherCapacity) {
		Searcher_Free();
		searcherCapacity = elements;
		Searcher_States  = (struct SearcherState*)Mem_Alloc(elements, sizeof(struct SearcherState), "collision search states");
	}
	curState = Searcher_States;

	/* Order loops so that we minimise cache misses */
	for (y = min.Y; y <= max.Y; y++) {
		for (z = min.Z; z <= max.Z; z++) {
			for (x = min.X; x <= max.X; x++) {
				block = World_GetPhysicsBlock(x, y, z);
				if (Blocks.Collide[block] != COLLIDE_SOLID) continue;

				xx = (float)x; yy = (float)y; zz = (float)z;
				blockBB.Min = Blocks.MinBB[block];
				blockBB.Min.X += xx; blockBB.Min.Y += yy; blockBB.Min.Z += zz;
				blockBB.Max = Blocks.MaxBB[block];
				blockBB.Max.X += xx; blockBB.Max.Y += yy; blockBB.Max.Z += zz;

				if (!AABB_Intersects(entityExtentBB, &blockBB)) continue; /* necessary for non whole blocks. (slabs) */
				Searcher_CalcTime(&vel, entityBB, &blockBB, &tx, &ty, &tz);
				if (tx > 1.0f || ty > 1.0f || tz > 1.0f) continue;

				curState->X = (x << 3) | (block  & 0x007);
				curState->Y = (y << 4) | ((block & 0x078) >> 3);
				curState->Z = (z << 3) | ((block & 0x380) >> 7);
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
	float dx = vel->X > 0.0f ? blockBB->Min.X - entityBB->Max.X : entityBB->Min.X - blockBB->Max.X;
	float dy = vel->Y > 0.0f ? blockBB->Min.Y - entityBB->Max.Y : entityBB->Min.Y - blockBB->Max.Y;
	float dz = vel->Z > 0.0f ? blockBB->Min.Z - entityBB->Max.Z : entityBB->Min.Z - blockBB->Max.Z;

	*tx = vel->X == 0.0f ? MATH_POS_INF : Math_AbsF(dx / vel->X);
	*ty = vel->Y == 0.0f ? MATH_POS_INF : Math_AbsF(dy / vel->Y);
	*tz = vel->Z == 0.0f ? MATH_POS_INF : Math_AbsF(dz / vel->Z);

	if (entityBB->Max.X >= blockBB->Min.X && entityBB->Min.X <= blockBB->Max.X) *tx = 0.0f; /* Inlined XIntersects */
	if (entityBB->Max.Y >= blockBB->Min.Y && entityBB->Min.Y <= blockBB->Max.Y) *ty = 0.0f; /* Inlined YIntersects */
	if (entityBB->Max.Z >= blockBB->Min.Z && entityBB->Min.Z <= blockBB->Max.Z) *tz = 0.0f; /* Inlined ZIntersects */
}

void Searcher_Free(void) {
	if (Searcher_States != searcherDefaultStates) Mem_Free(Searcher_States);
	Searcher_States  = searcherDefaultStates;
	searcherCapacity = SEARCHER_STATES_MIN;
}
