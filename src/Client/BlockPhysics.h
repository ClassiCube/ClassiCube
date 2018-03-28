#ifndef CC_BLOCKPHYSICS_H
#define CC_BLOCKPHYSICS_H
#include "Typedefs.h"
#include "BlockID.h"
/* Implements simple block physics.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef void (*PhysicsHandler)(Int32 index, BlockID block);
PhysicsHandler Physics_OnActivate[BLOCK_COUNT];
PhysicsHandler Physics_OnRandomTick[BLOCK_COUNT];
PhysicsHandler Physics_OnPlace[BLOCK_COUNT];
PhysicsHandler Physics_OnDelete[BLOCK_COUNT];

bool Physics_Enabled;
void Physics_SetEnabled(bool enabled);
void Physics_Init(void);
void Physics_Free(void);
void Physics_Tick(void);

void Physics_ActivateNeighbours(Int32 x, Int32 y, Int32 z, Int32 index);
void Physics_Activate(Int32 index);
bool Physics_IsEdgeWater(Int32 x, Int32 y, Int32 z);
#endif