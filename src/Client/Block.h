#ifndef CS_BLOCK_H
#define CS_BLOCK_H
#include "Typedefs.h"
#include "BlockID.h"
#include "String.h"
#include "PackedCol.h"
#include "Game.h"
#include "Vectors.h"
#include "Bitmap.h"
#include "Constants.h"
#include "Compiler.h"
/* Stores properties and data for blocks.
   Also performs automatic rotation of directional blocks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Sound types for blocks. */
typedef UInt8 SoundType;
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
typedef UInt8 DrawType;
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
typedef UInt8 CollideType;
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

/* Gets whether the given block stops sunlight. */
bool Block_BlocksLight[Block_Count];
/* Gets whether the given block should draw all its faces in a full white colour. */
bool Block_FullBright[Block_Count];
/* Gets the name of the given block, or 'Invalid' if the block is not defined. */
String Block_Name[Block_Count];
/* Gets the custom fog colour that should be used when the player is standing within this block.
   Note that this is only used for exponential fog mode. */
PackedCol Block_FogColour[Block_Count];
/* Gets the fog density for the given block.
   A value of 0 means this block does not apply fog.*/
Real32 Block_FogDensity[Block_Count];
/* Gets the basic collision type for the given block. */
CollideType Block_Collide[Block_Count];
/* Gets the action performed when colliding with the given block. */
CollideType Block_ExtendedCollide[Block_Count];
/* Speed modifier when colliding (or standing on for solid collide type) with the given block. */
Real32 Block_SpeedMultiplier[Block_Count];
/* Light offset of each block, as bitflags of 1 per face. */
UInt8 Block_LightOffset[Block_Count];
/* Gets the DrawType for the given block. */
DrawType Block_Draw[Block_Count];
/* Gets whether the given block has an opaque draw type and is also a full tile block.
   Full tile block means Min of (0, 0, 0) and max of (1, 1, 1).*/
bool Block_FullOpaque[Block_Count];
UInt32 DefinedCustomBlocks[Block_Count >> 5];
/* Gets the dig sound ID for the given block. */
SoundType Block_DigSounds[Block_Count];
/* Gets the step sound ID for the given block. */
SoundType Block_StepSounds[Block_Count];
/* Gets whether the given block has a tinting colour applied to it when rendered.
   The tinting colour used is the block's fog colour. */
bool Block_Tinted[Block_Count];

#define Block_Tint(col, block)\
if (Block_Tinted[block]) {\
	PackedCol tintCol = Block_FogColour[block];\
	col.R = (UInt8)(col.R * tintCol.R / 255);\
	col.G = (UInt8)(col.G * tintCol.G / 255);\
	col.B = (UInt8)(col.B * tintCol.B / 255);\
}


/* Min corner of a block. */
Vector3 Block_MinBB[Block_Count];
/* Max corner of a block. */
Vector3 Block_MaxBB[Block_Count];
/* Rendered min corner of a block. */
Vector3 Block_RenderMinBB[Block_Count];
/* Rendered max corner of a block. */
Vector3 Block_RenderMaxBB[Block_Count];

#define Block_TexturesCount Block_Count * Face_Count
/* Raw texture ids of each face of each block. */
TextureLoc Block_Textures[Block_TexturesCount];
/* Returns whether player is allowed to place the given block. */
bool Block_CanPlace[Block_Count];
/* Returns whether player is allowed to delete the given block. */
bool Block_CanDelete[Block_Count];

/* Gets a bit flags of faces hidden of two neighbouring blocks. */
UInt8 Block_Hidden[Block_Count * Block_Count];
/* Gets a bit flags of which faces of a block can stretch with greedy meshing. */
UInt8 Block_CanStretch[Block_Count];

/* Recalculates the initial properties and culling states for all blocks. */
void Block_Reset(void);
/* Calculates the initial properties and culling states for all blocks. */
void Block_Init(void);
/* Initialises the default blocks the player is allowed to place and delete. */
void Block_SetDefaultPerms(void);

/* Sets collision type of a block. */
void Block_SetCollide(BlockID block, UInt8 collide);
/* Sets draw method of a block. */
void Block_SetDrawType(BlockID block, UInt8 draw);
/* Resets the properties for the given block to their defaults. */
void Block_ResetProps(BlockID block);

/* Finds the ID of the block whose name caselessly matches the input, -1 otherwise. */
Int32 Block_FindID(STRING_TRANSIENT String* name);
/* Gets whether the given block is a liquid. (water and lava) */
bool Block_IsLiquid(BlockID b);

/* Calculates rendered corners of a block. */
void Block_CalcRenderBounds(BlockID block);
/* Calculates the light offset of all six faces of a block. */
UInt8 Block_CalcLightOffset(BlockID block);
/* Recalculates bounding boxes of all blocks that are sprites. */
void Block_RecalculateSpriteBB(void);
/* Recalculates bounding box of a sprite. */
void Block_RecalculateBB(BlockID block);

/* Sets the texture for the four side faces of the given block. */
void Block_SetSide(TextureLoc texLoc, BlockID blockId);
/* Sets the texture for the given face of the given block. */
void Block_SetTex(TextureLoc texLoc, Face face, BlockID blockId);
/* Gets the texture ID of the given face of the given block. */
#define Block_GetTexLoc(block, face) Block_Textures[(block) * Face_Count + face]

/* Recalculates culling state for all blocks. */
void Block_UpdateCullingAll(void);
/* Recalculates culling state for the given blocks. */
void Block_UpdateCulling(BlockID block);
/* Returns whether the face at the given face of the block
should be drawn with the neighbour 'other' present on the other side of the face. */
bool Block_IsFaceHidden(BlockID block, BlockID other, Face face);

void Block_SetHidden(BlockID block, BlockID other, Face face, bool value);

/* Attempts to find the rotated block based on the user's orientation and offset on selected block. */
BlockID AutoRotate_RotateBlock(BlockID block);
#endif