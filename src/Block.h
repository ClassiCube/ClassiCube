#ifndef CC_BLOCK_H
#define CC_BLOCK_H
#include "PackedCol.h"
#include "Vectors.h"
#include "Constants.h"
#include "BlockID.h"
/* Stores properties and data for blocks.
   Also performs automatic rotation of directional blocks.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Blocks_Component;

enum SoundType {
	SOUND_NONE,  SOUND_WOOD,  SOUND_GRAVEL, SOUND_GRASS, 
	SOUND_STONE, SOUND_METAL, SOUND_GLASS,  SOUND_CLOTH, 
	SOUND_SAND,  SOUND_SNOW,  SOUND_COUNT
};
extern const char* Sound_Names[SOUND_COUNT];

/* Describes how a block is rendered in the world. */
enum DrawType {
	DRAW_OPAQUE,            /* Completely covers blocks behind (e.g. dirt). */
	DRAW_TRANSPARENT,       /* Blocks behind show (e.g. glass). Pixels either fully visible or invisible. */
	DRAW_TRANSPARENT_THICK, /* Same as Transparent, but all neighbour faces show. (e.g. leaves) */
	DRAW_TRANSLUCENT,       /* Blocks behind show (e.g. water). Pixels blend with other blocks behind. */
	DRAW_GAS,               /* Does not show (e.g. air). Can still be collided with. */
	DRAW_SPRITE             /* Block renders as an X (e.g. sapling). Pixels either fully visible or invisible. */
};

/* Describes the interaction a block has with a player when they collide with it. */
enum CollideType {
	COLLIDE_GAS,          /* No interaction when player collides. */
	COLLIDE_LIQUID,       /* 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_SOLID,        /* Block completely stops the player when they are moving. */
	COLLIDE_ICE,          /* Block is solid and partially slidable on. */
	COLLIDE_SLIPPERY_ICE, /* Block is solid and fully slidable on. */
	COLLIDE_LIQUID_WATER, /* Water style 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_LIQUID_LAVA,  /* Lava style 'swimming'/'bobbing' interaction when player collides. */
	COLLIDE_CLIMB_ROPE    /* Rope/Ladder style climbing interaction when player collides. */
};

CC_VAR extern struct _BlockLists {
	/* Whether this block is a liquid. (Like water/lava) */
	bool IsLiquid[BLOCK_COUNT];
	/* Whether this block prevents lights from passing through it. */
	bool BlocksLight[BLOCK_COUNT];
	/* Whether this block is fully bright/light emitting. (Like lava) */
	bool FullBright[BLOCK_COUNT];
	/* Fog colour when player is inside this block. */
	/* NOTE: Only applies if fog density is not 0. */
	PackedCol FogCol[BLOCK_COUNT];
	/* How thick fog is when player is inside this block. */
	float FogDensity[BLOCK_COUNT];
	/* Basic collision type of this block. (gas, liquid, or solid) */
	uint8_t Collide[BLOCK_COUNT];
	/* Extended collision type of this block, usually same as basic. */
	/* NOTE: Not always the case. (e.g. ice, water, lava, rope differ) */
	uint8_t ExtendedCollide[BLOCK_COUNT];
	/* Speed multiplier when player is touching this block. */
	/* Can be < 1 to slow player down, or > 1 to speed up. */
	float SpeedMultiplier[BLOCK_COUNT];
	/* Bit flags of which faces of this block uses light colour from neighbouring blocks. */
	/* e.g. a block with Min.X of 0.0 uses light colour at X-1,Y,Z for XMIN face. */
	/* e.g. a block with Min.X of 0.1 uses light colour at X,Y,Z   for XMIN face. */
	uint8_t LightOffset[BLOCK_COUNT];
	/* Draw method used when rendering this block. See DrawType enum. */
	uint8_t Draw[BLOCK_COUNT];
	/* Sound played when the player manually destroys this block. See SoundType enum. */
	uint8_t DigSounds[BLOCK_COUNT];
	/* Sound played when the player walks on this block. See SoundType enum. */
	uint8_t StepSounds[BLOCK_COUNT];
	/* Whether fog colour is used to apply a tint effect to this block. */
	bool Tinted[BLOCK_COUNT];
	/* Whether this block has an opaque draw type, min of (0,0,0), and max of (1,1,1) */
	bool FullOpaque[BLOCK_COUNT];
	/* Offset/variation mode of this block. (only when drawn as a sprite) */
	/* Some modes slightly randomly offset blocks to produce nicer looking clumps. */
	uint8_t SpriteOffset[BLOCK_COUNT];

	/* Coordinates of min corner of this block for collisions. */
	Vector3 MinBB[BLOCK_COUNT];
	/* Coordinates of max corner of this block for collisions. */
	Vector3 MaxBB[BLOCK_COUNT];
	/* Coordinates of min corner of this block for rendering. */
	/* e.g. ice is very slightly offset horizontally. */
	Vector3 RenderMinBB[BLOCK_COUNT];
	/* Coordinates of max corner of this block for rendering. */
	/* e.g. ice is very slightly offset horizontally. */
	Vector3 RenderMaxBB[BLOCK_COUNT];

	/* Texture ids of each face of blocks. */
	TextureLoc Textures[BLOCK_COUNT * FACE_COUNT];
	/* Whether this block is allowed to be placed. */
	bool CanPlace[BLOCK_COUNT];
	/* Whether this block is allowed to be deleted. */
	bool CanDelete[BLOCK_COUNT];

	/* Bit flags of faces hidden of two neighbouring blocks. */
	uint8_t Hidden[BLOCK_COUNT * BLOCK_COUNT];
	/* Bit flags of which faces of this block can stretch with greedy meshing. */
	uint8_t CanStretch[BLOCK_COUNT];
} Blocks;

