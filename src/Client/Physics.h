#ifndef CS_PHYSICS_H
#define CS_PHYSICS_H
#include "Typedefs.h"
#include "BlockID.h"
#include "Vectors.h"
/* Implements simple block physics.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef void (*PhysicsHandler)(Int32 index, BlockID block);
PhysicsHandler Physics_OnActivate[Block_Count];
PhysicsHandler Physics_OnRandomTick[Block_Count];
PhysicsHandler Physics_OnPlace[Block_Count];
PhysicsHandler Physics_OnDelete[Block_Count];

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