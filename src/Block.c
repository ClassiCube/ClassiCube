#include "Block.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "TexturePack.h"
#include "Game.h"
#include "Entity.h"
#include "Inventory.h"
#include "Event.h"
#include "Picking.h"
#include "Lighting.h"

struct _BlockLists Blocks;

const char* const Sound_Names[SOUND_COUNT] = {
	"none", "wood", "gravel", "grass", "stone",
	"metal", "glass", "cloth", "sand", "snow",
};

/*########################################################################################################################*
*---------------------------------------------------Default properties----------------------------------------------------*
*#########################################################################################################################*/
#define FOG_NONE  0
#define FOG_WATER PackedCol_Make(  5,   5,  51, 255)
#define FOG_LAVA  PackedCol_Make(153,  25,   0, 255)

/* Brightness */
#define BRIT_NONE 0
#define BRIT_FULL FANCY_LIGHTING_MAX_LEVEL
#define BRIT_MAGM 10

struct SimpleBlockDef {
	const char* name;
	cc_uint8 topTexture, sideTexture, bottomTexture, height;
	PackedCol fogColor; cc_uint8 fogDensity;
	cc_uint8 brightness, blocksLight; cc_uint8 gravity;
	cc_uint8 draw, collide, digSound, stepSound;
};
static const struct SimpleBlockDef invalid_blockDef = { 
	"Invalid", 0,0,0,16, FOG_NONE,0, false,true, 100, DRAW_OPAQUE,COLLIDE_SOLID,0
};

