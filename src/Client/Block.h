#ifndef CS_BLOCK_H
#define CS_BLOCK_H
#include "Typedefs.h"
#include "BlockID.h"
#include "BlockEnums.h"
#include "String.h"
#include "PackedCol.h"
#include "Game.h"
#include "Vectors.h"
#include "Bitmap.h"
#include "Constants.h"
#include "Compiler.h"
/* Stores properties for blocks
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


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


static String Block_DefaultName(BlockID block);

static void Block_SplitUppercase(STRING_TRANSIENT String* buffer, STRING_TRANSIENT String* blockNames, 
	Int32 start, Int32 end);


static Real32 Block_GetSpriteBB_TopY(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp);

static Real32 Block_GetSpriteBB_BottomY(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp);

static Real32 Block_GetSpriteBB_LeftX(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp);

static Real32 Block_GetSpriteBB_RightX(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp);


static void Block_GetTextureRegion(BlockID block, Face face, Vector2* min, Vector2* max);

static bool Block_FaceOccluded(BlockID block, BlockID other, Face face);


static void Block_CalcCulling(BlockID block, BlockID other);

void Block_SetHidden(BlockID block, BlockID other, Face face, bool value);

static bool Block_IsHidden(BlockID block, BlockID other);

static void Block_SetXStretch(BlockID block, bool stretch);

static void Block_SetZStretch(BlockID block, bool stretch);


static TextureLoc topTex[Block_CpeCount] = { 0,  1,  0,  2, 16,  4, 15, 17, 14, 14,
30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 24, 23,  6,  6,  7,  9,  4,
36, 37, 16, 11, 25, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 26, 53, 52, };

static TextureLoc sideTex[Block_CpeCount] = { 0,  1,  3,  2, 16,  4, 15, 17, 14, 14,
30, 30, 18, 19, 32, 33, 34, 20, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 40, 39,  5,  5,  7,  8, 35,
36, 37, 16, 11, 41, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 42, 53, 52, };

static TextureLoc bottomTex[Block_CpeCount] = { 0,  1,  2,  2, 16,  4, 15, 17, 14, 14,
30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 56, 55,  6,  6,  7, 10,  4,
36, 37, 16, 11, 57, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 58, 53, 52 };

#endif