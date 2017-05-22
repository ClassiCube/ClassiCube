#include "Lighting.h"
#include "Block.h"
#include "BlockEnums.h"
#include "Funcs.h"
#include "Platform.h"
#include "World.h"
#include "WorldEnv.h"
#include "WorldEvents.h"
/* Manages lighting through a simple heightmap, where each block is either in sun or shadow. */

PackedCol shadow, shadowZSide, shadowXSide, shadowYBottom;

void Lighting_Init() {
	EventHandler_Register(WorldEvents_EnvVarChanged, &Lighting_EnvVariableChanged);
	Lighting_SetSun(WorldEnv_DefaultSunCol);
	Lighting_SetShadow(WorldEnv_DefaultShadowCol);
}

void Lighting_Reset() {
	if (Lighting_heightmap != NULL) {
		Platform_MemFree(Lighting_heightmap);
		Lighting_heightmap = NULL;
	}
}

void Lighting_OnNewMap() {
	SetSun(WorldEnv_DefaultSunCol);
	SetShadow(WorldEnv_DefaultShadowCol);
	Lighting_Reset();
}

void Lighting_OnNewMapLoaded() {
	UInt32 size = World_Width * World_Length * sizeof(Int16);
	Lighting_heightmap = Platform_MemAlloc(size);
	if (Lighting_heightmap == NULL) {
		ErrorHandler_Fail("WorldLighting - Failed to allocate heightmap");
	}
	Lighting_Refresh();
}

void Lighting_Free() {
	EventHandler_Unregister(WorldEvents_EnvVarChanged, &Lighting_EnvVariableChanged);
	Lighting_Reset();
}


void Lighting_EnvVariableChanged(EnvVar envVar) {
	if (envVar == EnvVar_SunCol) {
		SetSun(WorldEnv_SunCol);
	} else if (envVar == EnvVar_ShadowCol) {
		SetShadow(WorldEnv_ShadowCol);
	}
}

void Lighting_SetSun(PackedCol col) {
	Lighting_Outside = col;
	PackedCol_GetShaded(col, &Lighting_OutsideXSide,
		&Lighting_OutsideZSide, &Lighting_OutsideYBottom);
}

void Lighting_SetShadow(PackedCol col) {
	shadow = col;
	PackedCol_GetShaded(col, &shadowXSide,
		&shadowZSide, &shadowYBottom);
}


void Lighting_LightHint(Int32 startX, Int32 startZ, BlockID* mapPtr) {
	Int32 x1 = max(startX, 0), x2 = min(World_Width, startX + 18);
	Int32 z1 = max(startZ, 0), z2 = min(World_Length, startZ + 18);
	Int32 xCount = x2 - x1, zCount = z2 - z1;
	Int32 skip[18 * 18];

	Int32 elemsLeft = InitialHeightmapCoverage(x1, z1, xCount, zCount, skip);
	if (!CalculateHeightmapCoverage(x1, z1, xCount, zCount, elemsLeft, skip, mapPtr)) {
		FinishHeightmapCoverage(x1, z1, xCount, zCount, skip);
	}
}

Int32 GetLightHeight(Int32 x, Int32 z) {
	Int32 index = (z * World_Width) + x;
	Int32 lightH = Lighting_heightmap[index];
	return lightH == Int16_MaxValue ? CalcHeightAt(x, World_Height - 1, z, index) : lightH;
}


/* Outside colour is same as sunlight colour, so we reuse when possible */
bool Lighting_IsLit(Int32 x, Int32 y, Int32 z) {
	return y > GetLightHeight(x, z);
}

PackedCol Lighting_Col(Int32 x, Int32 y, Int32 z) {
	return y > GetLightHeight(x, z) ? Lighting_Outside : shadow;
}

PackedCol Lighting_Col_ZSide(Int32 x, Int32 y, Int32 z) {
	return y > GetLightHeight(x, z) ? Lighting_OutsideZSide : shadowZSide;
}


PackedCol Lighting_LightCol_Sprite_Fast(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_heightmap[(z * World_Width) + x] ? Lighting_Outside : shadow;
}

PackedCol Lighting_Col_YTop_Fast(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_heightmap[(z * World_Width) + x] ? Lighting_Outside : shadow;
}

PackedCol Lighting_Col_YBottom_Fast(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_heightmap[(z * World_Width) + x] ? Lighting_OutsideYBottom : shadowYBottom;
}

PackedCol Lighting_Col_XSide_Fast(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_heightmap[(z * World_Width) + x] ? Lighting_OutsideXSide : shadowXSide;
}

PackedCol Lighting_Col_ZSide_Fast(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_heightmap[(z * World_Width) + x] ? Lighting_OutsideZSide : shadowZSide;
}

void Lighting_Refresh() {
	Int32 i;
	for (i = 0; i < World_Width * World_Length; i++) {
		Lighting_heightmap[i] = Int16_MaxValue;
	}
}

void Lighting_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock) {
	Int32 index = (z * World_Width) + x;
	Int32 lightH = Lighting_heightmap[index];
	/* Since light wasn't checked to begin with, means column never had meshes for any of its chunks built. */
	/* So we don't need to do anything. */
	if (lightH == Int16_MaxValue) return;

	Lighting_UpdateLighting(x, y, z, oldBlock, newBlock, index, lightH);
	Int32 newHeight = Lighting_heightmap[index] + 1;
	Lighting_RefreshAffected(x, y, z, newBlock, lightH + 1, newHeight);
}

