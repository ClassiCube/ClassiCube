#include "Lighting.h"
#include "Block.h"
#include "Funcs.h"
#include "MapRenderer.h"
#include "Platform.h"
#include "World.h"
#include "ErrorHandler.h"
#include "Event.h"

Int16* Lighting_heightmap;
PackedCol shadow, shadowZSide, shadowXSide, shadowYBottom;
#define Lighting_Pack(x, z) ((x) + World_Width * (z))

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

void Lighting_EnvVariableChanged(void* obj, Int32 envVar) {
	if (envVar == ENV_VAR_SUN_COL) {
		Lighting_SetSun(WorldEnv_SunCol);
	} else if (envVar == ENV_VAR_SHADOW_COL) {
		Lighting_SetShadow(WorldEnv_ShadowCol);
	}
}

Int32 Lighting_CalcHeightAt(Int32 x, Int32 maxY, Int32 z, Int32 index) {
	Int32 y, i = World_Pack(x, maxY, z);

	for (y = maxY; y >= 0; y--, i -= World_OneY) {
		BlockID block = World_Blocks[i];
		if (Block_BlocksLight[block]) {
			Int32 offset = (Block_LightOffset[block] >> FACE_YMAX) & 1;
			Lighting_heightmap[index] = (Int16)(y - offset);
			return y - offset;
		}
	}
	Lighting_heightmap[index] = (Int16)-10;
	return -10;
}

Int32 Lighting_GetLightHeight(Int32 x, Int32 z) {
	Int32 index = (z * World_Width) + x;
	Int32 lightH = Lighting_heightmap[index];
	return lightH == Int16_MaxValue ? Lighting_CalcHeightAt(x, World_Height - 1, z, index) : lightH;
}

/* Outside colour is same as sunlight colour, so we reuse when possible */
bool Lighting_IsLit(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_GetLightHeight(x, z);
}

PackedCol Lighting_Col(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_GetLightHeight(x, z) ? Lighting_Outside : shadow;
}

PackedCol Lighting_Col_XSide(Int32 x, Int32 y, Int32 z) {
	return y > Lighting_GetLightHeight(x, z) ? Lighting_OutsideXSide : shadowXSide;
}

