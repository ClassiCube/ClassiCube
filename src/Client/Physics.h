#ifndef CS_PHYSICS_H
#define CS_PHYSICS_H
#include "Typedefs.h"
#include "BlockID.h"
#include "Vector3I.h"
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


static void Physics_BlockChanged(Vector3I p, BlockID oldBlock, BlockID block);

static void Physics_OnNewMapLoaded(void);

static void Physics_TickRandomBlocks(void);


static void Physics_DoFalling(Int32 index, BlockID block);

static bool Physics_CheckItem(TickQueue* queue, Int32* posIndex);


static void Physics_HandleSapling(Int32 index, BlockID block);

static void Physics_HandleDirt(Int32 index, BlockID block);

static void Physics_HandleGrass(Int32 index, BlockID block);

static void Physics_HandleFlower(Int32 index, BlockID block);

static void Physics_HandleMushroom(Int32 index, BlockID block);


static void Physics_TickLava(void);

static void Physics_PlaceLava(Int32 index, BlockID block);

static void Physics_ActivateLava(Int32 index, BlockID block);

static void Physics_PropagateLava(Int32 posIndex, Int32 x, Int32 y, Int32 z);


static void Physics_TickWater(void);

static void Physics_PlaceWater(Int32 index, BlockID block);

static void Physics_ActivateWater(Int32 index, BlockID block);

static void Physics_PropagateWater(Int32 posIndex, Int32 x, Int32 y, Int32 z);


static void Physics_PlaceSponge(Int32 index, BlockID block);

static void Physics_DeleteSponge(Int32 index, BlockID block);


static void Physics_HandleSlab(Int32 index, BlockID block);

static void Physics_HandleCobblestoneSlab(Int32 index, BlockID block);

static void Physics_HandleTnt(Int32 index, BlockID block);

static void Physics_Explode(Int32 x, Int32 y, Int32 z, Int32 power);
#endif