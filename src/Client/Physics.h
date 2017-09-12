#ifndef CS_PHYSICS_H
#define CS_PHYSICS_H
#include "Typedefs.h"
#include "BlockID.h"
#include "Vectors.h"
#include "TickQueue.h"
/* Implements simple block physics.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents a handler for physics actions. */
typedef void (*PhysicsHandler)(Int32 index, BlockID block);
/* Handles when a block is activated. (E.g. by a neighbouring block changing) */
PhysicsHandler Physics_OnActivate[Block_Count];
/* Handles when a block is randomly ticked. */
PhysicsHandler Physics_OnRandomTick[Block_Count];
/* Handles user placing a block. */
PhysicsHandler Physics_OnPlace[Block_Count];
/* Handles user deleting a block.*/
PhysicsHandler Physics_OnDelete[Block_Count];

/* Whether physics is enabled or not.*/
bool Physics_Enabled;
/* Sets whether physics is enabled or not, also resetting state. */
void Physics_SetEnabled(bool enabled);

/* Initalises physics state and hooks into events. */
void Physics_Init(void);
/* Clears physics state and unhooks from events.*/
void Physics_Free(void);
/* Performs a physics tick.*/
void Physics_Tick(void);

/* Activates the direct neighbouring blocks of the given coordinates.*/
void Physics_ActivateNeighbours(Int32 x, Int32 y, Int32 z, Int32 index);
/* Activates the block at the given coordinates. */
void Physics_Activate(Int32 index);
/* Returns whether edge water should flood in at the given coords. */
bool Physics_IsEdgeWater(Int32 x, Int32 y, Int32 z);
#endif