PackedCol Lighting_Col_Sprite_Fast(Int32 x, Int32 y, Int32 z) {
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

void Lighting_Refresh(void) {
	Int32 i;
	for (i = 0; i < World_Width * World_Length; i++) {
		Lighting_heightmap[i] = Int16_MaxValue;
	}
}


void Lighting_UpdateLighting(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock, Int32 index, Int32 lightH) {
	bool didBlock = Block_BlocksLight[oldBlock];
	bool nowBlocks = Block_BlocksLight[newBlock];
	Int32 oldOffset = (Block_LightOffset[oldBlock] >> FACE_YMAX) & 1;
	Int32 newOffset = (Block_LightOffset[newBlock] >> FACE_YMAX) & 1;

	/* Two cases we need to handle here: */
	if (didBlock == nowBlocks) {
		if (!didBlock) return;              /* a) both old and new block do not block light */
		if (oldOffset == newOffset) return; /* b) both blocks blocked light at the same Y coordinate */
	}

	if ((y - newOffset) >= lightH) {
		if (nowBlocks) {
			Lighting_heightmap[index] = (Int16)(y - newOffset);
		} else {
			/* Part of the column is now visible to light, we don't know how exactly how high it should be though. */
			/* However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y */
			Lighting_CalcHeightAt(x, y, z, index);
		}
	} else if (y == lightH && oldOffset == 0) {
		/* For a solid block on top of an upside down slab, they will both have the same light height. */
		/* So we need to account for this particular case. */
		BlockID above = y == (World_Height - 1) ? BLOCK_AIR : World_GetBlock(x, y + 1, z);
		if (Block_BlocksLight[above]) return;

		if (nowBlocks) {
			Lighting_heightmap[index] = (Int16)(y - newOffset);
		} else {
			Lighting_CalcHeightAt(x, y - 1, z, index);
		}
	}
}


bool Lighting_Needs(BlockID block, BlockID other) {
	return Block_Draw[block] != DRAW_OPAQUE || Block_Draw[other] != DRAW_GAS;
}

bool Lighting_NeedsNeighour(BlockID block, Int32 index, Int32 minY, Int32 y, Int32 nY) {
	/* Update if any blocks in the chunk are affected by light change. */
	for (; y >= minY; y--, index -= World_OneY) {
		BlockID other = World_Blocks[index];
		bool affected = y == nY ? Lighting_Needs(block, other) : Block_Draw[other] != DRAW_GAS;
		if (affected) return true;
	}
	return false;
}

void Lighting_ResetNeighbour(Int32 x, Int32 y, Int32 z, BlockID block,
	Int32 cx, Int32 cy, Int32 cz, Int32 minCy, Int32 maxCy) {
	if (minCy == maxCy) {
		Int32 minY = cy << 4;

		if (Lighting_NeedsNeighour(block, World_Pack(x, y, z), minY, y, y)) {
			MapRenderer_RefreshChunk(cx, cy, cz);
		}
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			Int32 minY = cy << 4, maxY = (cy << 4) + 15;
			if (maxY > World_MaxY) maxY = World_MaxY;

			if (Lighting_NeedsNeighour(block, World_Pack(x, maxY, z), minY, maxY, y)) {
				MapRenderer_RefreshChunk(cx, cy, cz);
			}
		}
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

void Lighting_RefreshAffected(Int32 x, Int32 y, Int32 z, BlockID block, Int32 oldHeight, Int32 newHeight) {
	Int32 cx = x >> 4, cy = y >> 4, cz = z >> 4;

	/* NOTE: much faster to only update the chunks that are affected by the change in shadows, rather than the entire column. */
	Int32 newCy = newHeight < 0 ? 0 : newHeight >> 4;
	Int32 oldCy = oldHeight < 0 ? 0 : oldHeight >> 4;
	Int32 minCy = min(oldCy, newCy), maxCy = max(oldCy, newCy);
	Lighting_ResetColumn(cx, cy, cz, minCy, maxCy);

	Int32 bX = x & 0x0F, bY = y & 0x0F, bZ = z & 0x0F;
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


Int32 Lighting_InitialHeightmapCoverage(Int32 x1, Int32 z1, Int32 xCount, Int32 zCount, Int32* skip) {
	Int32 elemsLeft = 0, index = 0, curRunCount = 0;
	Int32 x, z;

	for (z = 0; z < zCount; z++) {
		Int32 heightmapIndex = Lighting_Pack(x1, z1 + z);
		for (x = 0; x < xCount; x++) {
			Int32 lightH = Lighting_heightmap[heightmapIndex++];
			skip[index] = 0;
			if (lightH == Int16_MaxValue) {
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

bool Lighting_CalculateHeightmapCoverage(Int32 x1, Int32 z1, Int32 xCount, Int32 zCount, Int32 elemsLeft, Int32* skip) {
	Int32 prevRunCount = 0;
	Int32 x, y, z;

	for (y = World_Height - 1; y >= 0; y--) {
		if (elemsLeft <= 0) return true;
		Int32 mapIndex = World_Pack(x1, y, z1);
		Int32 heightmapIndex = Lighting_Pack(x1, z1);

		for (z = 0; z < zCount; z++) {
			Int32 baseIndex = mapIndex;
			Int32 index = z * xCount;
			for (x = 0; x < xCount;) {
				Int32 curRunCount = skip[index];
				x += curRunCount; mapIndex += curRunCount; index += curRunCount;

				if (x < xCount && Block_BlocksLight[World_Blocks[mapIndex]]) {
					Int32 lightOffset = (Block_LightOffset[World_Blocks[mapIndex]] >> FACE_YMAX) & 1;
					Lighting_heightmap[heightmapIndex + x] = (short)(y - lightOffset);
					elemsLeft--;
					skip[index] = 0;
					Int32 offset = prevRunCount + curRunCount;
					Int32 newRunCount = skip[index - offset] + 1;

					/* consider case 1 0 1 0, where we are at 0 */
					/* we need to make this 3 0 0 0 and advance by 1 */
					Int32 oldRunCount = (x - offset + newRunCount) < xCount ? skip[index - offset + newRunCount] : 0;
					if (oldRunCount != 0) {
						skip[index - offset + newRunCount] = 0;
						newRunCount += oldRunCount;
					}
					skip[index - offset] = newRunCount;
					x += oldRunCount; index += oldRunCount; mapIndex += oldRunCount;
					prevRunCount = newRunCount;
				} else {
					prevRunCount = 0;
				}
				x++; mapIndex++; index++;
			}
			prevRunCount = 0;
			heightmapIndex += World_Width;
			mapIndex = baseIndex + World_Width; /* advance one Z */
		}
	}
	return false;
}

void Lighting_FinishHeightmapCoverage(Int32 x1, Int32 z1, Int32 xCount, Int32 zCount) {
	Int32 x, z;

	for (z = 0; z < zCount; z++) {
		Int32 heightmapIndex = Lighting_Pack(x1, z1 + z);
		for (x = 0; x < xCount; x++) {
			Int32 lightH = Lighting_heightmap[heightmapIndex];
			if (lightH == Int16_MaxValue)
				Lighting_heightmap[heightmapIndex] = -10;
			heightmapIndex++;
		}
	}
}


void Lighting_LightHint(Int32 startX, Int32 startZ) {
	Int32 x1 = max(startX, 0), x2 = min(World_Width, startX + 18);
	Int32 z1 = max(startZ, 0), z2 = min(World_Length, startZ + 18);
	Int32 xCount = x2 - x1, zCount = z2 - z1;
	Int32 skip[18 * 18];

	Int32 elemsLeft = Lighting_InitialHeightmapCoverage(x1, z1, xCount, zCount, skip);
	if (!Lighting_CalculateHeightmapCoverage(x1, z1, xCount, zCount, elemsLeft, skip)) {
		Lighting_FinishHeightmapCoverage(x1, z1, xCount, zCount);
	}
}


void Lighting_Init(void) {
	Event_RegisterInt32(&WorldEvents_EnvVarChanged, NULL, &Lighting_EnvVariableChanged);
	Lighting_SetSun(WorldEnv_DefaultSunCol);
	Lighting_SetShadow(WorldEnv_DefaultShadowCol);
}

void Lighting_Reset(void) {
	if (Lighting_heightmap != NULL) {
		Platform_MemFree(Lighting_heightmap);
		Lighting_heightmap = NULL;
	}
}

void Lighting_OnNewMap(void) {
	Lighting_SetSun(WorldEnv_DefaultSunCol);
	Lighting_SetShadow(WorldEnv_DefaultShadowCol);
	Lighting_Reset();
}

void Lighting_OnNewMapLoaded(void) {
	UInt32 size = World_Width * World_Length * sizeof(Int16);
	Lighting_heightmap = Platform_MemAlloc(size);
	if (Lighting_heightmap == NULL) {
		ErrorHandler_Fail("WorldLighting - failed to allocate heightmap");
	}
	Lighting_Refresh();
}

void Lighting_Free(void) {
	Event_UnregisterInt32(&WorldEvents_EnvVarChanged, NULL, &Lighting_EnvVariableChanged);
	Lighting_Reset();
}


IGameComponent Lighting_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = Lighting_Init;
	comp.Free = Lighting_Free;
	comp.OnNewMap = Lighting_OnNewMap;
	comp.OnNewMapLoaded = Lighting_OnNewMapLoaded;
	comp.Reset = Lighting_Reset;
	return comp;
}