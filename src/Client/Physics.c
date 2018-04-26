#include "Physics.h"
#include "ExtMath.h"
#include "Block.h"
#include "World.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Entity.h"

void AABB_FromCoords6(AABB* result, Real32 x1, Real32 y1, Real32 z1, Real32 x2, Real32 y2, Real32 z2) {
	result->Min = Vector3_Create3(x1, y1, z1); result->Max = Vector3_Create3(x2, y2, z2);
}

void AABB_FromCoords(AABB* result, Vector3* min, Vector3* max) {
	result->Min = *min; result->Max = *max;
}

void AABB_Make(AABB* result, Vector3* pos, Vector3* size) {
	result->Min.X = pos->X - size->X * 0.5f;
	result->Min.Y = pos->Y;
	result->Min.Z = pos->Z - size->Z * 0.5f;

	result->Max.X = pos->X + size->X * 0.5f;
	result->Max.Y = pos->Y + size->Y;
	result->Max.Z = pos->Z + size->Z * 0.5f;
}

void AABB_Offset(AABB* result, AABB* bb, Vector3* amount) {
	Vector3_Add(&result->Min, &bb->Min, amount);
	Vector3_Add(&result->Max, &bb->Max, amount);
}

bool AABB_Intersects(AABB* bb, AABB* other) {
	if (bb->Max.X >= other->Min.X && bb->Min.X <= other->Max.X) {
		if (bb->Max.Y < other->Min.Y || bb->Min.Y > other->Max.Y) {
			return false;
		}
		return bb->Max.Z >= other->Min.Z && bb->Min.Z <= other->Max.Z;
	}
	return false;
}

bool AABB_Contains(AABB* parent, AABB* child) {
	return
		child->Min.X >= parent->Min.X && child->Min.Y >= parent->Min.Y && child->Min.Z >= parent->Min.Z &&
		child->Max.X <= parent->Max.X && child->Max.Y <= parent->Max.Y && child->Max.Z <= parent->Max.Z;
}

bool AABB_ContainsPoint(AABB* parent, Vector3* P) {
	return
		P->X >= parent->Min.X && P->Y >= parent->Min.Y && P->Z >= parent->Min.Z &&
		P->X <= parent->Max.X && P->Y <= parent->Max.Y && P->Z <= parent->Max.Z;
}



Vector3 Intersection_InverseRotate(Vector3 pos, Entity* target) {
	pos = Vector3_RotateY(pos, -target->RotY * MATH_DEG2RAD);
	pos = Vector3_RotateZ(pos, -target->RotZ * MATH_DEG2RAD);
	pos = Vector3_RotateX(pos, -target->RotX * MATH_DEG2RAD);
	return pos;
}

bool Intersection_RayIntersectsRotatedBox(Vector3 origin, Vector3 dir, Entity* target, Real32* tMin, Real32* tMax) {
	/* This is the rotated AABB of the model we want to test for intersection
			  *
			 / \     we then perform a counter       *---*   and we can then do
	   ====>* x *    rotation to the rotated AABB    | x |   a standard AABB test
			 \ /     and ray origin and direction    *---*   with the rotated ray
			  *                                     /
												   /                           
	*/
	Vector3 delta; Vector3_Sub(&delta, &origin, &target->Position); /* delta  = origin - target.Position */
	delta = Intersection_InverseRotate(delta, target);                   /* delta  = UndoRotation(delta) */
	Vector3_Add(&origin, &delta, &target->Position);                     /* origin = delta + target.Position */

	dir = Intersection_InverseRotate(dir, target);
	AABB bb;
	Entity_GetPickingBounds(target, &bb);
	return Intersection_RayIntersectsBox(origin, dir, bb.Min, bb.Max, tMin, tMax);
}

bool Intersection_RayIntersectsBox(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, Real32* t0, Real32* t1) {
	*t0 = 0; *t1 = 0;
	Real32 tmin, tmax, tymin, tymax, tzmin, tzmax;
	Real32 invDirX = 1.0f / dir.X;
	if (invDirX >= 0) {
		tmin = (min.X - origin.X) * invDirX;
		tmax = (max.X - origin.X) * invDirX;
	} else {
		tmin = (max.X - origin.X) * invDirX;
		tmax = (min.X - origin.X) * invDirX;
	}

	Real32 invDirY = 1.0f / dir.Y;
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

	Real32 invDirZ = 1.0f / dir.Z;
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
	return *t0 >= 0;
}


#define SEARCHER_STATES_MIN 64
SearcherState Searcher_StatesInitial[SEARCHER_STATES_MIN];
extern SearcherState* Searcher_States = Searcher_StatesInitial;
UInt32 Searcher_StatesCount = SEARCHER_STATES_MIN;