/* Properties for all built-in blocks (Classic and CPE blocks) */
static const struct SimpleBlockDef core_blockDefs[] = {
/*NAME                TOP SID BOT HEI FOG_COLOR  DENS  BRIT      BLOCKS GRAV DRAW_MODE    COLLIDE_MODE   DIG_SOUND     STEP_SOUND   */
{ "Air",               0,  0,  0, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_GAS,    COLLIDE_NONE,  SOUND_NONE,   SOUND_NONE   },
{ "Stone",             1,  1,  1, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Grass",             0,  3,  2, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_GRASS,  SOUND_GRASS  },
{ "Dirt",              2,  2,  2, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_GRAVEL, SOUND_GRAVEL },
{ "Cobblestone",      16, 16, 16, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Wood",              4,  4,  4, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_WOOD,   SOUND_WOOD   },
{ "Sapling",          15, 15, 15, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_SPRITE, COLLIDE_NONE,  SOUND_GRASS,  SOUND_NONE   },
{ "Bedrock",          17, 17, 17, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },

{ "Water",            14, 14, 14, 16, FOG_WATER,  10, BRIT_NONE,  true, 100, DRAW_TRANSLUCENT, COLLIDE_WATER, SOUND_NONE, SOUND_NONE },
{ "Still water",      14, 14, 14, 16, FOG_WATER,  10, BRIT_NONE,  true, 100, DRAW_TRANSLUCENT, COLLIDE_WATER, SOUND_NONE, SOUND_NONE },
{ "Lava",             30, 30, 30, 16, FOG_LAVA , 180, BRIT_FULL,  true, 100, DRAW_OPAQUE, COLLIDE_LAVA,  SOUND_NONE,   SOUND_NONE   },
{ "Still lava",       30, 30, 30, 16, FOG_LAVA , 180, BRIT_FULL,  true, 100, DRAW_OPAQUE, COLLIDE_LAVA,  SOUND_NONE,   SOUND_NONE   },
{ "Sand",             18, 18, 18, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_SAND,   SOUND_SAND   },
{ "Gravel",           19, 19, 19, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_GRAVEL, SOUND_GRAVEL },
{ "Gold ore",         32, 32, 32, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Iron ore",         33, 33, 33, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },

{ "Coal ore",         34, 34, 34, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Log",              21, 20, 21, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_WOOD,   SOUND_WOOD   },
{ "Leaves",           22, 22, 22, 16, FOG_NONE ,   0, BRIT_NONE, false,  40, DRAW_TRANSPARENT_THICK, COLLIDE_SOLID, SOUND_GRASS, SOUND_GRASS },
{ "Sponge",           48, 48, 48, 16, FOG_NONE ,   0, BRIT_NONE,  true,  90, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_GRASS,  SOUND_GRASS  },
{ "Glass",            49, 49, 49, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_TRANSPARENT, COLLIDE_SOLID, SOUND_GLASS,SOUND_STONE},
{ "Red",              64, 64, 64, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Orange",           65, 65, 65, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Yellow",           66, 66, 66, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
	
{ "Lime",             67, 67, 67, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Green",            68, 68, 68, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Teal",             69, 69, 69, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Aqua",             70, 70, 70, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Cyan",             71, 71, 71, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Blue",             72, 72, 72, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Indigo",           73, 73, 73, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Violet",           74, 74, 74, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },

{ "Magenta",          75, 75, 75, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Pink",             76, 76, 76, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Black",            77, 77, 77, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Gray",             78, 78, 78, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "White",            79, 79, 79, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Dandelion",        13, 13, 13, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_SPRITE, COLLIDE_NONE,  SOUND_GRASS,  SOUND_NONE   },
{ "Rose",             12, 12, 12, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_SPRITE, COLLIDE_NONE,  SOUND_GRASS,  SOUND_NONE   },
{ "Brown mushroom",   29, 29, 29, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_SPRITE, COLLIDE_NONE,  SOUND_GRASS,  SOUND_NONE   },

{ "Red mushroom",     28, 28, 28, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_SPRITE, COLLIDE_NONE,  SOUND_GRASS,  SOUND_NONE   },
{ "Gold",             24, 40, 56, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_METAL,  SOUND_METAL  },
{ "Iron",             23, 39, 55, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_METAL,  SOUND_METAL  },
{ "Double slab",       6,  5,  6, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Slab",              6,  5,  6,  8, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Brick",             7,  7,  7, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "TNT",               9,  8, 10, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_GRASS,  SOUND_GRASS  },
{ "Bookshelf",         4, 35,  4, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_WOOD,   SOUND_WOOD   },

{ "Mossy rocks",      36, 36, 36, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Obsidian",         37, 37, 37, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Cobblestone slab", 16, 16, 16,  8, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Rope",             11, 11, 11, 16, FOG_NONE ,   0, BRIT_NONE, false, 100, DRAW_SPRITE, COLLIDE_CLIMB, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Sandstone",        25, 41, 57, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Snow",             50, 50, 50,  4, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_NONE,  SOUND_SNOW,   SOUND_SNOW   },
{ "Fire",             38, 38, 38, 16, FOG_NONE ,   0, BRIT_FULL, false, 100, DRAW_SPRITE, COLLIDE_NONE,  SOUND_WOOD,   SOUND_NONE   },
{ "Light pink",       80, 80, 80, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },

{ "Forest green",     81, 81, 81, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Brown",            82, 82, 82, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Deep blue",        83, 83, 83, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Turquoise",        84, 84, 84, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_CLOTH,  SOUND_CLOTH  },
{ "Ice",              51, 51, 51, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_TRANSLUCENT, COLLIDE_ICE, SOUND_STONE, SOUND_STONE },
{ "Ceramic tile",     54, 54, 54, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Magma",            86, 86, 86, 16, FOG_NONE ,   0, BRIT_MAGM,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },
{ "Pillar",           26, 42, 58, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  },

{ "Crate",            53, 53, 53, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_WOOD,   SOUND_WOOD   },
{ "Stone brick",      52, 52, 52, 16, FOG_NONE ,   0, BRIT_NONE,  true, 100, DRAW_OPAQUE, COLLIDE_SOLID, SOUND_STONE,  SOUND_STONE  }
/*NAME                TOP SID BOT HEI FOG_COLOR  DENS  BRIT      BLOCKS GRAV DRAW_MODE    COLLIDE_MODE   DIG_SOUND     STEP_SOUND   */
};

/* Returns a backwards compatible collide type of a block */
static cc_uint8 DefaultSet_MapOldCollide(BlockID b, cc_uint8 collide) {
	if (b == BLOCK_ROPE && collide == COLLIDE_NONE)  return COLLIDE_CLIMB;
	if (b == BLOCK_ICE  && collide == COLLIDE_SOLID) return COLLIDE_ICE;

	if ((b == BLOCK_WATER || b == BLOCK_STILL_WATER) && collide == COLLIDE_LIQUID)
		return COLLIDE_WATER;
	if ((b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA)  && collide == COLLIDE_LIQUID)
		return COLLIDE_LAVA;
	return collide;
}


/*########################################################################################################################*
*---------------------------------------------------Block properties------------------------------------------------------*
*#########################################################################################################################*/
static void Block_RecalcIsLiquid(BlockID b) {
	cc_uint8 collide = Blocks.ExtendedCollide[b];
	Blocks.IsLiquid[b] =
		(collide == COLLIDE_WATER && Blocks.Draw[b] == DRAW_TRANSLUCENT) ||
		(collide == COLLIDE_LAVA  && Blocks.Draw[b] == DRAW_TRANSPARENT);
}

/* Sets the basic and extended collide types of the given block */
static void Block_SetCollide(BlockID block, cc_uint8 collide) {
	Blocks.ExtendedCollide[block] = collide;
	Block_RecalcIsLiquid(block);

	/* Reduce extended collision types to their simpler forms */
	if (collide == COLLIDE_ICE)          collide = COLLIDE_SOLID;
	if (collide == COLLIDE_SLIPPERY_ICE) collide = COLLIDE_SOLID;

	if (collide == COLLIDE_WATER) collide = COLLIDE_LIQUID;
	if (collide == COLLIDE_LAVA)  collide = COLLIDE_LIQUID;
	Blocks.Collide[block] = collide;
}

/* Sets draw type and updates related state (e.g. FullOpaque) for the given block */
static void Block_SetDrawType(BlockID block, cc_uint8 draw) {
	if (draw == DRAW_OPAQUE && Blocks.Collide[block] != COLLIDE_SOLID) draw = DRAW_TRANSPARENT;
	Blocks.Draw[block] = draw;
	Block_RecalcIsLiquid(block);

	/* Whether a block is opaque and exactly occupies a cell in the world */
	/* The mesh builder module needs this information for optimisation purposes */
	Blocks.FullOpaque[block] = draw == DRAW_OPAQUE
		&& Blocks.MinBB[block].x == 0 && Blocks.MinBB[block].y == 0 && Blocks.MinBB[block].z == 0
		&& Blocks.MaxBB[block].x == 1 && Blocks.MaxBB[block].y == 1 && Blocks.MaxBB[block].z == 1;
}

void Block_SetSide(TextureLoc texLoc, BlockID blockId) {
	int index = blockId * FACE_COUNT;
	Blocks.Textures[index + FACE_XMIN] = texLoc;
	Blocks.Textures[index + FACE_XMAX] = texLoc;
	Blocks.Textures[index + FACE_ZMIN] = texLoc;
	Blocks.Textures[index + FACE_ZMAX] = texLoc;
}


/*########################################################################################################################*
*--------------------------------------------------Block bounds/culling---------------------------------------------------*
*#########################################################################################################################*/
/* Calculates render min/max corners of this block */
/* Works by slightly offsetting collision min/max corners */
static void Block_CalcRenderBounds(BlockID block) {
	Vec3 min = Blocks.MinBB[block], max = Blocks.MaxBB[block];

	if (Blocks.IsLiquid[block]) {
		min.x += 0.1f/16.0f; max.x += 0.1f/16.0f;
		min.z += 0.1f/16.0f; max.z += 0.1f/16.0f;
		min.y -= 1.5f/16.0f; max.y -= 1.5f/16.0f;
	} else if (Blocks.Draw[block] == DRAW_TRANSLUCENT && Blocks.Collide[block] != COLLIDE_SOLID) {
		min.x += 0.1f/16.0f; max.x += 0.1f/16.0f;
		min.z += 0.1f/16.0f; max.z += 0.1f/16.0f;
		min.y -= 0.1f/16.0f; max.y -= 0.1f/16.0f;
	}

	Blocks.RenderMinBB[block] = min; Blocks.RenderMaxBB[block] = max;
}

/* Calculates light colour offset for each face of the given block */
static void Block_CalcLightOffset(BlockID block) {
	int flags = 0xFF;
	Vec3 min = Blocks.MinBB[block], max = Blocks.MaxBB[block];

	if (min.x != 0) flags &= ~FACE_BIT_XMIN;
	if (max.x != 1) flags &= ~FACE_BIT_XMAX;
	if (min.z != 0) flags &= ~FACE_BIT_ZMIN;
	if (max.z != 1) flags &= ~FACE_BIT_ZMAX;
	if (min.y != 0) flags &= ~FACE_BIT_YMIN;
	if (max.y != 1) flags &= ~FACE_BIT_YMAX;

	if ((min.y != 0 && max.y == 1) && Blocks.Draw[block] != DRAW_GAS) {
		flags &= ~(1 << LIGHT_FLAG_SHADES_FROM_BELOW);
	}
	Blocks.LightOffset[block] = flags;
}


static float GetSpriteBB_MinX(int size, int tileX, int tileY, const struct Bitmap* bmp) {
	BitmapCol* row;
	int x, y;

	for (x = 0; x < size; x++) {
		for (y = 0; y < size; y++) {
			row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
			if (BitmapCol_A(row[x])) { return (float)x / size; }
		}
	}
	return 1.0f;
}

static float GetSpriteBB_MinY(int size, int tileX, int tileY, const struct Bitmap* bmp) {
	BitmapCol* row;
	int x, y;

	for (y = size - 1; y >= 0; y--) {
		row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
		for (x = 0; x < size; x++) {
			if (BitmapCol_A(row[x])) { return 1.0f - (float)(y + 1) / size; }
		}
	}
	return 1.0f;
}

static float GetSpriteBB_MaxX(int size, int tileX, int tileY, const struct Bitmap* bmp) {
	BitmapCol* row;
	int x, y;

	for (x = size - 1; x >= 0; x--) {
		for (y = 0; y < size; y++) {
			row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
			if (BitmapCol_A(row[x])) { return (float)(x + 1) / size; }
		}
	}
	return 0.0f;
}

static float GetSpriteBB_MaxY(int size, int tileX, int tileY, const struct Bitmap* bmp) {
	BitmapCol* row;
	int x, y;

	for (y = 0; y < size; y++) {
		row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
		for (x = 0; x < size; x++) {
			if (BitmapCol_A(row[x])) { return 1.0f - (float)y / size; }
		}
	}
	return 0.0f;
}

/* Recalculates bounding box of the given sprite block */
static void Block_RecalculateBB(BlockID block) {
	struct Bitmap* bmp = &Atlas2D.Bmp;
	int tileSize = Atlas2D.TileSize;
	TextureLoc texLoc = Block_Tex(block, FACE_XMAX);
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);

	Vec3 centre = { 0.5f, 0.0f, 0.5f };
	float minX = 0, minY = 0, maxX = 1, maxY = 1;
	Vec3 minRaw, maxRaw;

	if (y < Atlas2D.RowsCount) {
		minX = GetSpriteBB_MinX(tileSize, x, y, bmp);
		minY = GetSpriteBB_MinY(tileSize, x, y, bmp);
		maxX = GetSpriteBB_MaxX(tileSize, x, y, bmp);
		maxY = GetSpriteBB_MaxY(tileSize, x, y, bmp);
	}

	minRaw = Vec3_RotateY3(minX - 0.5f, minY, 0.0f, 45.0f * MATH_DEG2RAD);
	maxRaw = Vec3_RotateY3(maxX - 0.5f, maxY, 0.0f, 45.0f * MATH_DEG2RAD);
	Vec3_Add(&Blocks.MinBB[block], &minRaw, &centre);
	Vec3_Add(&Blocks.MaxBB[block], &maxRaw, &centre);
	Block_CalcRenderBounds(block);
}

/* Recalculates bounding boxes of all sprite blocks */
static void Block_RecalculateAllSpriteBB(void) {
	int block;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		if (Blocks.Draw[block] != DRAW_SPRITE) continue;

		Block_RecalculateBB((BlockID)block);
	}
}


static void Block_CalcStretch(BlockID block) {
	/* faces which can be stretched on X axis */
	if (Blocks.MinBB[block].x == 0.0f && Blocks.MaxBB[block].x == 1.0f) {
		Blocks.CanStretch[block] |= 0x3C;
	} else {
		Blocks.CanStretch[block] &= 0xC3; /* ~0x3C */
	}

	/* faces which can be stretched on Z axis */
	if (Blocks.MinBB[block].z == 0.0f && Blocks.MaxBB[block].z == 1.0f) {
		Blocks.CanStretch[block] |= 0x03;
	} else {
		Blocks.CanStretch[block] &= 0xFC; /* ~0x03 */
	}

	#if defined CC_BUILD_PS1 || defined CC_BUILD_SATURN
	Blocks.CanStretch[block] = 0;
	#endif
}

static cc_bool Block_MightCull(BlockID block, BlockID other) {
	cc_uint8 bType, oType;
	/* Sprite blocks can never cull blocks. */
	if (Blocks.Draw[block] == DRAW_SPRITE) return false;

	/* NOTE: Water is always culled by lava */
	if ((block == BLOCK_WATER || block == BLOCK_STILL_WATER)
		&& (other == BLOCK_LAVA || other == BLOCK_STILL_LAVA))
		return true;

	/* All blocks (except for say leaves) cull with themselves */
	if (block == other) return Blocks.Draw[block] != DRAW_TRANSPARENT_THICK;

	/* An opaque neighbour (asides from lava) culls this block. */
	if (Blocks.Draw[other] == DRAW_OPAQUE && !Blocks.IsLiquid[other]) return true;
	/* Transparent/Gas blocks don't cull other blocks (except themselves) */
	if (Blocks.Draw[block] != DRAW_TRANSLUCENT || Blocks.Draw[other] != DRAW_TRANSLUCENT) return false;

	/* Some translucent blocks may still cull other translucent blocks */
	/* e.g. for water/ice, don't need to draw faces of water */
	bType = Blocks.Collide[block]; oType = Blocks.Collide[other]; 
	return (bType == COLLIDE_SOLID && oType == COLLIDE_SOLID) || bType != COLLIDE_SOLID;
}

static void Block_CalcCulling(BlockID block, BlockID other) {
	Vec3 bMin, bMax, oMin, oMax;
	cc_bool occludedX, occludedY, occludedZ, bothLiquid;
	int f;

	/* Fast path: Full opaque neighbouring blocks will always have all shared faces hidden */
	if (Blocks.FullOpaque[block] && Blocks.FullOpaque[other]) {
		Blocks.Hidden[(block * BLOCK_COUNT) + other] = 0x3F;
		return;
	}

	/* Some blocks may not cull 'other' block, in which case just skip detailed check */
	/* e.g. sprite blocks, default leaves, will not cull any other blocks */
	if (!Block_MightCull(block, other)) {	
		Blocks.Hidden[(block * BLOCK_COUNT) + other] = 0;
		return;
	}

	bMin = Blocks.MinBB[block]; bMax = Blocks.MaxBB[block];
	oMin = Blocks.MinBB[other]; oMax = Blocks.MaxBB[other];

	/* Extend offsets of liquid down to match rendered position */
	/* This isn't completely correct, but works well enough */
	if (Blocks.IsLiquid[block]) bMax.y -= 1.50f / 16.0f;
	if (Blocks.IsLiquid[other]) oMax.y -= 1.50f / 16.0f;

	bothLiquid = Blocks.IsLiquid[block] && Blocks.IsLiquid[other];
	f = 0; /* mark all faces initially 'not hidden' */

	/* Whether the 'texture region' of a face on block fits inside corresponding region on other block */
	occludedX = (bMin.z >= oMin.z && bMax.z <= oMax.z) && (bMin.y >= oMin.y && bMax.y <= oMax.y);
	occludedY = (bMin.x >= oMin.x && bMax.x <= oMax.x) && (bMin.z >= oMin.z && bMax.z <= oMax.z);
	occludedZ = (bMin.x >= oMin.x && bMax.x <= oMax.x) && (bMin.y >= oMin.y && bMax.y <= oMax.y);

	f |= occludedX && oMax.x == 1.0f && bMin.x == 0.0f ? FACE_BIT_XMIN : 0;
	f |= occludedX && oMin.x == 0.0f && bMax.x == 1.0f ? FACE_BIT_XMAX : 0;
	f |= occludedZ && oMax.z == 1.0f && bMin.z == 0.0f ? FACE_BIT_ZMIN : 0;
	f |= occludedZ && oMin.z == 0.0f && bMax.z == 1.0f ? FACE_BIT_ZMAX : 0;
	f |= occludedY && (bothLiquid || (oMax.y == 1.0f && bMin.y == 0.0f)) ? FACE_BIT_YMIN : 0;
	f |= occludedY && (bothLiquid || (oMin.y == 0.0f && bMax.y == 1.0f)) ? FACE_BIT_YMAX : 0;
	Blocks.Hidden[(block * BLOCK_COUNT) + other] = f;
}

/* Updates culling data of all blocks */
static void Block_UpdateAllCulling(void) {
	int block, neighbour;

	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Block_CalcStretch((BlockID)block);
		for (neighbour = BLOCK_AIR; neighbour < BLOCK_COUNT; neighbour++) {
			Block_CalcCulling((BlockID)block, (BlockID)neighbour);
		}
	}
}

/* Updates culling data just for this block */
/* (e.g. whether block can be stretched, visibility with other blocks) */
static void Block_UpdateCulling(BlockID block) {
	int neighbour;
	Block_CalcStretch(block);
	
	for (neighbour = BLOCK_AIR; neighbour < BLOCK_COUNT; neighbour++) {
		Block_CalcCulling(block, (BlockID)neighbour);
		Block_CalcCulling((BlockID)neighbour, block);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Block-----------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint32 definedCustomBlocks[BLOCK_COUNT >> 5];
static CC_BIG_VAR char Block_NamesBuffer[STRING_SIZE * BLOCK_COUNT];
#define Block_NamePtr(i) &Block_NamesBuffer[STRING_SIZE * i]

cc_bool Block_IsCustomDefined(BlockID block) {
	return (definedCustomBlocks[block >> 5] & (1u << (block & 0x1F))) != 0;
}

/* Sets whether the given block has been changed from default */
static void Block_SetCustomDefined(BlockID block, cc_bool defined) {
	if (defined) {
		definedCustomBlocks[block >> 5] |=  (1u << (block & 0x1F));
	} else {
		definedCustomBlocks[block >> 5] &= ~(1u << (block & 0x1F));
	}
}

void Block_DefineCustom(BlockID block, cc_bool checkSprite) {
	PackedCol black  = PackedCol_Make(0, 0, 0, 255);
	cc_string name   = Block_UNSAFE_GetName(block);
	/* necessary if servers redefined core blocks, before extended collide types were added */
	cc_uint8 collide = DefaultSet_MapOldCollide(block, Blocks.Collide[block]);
	Blocks.Tinted[block] = Blocks.FogCol[block] != black && String_IndexOf(&name, '#') >= 0;

	Block_SetCollide(block,  collide);
	Block_SetDrawType(block, Blocks.Draw[block]);
	Block_CalcRenderBounds(block);
	Block_UpdateCulling(block);
	Block_CalcLightOffset(block);

	Inventory_AddDefault(block);
	Block_SetCustomDefined(block, true);
	Event_RaiseVoid(&BlockEvents.BlockDefChanged);

	if (!checkSprite) return; /* TODO eliminate this */
	/* Update sprite BoundingBox if necessary */
	if (Blocks.Draw[block] == DRAW_SPRITE) Block_RecalculateBB(block);
}

void Block_UndefineCustom(BlockID block) {
	Block_ResetProps(block);
	Block_UpdateCulling(block);

	Inventory_Remove(block);
	if (block <= BLOCK_MAX_CPE) { Inventory_AddDefault(block); }

	Block_SetCustomDefined(block, false);
	Event_RaiseVoid(&BlockEvents.BlockDefChanged);

	/* Update sprite BoundingBox if necessary */
	if (Blocks.Draw[block] == DRAW_SPRITE) Block_RecalculateBB(block);
}

void Block_ResetProps(BlockID block) {
	const struct SimpleBlockDef* def = block <= Game_Version.MaxCoreBlock ? &core_blockDefs[block] : &invalid_blockDef;
	const cc_string name = String_FromReadonly(def->name);

	Blocks.BlocksLight[block] = def->blocksLight;
	Blocks.Brightness[block]  = def->brightness;
	Blocks.FogCol[block]      = def->fogColor;
	Blocks.FogDensity[block]  = def->fogDensity / 100.0f;
	Block_SetCollide(block,     def->collide);
	Blocks.DigSounds[block]   = def->digSound;
	Blocks.StepSounds[block]  = def->stepSound;
	Blocks.SpeedMultiplier[block] = 1.0f;
	Block_SetName(block, &name);
	Blocks.Tinted[block]       = false;
	Blocks.SpriteOffset[block] = 0;

	Blocks.Draw[block] = def->draw;
	if (def->draw == DRAW_SPRITE) {
		Vec3_Set(Blocks.MinBB[block], 2.50f/16.0f, 0, 2.50f/16.0f);
		Vec3_Set(Blocks.MaxBB[block], 13.5f/16.0f, 1, 13.5f/16.0f);
	} else {		
		Vec3_Set(Blocks.MinBB[block], 0, 0,                   0);
		Vec3_Set(Blocks.MaxBB[block], 1, def->height / 16.0f, 1);
	}

	Block_SetDrawType(block, def->draw);
	Block_CalcRenderBounds(block);
	Block_CalcLightOffset(block);

	Block_Tex(block, FACE_YMAX) = def->topTexture;
	Block_Tex(block, FACE_YMIN) = def->bottomTexture;
	Block_SetSide(def->sideTexture, block);

	Blocks.ParticleGravity[block] = 5.4f * (def->gravity / 100.0f);
}

STRING_REF cc_string Block_UNSAFE_GetName(BlockID block) {
	return String_FromRaw(Block_NamePtr(block), STRING_SIZE);
}

void Block_SetName(BlockID block, const cc_string* name) {
	String_CopyToRaw(Block_NamePtr(block), STRING_SIZE, name);
}

int Block_FindID(const cc_string* name) {
	cc_string blockName;
	int block;

	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) 
	{
		blockName = Block_UNSAFE_GetName(block);
		if (String_CaselessEquals(&blockName, name)) return block;
	}
	return -1;
}

int Block_Parse(const cc_string* name) {
	int b;
	if (Convert_ParseInt(name, &b) && b < BLOCK_COUNT) return b;
	return Block_FindID(name);
}

/* 0b_1000_0000 */
#define USE_MODERN_BRIGHTNESS_FLAG 1 << 7
/* 0b_0100_0000 */
#define USE_LAMP_COLOR 1 << 6
/* 0b_0000_1111 */
#define BRIGHTNESS_MASK FANCY_LIGHTING_MAX_LEVEL

/* Reads network format    0b_US--_LLLL where U = uses fancy brightness, S = uses lamp brightness, and L = brightness */
/* Into CC's native format 0b_SSSS_BBBB where S = lamp brightness and B = lava brightness */
cc_uint8 Block_ReadBrightness(cc_uint8 fullBright) {
	cc_bool useSun;
	/* If the fullBright byte does not use the flag, we should interpret it as either completely dark or casting max block light */
	if ((fullBright & USE_MODERN_BRIGHTNESS_FLAG) == 0) { return fullBright > 0 ? FANCY_LIGHTING_MAX_LEVEL : 0; }

	useSun = fullBright & USE_LAMP_COLOR;

	/* Preserve only the least significant four bits. This gives us our raw brightness level for sun or block light. */
	fullBright &= BRIGHTNESS_MASK;

	/* Sun light is stored in the upper four bits */
	if (useSun) { fullBright <<= FANCY_LIGHTING_LAMP_SHIFT; }
	return fullBright;
}

/* Writes CC's native format 0b_SSSS_BBBB where S = lamp brightness and B = lava brightness */
/* into network format       0b_US--_LLLL where U = uses fancy brightness, S = uses lamp brightness, and L = brightness */
cc_uint8 Block_WriteFullBright(cc_uint8 brightness) {
	cc_uint8 lavaBrightness, lampBrightness, fullBright;
	lavaBrightness = brightness & BRIGHTNESS_MASK;
	lampBrightness = brightness >> FANCY_LIGHTING_LAMP_SHIFT;
	fullBright     = USE_MODERN_BRIGHTNESS_FLAG;

	/* Modern brightness stored in a fullbright value is mutually exclusive between using block and using sun light */
	if (lavaBrightness > 0) {
		fullBright |= lavaBrightness;
	} else if (lampBrightness > 0) {
		fullBright |= USE_LAMP_COLOR; /* Insert flag that tells us this fullbright value should be interpreted as sun brightness */
		fullBright |= lampBrightness;
	} else {
		return 0;
	}
	return fullBright;
}

/*########################################################################################################################*
*-------------------------------------------------------AutoRotate--------------------------------------------------------*
*#########################################################################################################################*/
cc_bool AutoRotate_Enabled;

#define AR_GROUP_CORNERS 0
#define AR_GROUP_VERTICAL 1
#define AR_GROUP_DIRECTION 2
#define AR_GROUP_PILLAR  3

#define AR_EQ1(x)    (dir0 == x && dir1 == '\0')
#define AR_EQ2(x, y) (dir0 == x && dir1 == y)
static int AR_CalcGroup(const cc_string* dir) {
	char dir0, dir1;
	dir0 = dir->length > 1 ? dir->buffer[1] : '\0'; Char_MakeLower(dir0);
	dir1 = dir->length > 2 ? dir->buffer[2] : '\0'; Char_MakeLower(dir1);

	if (AR_EQ2('n','w') || AR_EQ2('n','e') || AR_EQ2('s','w') || AR_EQ2('s','e')) {
		return AR_GROUP_CORNERS;
	} else if (AR_EQ1('u') || AR_EQ1('d')) {
		return AR_GROUP_VERTICAL;
	} else if (AR_EQ1('n') || AR_EQ1('w') || AR_EQ1('s') || AR_EQ1('e')) {
		return AR_GROUP_DIRECTION;
	} else if (AR_EQ2('u','d') || AR_EQ2('w','e') || AR_EQ2('n','s')) {
		return AR_GROUP_PILLAR;
	}
	return -1;
}

/* replaces a portion of a string, appends otherwise */
static void AutoRotate_Insert(cc_string* str, int offset, const char* suffix) {
	int i = str->length - offset;

	for (; *suffix; suffix++, i++) {
		if (i < str->length) {
			str->buffer[i] = *suffix;
		} else {
			String_Append(str, *suffix);
		}
	}
}
/* finds proper rotated form of a block, based on the given name */
static int FindRotated(cc_string* name, int offset);

static int GetRotated(cc_string* name, int offset) {
	int rotated = FindRotated(name, offset);
	return rotated == -1 ? Block_FindID(name) : rotated;
}

static int RotateCorner(cc_string* name, int offset) {
	float x = Game_SelectedPos.intersect.x - (float)Game_SelectedPos.translatedPos.x;
	float z = Game_SelectedPos.intersect.z - (float)Game_SelectedPos.translatedPos.z;

	if (x < 0.5f && z < 0.5f) {
		AutoRotate_Insert(name, offset, "-NW");
	} else if (x >= 0.5f && z < 0.5f) {
		AutoRotate_Insert(name, offset, "-NE");
	} else if (x < 0.5f && z >= 0.5f) {
		AutoRotate_Insert(name, offset, "-SW");
	} else if (x >= 0.5f && z >= 0.5f) {
		AutoRotate_Insert(name, offset, "-SE");
	}
	return GetRotated(name, offset);
}

static int RotateVertical(cc_string* name, int offset) {
	float y = Game_SelectedPos.intersect.y - (float)Game_SelectedPos.translatedPos.y;

	if (y >= 0.5f) {
		AutoRotate_Insert(name, offset, "-U");
	} else {
		AutoRotate_Insert(name, offset, "-D");
	}
	return GetRotated(name, offset);
}

static int RotateFence(cc_string* name, int offset) {
	/* Fence type blocks */
	float yaw = Math_ClampAngle(Entities.CurPlayer->Base.Yaw);

	if (yaw < 45.0f || (yaw >= 135.0f && yaw < 225.0f) || yaw > 315.0f) {
		AutoRotate_Insert(name, offset, "-WE");
	} else {
		AutoRotate_Insert(name, offset, "-NS");
	}
	return GetRotated(name, offset);
}

static int RotatePillar(cc_string* name, int offset) {
	/* Thin pillar type blocks */
	Face face = Game_SelectedPos.closest;

	if (face == FACE_YMAX || face == FACE_YMIN) {
		AutoRotate_Insert(name, offset, "-UD");
	} else if (face == FACE_XMAX || face == FACE_XMIN) {
		AutoRotate_Insert(name, offset, "-WE");
	} else if (face == FACE_ZMAX || face == FACE_ZMIN) {
		AutoRotate_Insert(name, offset, "-NS");
	}
	return GetRotated(name, offset);
}

static int RotateDirection(cc_string* name, int offset) {
	float yaw = Math_ClampAngle(Entities.CurPlayer->Base.Yaw);

	if (yaw >= 45.0f && yaw < 135.0f) {
		AutoRotate_Insert(name, offset, "-E");
	} else if (yaw >= 135.0f && yaw < 225.0f) {
		AutoRotate_Insert(name, offset, "-S");
	} else if (yaw >= 225.0f && yaw < 315.0f) {
		AutoRotate_Insert(name, offset, "-W");
	} else {
		AutoRotate_Insert(name, offset, "-N");
	}
	return GetRotated(name, offset);
}

static int FindRotated(cc_string* name, int offset) {
	cc_string dir;
	int group;
	int dirIndex = String_LastIndexOfAt(name, offset, '-');
	if (dirIndex == -1) return -1; /* not a directional block */

	dir = String_UNSAFE_SubstringAt(name, dirIndex);
	dir.length -= offset;
	if (dir.length > 3) return -1;

	offset += dir.length;
	group = AR_CalcGroup(&dir);

	if (group == AR_GROUP_CORNERS) return RotateCorner(name, offset);
	if (group == AR_GROUP_VERTICAL) return RotateVertical(name, offset);
	if (group == AR_GROUP_DIRECTION) return RotateDirection(name, offset);

	if (group == AR_GROUP_PILLAR) {
		AutoRotate_Insert(name, offset, "-UD");
		if (Block_FindID(name) == -1) {
			return RotateFence(name, offset);
		} else {
			return RotatePillar(name, offset);
		}
	}
	return -1;
}

BlockID AutoRotate_RotateBlock(BlockID block) {
	cc_string str; char strBuffer[STRING_SIZE * 2];
	cc_string name;
	int rotated;
	
	name = Block_UNSAFE_GetName(block);
	String_InitArray(str, strBuffer);
	String_AppendString(&str, &name);

	/* need to copy since we change characters in name */
	rotated = FindRotated(&str, 0);
	return rotated == -1 ? block : (BlockID)rotated;
}

static void GetAutoRotateTypes(cc_string* name, int* dirTypes) {
	int dirIndex, i;
	cc_string dir;
	dirTypes[0] = -1;
	dirTypes[1] = -1;

	for (i = 0; i < 2; i++) {
		/* index of rightmost group separated by dashes */
		dirIndex = String_LastIndexOf(name, '-');
		if (dirIndex == -1) return;

		dir = String_UNSAFE_SubstringAt(name, dirIndex);
		dirTypes[i]  = AR_CalcGroup(&dir);
		name->length = dirIndex;
	}
}

cc_bool AutoRotate_BlocksShareGroup(BlockID block, BlockID other) {
	cc_string bName, oName;
	int bDirTypes[2], oDirTypes[2];

	bName = Block_UNSAFE_GetName(block);
	GetAutoRotateTypes(&bName, bDirTypes);
	if (bDirTypes[0] == -1) return false;

	oName = Block_UNSAFE_GetName(other);
	GetAutoRotateTypes(&oName, oDirTypes);
	if (oDirTypes[0] == -1) return false;

	return bDirTypes[0] == oDirTypes[0] && bDirTypes[1] == oDirTypes[1] 
		&& String_CaselessEquals(&bName, &oName);
}


/*########################################################################################################################*
*----------------------------------------------------Blocks component-----------------------------------------------------*
*#########################################################################################################################*/
static void OnReset(void) {
	int i, block;
	for (i = 0; i < Array_Elems(definedCustomBlocks); i++) {
		definedCustomBlocks[i] = 0;
	}

	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Block_ResetProps((BlockID)block);
	}

	Block_UpdateAllCulling();
	Block_RecalculateAllSpriteBB();

	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Blocks.CanPlace[block]  = true;
		Blocks.CanDelete[block] = true;
	}
}

static void OnAtlasChanged(void* obj) { Block_RecalculateAllSpriteBB(); }
static void OnInit(void) {
	AutoRotate_Enabled = true;
	Event_Register_(&TextureEvents.AtlasChanged, NULL, OnAtlasChanged);
	OnReset();
}

struct IGameComponent Blocks_Component = {
	OnInit,  /* Init  */
	NULL,    /* Free  */
	OnReset, /* Reset */
};
