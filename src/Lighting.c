#include "Lighting.h"
#include "Block.h"
#include "Funcs.h"
#include "MapRenderer.h"
#include "Platform.h"
#include "World.h"
#include "Logger.h"
#include "Event.h"
#include "Game.h"

cc_int16* Lighting_Heightmap;
#define HEIGHT_UNCALCULATED Int16_MaxValue

#define Lighting_CalcBody(get_block)\
for (y = maxY; y >= 0; y--, i -= World.OneY) {\
	block = get_block;\
\
	if (Blocks.BlocksLight[block]) {\
		offset = (Blocks.LightOffset[block] >> FACE_YMAX) & 1;\
		Lighting_Heightmap[hIndex] = y - offset;\
		return y - offset;\
	}\
}

static int Lighting_CalcHeightAt(int x, int maxY, int z, int hIndex) {
	int i = World_Pack(x, maxY, z);
	BlockID block;
	int y, offset;

#ifndef EXTENDED_BLOCKS
	Lighting_CalcBody(World.Blocks[i]);
#else
	if (World.IDMask <= 0xFF) {
		Lighting_CalcBody(World.Blocks[i]);
	} else {
		Lighting_CalcBody(World.Blocks[i] | (World.Blocks2[i] << 8));
	}
#endif

	Lighting_Heightmap[hIndex] = -10;
	return -10;
}

static int Lighting_GetLightHeight(int x, int z) {
	int hIndex = Lighting_Pack(x, z);
	int lightH = Lighting_Heightmap[hIndex];
	return lightH == HEIGHT_UNCALCULATED ? Lighting_CalcHeightAt(x, World.Height - 1, z, hIndex) : lightH;
}

/* Outside colour is same as sunlight colour, so we reuse when possible */
cc_bool Lighting_IsLit(int x, int y, int z) {
	return y > Lighting_GetLightHeight(x, z);
}

PackedCol Lighting_Col(int x, int y, int z) {
	return y > Lighting_GetLightHeight(x, z) ? Env.SunCol : Env.ShadowCol;
}

PackedCol Lighting_Col_XSide(int x, int y, int z) {
	return y > Lighting_GetLightHeight(x, z) ? Env.SunXSide : Env.ShadowXSide;
}

PackedCol Lighting_Col_Sprite_Fast(int x, int y, int z) {
	return y > Lighting_Heightmap[Lighting_Pack(x, z)] ? Env.SunCol : Env.ShadowCol;
}

PackedCol Lighting_Col_YMax_Fast(int x, int y, int z) {
	return y > Lighting_Heightmap[Lighting_Pack(x, z)] ? Env.SunCol : Env.ShadowCol;
}

PackedCol Lighting_Col_YMin_Fast(int x, int y, int z) {
	return y > Lighting_Heightmap[Lighting_Pack(x, z)] ? Env.SunYMin : Env.ShadowYMin;
}

PackedCol Lighting_Col_XSide_Fast(int x, int y, int z) {
	return y > Lighting_Heightmap[Lighting_Pack(x, z)] ? Env.SunXSide : Env.ShadowXSide;
}

PackedCol Lighting_Col_ZSide_Fast(int x, int y, int z) {
	return y > Lighting_Heightmap[Lighting_Pack(x, z)] ? Env.SunZSide : Env.ShadowZSide;
}

void Lighting_Refresh(void) {
	int i;
	for (i = 0; i < World.Width * World.Length; i++) {
		Lighting_Heightmap[i] = HEIGHT_UNCALCULATED;
	}
}


