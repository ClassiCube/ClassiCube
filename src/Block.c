#include "Block.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "TexturePack.h"
#include "Game.h"
#include "Entity.h"
#include "Inventory.h"
#include "Event.h"
#include "Platform.h"
#include "Picking.h"

struct _BlockLists Blocks;

const char* const Sound_Names[SOUND_COUNT] = {
	"none", "wood", "gravel", "grass", "stone",
	"metal", "glass", "cloth", "sand", "snow",
};

/*########################################################################################################################*
*---------------------------------------------------Default properties----------------------------------------------------*
*#########################################################################################################################*/
static float DefaultSet_Height(BlockID b) {
	if (b == BLOCK_SLAB)        return 0.50f;
	if (b == BLOCK_COBBLE_SLAB) return 0.50f;
	if (b == BLOCK_SNOW)        return 0.25f;
	return 1.00f;
}

static cc_bool DefaultSet_FullBright(BlockID b) {
	return b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA
		|| b == BLOCK_MAGMA || b == BLOCK_FIRE;
}

static float DefaultSet_FogDensity(BlockID b) {
	if (b == BLOCK_WATER || b == BLOCK_STILL_WATER) return 0.1f;
	if (b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA)  return 1.8f;
	return 0.0f;
}

static PackedCol DefaultSet_FogColour(BlockID b) {
	PackedCol colWater = PackedCol_Make(  5,   5,  51, 255);
	PackedCol colLava  = PackedCol_Make(153,  25,   0, 255);

	if (b == BLOCK_WATER || b == BLOCK_STILL_WATER) return colWater;
	if (b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA)  return colLava;
	return 0;
}

static cc_uint8 DefaultSet_Draw(BlockID b) {
	if (b == BLOCK_AIR)    return DRAW_GAS;
	if (b == BLOCK_LEAVES) return DRAW_TRANSPARENT_THICK;

	if (b == BLOCK_ICE || b == BLOCK_WATER || b == BLOCK_STILL_WATER)
		return DRAW_TRANSLUCENT;
	if (b == BLOCK_GLASS || b == BLOCK_LEAVES)
		return DRAW_TRANSPARENT;

	if (b >= BLOCK_DANDELION && b <= BLOCK_RED_SHROOM)
		return DRAW_SPRITE;
	if (b == BLOCK_SAPLING || b == BLOCK_ROPE || b == BLOCK_FIRE)
		return DRAW_SPRITE;
	return DRAW_OPAQUE;
}

static cc_bool DefaultSet_BlocksLight(BlockID b) {
	return !(b == BLOCK_GLASS || b == BLOCK_LEAVES
		|| b == BLOCK_AIR || DefaultSet_Draw(b) == DRAW_SPRITE);
}

static cc_uint8 DefaultSet_Collide(BlockID b) {
	if (b == BLOCK_ICE) return COLLIDE_ICE;
	if (b == BLOCK_WATER || b == BLOCK_STILL_WATER) return COLLIDE_LIQUID_WATER;
	if (b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA)  return COLLIDE_LIQUID_LAVA;

	if (b == BLOCK_SNOW || b == BLOCK_AIR || DefaultSet_Draw(b) == DRAW_SPRITE)
		return COLLIDE_GAS;
	return COLLIDE_SOLID;
}

/* Returns a backwards compatible collide type of a block. */
static cc_uint8 DefaultSet_MapOldCollide(BlockID b, cc_uint8 collide) {
	if (b == BLOCK_ROPE && collide == COLLIDE_GAS)   return COLLIDE_CLIMB_ROPE;
	if (b == BLOCK_ICE  && collide == COLLIDE_SOLID) return COLLIDE_ICE;

	if ((b == BLOCK_WATER || b == BLOCK_STILL_WATER) && collide == COLLIDE_LIQUID)
		return COLLIDE_LIQUID_WATER;
	if ((b == BLOCK_LAVA  || b == BLOCK_STILL_LAVA)  && collide == COLLIDE_LIQUID)
		return COLLIDE_LIQUID_LAVA;
	return collide;
}

