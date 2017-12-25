#include "Searcher.h"
#include "Block.h"
#include "World.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Funcs.h"

void SearcherState_Init(SearcherState* state, Int32 x, Int32 y, Int32 z, BlockID block, Real32 tSquared) {
	state->X = (x << 3) | (block & 0x07);
	state->Y = (y << 3) | (block & 0x38) >> 3;
	state->Z = (z << 3) | (block & 0xC0) >> 6;
	state->tSquared = tSquared;
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

UInt32 Searcher_FindReachableBlocks(Entity* entity, AABB* entityBB, AABB* entityExtentBB) {
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
			Platform_MemFree(Searcher_States);
		}
		Searcher_StatesCount = elements;

		Searcher_States = Platform_MemAlloc(elements * sizeof(SearcherState));
		if (Searcher_States == NULL) {
			ErrorHandler_Fail("Failed to allocate memory for Searcher_FindReachableBlocks");
		}
	}

	/* Order loops so that we minimise cache misses */
	AABB blockBB;
	UInt32 count = 0;
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
				Real32 tSquared = tx * tx + ty * ty + tz * tz;

				SearcherState_Init(curState, x, y, z, block, tSquared);
				curState++;
			}
		}
	}

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
		Platform_MemFree(Searcher_States);
	}
}