/*########################################################################################################################*
*----------------------------------------------------Lighting update------------------------------------------------------*
*#########################################################################################################################*/
static void Lighting_UpdateLighting(int x, int y, int z, BlockID oldBlock, BlockID newBlock, int index, int lightH) {
	cc_bool didBlock  = Blocks.BlocksLight[oldBlock];
	cc_bool nowBlocks = Blocks.BlocksLight[newBlock];
	int oldOffset     = (Blocks.LightOffset[oldBlock] >> FACE_YMAX) & 1;
	int newOffset     = (Blocks.LightOffset[newBlock] >> FACE_YMAX) & 1;
	BlockID above;

	/* Two cases we need to handle here: */
	if (didBlock == nowBlocks) {
		if (!didBlock) return;              /* a) both old and new block do not block light */
		if (oldOffset == newOffset) return; /* b) both blocks blocked light at the same Y coordinate */
	}

	if ((y - newOffset) >= lightH) {
		if (nowBlocks) {
			Lighting_Heightmap[index] = y - newOffset;
		} else {
			/* Part of the column is now visible to light, we don't know how exactly how high it should be though. */
			/* However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y */
			Lighting_CalcHeightAt(x, y, z, index);
		}
	} else if (y == lightH && oldOffset == 0) {
		/* For a solid block on top of an upside down slab, they will both have the same light height. */
		/* So we need to account for this particular case. */
		above = y == (World.Height - 1) ? BLOCK_AIR : World_GetBlock(x, y + 1, z);
		if (Blocks.BlocksLight[above]) return;

		if (nowBlocks) {
			Lighting_Heightmap[index] = y - newOffset;
		} else {
			Lighting_CalcHeightAt(x, y - 1, z, index);
		}
	}
}

static cc_bool Lighting_Needs(BlockID block, BlockID other) {
	return Blocks.Draw[block] != DRAW_OPAQUE || Blocks.Draw[other] != DRAW_GAS;
}

#define Lighting_NeedsNeighourBody(get_block)\
/* Update if any blocks in the chunk are affected by light change. */ \
for (; y >= minY; y--, i -= World.OneY) {\
	other    = get_block;\
	affected = y == nY ? Lighting_Needs(block, other) : Blocks.Draw[other] != DRAW_GAS;\
	if (affected) return true;\
}

static cc_bool Lighting_NeedsNeighour(BlockID block, int i, int minY, int y, int nY) {
	BlockID other;
	cc_bool affected;

#ifndef EXTENDED_BLOCKS
	Lighting_NeedsNeighourBody(World.Blocks[i]);
#else
	if (World.IDMask <= 0xFF) {
		Lighting_NeedsNeighourBody(World.Blocks[i]);
	} else {
		Lighting_NeedsNeighourBody(World.Blocks[i] | (World.Blocks2[i] << 8));
	}
#endif
	return false;
}

static void Lighting_ResetNeighbour(int x, int y, int z, BlockID block, int cx, int cy, int cz, int minCy, int maxCy) {
	int minY, maxY;

	if (minCy == maxCy) {
		minY = cy << CHUNK_SHIFT;

		if (Lighting_NeedsNeighour(block, World_Pack(x, y, z), minY, y, y)) {
			MapRenderer_RefreshChunk(cx, cy, cz);
		}
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			minY = (cy << CHUNK_SHIFT); 
			maxY = (cy << CHUNK_SHIFT) + CHUNK_MAX;
			if (maxY > World.MaxY) maxY = World.MaxY;

			if (Lighting_NeedsNeighour(block, World_Pack(x, maxY, z), minY, maxY, y)) {
				MapRenderer_RefreshChunk(cx, cy, cz);
			}
		}
	}
}

static void Lighting_ResetColumn(int cx, int cy, int cz, int minCy, int maxCy) {
	if (minCy == maxCy) {
		MapRenderer_RefreshChunk(cx, cy, cz);
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			MapRenderer_RefreshChunk(cx, cy, cz);
		}
	}
}