static cc_uint8 DefaultSet_DigSound(BlockID b) {
	if (b >= BLOCK_RED && b <= BLOCK_WHITE)            return SOUND_CLOTH;
	if (b >= BLOCK_LIGHT_PINK && b <= BLOCK_TURQUOISE) return SOUND_CLOTH;
	if (b == BLOCK_IRON || b == BLOCK_GOLD)            return SOUND_METAL;

	if (b == BLOCK_BOOKSHELF || b == BLOCK_WOOD || b == BLOCK_LOG || b == BLOCK_CRATE || b == BLOCK_FIRE)
		return SOUND_WOOD;

	if (b == BLOCK_ROPE)  return SOUND_CLOTH;
	if (b == BLOCK_SAND)  return SOUND_SAND;
	if (b == BLOCK_SNOW)  return SOUND_SNOW;
	if (b == BLOCK_GLASS) return SOUND_GLASS;

	if (b == BLOCK_DIRT  || b == BLOCK_GRAVEL) return SOUND_GRAVEL;
	if (b == BLOCK_GRASS || b == BLOCK_SAPLING || b == BLOCK_TNT || b == BLOCK_LEAVES || b == BLOCK_SPONGE)
		return SOUND_GRASS;

	if (b >= BLOCK_DANDELION && b <= BLOCK_RED_SHROOM)  return SOUND_GRASS;
	if (b >= BLOCK_WATER     && b <= BLOCK_STILL_LAVA)  return SOUND_NONE;
	if (b >= BLOCK_STONE     && b <= BLOCK_STONE_BRICK) return SOUND_STONE;
	return SOUND_NONE;
}

static cc_uint8 DefaultSet_StepSound(BlockID b) {
	if (b == BLOCK_GLASS) return SOUND_STONE;
	if (b == BLOCK_ROPE)  return SOUND_CLOTH;
	if (DefaultSet_Draw(b) == DRAW_SPRITE) return SOUND_NONE;
	return DefaultSet_DigSound(b);
}


/*########################################################################################################################*
*---------------------------------------------------------Block-----------------------------------------------------------*
*#########################################################################################################################*/
static cc_uint32 definedCustomBlocks[BLOCK_COUNT >> 5];
static char Block_NamesBuffer[STRING_SIZE * BLOCK_COUNT];
#define Block_NamePtr(i) &Block_NamesBuffer[STRING_SIZE * i]

static const cc_uint8 topTex[BLOCK_CPE_COUNT]     = { 0,  1,  0,  2, 16,  4, 15, 
17, 14, 14, 30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 
70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 24, 23,  6,  6,  7,  9,  
 4, 36, 37, 16, 11, 25, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 26, 53, 52 };

static const cc_uint8 sideTex[BLOCK_CPE_COUNT]    = { 0,  1,  3,  2, 16,  4, 15, 
17, 14, 14, 30, 30, 18, 19, 32, 33, 34, 20, 22, 48, 49, 64, 65, 66, 67, 68, 69, 
70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 40, 39,  5,  5,  7,  8, 
35, 36, 37, 16, 11, 41, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 42, 53, 52 };

static const cc_uint8 bottomTex[BLOCK_CPE_COUNT]  = { 0,  1,  2,  2, 16,  4, 15, 
17, 14, 14, 30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 
70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 56, 55,  6,  6,  7, 10,  
 4, 36, 37, 16, 11, 57, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 58, 53, 52 };

cc_bool Block_IsCustomDefined(BlockID block) {
	return (definedCustomBlocks[block >> 5] & (1u << (block & 0x1F))) != 0;
}

void Block_SetCustomDefined(BlockID block, cc_bool defined) {
	if (defined) {
		definedCustomBlocks[block >> 5] |=  (1u << (block & 0x1F));
	} else {
		definedCustomBlocks[block >> 5] &= ~(1u << (block & 0x1F));
	}
}

