#ifndef CC_PHYSICS_H
#define CC_PHYSICS_H
#include "Typedefs.h"
#include "BlockID.h"
#include "Vectors.h"
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

/* Activates the direct neighbouring blocks of the given coordinates.*/
void Physics_ActivateNeighbours(Int32 x, Int32 y, Int32 z, Int32 index);
/* Activates the block at the given coordinates. */
void Physics_Activate(Int32 index);
/* Returns whether edge water should flood in at the given coords. */
bool Physics_IsEdgeWater(Int32 x, Int32 y, Int32 z);
#endif