void Searcher_QuickSort(Int32 left, Int32 right) {
	SearcherState* keys = Searcher_States; SearcherState key;
	while (left < right) {
		Int32 i = left, j = right;
		Real32 pivot = keys[(i + j) >> 1].tSquared;

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

Int32 Searcher_FindReachableBlocks(Entity* entity, AABB* entityBB, AABB* entityExtentBB) {
	Vector3 vel = entity->Velocity;
	Entity_GetBounds(entity, entityBB);

	/* Exact maximum extent the entity can reach, and the equivalent map coordinates. */
	entityExtentBB->Min.X = entityBB->Min.X + (vel.X < 0.0f ? vel.X : 0.0f);
	entityExtentBB->Min.Y = entityBB->Min.Y + (vel.Y < 0.0f ? vel.Y : 0.0f);
	entityExtentBB->Min.Z = entityBB->Min.Z + (vel.Z < 0.0f ? vel.Z : 0.0f);

	entityExtentBB->Max.X = entityBB->Max.X + (vel.X > 0.0f ? vel.X : 0.0f);
	entityExtentBB->Max.Y = entityBB->Max.Y + (vel.Y > 0.0f ? vel.Y : 0.0f);
	entityExtentBB->Max.Z = entityBB->Max.Z + (vel.Z > 0.0f ? vel.Z : 0.0f);

	Vector3I min, max;
	Vector3I_Floor(&min, &entityExtentBB->Min);
	Vector3I_Floor(&max, &entityExtentBB->Max);

	UInt32 elements = (max.X - min.X + 1) * (max.Y - min.Y + 1) * (max.Z - min.Z + 1);
	if (elements > Searcher_StatesCount) {
		if (Searcher_StatesCount > SEARCHER_STATES_MIN) {
			Platform_MemFree(&Searcher_States);
		}
		Searcher_StatesCount = elements;

		Searcher_States = Platform_MemAlloc(elements, sizeof(SearcherState));
		if (Searcher_States == NULL) {
			ErrorHandler_Fail("Failed to allocate memory for Searcher_FindReachableBlocks");
		}
	}

	/* Order loops so that we minimise cache misses */
	AABB blockBB;
	Int32 x, y, z;
	SearcherState* curState = Searcher_States;

	for (y = min.Y; y <= max.Y; y++) {
		for (z = min.Z; z <= max.Z; z++) {
			for (x = min.X; x <= max.X; x++) {
				BlockID block = World_GetPhysicsBlock(x, y, z);
				if (Block_Collide[block] != COLLIDE_SOLID) continue;
				Real32 xx = (Real32)x, yy = (Real32)y, zz = (Real32)z;

				blockBB.Min = Block_MinBB[block];
				blockBB.Min.X += xx; blockBB.Min.Y += yy; blockBB.Min.Z += zz;
				blockBB.Max = Block_MaxBB[block];
				blockBB.Max.X += xx; blockBB.Max.Y += yy; blockBB.Max.Z += zz;

				if (!AABB_Intersects(entityExtentBB, &blockBB)) continue; /* necessary for non whole blocks. (slabs) */

				Real32 tx, ty, tz;
				Searcher_CalcTime(&vel, entityBB, &blockBB, &tx, &ty, &tz);
				if (tx > 1.0f || ty > 1.0f || tz > 1.0f) continue;

				curState->X = (x << 3) | (block  & 0x07);
				curState->Y = (y << 3) | ((block & 0x38) >> 3);
				curState->Z = (z << 3) | ((block & 0xC0) >> 6);
				curState->tSquared = tx * tx + ty * ty + tz * tz;
				curState++;
			}
		}
	}

	Int32 count = (Int32)(curState - Searcher_States);
	if (count > 0) Searcher_QuickSort(0, count - 1);
	return count;
}

void Searcher_CalcTime(Vector3* vel, AABB *entityBB, AABB* blockBB, Real32* tx, Real32* ty, Real32* tz) {
	Real32 dx = vel->X > 0.0f ? blockBB->Min.X - entityBB->Max.X : entityBB->Min.X - blockBB->Max.X;
	Real32 dy = vel->Y > 0.0f ? blockBB->Min.Y - entityBB->Max.Y : entityBB->Min.Y - blockBB->Max.Y;
	Real32 dz = vel->Z > 0.0f ? blockBB->Min.Z - entityBB->Max.Z : entityBB->Min.Z - blockBB->Max.Z;

	*tx = vel->X == 0.0f ? MATH_POS_INF : Math_AbsF(dx / vel->X);
	*ty = vel->Y == 0.0f ? MATH_POS_INF : Math_AbsF(dy / vel->Y);
	*tz = vel->Z == 0.0f ? MATH_POS_INF : Math_AbsF(dz / vel->Z);

	if (entityBB->Max.X >= blockBB->Min.X && entityBB->Min.X <= blockBB->Max.X) *tx = 0.0f; /* Inlined XIntersects */
	if (entityBB->Max.Y >= blockBB->Min.Y && entityBB->Min.Y <= blockBB->Max.Y) *ty = 0.0f; /* Inlined YIntersects */
	if (entityBB->Max.Z >= blockBB->Min.Z && entityBB->Min.Z <= blockBB->Max.Z) *tz = 0.0f; /* Inlined ZIntersects */
}

void Searcher_Free(void) {
	if (Searcher_StatesCount > SEARCHER_STATES_MIN) {
		Platform_MemFree(&Searcher_States);
	}
}