static void Lighting_RefreshAffected(int x, int y, int z, BlockID block, int oldHeight, int newHeight) {
	int cx = x >> CHUNK_SHIFT, bX = x & CHUNK_MASK;
	int cy = y >> CHUNK_SHIFT, bY = y & CHUNK_MASK;
	int cz = z >> CHUNK_SHIFT, bZ = z & CHUNK_MASK;

	/* NOTE: much faster to only update the chunks that are affected by the change in shadows, rather than the entire column. */
	int newCy = newHeight < 0 ? 0 : newHeight >> 4;
	int oldCy = oldHeight < 0 ? 0 : oldHeight >> 4;
	int minCy = min(oldCy, newCy), maxCy = max(oldCy, newCy);
	Lighting_ResetColumn(cx, cy, cz, minCy, maxCy);

	if (bX == 0 && cx > 0) {
		Lighting_ResetNeighbour(x - 1, y, z, block, cx - 1, cy, cz, minCy, maxCy);
	}
	if (bY == 0 && cy > 0 && Lighting_Needs(block, World_GetBlock(x, y - 1, z))) {
		MapRenderer_RefreshChunk(cx, cy - 1, cz);
	}
	if (bZ == 0 && cz > 0) {
		Lighting_ResetNeighbour(x, y, z - 1, block, cx, cy, cz - 1, minCy, maxCy);
	}

	if (bX == 15 && cx < MapRenderer_ChunksX - 1) {
		Lighting_ResetNeighbour(x + 1, y, z, block, cx + 1, cy, cz, minCy, maxCy);
	}
	if (bY == 15 && cy < MapRenderer_ChunksY - 1 && Lighting_Needs(block, World_GetBlock(x, y + 1, z))) {
		MapRenderer_RefreshChunk(cx, cy + 1, cz);
	}
	if (bZ == 15 && cz < MapRenderer_ChunksZ - 1) {
		Lighting_ResetNeighbour(x, y, z + 1, block, cx, cy, cz + 1, minCy, maxCy);
	}
}

void Lighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {
	int hIndex = Lighting_Pack(x, z);
	int lightH = Lighting_Heightmap[hIndex];
	int newHeight;

	/* Since light wasn't checked to begin with, means column never had meshes for any of its chunks built. */
	/* So we don't need to do anything. */
	if (lightH == HEIGHT_UNCALCULATED) return;

	Lighting_UpdateLighting(x, y, z, oldBlock, newBlock, hIndex, lightH);
	newHeight = Lighting_Heightmap[hIndex] + 1;
	Lighting_RefreshAffected(x, y, z, newBlock, lightH + 1, newHeight);
}


/*########################################################################################################################*
*---------------------------------------------------Lighting heightmap----------------------------------------------------*
*#########################################################################################################################*/
static int Lighting_InitialHeightmapCoverage(int x1, int z1, int xCount, int zCount, cc_int32* skip) {
	int elemsLeft = 0, index = 0, curRunCount = 0;
	int x, z, hIndex, lightH;

	for (z = 0; z < zCount; z++) {
		hIndex = Lighting_Pack(x1, z1 + z);
		for (x = 0; x < xCount; x++) {
			lightH = Lighting_Heightmap[hIndex++];

			skip[index] = 0;
			if (lightH == HEIGHT_UNCALCULATED) {
				elemsLeft++;
				curRunCount = 0;
			} else {
				skip[index - curRunCount]++;
				curRunCount++;
			}
			index++;
		}
		curRunCount = 0; /* We can only skip an entire X row at most. */
	}
	return elemsLeft;
}

