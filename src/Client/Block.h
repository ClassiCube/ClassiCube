#ifndef CC_BLOCK_H
#define CC_BLOCK_H
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
#define DrawType_Opaque 0           /* Completely covers blocks behind (e.g. dirt). */
#define DrawType_Transparent 1      /* Blocks behind show (e.g. glass). Pixels either fully visible or invisible. */
#define DrawType_TransparentThick 2 /* Same as Transparent, but all neighbour faces show. (e.g. leaves) */
#define DrawType_Translucent 3      /* Blocks behind show (e.g. water). Pixels blend with other blocks behind. */
#define DrawType_Gas 4              /* Does not show (e.g. air). Can still be collided with. */
#define DrawType_Sprite 5           /* Block renders as an X (e.g. sapling). Pixels either fully visible or invisible. */

/* Describes the interaction a block has with a player when they collide with it. */
typedef UInt8 CollideType;
#define CollideType_Gas 0         /* No interaction when player collides. */
#define CollideType_Liquid 1      /* 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_Solid 2       /* Block completely stops the player when they are moving. */
#define CollideType_Ice 3         /* Block is solid and partially slidable on. */
#define CollideType_SlipperyIce 4 /* Block is solid and fully slidable on. */
#define CollideType_LiquidWater 5 /* Water style 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_LiquidLava 6  /* Lava style 'swimming'/'bobbing' interaction when player collides. */

UInt8 Block_NamesBuffer[String_BufferSize(STRING_SIZE) * BLOCK_COUNT];
#define Block_NamePtr(i) &Block_NamesBuffer[String_BufferSize(STRING_SIZE) * i]

bool Block_BlocksLight[BLOCK_COUNT];
bool Block_FullBright[BLOCK_COUNT];
String Block_Name[BLOCK_COUNT];
PackedCol Block_FogColour[BLOCK_COUNT];
Real32 Block_FogDensity[BLOCK_COUNT];
CollideType Block_Collide[BLOCK_COUNT];
CollideType Block_ExtendedCollide[BLOCK_COUNT];
Real32 Block_SpeedMultiplier[BLOCK_COUNT];
UInt8 Block_LightOffset[BLOCK_COUNT];
DrawType Block_Draw[BLOCK_COUNT];
UInt32 DefinedCustomBlocks[BLOCK_COUNT >> 5];
SoundType Block_DigSounds[BLOCK_COUNT];
SoundType Block_StepSounds[BLOCK_COUNT];
bool Block_Tinted[BLOCK_COUNT];
/* Gets whether the given block has an opaque draw type and is also a full tile block.
Full tile block means Min of (0, 0, 0) and max of (1, 1, 1).*/
bool Block_FullOpaque[BLOCK_COUNT];

#define Block_Tint(col, block)\
if (Block_Tinted[block]) {\
	PackedCol tintCol = Block_FogColour[block];\
	col.R = (UInt8)(col.R * tintCol.R / 255);\
	col.G = (UInt8)(col.G * tintCol.G / 255);\
	col.B = (UInt8)(col.B * tintCol.B / 255);\
}

Vector3 Block_MinBB[BLOCK_COUNT];
Vector3 Block_MaxBB[BLOCK_COUNT];
Vector3 Block_RenderMinBB[BLOCK_COUNT];
Vector3 Block_RenderMaxBB[BLOCK_COUNT];

TextureLoc Block_Textures[BLOCK_COUNT * Face_Count];
bool Block_CanPlace[BLOCK_COUNT];
bool Block_CanDelete[BLOCK_COUNT];

/* Gets a bit flags of faces hidden of two neighbouring blocks. */
UInt8 Block_Hidden[BLOCK_COUNT * BLOCK_COUNT];
/* Gets a bit flags of which faces of a block can stretch with greedy meshing. */
UInt8 Block_CanStretch[BLOCK_COUNT];

void Block_Reset(void);
void Block_Init(void);
void Block_SetDefaultPerms(void);

void Block_SetCollide(BlockID block, UInt8 collide);
void Block_SetDrawType(BlockID block, UInt8 draw);
void Block_ResetProps(BlockID block);

Int32 Block_FindID(STRING_PURE String* name);
bool Block_IsLiquid(BlockID b);

void Block_CalcRenderBounds(BlockID block);
UInt8 Block_CalcLightOffset(BlockID block);
void Block_RecalculateSpriteBB(void);
void Block_RecalculateBB(BlockID block);

void Block_SetSide(TextureLoc texLoc, BlockID blockId);
void Block_SetTex(TextureLoc texLoc, Face face, BlockID blockId);
#define Block_GetTexLoc(block, face) Block_Textures[(block) * Face_Count + face]

void Block_UpdateCullingAll(void);
void Block_UpdateCulling(BlockID block);
/* Returns whether the face at the given face of the block
should be drawn with the neighbour 'other' present on the other side of the face. */
bool Block_IsFaceHidden(BlockID block, BlockID other, Face face);
void Block_SetHidden(BlockID block, BlockID other, Face face, bool value);

/* Attempts to find the rotated block based on the user's orientation and offset on selected block. */
BlockID AutoRotate_RotateBlock(BlockID block);
#endif