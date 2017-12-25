#ifndef CC_SEARCHER_H
#define CC_SEARCHER_H
#include "Typedefs.h"
#include "Entity.h"

/* Calculates all possible blocks that a moving entity can intersect with.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct SearcherState_ {
	Int32 X, Y, Z;
	Real32 tSquared;
} SearcherState;
extern SearcherState* Searcher_States;

UInt32 Searcher_FindReachableBlocks(Entity* entity, AABB* entityBB, AABB* entityExtentBB);
void Searcher_CalcTime(Vector3* vel, AABB *entityBB, AABB* blockBB, Real32* tx, Real32* ty, Real32* tz);
void Searcher_Free(void);
#endif