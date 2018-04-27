#ifndef CC_BLOCK_H
#define CC_BLOCK_H
#include "BlockID.h"
#include "PackedCol.h"
#include "Vectors.h"
#include "Constants.h"
/* Stores properties and data for blocks.
   Also performs automatic rotation of directional blocks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

enum SOUND {
	SOUND_NONE,  SOUND_WOOD,  SOUND_GRAVEL, SOUND_GRASS, 
	SOUND_STONE, SOUND_METAL, SOUND_GLASS,  SOUND_CLOTH, 
	SOUND_SAND,  SOUND_SNOW,
};

/* Describes how a block is rendered in the world. */
enum DRAWTYPE {
	DRAW_OPAQUE,            /* Completely covers blocks behind (e.g. dirt). */
	DRAW_TRANSPARENT,       /* Blocks behind show (e.g. glass). Pixels either fully visible or invisible. */
	DRAW_TRANSPARENT_THICK, /* Same as Transparent, but all neighbour faces show. (e.g. leaves) */
	DRAW_TRANSLUCENT,       /* Blocks behind show (e.g. water). Pixels blend with other blocks behind. */
	DRAW_GAS,               /* Does not show (e.g. air). Can still be collided with. */
	DRAW_SPRITE,            /* Block renders as an X (e.g. sapling). Pixels either fully visible or invisible. */
};

/* Describes the interaction a block has with a player when they collide with it. */
enum COLLIDETYPE {
	COLLIDE_GAS,          /* No interaction when player collides. */
	COLLIDE_LIQUID,       /* 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_SOLID,        /* Block completely stops the player when they are moving. */
	COLLIDE_ICE,          /* Block is solid and partially slidable on. */
	COLLIDE_SLIPPERY_ICE, /* Block is solid and fully slidable on. */
	COLLIDE_LIQUID_WATER, /* Water style 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_LIQUID_LAVA,  /* Lava style 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_CLIMB_ROPE,   /* Rope/Ladder style climbing interaction when player collides */
};

bool Block_IsLiquid[BLOCK_COUNT];
bool Block_BlocksLight[BLOCK_COUNT];
bool Block_FullBright[BLOCK_COUNT];
PackedCol Block_FogCol[BLOCK_COUNT];
Real32 Block_FogDensity[BLOCK_COUNT];
UInt8 Block_Collide[BLOCK_COUNT];
UInt8 Block_ExtendedCollide[BLOCK_COUNT];
Real32 Block_SpeedMultiplier[BLOCK_COUNT];
UInt8 Block_LightOffset[BLOCK_COUNT];
UInt8 Block_Draw[BLOCK_COUNT];
UInt8 Block_DigSounds[BLOCK_COUNT];
UInt8 Block_StepSounds[BLOCK_COUNT];
bool Block_Tinted[BLOCK_COUNT];
/* Gets whether the given block has an opaque draw type and is also a full tile block.
Full tile block means Min of (0, 0, 0) and max of (1, 1, 1).*/
bool Block_FullOpaque[BLOCK_COUNT];
UInt8 Block_SpriteOffset[BLOCK_COUNT];

#define Block_Tint(col, block)\
if (Block_Tinted[block]) {\
	PackedCol tintCol = Block_FogCol[block];\
	col.R = (UInt8)(col.R * tintCol.R / 255);\
	col.G = (UInt8)(col.G * tintCol.G / 255);\
	col.B = (UInt8)(col.B * tintCol.B / 255);\
}

Vector3 Block_MinBB[BLOCK_COUNT];
Vector3 Block_MaxBB[BLOCK_COUNT];
Vector3 Block_RenderMinBB[BLOCK_COUNT];
Vector3 Block_RenderMaxBB[BLOCK_COUNT];

TextureLoc Block_Textures[BLOCK_COUNT * FACE_COUNT];
bool Block_CanPlace[BLOCK_COUNT];
bool Block_CanDelete[BLOCK_COUNT];

/* Gets a bit flags of faces hidden of two neighbouring blocks. */
UInt8 Block_Hidden[BLOCK_COUNT * BLOCK_COUNT];
/* Gets a bit flags of which faces of a block can stretch with greedy meshing. */
UInt8 Block_CanStretch[BLOCK_COUNT];

void Block_Reset(void);
void Block_Init(void);
void Block_SetDefaultPerms(void);
bool Block_IsCustomDefined(BlockID block);
void Block_SetCustomDefined(BlockID block, bool defined);
void Block_DefineCustom(BlockID block);

void Block_SetCollide(BlockID block, UInt8 collide);
void Block_SetDrawType(BlockID block, UInt8 draw);
void Block_ResetProps(BlockID block);

STRING_REF String Block_UNSAFE_GetName(BlockID block);
void Block_SetName(BlockID block, STRING_PURE String* name);
Int32 Block_FindID(STRING_PURE String* name);

void Block_CalcRenderBounds(BlockID block);
void Block_CalcLightOffset(BlockID block);
void Block_RecalculateSpriteBB(void);
void Block_RecalculateBB(BlockID block);

void Block_SetSide(TextureLoc texLoc, BlockID blockId);
void Block_SetTex(TextureLoc texLoc, Face face, BlockID blockId);
#define Block_GetTexLoc(block, face) Block_Textures[(block) * FACE_COUNT + (face)]

bool Block_IsFaceHidden(BlockID block, BlockID other, Face face);
void Block_UpdateCullingAll(void);
void Block_UpdateCulling(BlockID block);

/* Attempts to find the rotated block based on the user's orientation and offset on selected block. */
BlockID AutoRotate_RotateBlock(BlockID block);

Real32 DefaultSet_Height(BlockID b);
bool DefaultSet_FullBright(BlockID b);
Real32 DefaultSet_FogDensity(BlockID b);
PackedCol DefaultSet_FogColour(BlockID b);
UInt8 DefaultSet_Collide(BlockID b);
/* Gets a backwards compatible collide type of a block. */
UInt8 DefaultSet_MapOldCollide(BlockID b, UInt8 collide);
bool DefaultSet_BlocksLight(BlockID b);
UInt8 DefaultSet_StepSound(BlockID b);
UInt8 DefaultSet_Draw(BlockID b);
UInt8 DefaultSet_DigSound(BlockID b);
#endif