void Block_DefineCustom(BlockID block) {
	PackedCol black = PackedCol_Make(0, 0, 0, 255);
	String name     = Block_UNSAFE_GetName(block);
	Blocks.Tinted[block] = Blocks.FogCol[block] != black && String_IndexOf(&name, '#') >= 0;

	Block_SetDrawType(block, Blocks.Draw[block]);
	Block_CalcRenderBounds(block);
	Block_UpdateCulling(block);
	Block_CalcLightOffset(block);

	Inventory_AddDefault(block);
	Block_SetCustomDefined(block, true);
	Event_RaiseVoid(&BlockEvents.BlockDefChanged);
}

static void Block_RecalcIsLiquid(BlockID b) {
	cc_uint8 collide = Blocks.ExtendedCollide[b];
	Blocks.IsLiquid[b] =
		(collide == COLLIDE_LIQUID_WATER && Blocks.Draw[b] == DRAW_TRANSLUCENT) ||
		(collide == COLLIDE_LIQUID_LAVA  && Blocks.Draw[b] == DRAW_TRANSPARENT);
}

void Block_SetCollide(BlockID block, cc_uint8 collide) {
	/* necessary if servers redefined core blocks, before extended collide types were added */
	collide = DefaultSet_MapOldCollide(block, collide);
	Blocks.ExtendedCollide[block] = collide;
	Block_RecalcIsLiquid(block);

	/* Reduce extended collision types to their simpler forms */
	if (collide == COLLIDE_ICE)          collide = COLLIDE_SOLID;
	if (collide == COLLIDE_SLIPPERY_ICE) collide = COLLIDE_SOLID;

	if (collide == COLLIDE_LIQUID_WATER) collide = COLLIDE_LIQUID;
	if (collide == COLLIDE_LIQUID_LAVA)  collide = COLLIDE_LIQUID;
	Blocks.Collide[block] = collide;
}

void Block_SetDrawType(BlockID block, cc_uint8 draw) {
	if (draw == DRAW_OPAQUE && Blocks.Collide[block] != COLLIDE_SOLID) draw = DRAW_TRANSPARENT;
	Blocks.Draw[block] = draw;
	Block_RecalcIsLiquid(block);

	/* Whether a block is opaque and exactly occupies a cell in the world */
	/* The mesh builder module needs this information for optimisation purposes */
	Blocks.FullOpaque[block] = draw == DRAW_OPAQUE
		&& Blocks.MinBB[block].X == 0 && Blocks.MinBB[block].Y == 0 && Blocks.MinBB[block].Z == 0
		&& Blocks.MaxBB[block].X == 1 && Blocks.MaxBB[block].Y == 1 && Blocks.MaxBB[block].Z == 1;
}


#define BLOCK_RAW_NAMES "Air_Stone_Grass_Dirt_Cobblestone_Wood_Sapling_Bedrock_Water_Still water_Lava"\
"_Still lava_Sand_Gravel_Gold ore_Iron ore_Coal ore_Log_Leaves_Sponge_Glass_Red_Orange_Yellow_Lime_Green_Teal"\
"_Aqua_Cyan_Blue_Indigo_Violet_Magenta_Pink_Black_Gray_White_Dandelion_Rose_Brown mushroom_Red mushroom_Gold"\
"_Iron_Double slab_Slab_Brick_TNT_Bookshelf_Mossy rocks_Obsidian_Cobblestone slab_Rope_Sandstone_Snow_Fire_Light pink"\
"_Forest green_Brown_Deep blue_Turquoise_Ice_Ceramic tile_Magma_Pillar_Crate_Stone brick"

static const String Block_DefaultName(BlockID block) {
	static const String names   = String_FromConst(BLOCK_RAW_NAMES);
	static const String invalid = String_FromConst("Invalid");
	int i, beg = 0, end;

	if (block >= BLOCK_CPE_COUNT) return invalid;
	/* Find start and end of this particular block name. */
	for (i = 0; i < block; i++) {
		beg = String_IndexOfAt(&names, beg, '_') + 1;
	}

	end = String_IndexOfAt(&names, beg, '_');
	if (end == -1) end = names.length;
	return String_UNSAFE_Substring(&names, beg, end - beg);
}