#define Block_Tint(col, block)\
if (Blocks.Tinted[block]) {\
	PackedCol tintCol = Blocks.FogCol[block];\
	col.R = (uint8_t)(col.R * tintCol.R / 255);\
	col.G = (uint8_t)(col.G * tintCol.G / 255);\
	col.B = (uint8_t)(col.B * tintCol.B / 255);\
}

/* Returns whether the given block has been changed from default. */
bool Block_IsCustomDefined(BlockID block);
/* Sets whether the given block has been changed from default. */
void Block_SetCustomDefined(BlockID block, bool defined);
void Block_DefineCustom(BlockID block);

/* Sets the basic and extended collide types of the given block. */
void Block_SetCollide(BlockID block, uint8_t collide);
/* Sets draw type and updates related state (e.g. FullOpaque) for the given block. */
void Block_SetDrawType(BlockID block, uint8_t draw);
/* Resets all the properties of the given block to default. */
void Block_ResetProps(BlockID block);

/* Gets the name of the given block. */
/* NOTE: Name points directly within underlying buffer, you MUST NOT persist this string. */
CC_API STRING_REF String Block_UNSAFE_GetName(BlockID block);
/* Sets the name of the given block. */
void Block_SetName(BlockID block, const String* name);
/* Finds the ID of the block whose name caselessly matches given name. */
CC_API int Block_FindID(const String* name);
/* Attempts to parse given name as a numerical block ID. */
/* Falls back to Block_FindID if this fails. */
CC_API int Block_Parse(const String* name);

/* Calculates render min/max corners of this block. */
/* Works by slightly offsetting collision min/max corners. */
void Block_CalcRenderBounds(BlockID block);
/* Calculates light colour offset for each face of the given block. */
void Block_CalcLightOffset(BlockID block);
/* Recalculates bounding boxes of all sprite blocks. */
void Block_RecalculateAllSpriteBB(void);
/* Recalculates bounding box of the given sprite block. */
void Block_RecalculateBB(BlockID block);

/* Sets the textures of the side faces of the given block. */
void Block_SetSide(TextureLoc texLoc, BlockID blockId);
/* The texture for the given face of the given block. */
#define Block_Tex(block, face) Blocks.Textures[(block) * FACE_COUNT + (face)]

bool Block_IsFaceHidden(BlockID block, BlockID other, Face face);
/* Updates culling data of all blocks. */
void Block_UpdateAllCulling(void);
/* Updates culling data just for this block. */
/* (e.g. whether block can be stretched, visibility with other blocks) */
void Block_UpdateCulling(BlockID block);

/* Whether blocks can be automatically rotated. */
extern bool AutoRotate_Enabled;
/* Attempts to find the rotated block based on the user's orientation and offset on selected block. */
/* If no rotated block is found, returns given block. */
BlockID AutoRotate_RotateBlock(BlockID block);
#endif