#define Lighting_CalculateBody(get_block)\
for (y = World.Height - 1; y >= 0; y--) {\
	if (elemsLeft <= 0) { return true; } \
	mapIndex = World_Pack(x1, y, z1);\
	hIndex   = Lighting_Pack(x1, z1);\
\
	for (z = 0; z < zCount; z++) {\
		baseIndex = mapIndex;\
		index = z * xCount;\
		for (x = 0; x < xCount;) {\
			curRunCount = skip[index];\
			x += curRunCount; mapIndex += curRunCount; index += curRunCount;\
\
			if (x < xCount && Blocks.BlocksLight[get_block]) {\
				lightOffset = (Blocks.LightOffset[get_block] >> FACE_YMAX) & 1;\
				Lighting_Heightmap[hIndex + x] = (cc_int16)(y - lightOffset);\
				elemsLeft--;\
				skip[index] = 0;\
\
				offset = prevRunCount + curRunCount;\
				newRunCount = skip[index - offset] + 1;\
\
				/* consider case 1 0 1 0, where we are at last 0 */ \
				/* we need to make this 3 0 0 0 and advance by 1 */ \
				oldRunCount = (x - offset + newRunCount) < xCount ? skip[index - offset + newRunCount] : 0; \
				if (oldRunCount != 0) {\
					skip[index - offset + newRunCount] = 0; \
					newRunCount += oldRunCount; \
				} \
				skip[index - offset] = newRunCount; \
				x += oldRunCount; index += oldRunCount; mapIndex += oldRunCount; \
				prevRunCount = newRunCount; \
			} else { \
				prevRunCount = 0; \
			}\
			x++; mapIndex++; index++; \
		}\
		prevRunCount = 0;\
		hIndex += World.Width;\
		mapIndex = baseIndex + World.Width; /* advance one Z */ \
	}\
}

static cc_bool Lighting_CalculateHeightmapCoverage(int x1, int z1, int xCount, int zCount, int elemsLeft, cc_int32* skip) {
	int prevRunCount = 0, curRunCount, newRunCount, oldRunCount;
	int lightOffset, offset;
	int mapIndex, hIndex, baseIndex, index;
	int x, y, z;

#ifndef EXTENDED_BLOCKS
	Lighting_CalculateBody(World.Blocks[mapIndex]);
#else
	if (World.IDMask <= 0xFF) {
		Lighting_CalculateBody(World.Blocks[mapIndex]);
	} else {
		Lighting_CalculateBody(World.Blocks[mapIndex] | (World.Blocks2[mapIndex] << 8));
	}
#endif
	return false;
}

static void Lighting_FinishHeightmapCoverage(int x1, int z1, int xCount, int zCount) {
	int x, z, hIndex, lightH;

	for (z = 0; z < zCount; z++) {
		hIndex = Lighting_Pack(x1, z1 + z);
		for (x = 0; x < xCount; x++, hIndex++) {
			lightH = Lighting_Heightmap[hIndex];

			if (lightH == HEIGHT_UNCALCULATED) {
				Lighting_Heightmap[hIndex] = -10;
			}
		}
	}
}

void Lighting_LightHint(int startX, int startZ) {
	int x1 = max(startX, 0), x2 = min(World.Width,  startX + EXTCHUNK_SIZE);
	int z1 = max(startZ, 0), z2 = min(World.Length, startZ + EXTCHUNK_SIZE);
	int xCount = x2 - x1, zCount = z2 - z1;
	cc_int32 skip[EXTCHUNK_SIZE * EXTCHUNK_SIZE];

	int elemsLeft = Lighting_InitialHeightmapCoverage(x1, z1, xCount, zCount, skip);
	if (!Lighting_CalculateHeightmapCoverage(x1, z1, xCount, zCount, elemsLeft, skip)) {
		Lighting_FinishHeightmapCoverage(x1, z1, xCount, zCount);
	}
}


/*########################################################################################################################*
*---------------------------------------------------Lighting component----------------------------------------------------*
*#########################################################################################################################*/
static void Lighting_Reset(void) {
	Mem_Free(Lighting_Heightmap);
	Lighting_Heightmap = NULL;
}

static void Lighting_OnNewMapLoaded(void) {
	Lighting_Heightmap = (cc_int16*)Mem_Alloc(World.Width * World.Length, 2, "lighting heightmap");
	Lighting_Refresh();
}

struct IGameComponent Lighting_Component = {
	NULL,           /* Init  */
	Lighting_Reset, /* Free  */
	Lighting_Reset, /* Reset */
	Lighting_Reset, /* OnNewMap */
	Lighting_OnNewMapLoaded /* OnNewMapLoaded */
};