void Lighting_UpdateLighting(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock, Int32 index, Int32 lightH) {
	bool didBlock = Block_BlocksLight[oldBlock];
	bool nowBlocks = Block_BlocksLight[newBlock];
	Int32 oldOffset = (Block_LightOffset[oldBlock] >> Face_YMax) & 1;
	Int32 newOffset = (Block_LightOffset[newBlock] >> Face_YMax) & 1;

	// Two cases we need to handle here:
	if (didBlock == nowBlocks) {
		if (!didBlock) return;              // a) both old and new block do not block light
		if (oldOffset == newOffset) return; // b) both blocks blocked light at the same Y coordinate
	}

	if ((y - newOffset) >= lightH) {
		if (nowBlocks) {
			Lighting_heightmap[index] = (Int16)(y - newOffset);
		} else {
			/* Part of the column is now visible to light, we don't know how exactly how high it should be though. */
			/* However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y */
			CalcHeightAt(x, y, z, index);
		}
	} else if (y == lightH && oldOffset == 0) {
		/* For a solid block on top of an upside down slab, they will both have the same light height. */
		/* So we need to account for this particular case. */
		BlockID above = y == (World_Height - 1) ? BlockID_Air : World_GetBlock(x, y + 1, z);
		if (Block_BlocksLight[above]) return;

		if (nowBlocks) {
			Lighting_heightmap[index] = (Int16)(y - newOffset);
		} else {
			CalcHeightAt(x, y - 1, z, index);
		}
	}
}

void RefreshAffected(Int32 x, Int32 y, Int32 z, BlockID block, Int32 oldHeight, Int32 newHeight) {
	Int32 cx = x >> 4, cy = y >> 4, cz = z >> 4;

	/* NOTE: much faster to only update the chunks that are affected by the change in shadows, rather than the entire column. */
	Int32 newCy = newHeight < 0 ? 0 : newHeight >> 4;
	Int32 oldCy = oldHeight < 0 ? 0 : oldHeight >> 4;
	Int32 minCy = min(oldCy, newCy), maxCy = max(oldCy, newCy);
	ResetColumn(cx, cy, cz, minCy, maxCy);

	Int32 bX = x & 0x0F, bY = y & 0x0F, bZ = z & 0x0F;
	if (bX == 0 && cx > 0) {
		Lighting_ResetNeighbour(x - 1, y, z, block, cx - 1, cy, cz, minCy, maxCy);
	}
	if (bY == 0 && cy > 0 && Needs(block, World_GetBlock(x, y - 1, z))) {
		MapRenderer_RefreshChunk(cx, cy - 1, cz);
	}
	if (bZ == 0 && cz > 0) {
		Lighting_ResetNeighbour(x, y, z - 1, block, cx, cy, cz - 1, minCy, maxCy);
	}

	if (bX == 15 && cx < MapRenderer_ChunksX - 1) {
		ResetNeighbour(x + 1, y, z, block, cx + 1, cy, cz, minCy, maxCy);
	}
	if (bY == 15 && cy < MapRenderer_ChunksY - 1 && Lighting_Needs(block, World_GetBlock(x, y + 1, z))) {
		MapRenderer_RefreshChunk(cx, cy + 1, cz);
	}
	if (bZ == 15 && cz < MapRenderer_ChunksZ - 1) {
		Lighting_ResetNeighbour(x, y, z + 1, block, cx, cy, cz + 1, minCy, maxCy);
	}
}

bool Lighting_Needs(BlockID block, BlockID other) {
	return Block_Draw[block] != DrawType_Opaque || Block_Draw[other] != DrawType_Gas;
}

void Lighting_ResetNeighbour(Int32 x, Int32 y, Int32 z, BlockID block,
	Int32 cx, Int32 cy, Int32 cz, Int32 minCy, Int32 maxCy) {
	if (minCy == maxCy) {
		ResetNeighourChunk(cx, cy, cz, block, y, World_Pack(x, y, z), y);
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			Int32 maxY = (cy << 4) + 15;
			if (maxY > World_MaxY) maxY = World_MaxY;
			Lighting_ResetNeighourChunk(cx, cy, cz, block, maxY, World_Pack(x, maxY, z), y);
		}
	}
}

void ResetNeighourChunk(Int32 cx, Int32 cy, Int32 cz, BlockID block, Int32 y, Int32 index, Int32 nY) {
	Int32 minY = cy << 4;

	/* Update if any blocks in the chunk are affected by light change. */
	for (; y >= minY; y--) {
		BlockID other = World_Blocks[index];
		bool affected = y == nY ? Lighting_Needs(block, other) : Block_Draw[other] != DrawType_Gas;
		if (affected) { MapRenderer_RefreshChunk(cx, cy, cz); return; }
		index -= World_OneY;
	}
}

void Lighting_ResetColumn(Int32 cx, Int32 cy, Int32 cz, Int32 minCy, Int32 maxCy) {
	if (minCy == maxCy) {
		MapRenderer_RefreshChunk(cx, cy, cz);
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			MapRenderer_RefreshChunk(cx, cy, cz);
		}
	}
}