void Block_ResetProps(BlockID block) {
	const String name = Block_DefaultName(block);

	Blocks.BlocksLight[block] = DefaultSet_BlocksLight(block);
	Blocks.FullBright[block] = DefaultSet_FullBright(block);
	Blocks.FogCol[block] = DefaultSet_FogColour(block);
	Blocks.FogDensity[block] = DefaultSet_FogDensity(block);
	Block_SetCollide(block, DefaultSet_Collide(block));
	Blocks.DigSounds[block] = DefaultSet_DigSound(block);
	Blocks.StepSounds[block] = DefaultSet_StepSound(block);
	Blocks.SpeedMultiplier[block] = 1.0f;
	Block_SetName(block, &name);
	Blocks.Tinted[block] = false;
	Blocks.SpriteOffset[block] = 0;

	Blocks.Draw[block] = DefaultSet_Draw(block);
	if (Blocks.Draw[block] == DRAW_SPRITE) {
		Vec3_Set(Blocks.MinBB[block], 2.50f/16.0f, 0, 2.50f/16.0f);
		Vec3_Set(Blocks.MaxBB[block], 13.5f/16.0f, 1, 13.5f/16.0f);
	} else {		
		Vec3_Set(Blocks.MinBB[block], 0, 0,                        0);
		Vec3_Set(Blocks.MaxBB[block], 1, DefaultSet_Height(block), 1);
	}

	Block_SetDrawType(block, Blocks.Draw[block]);
	Block_CalcRenderBounds(block);
	Block_CalcLightOffset(block);

	if (block >= BLOCK_CPE_COUNT) {
		Block_Tex(block, FACE_YMAX) = 0;
		Block_Tex(block, FACE_YMIN) = 0;
		Block_SetSide(0, block);
	} else {
		Block_Tex(block, FACE_YMAX) = topTex[block];
		Block_Tex(block, FACE_YMIN) = bottomTex[block];
		Block_SetSide(sideTex[block], block);
	}
}

STRING_REF String Block_UNSAFE_GetName(BlockID block) {
	return String_FromRaw(Block_NamePtr(block), STRING_SIZE);
}

void Block_SetName(BlockID block, const String* name) {
	String_CopyToRaw(Block_NamePtr(block), STRING_SIZE, name);
}

int Block_FindID(const String* name) {
	String blockName;
	int block;

	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		blockName = Block_UNSAFE_GetName(block);
		if (String_CaselessEquals(&blockName, name)) return block;
	}
	return -1;
}

int Block_Parse(const String* name) {
	int b;
	if (Convert_ParseInt(name, &b) && b < BLOCK_COUNT) return b;
	return Block_FindID(name);
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
void Block_CalcRenderBounds(BlockID block) {
	Vec3 min = Blocks.MinBB[block], max = Blocks.MaxBB[block];

	if (Blocks.IsLiquid[block]) {
		min.X -= 0.1f/16.0f; max.X -= 0.1f/16.0f;
		min.Z -= 0.1f/16.0f; max.Z -= 0.1f/16.0f;
		min.Y -= 1.5f/16.0f; max.Y -= 1.5f/16.0f;
	} else if (Blocks.Draw[block] == DRAW_TRANSLUCENT && Blocks.Collide[block] != COLLIDE_SOLID) {
		min.X += 0.1f/16.0f; max.X += 0.1f/16.0f;
		min.Z += 0.1f/16.0f; max.Z += 0.1f/16.0f;
		min.Y -= 0.1f/16.0f; max.Y -= 0.1f/16.0f;
	}

	Blocks.RenderMinBB[block] = min; Blocks.RenderMaxBB[block] = max;
}

void Block_CalcLightOffset(BlockID block) {
	int flags = 0xFF;
	Vec3 min = Blocks.MinBB[block], max = Blocks.MaxBB[block];

	if (min.X != 0) flags &= ~(1 << FACE_XMIN);
	if (max.X != 1) flags &= ~(1 << FACE_XMAX);
	if (min.Z != 0) flags &= ~(1 << FACE_ZMIN);
	if (max.Z != 1) flags &= ~(1 << FACE_ZMAX);

	if ((min.Y != 0 && max.Y == 1) && Blocks.Draw[block] != DRAW_GAS) {
		flags &= ~(1 << FACE_YMAX);
		flags &= ~(1 << FACE_YMIN);
	}
	Blocks.LightOffset[block] = flags;
}

void Block_RecalculateAllSpriteBB(void) {
	int block;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		if (Blocks.Draw[block] != DRAW_SPRITE) continue;

		Block_RecalculateBB((BlockID)block);
	}
}

