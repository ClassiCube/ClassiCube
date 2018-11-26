#ifndef CC_BLOCK_H
#define CC_BLOCK_H
#include "PackedCol.h"
#include "Vectors.h"
#include "Constants.h"
#include "BlockID.h"
/* Stores properties and data for blocks.
   Also performs automatic rotation of directional blocks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Blocks_Component;

typedef enum SoundType_ {
	SOUND_NONE,  SOUND_WOOD,  SOUND_GRAVEL, SOUND_GRASS, 
	SOUND_STONE, SOUND_METAL, SOUND_GLASS,  SOUND_CLOTH, 
	SOUND_SAND,  SOUND_SNOW,  SOUND_COUNT
} SoundType;
extern const char* Sound_Names[SOUND_COUNT];

/* Describes how a block is rendered in the world. */
typedef enum DrawType_ {
	DRAW_OPAQUE,            /* Completely covers blocks behind (e.g. dirt). */
	DRAW_TRANSPARENT,       /* Blocks behind show (e.g. glass). Pixels either fully visible or invisible. */
	DRAW_TRANSPARENT_THICK, /* Same as Transparent, but all neighbour faces show. (e.g. leaves) */
	DRAW_TRANSLUCENT,       /* Blocks behind show (e.g. water). Pixels blend with other blocks behind. */
	DRAW_GAS,               /* Does not show (e.g. air). Can still be collided with. */
	DRAW_SPRITE             /* Block renders as an X (e.g. sapling). Pixels either fully visible or invisible. */
} DrawType;

/* Describes the interaction a block has with a player when they collide with it. */
typedef enum CollideType_ {
	COLLIDE_GAS,          /* No interaction when player collides. */
	COLLIDE_LIQUID,       /* 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_SOLID,        /* Block completely stops the player when they are moving. */
	COLLIDE_ICE,          /* Block is solid and partially slidable on. */
	COLLIDE_SLIPPERY_ICE, /* Block is solid and fully slidable on. */
	COLLIDE_LIQUID_WATER, /* Water style 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_LIQUID_LAVA,  /* Lava style 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_CLIMB_ROPE    /* Rope/Ladder style climbing interaction when player collides. */
} CollideType;

bool Block_IsLiquid[BLOCK_COUNT];
/* Whether a block prevents lights from passing through it. */
bool Block_BlocksLight[BLOCK_COUNT];
bool Block_FullBright[BLOCK_COUNT];
PackedCol Block_FogCol[BLOCK_COUNT];
float Block_FogDensity[BLOCK_COUNT];
/* Basic collide type of a block. (gas, liquid, or solid) */
uint8_t Block_Collide[BLOCK_COUNT];
uint8_t Block_ExtendedCollide[BLOCK_COUNT];
float Block_SpeedMultiplier[BLOCK_COUNT];
uint8_t Block_LightOffset[BLOCK_COUNT];
uint8_t Block_Draw[BLOCK_COUNT];
uint8_t Block_DigSounds[BLOCK_COUNT];
uint8_t Block_StepSounds[BLOCK_COUNT];
uint8_t Block_Tinted[BLOCK_COUNT];
/* Whether a block has an opaque draw type, a min of (0,0,0), and a max of (1,1,1) */
bool Block_FullOpaque[BLOCK_COUNT];
uint8_t Block_SpriteOffset[BLOCK_COUNT];

#define Block_Tint(col, block)\
if (Block_Tinted[block]) {\
	PackedCol tintCol = Block_FogCol[block];\
	col.R = (uint8_t)(col.R * tintCol.R / 255);\
	col.G = (uint8_t)(col.G * tintCol.G / 255);\
	col.B = (uint8_t)(col.B * tintCol.B / 255);\
}

Vector3 Block_MinBB[BLOCK_COUNT];
Vector3 Block_MaxBB[BLOCK_COUNT];
Vector3 Block_RenderMinBB[BLOCK_COUNT];
Vector3 Block_RenderMaxBB[BLOCK_COUNT];

TextureLoc Block_Textures[BLOCK_COUNT * FACE_COUNT];
bool Block_CanPlace[BLOCK_COUNT];
bool Block_CanDelete[BLOCK_COUNT];

/* Bit flags of faces hidden of two neighbouring blocks. */
uint8_t Block_Hidden[BLOCK_COUNT * BLOCK_COUNT];
/* Bit flags of which faces of a block can stretch with greedy meshing. */
uint8_t Block_CanStretch[BLOCK_COUNT];

#ifdef EXTENDED_BLOCKS
int Block_UsedCount, Block_IDMask;
void Block_SetUsedCount(int count);
#endif

void Block_Reset(void);
void Block_Init(void);
void Block_SetDefaultPerms(void);
bool Block_IsCustomDefined(BlockID block);
void Block_SetCustomDefined(BlockID block, bool defined);
void Block_DefineCustom(BlockID block);

void Block_SetCollide(BlockID block, CollideType collide);
void Block_SetDrawType(BlockID block, DrawType draw);
void Block_ResetProps(BlockID block);

STRING_REF String Block_UNSAFE_GetName(BlockID block);
void Block_SetName(BlockID block, const String* name);
int Block_FindID(const String* name);
int Block_Parse(const String* name);

void Block_CalcRenderBounds(BlockID block);
void Block_CalcLightOffset(BlockID block);
void Block_RecalculateAllSpriteBB(void);
void Block_RecalculateBB(BlockID block);

void Block_SetSide(TextureLoc texLoc, BlockID blockId);
void Block_SetTex(TextureLoc texLoc, Face face, BlockID blockId);
#define Block_GetTex(block, face) Block_Textures[(block) * FACE_COUNT + (face)]

bool Block_IsFaceHidden(BlockID block, BlockID other, Face face);
void Block_UpdateAllCulling(void);
void Block_UpdateCulling(BlockID block);

/* Attempts to find the rotated block based on the user's orientation and offset on selected block. */
BlockID AutoRotate_RotateBlock(BlockID block);
#endif
