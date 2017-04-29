#ifndef CS_BLOCK_H
#define CS_BLOCK_H
#include "Typedefs.h"
#include "BlockID.h"
#include "String.h"
#include "FastColour.h"
/* Stores properties for blocks
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Sound types for blocks. */

#define SoundType_None 0
#define SoundType_Wood 1
#define SoundType_Gravel 2
#define SoundType_Grass 3
#define SoundType_Stone 4
#define SoundType_Metal 5
#define SoundType_Glass 6
#define SoundType_Cloth 7
#define SoundType_Sand 8
#define SoundType_Snow 9


/* Describes how a block is rendered in the world. */
/* Completely covers blocks behind (e.g. dirt). */
#define DrawType_Opaque 0
/*  Blocks behind show (e.g. glass). Pixels are either fully visible or invisible.  */
#define DrawType_Transparent 1
/*  Same as Transparent, but all neighbour faces show. (e.g. leaves) */
#define DrawType_TransparentThick 2
/* Blocks behind show (e.g. water). Pixels blend with other blocks behind. */
#define DrawType_Translucent 3
/* Does not show (e.g. air). Can still be collided with. */
#define DrawType_Gas 4
/* Block renders as an X sprite (e.g. sapling). Pixels are either fully visible or invisible. */
#define DrawType_Sprite 5


/* Describes the interaction a block has with a player when they collide with it. */
/* No interaction when player collides. */
#define CollideType_Gas 0
/* 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_Liquid 1
/* Block completely stops the player when they are moving. */
#define CollideType_Solid 2
/* Block is solid and partially slidable on. */
#define CollideType_Ice 3
/* Block is solid and fully slidable on. */
#define CollideType_SlipperyIce 4
/* Water style 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_LiquidWater 5
/* Lava style 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_LiquidLava 6

/* Array of block names. */
UInt8 Block_NamesBuffer[String_BufferSize(STRING_SIZE) * Block_Count];

/* Index of given block name. */
#define Block_NamePtr(i) &Block_NamesBuffer[String_BufferSize(STRING_SIZE) * i]


/* Gets whether the given block is a liquid. (water and lava) */
bool Block_IsLiquid(BlockID b) { return b >= BlockID_Water && b <= BlockID_StillLava; }

/* Gets whether the given block stops sunlight. */
bool BlocksLight[Block_Count];

/* Gets whether the given block should draw all its faces in a full white colour. */
bool FullBright[Block_Count];

/* Gets the name of the given block, or 'Invalid' if the block is not defined. */
String Name[Block_Count];

/* Gets the custom fog colour that should be used when the player is standing within this block.
   Note that this is only used for exponential fog mode. */
FastColour FogColour[Block_Count];

/* Gets the fog density for the given block.
   A value of 0 means this block does not apply fog.*/
Real32 FogDensity[Block_Count];

/* Gets the basic collision type for the given block. */
UInt8 Collide[Block_Count];

/* Gets the action performed when colliding with the given block. */
UInt8 ExtendedCollide[Block_Count];

/* Speed modifier when colliding (or standing on for solid collide type) with the given block. */
Real32 SpeedMultiplier[Block_Count];

/* Light offset of each block, as bitflags of 1 per face. */
UInt8 LightOffset[Block_Count];

/* Gets the DrawType for the given block. */
UInt8 Draw[Block_Count];

/* Gets whether the given block has an opaque draw type and is also a full tile block.
   Full tile block means Min of (0, 0, 0) and max of (1, 1, 1).*/
bool FullOpaque[Block_Count];

UInt32 DefinedCustomBlocks[Block_Count >> 5];

/* Gets the dig sound ID for the given block. */
UInt8 DigSounds[Block_Count];

/* Gets the step sound ID for the given block. */
UInt8 StepSounds[Block_Count];

/* Gets whether the given block has a tinting colour applied to it when rendered.
   The tinting colour used is the block's fog colour. */
bool Tinted[Block_Count];

/* Recalculates the initial properties and culling states for all blocks. */
void Block_Reset(Game* game);

/* Calculates the initial properties and culling states for all blocks. */
void Block_Init();

/* Initialises the default blocks the player is allowed to place and delete. */
void Block_SetDefaultPerms(InventoryPermissions place, InventoryPermissions del);

/* Sets collision type of a block. */
void Block_SetCollide(BlockID block, UInt8 collide);

void Block_SetDrawType(BlockID block, UInt8 draw);

/* Resets the properties for the given block to their defaults. */
void Block_ResetProps(BlockID block);

/* Finds the ID of the block whose name caselessly matches the input, -1 otherwise. */
Int32 Block_FindID(String* name);

/* Gets the default name of a block. */
static String Block_DefaultName(BlockID block);

static void Block_SplitUppercase(String* buffer, String* blockNames, Int32 start, Int32 end);

#endif