static float Block_GetSpriteBB_MinX(int size, int tileX, int tileY, const Bitmap* bmp) {
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

static float Block_GetSpriteBB_MinY(int size, int tileX, int tileY, const Bitmap* bmp) {
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

static float Block_GetSpriteBB_MaxX(int size, int tileX, int tileY, const Bitmap* bmp) {
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

static float Block_GetSpriteBB_MaxY(int size, int tileX, int tileY, const Bitmap* bmp) {
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

void Block_RecalculateBB(BlockID block) {
	Bitmap* bmp  = &Atlas2D.Bmp;
	int tileSize = Atlas2D.TileSize;
	TextureLoc texLoc = Block_Tex(block, FACE_XMAX);
	int x = Atlas2D_TileX(texLoc), y = Atlas2D_TileY(texLoc);

	Vec3 centre = { 0.5f, 0.0f, 0.5f };
	float minX = 0, minY = 0, maxX = 1, maxY = 1;
	Vec3 minRaw, maxRaw;

	if (y < Atlas2D.RowsCount) {
		minX = Block_GetSpriteBB_MinX(tileSize, x, y, bmp);
		minY = Block_GetSpriteBB_MinY(tileSize, x, y, bmp);
		maxX = Block_GetSpriteBB_MaxX(tileSize, x, y, bmp);
		maxY = Block_GetSpriteBB_MaxY(tileSize, x, y, bmp);
	}

	minRaw = Vec3_RotateY3(minX - 0.5f, minY, 0.0f, 45.0f * MATH_DEG2RAD);
	maxRaw = Vec3_RotateY3(maxX - 0.5f, maxY, 0.0f, 45.0f * MATH_DEG2RAD);
	Vec3_Add(&Blocks.MinBB[block], &minRaw, &centre);
	Vec3_Add(&Blocks.MaxBB[block], &maxRaw, &centre);
	Block_CalcRenderBounds(block);
}


static void Block_CalcStretch(BlockID block) {
	/* faces which can be stretched on X axis */
	if (Blocks.MinBB[block].X == 0.0f && Blocks.MaxBB[block].X == 1.0f) {
		Blocks.CanStretch[block] |= 0x3C;
	} else {
		Blocks.CanStretch[block] &= 0xC3; /* ~0x3C */
	}

	/* faces which can be stretched on Z axis */
	if (Blocks.MinBB[block].Z == 0.0f && Blocks.MaxBB[block].Z == 1.0f) {
		Blocks.CanStretch[block] |= 0x03;
	} else {
		Blocks.CanStretch[block] &= 0xFC; /* ~0x03 */
	}
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

	/* Some blocks may not cull 'other' block, in which case just skip per-face check */
	/* e.g. sprite blocks, default leaves, will not cull any other blocks */
	if (!Block_MightCull(block, other)) {	
		Blocks.Hidden[(block * BLOCK_COUNT) + other] = 0;
		return;
	}

	bMin = Blocks.MinBB[block]; bMax = Blocks.MaxBB[block];
	oMin = Blocks.MinBB[other]; oMax = Blocks.MaxBB[other];

	/* Extend offsets of liquid down to match rendered position */
	/* This isn't completely correct, but works well enough */
	if (Blocks.IsLiquid[block]) bMax.Y -= 1.50f / 16.0f;
	if (Blocks.IsLiquid[other]) oMax.Y -= 1.50f / 16.0f;

	bothLiquid = Blocks.IsLiquid[block] && Blocks.IsLiquid[other];
	f = 0; /* mark all faces initially 'not hidden' */

	/* Whether the 'texture region' of a face on block fits inside corresponding region on other block */
	occludedX = (bMin.Z >= oMin.Z && bMax.Z <= oMax.Z) && (bMin.Y >= oMin.Y && bMax.Y <= oMax.Y);
	occludedY = (bMin.X >= oMin.X && bMax.X <= oMax.X) && (bMin.Z >= oMin.Z && bMax.Z <= oMax.Z);
	occludedZ = (bMin.X >= oMin.X && bMax.X <= oMax.X) && (bMin.Y >= oMin.Y && bMax.Y <= oMax.Y);

	f |= occludedX && oMax.X == 1.0f && bMin.X == 0.0f ? (1 << FACE_XMIN) : 0;
	f |= occludedX && oMin.X == 0.0f && bMax.X == 1.0f ? (1 << FACE_XMAX) : 0;
	f |= occludedZ && oMax.Z == 1.0f && bMin.Z == 0.0f ? (1 << FACE_ZMIN) : 0;
	f |= occludedZ && oMin.Z == 0.0f && bMax.Z == 1.0f ? (1 << FACE_ZMAX) : 0;
	f |= occludedY && (bothLiquid || (oMax.Y == 1.0f && bMin.Y == 0.0f)) ? (1 << FACE_YMIN) : 0;
	f |= occludedY && (bothLiquid || (oMin.Y == 0.0f && bMax.Y == 1.0f)) ? (1 << FACE_YMAX) : 0;
	Blocks.Hidden[(block * BLOCK_COUNT) + other] = f;
}

cc_bool Block_IsFaceHidden(BlockID block, BlockID other, Face face) {
	return (Blocks.Hidden[(block * BLOCK_COUNT) + other] & (1 << face)) != 0;
}

void Block_UpdateAllCulling(void) {
	int block, neighbour;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Block_CalcStretch((BlockID)block);
		for (neighbour = BLOCK_AIR; neighbour < BLOCK_COUNT; neighbour++) {
			Block_CalcCulling((BlockID)block, (BlockID)neighbour);
		}
	}
}

void Block_UpdateCulling(BlockID block) {
	int neighbour;
	Block_CalcStretch(block);
	
	for (neighbour = BLOCK_AIR; neighbour < BLOCK_COUNT; neighbour++) {
		Block_CalcCulling(block, (BlockID)neighbour);
		Block_CalcCulling((BlockID)neighbour, block);
	}
}


/*########################################################################################################################*
*-------------------------------------------------------AutoRotate--------------------------------------------------------*
*#########################################################################################################################*/
cc_bool AutoRotate_Enabled;

/* replaces a portion of a string, appends otherwise */
static void AutoRotate_Insert(String* str, int offset, const char* suffix) {
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
static int FindRotated(String* name, int offset);

static int GetRotated(String* name, int offset) {
	int rotated = FindRotated(name, offset);
	return rotated == -1 ? Block_FindID(name) : rotated;
}

static int RotateCorner(String* name, int offset) {
	float x = Game_SelectedPos.Intersect.X - (float)Game_SelectedPos.TranslatedPos.X;
	float z = Game_SelectedPos.Intersect.Z - (float)Game_SelectedPos.TranslatedPos.Z;

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

static int RotateVertical(String* name, int offset) {
	float y = Game_SelectedPos.Intersect.Y - (float)Game_SelectedPos.TranslatedPos.Y;

	if (y >= 0.5f) {
		AutoRotate_Insert(name, offset, "-U");
	} else {
		AutoRotate_Insert(name, offset, "-D");
	}
	return GetRotated(name, offset);
}

static int RotateFence(String* name, int offset) {
	float yaw;
	/* Fence type blocks */
	yaw = LocalPlayer_Instance.Base.Yaw;
	yaw = LocationUpdate_Clamp(yaw);

	if (yaw < 45.0f || (yaw >= 135.0f && yaw < 225.0f) || yaw > 315.0f) {
		AutoRotate_Insert(name, offset, "-WE");
	} else {
		AutoRotate_Insert(name, offset, "-NS");
	}
	return GetRotated(name, offset);
}

static int RotatePillar(String* name, int offset) {
	/* Thin pillar type blocks */
	Face face = Game_SelectedPos.Closest;

	if (face == FACE_YMAX || face == FACE_YMIN) {
		AutoRotate_Insert(name, offset, "-UD");
	} else if (face == FACE_XMAX || face == FACE_XMIN) {
		AutoRotate_Insert(name, offset, "-WE");
	} else if (face == FACE_ZMAX || face == FACE_ZMIN) {
		AutoRotate_Insert(name, offset, "-NS");
	}
	return GetRotated(name, offset);
}

static int RotateDirection(String* name, int offset) {
	float yaw;
	yaw = LocalPlayer_Instance.Base.Yaw;
	yaw = LocationUpdate_Clamp(yaw);

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

#define AR_EQ1(s, x)    (dir0 == x && dir1 == '\0')
#define AR_EQ2(s, x, y) (dir0 == x && dir1 == y)
static int FindRotated(String* name, int offset) {	
	String dir;
	char dir0, dir1;

	int dirIndex = String_LastIndexOfAt(name, offset, '-');
	if (dirIndex == -1) return -1; /* not a directional block */

	dir = String_UNSAFE_SubstringAt(name, dirIndex);
	dir.length -= offset;
	if (dir.length > 3) return -1;
	offset += dir.length;

	/* e.g. -D or -ns */
	dir0 = dir.length > 1 ? dir.buffer[1] : '\0'; Char_MakeLower(dir0);
	dir1 = dir.length > 2 ? dir.buffer[2] : '\0'; Char_MakeLower(dir1);

	if (AR_EQ2(dir, 'n','w') || AR_EQ2(dir, 'n','e') || AR_EQ2(dir, 's','w') || AR_EQ2(dir, 's','e')) {
		return RotateCorner(name, offset);
	} else if (AR_EQ1(dir, 'u') || AR_EQ1(dir, 'd')) {
		return RotateVertical(name, offset);
	} else if (AR_EQ1(dir, 'n') || AR_EQ1(dir, 'w') || AR_EQ1(dir, 's') || AR_EQ1(dir, 'e')) {
		return RotateDirection(name, offset);
	} else if (AR_EQ2(dir, 'u','d') || AR_EQ2(dir, 'w','e') || AR_EQ2(dir, 'n','s')) {
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
	String str; char strBuffer[STRING_SIZE * 2];
	String name;
	int rotated;
	
	name = Block_UNSAFE_GetName(block);
	String_InitArray(str, strBuffer);
	String_AppendString(&str, &name);

	/* need to copy since we change characters in name */
	rotated = FindRotated(&str, 0);
	return rotated == -1 ? block : (BlockID)rotated;
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
}

static void OnAtlasChanged(void* obj) { Block_RecalculateAllSpriteBB(); }
static void OnInit(void) {
	int block;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Blocks.CanPlace[block]  = true;
		Blocks.CanDelete[block] = true;
	}

	AutoRotate_Enabled = true;
	OnReset();
	Event_RegisterVoid(&TextureEvents.AtlasChanged, NULL, OnAtlasChanged);

	Blocks.CanPlace[BLOCK_AIR] = false;         Blocks.CanDelete[BLOCK_AIR] = false;
	Blocks.CanPlace[BLOCK_LAVA] = false;        Blocks.CanDelete[BLOCK_LAVA] = false;
	Blocks.CanPlace[BLOCK_WATER] = false;       Blocks.CanDelete[BLOCK_WATER] = false;
	Blocks.CanPlace[BLOCK_STILL_LAVA] = false;  Blocks.CanDelete[BLOCK_STILL_LAVA] = false;
	Blocks.CanPlace[BLOCK_STILL_WATER] = false; Blocks.CanDelete[BLOCK_STILL_WATER] = false;
	Blocks.CanPlace[BLOCK_BEDROCK] = false;     Blocks.CanDelete[BLOCK_BEDROCK] = false;
}

struct IGameComponent Blocks_Component = {
	OnInit,  /* Init  */
	NULL,    /* Free  */
	OnReset, /* Reset */
};
