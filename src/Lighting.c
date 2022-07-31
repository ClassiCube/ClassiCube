#include "Lighting.h"
#include "Block.h"
#include "Funcs.h"
#include "MapRenderer.h"
#include "Platform.h"
#include "World.h"
#include "Logger.h"
#include "Event.h"
#include "Game.h"
#include "String.h"
#include "Chat.h"
#include "ExtMath.h"
#include "Options.h"
cc_bool Lighting_Modern;
struct _Lighting Lighting;

/*########################################################################################################################*
*---------------------------------------------------Lighting heightmap----------------------------------------------------*
*#########################################################################################################################*/
static cc_int16* heightmap;
#define HEIGHT_UNCALCULATED Int16_MaxValue
#define Heightmap_Pack(x, z) ((x) + World.Width * (z))

static int Heightmap_InitialCoverage(int x1, int z1, int xCount, int zCount, int* skip) {
	int elemsLeft = 0, index = 0, curRunCount = 0;
	int x, z, hIndex, lightH;

	for (z = 0; z < zCount; z++) {
		hIndex = Heightmap_Pack(x1, z1 + z);
		for (x = 0; x < xCount; x++) {
			lightH = heightmap[hIndex++];

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

#define Heightmap_CalculateBody(get_block)\
for (y = World.Height - 1; y >= 0; y--) {\
	if (elemsLeft <= 0) { return true; } \
	mapIndex = World_Pack(x1, y, z1);\
	hIndex   = Heightmap_Pack(x1, z1);\
\
	for (z = 0; z < zCount; z++) {\
		baseIndex = mapIndex;\
		index = z * xCount;\
		for (x = 0; x < xCount;) {\
			curRunCount = skip[index];\
			x += curRunCount; mapIndex += curRunCount; index += curRunCount;\
\
			if (x < xCount && Blocks.BlocksLight[get_block]) {\
				lightOffset = (Blocks.LightOffset[get_block] >> LIGHT_FLAG_SHADES_FROM_BELOW) & 1;\
				heightmap[hIndex + x] = (cc_int16)(y - lightOffset);\
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

static cc_bool Heightmap_CalculateCoverage(int x1, int z1, int xCount, int zCount, int elemsLeft, int* skip) {
	int prevRunCount = 0, curRunCount, newRunCount, oldRunCount;
	int lightOffset, offset;
	int mapIndex, hIndex, baseIndex, index;
	int x, y, z;

#ifndef EXTENDED_BLOCKS
	Heightmap_CalculateBody(World.Blocks[mapIndex]);
#else
	if (World.IDMask <= 0xFF) {
		Heightmap_CalculateBody(World.Blocks[mapIndex]);
	} else {
		Heightmap_CalculateBody(World.Blocks[mapIndex] | (World.Blocks2[mapIndex] << 8));
	}
#endif
	return false;
}

static void Heightmap_FinishCoverage(int x1, int z1, int xCount, int zCount) {
	int x, z, hIndex, lightH;

	for (z = 0; z < zCount; z++) {
		hIndex = Heightmap_Pack(x1, z1 + z);
		for (x = 0; x < xCount; x++, hIndex++) {
			lightH = heightmap[hIndex];

			if (lightH == HEIGHT_UNCALCULATED) {
				heightmap[hIndex] = -10;
			}
		}
	}
}

static void Heightmap_PreCalcColumns(int startX, int startZ) {
	int x1 = max(startX, 0), x2 = min(World.Width,  startX + EXTCHUNK_SIZE);
	int z1 = max(startZ, 0), z2 = min(World.Length, startZ + EXTCHUNK_SIZE);
	int xCount = x2 - x1, zCount = z2 - z1;
	int skip[EXTCHUNK_SIZE * EXTCHUNK_SIZE];

	int elemsLeft = Heightmap_InitialCoverage(x1, z1, xCount, zCount, skip);
	if (!Heightmap_CalculateCoverage(x1, z1, xCount, zCount, elemsLeft, skip)) {
		Heightmap_FinishCoverage(x1, z1, xCount, zCount);
	}
}


#define Heightmap_CalcBody(get_block)\
for (y = maxY; y >= 0; y--, i -= World.OneY) {\
	block = get_block;\
\
	if (Blocks.BlocksLight[block]) {\
		offset = (Blocks.LightOffset[block] >> LIGHT_FLAG_SHADES_FROM_BELOW) & 1;\
		heightmap[hIndex] = y - offset;\
		return y - offset;\
	}\
}

static int Heightmap_CalcHeightAt(int x, int maxY, int z, int hIndex) {
	int i = World_Pack(x, maxY, z);
	BlockID block;
	int y, offset;

#ifndef EXTENDED_BLOCKS
	Heightmap_CalcBody(World.Blocks[i]);
#else
	if (World.IDMask <= 0xFF) {
		Heightmap_CalcBody(World.Blocks[i]);
	} else {
		Heightmap_CalcBody(World.Blocks[i] | (World.Blocks2[i] << 8));
	}
#endif

	heightmap[hIndex] = -10;
	return -10;
}

static int Heightmap_GetLightHeight(int x, int z) {
	int hIndex = Heightmap_Pack(x, z);
	int lightH = heightmap[hIndex];
	return lightH == HEIGHT_UNCALCULATED ? Heightmap_CalcHeightAt(x, World.Height - 1, z, hIndex) : lightH;
}

static void Heightmap_Reset(void) {
	int i;
	for (i = 0; i < World.Width * World.Length; i++) {
		heightmap[i] = HEIGHT_UNCALCULATED;
	}
}

static void Heightmap_Free(void) {
	Mem_Free(heightmap);
	heightmap = NULL;
}

static void Heightmap_Allocate(void) {
	heightmap = (cc_int16*)Mem_TryAlloc(World.Width * World.Length, 2);
	if (heightmap) {
		Heightmap_Reset();
	} else {
		World_OutOfMemory();
	}
}


/*########################################################################################################################*
*----------------------------------------------------Classic lighting-----------------------------------------------------*
*#########################################################################################################################*/
/* Outside color is same as sunlight color, so we reuse when possible */
static cc_bool ClassicLighting_IsLit(int x, int y, int z) {
	return y > Heightmap_GetLightHeight(x, z);
}

static cc_bool ClassicLighting_IsLit_Fast(int x, int y, int z) {
	return y > heightmap[Heightmap_Pack(x, z)];
}

static PackedCol ClassicLighting_Color(int x, int y, int z) {
	if (!World_Contains(x, y, z)) return Env.SunCol;
	return y > Heightmap_GetLightHeight(x, z) ? Env.SunCol : Env.ShadowCol;
}

static PackedCol ClassicLighting_Color_XSide(int x, int y, int z) {
	if (!World_Contains(x, y, z)) return Env.SunXSide;
	return y > Heightmap_GetLightHeight(x, z) ? Env.SunXSide : Env.ShadowXSide;
}

static PackedCol ClassicLighting_Color_Fast(int x, int y, int z) {
	return y > heightmap[Heightmap_Pack(x, z)] ? Env.SunCol : Env.ShadowCol;
}

static PackedCol ClassicLighting_Color_YMin_Fast(int x, int y, int z) {
	return y > heightmap[Heightmap_Pack(x, z)] ? Env.SunYMin : Env.ShadowYMin;
}

static PackedCol ClassicLighting_Color_XSide_Fast(int x, int y, int z) {
	return y > heightmap[Heightmap_Pack(x, z)] ? Env.SunXSide : Env.ShadowXSide;
}

static PackedCol ClassicLighting_Color_ZSide_Fast(int x, int y, int z) {
	return y > heightmap[Heightmap_Pack(x, z)] ? Env.SunZSide : Env.ShadowZSide;
}

static void ClassicLighting_LightHint(int startX, int startY, int startZ) {
	Heightmap_PreCalcColumns(startX, startZ);
}


static void ClassicLighting_UpdateLighting(int x, int y, int z, BlockID oldBlock, BlockID newBlock, int index, int lightH) {
	cc_bool didBlock = Blocks.BlocksLight[oldBlock];
	cc_bool nowBlocks = Blocks.BlocksLight[newBlock];
	int oldOffset = (Blocks.LightOffset[oldBlock] >> LIGHT_FLAG_SHADES_FROM_BELOW) & 1;
	int newOffset = (Blocks.LightOffset[newBlock] >> LIGHT_FLAG_SHADES_FROM_BELOW) & 1;
	BlockID above;

	/* Two cases we need to handle here: */
	if (didBlock == nowBlocks) {
		if (!didBlock) return;              /* a) both old and new block do not block light */
		if (oldOffset == newOffset) return; /* b) both blocks blocked light at the same Y coordinate */
	}

	if ((y - newOffset) >= lightH) {
		if (nowBlocks) {
			heightmap[index] = y - newOffset;
		} else {
			/* Part of the column is now visible to light, we don't know how exactly how high it should be though. */
			/* However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y */
			Heightmap_CalcHeightAt(x, y, z, index);
		}
	} else if (y == lightH && oldOffset == 0) {
		/* For a solid block on top of an upside down slab, they will both have the same light height. */
		/* So we need to account for this particular case. */
		above = y == (World.Height - 1) ? BLOCK_AIR : World_GetBlock(x, y + 1, z);
		if (Blocks.BlocksLight[above]) return;

		if (nowBlocks) {
			heightmap[index] = y - newOffset;
		} else {
			Heightmap_CalcHeightAt(x, y - 1, z, index);
		}
	}
}

static cc_bool ClassicLighting_Needs(BlockID block, BlockID other) {
	return Blocks.Draw[block] != DRAW_OPAQUE || Blocks.Draw[other] != DRAW_GAS;
}

#define ClassicLighting_NeedsNeighourBody(get_block)\
/* Update if any blocks in the chunk are affected by light change. */ \
for (; y >= minY; y--, i -= World.OneY) {\
	other    = get_block;\
	affected = y == nY ? ClassicLighting_Needs(block, other) : Blocks.Draw[other] != DRAW_GAS;\
	if (affected) return true;\
}

static cc_bool ClassicLighting_NeedsNeighour(BlockID block, int i, int minY, int y, int nY) {
	BlockID other;
	cc_bool affected;

#ifndef EXTENDED_BLOCKS
	Lighting_NeedsNeighourBody(World.Blocks[i]);
#else
	if (World.IDMask <= 0xFF) {
		ClassicLighting_NeedsNeighourBody(World.Blocks[i]);
	} else {
		ClassicLighting_NeedsNeighourBody(World.Blocks[i] | (World.Blocks2[i] << 8));
	}
#endif
	return false;
}

static void ClassicLighting_ResetNeighbour(int x, int y, int z, BlockID block, int cx, int cy, int cz, int minCy, int maxCy) {
	int minY, maxY;

	if (minCy == maxCy) {
		minY = cy << CHUNK_SHIFT;

		if (ClassicLighting_NeedsNeighour(block, World_Pack(x, y, z), minY, y, y)) {
			MapRenderer_RefreshChunk(cx, cy, cz);
		}
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			minY = (cy << CHUNK_SHIFT);
			maxY = (cy << CHUNK_SHIFT) + CHUNK_MAX;
			if (maxY > World.MaxY) maxY = World.MaxY;

			if (ClassicLighting_NeedsNeighour(block, World_Pack(x, maxY, z), minY, maxY, y)) {
				MapRenderer_RefreshChunk(cx, cy, cz);
			}
		}
	}
}

static void ClassicLighting_ResetColumn(int cx, int cy, int cz, int minCy, int maxCy) {
	if (minCy == maxCy) {
		MapRenderer_RefreshChunk(cx, cy, cz);
	} else {
		for (cy = maxCy; cy >= minCy; cy--) {
			MapRenderer_RefreshChunk(cx, cy, cz);
		}
	}
}

static void ClassicLighting_RefreshAffected(int x, int y, int z, BlockID block, int oldHeight, int newHeight) {
	int cx = x >> CHUNK_SHIFT, bX = x & CHUNK_MASK;
	int cy = y >> CHUNK_SHIFT, bY = y & CHUNK_MASK;
	int cz = z >> CHUNK_SHIFT, bZ = z & CHUNK_MASK;

	/* NOTE: much faster to only update the chunks that are affected by the change in shadows, rather than the entire column. */
	int newCy = newHeight < 0 ? 0 : newHeight >> 4;
	int oldCy = oldHeight < 0 ? 0 : oldHeight >> 4;
	int minCy = min(oldCy, newCy), maxCy = max(oldCy, newCy);
	ClassicLighting_ResetColumn(cx, cy, cz, minCy, maxCy);

	if (bX == 0 && cx > 0) {
		ClassicLighting_ResetNeighbour(x - 1, y, z, block, cx - 1, cy, cz, minCy, maxCy);
	}
	if (bY == 0 && cy > 0 && ClassicLighting_Needs(block, World_GetBlock(x, y - 1, z))) {
		MapRenderer_RefreshChunk(cx, cy - 1, cz);
	}
	if (bZ == 0 && cz > 0) {
		ClassicLighting_ResetNeighbour(x, y, z - 1, block, cx, cy, cz - 1, minCy, maxCy);
	}

	if (bX == 15 && cx < World.ChunksX - 1) {
		ClassicLighting_ResetNeighbour(x + 1, y, z, block, cx + 1, cy, cz, minCy, maxCy);
	}
	if (bY == 15 && cy < World.ChunksY - 1 && ClassicLighting_Needs(block, World_GetBlock(x, y + 1, z))) {
		MapRenderer_RefreshChunk(cx, cy + 1, cz);
	}
	if (bZ == 15 && cz < World.ChunksZ - 1) {
		ClassicLighting_ResetNeighbour(x, y, z + 1, block, cx, cy, cz + 1, minCy, maxCy);
	}
}

static void ClassicLighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {
	int hIndex = Heightmap_Pack(x, z);
	int lightH = heightmap[hIndex];
	int newHeight;

	/* Since light wasn't checked to begin with, means column never had meshes for any of its chunks built. */
	/* So we don't need to do anything. */
	if (lightH == HEIGHT_UNCALCULATED) return;

	ClassicLighting_UpdateLighting(x, y, z, oldBlock, newBlock, hIndex, lightH);
	newHeight = heightmap[hIndex] + 1;
	ClassicLighting_RefreshAffected(x, y, z, newBlock, lightH + 1, newHeight);
}

static void ClassicLighting_SetActive(void) {
	Lighting.OnBlockChanged = ClassicLighting_OnBlockChanged;
	Lighting.Refresh = Heightmap_Reset;;
	Lighting.IsLit   = ClassicLighting_IsLit;
	Lighting.Color   = ClassicLighting_Color;
	Lighting.Color_XSide = ClassicLighting_Color_XSide;

	Lighting.IsLit_Fast = ClassicLighting_IsLit_Fast;
	Lighting.Color_Fast = ClassicLighting_Color_Fast;
	Lighting.Color_YMin_Fast  = ClassicLighting_Color_YMin_Fast;
	Lighting.Color_XSide_Fast = ClassicLighting_Color_XSide_Fast;
	Lighting.Color_ZSide_Fast = ClassicLighting_Color_ZSide_Fast;

	Lighting.FreeState  = Heightmap_Free;
	Lighting.AllocState = Heightmap_Allocate;
	Lighting.LightHint  = ClassicLighting_LightHint;
}


/*########################################################################################################################*
*----------------------------------------------------Queue thing ---------------------------------------------------------*
*#########################################################################################################################*/

struct LightQueue {
	IVec3* entries;     /* Buffer holding the items in the Block queue */
	int capacity; /* Max number of elements in the buffer */
	int mask;     /* capacity - 1, as capacity is always a power of two */
	int count;    /* Number of used elements */
	int head;     /* Head index into the buffer */
	int tail;     /* Tail index into the buffer */
};
IVec3 LightQueue_EntryAtIndex(struct LightQueue* queue, int index) {
	return queue->entries[(queue->head + index) & queue->mask];
}
void LightQueue_Init(struct LightQueue* queue) {
	queue->entries = NULL;
	queue->capacity = 0;
	queue->mask = 0;
	queue->count = 0;
	queue->head = 0;
	queue->tail = 0;
}
static void LightQueue_Clear(struct LightQueue* queue) {
	if (!queue->entries) return;
	Mem_Free(queue->entries);
	LightQueue_Init(queue);
}
static void LightQueue_Resize(struct LightQueue* queue) {
	IVec3* entries;
	int i, idx, capacity;
	if (queue->capacity >= (Int32_MaxValue / 4)) {
		Chat_AddRaw("&cToo many block queue entries, clearing");
		LightQueue_Clear(queue);
		return;
	}
	capacity = queue->capacity * 2;
	if (capacity < 32) capacity = 32;
	entries = (IVec3*)Mem_Alloc(capacity, sizeof(IVec3), "Light queue");
	for (i = 0; i < queue->count; i++) {
		idx = (queue->head + i) & queue->mask;
		entries[i] = queue->entries[idx];
	}
	Mem_Free(queue->entries);
	queue->entries = entries;
	queue->capacity = capacity;
	queue->mask = capacity - 1; /* capacity is power of two */
	queue->head = 0;
	queue->tail = queue->count;
}
/* Appends an entry to the end of the queue, resizing if necessary. */
void LightQueue_Enqueue(struct LightQueue* queue, IVec3 item) {
	if (queue->count == queue->capacity)
		LightQueue_Resize(queue);
	queue->entries[queue->tail] = item;
	queue->tail = (queue->tail + 1) & queue->mask;
	queue->count++;
}
/* Retrieves the entry from the front of the queue. */
IVec3 LightQueue_Dequeue(struct LightQueue* queue) {
	IVec3 result = queue->entries[queue->head];
	queue->head = (queue->head + 1) & queue->mask;
	queue->count--;
	return result;
}
static struct LightQueue lightQueue;


/*########################################################################################################################*
*----------------------------------------------------Modern lighting------------------------------------------------------*
*#########################################################################################################################*/
/* How many unique "levels" of light there are when modern lighting is used. */
#define MODERN_LIGHTING_LEVELS 16
#define MODERN_LIGHTING_MAX_LEVEL MODERN_LIGHTING_LEVELS - 1
/* How many bits to shift sunlight level to the left when storing it in a byte along with blocklight level*/
#define MODERN_LIGHTING_SUN_SHIFT 4

/* A 16x16 palette of sun and block light colors. */
/* It is indexed by a byte where the leftmost 4 bits represent sunlight level and the rightmost 4 bits represent blocklight level */
/* E.G. modernLighting_palette[0b_0010_0001] will give us the color for sun level 2 and block level 1 (lowest level is 0) */
static PackedCol modernLighting_palette[MODERN_LIGHTING_LEVELS * MODERN_LIGHTING_LEVELS];
static PackedCol modernLighting_paletteX[MODERN_LIGHTING_LEVELS * MODERN_LIGHTING_LEVELS];
static PackedCol modernLighting_paletteZ[MODERN_LIGHTING_LEVELS * MODERN_LIGHTING_LEVELS];
static PackedCol modernLighting_paletteY[MODERN_LIGHTING_LEVELS * MODERN_LIGHTING_LEVELS];

typedef cc_uint8* LightingChunk;
static cc_uint8* chunkLightingDataFlags;
#define CHUNK_UNCALCULATED 0
#define CHUNK_SELF_CALCULATED 1
#define CHUNK_ALL_CALCULATED 2
static LightingChunk* chunkLightingData;
static cc_uint8 allDarkChunkLightingData[CHUNK_SIZE_3];

#define Modern_MakePaletteIndex(sun, block) ((sun << MODERN_LIGHTING_SUN_SHIFT) | block)

/* Fill in modernLighting_palette with values based on the current environment colors in lieu of recieving a palette from the server */
static void ModernLighting_InitPalette(PackedCol* palette, float shaded) {
	PackedCol darkestShadow, defaultBlockLight, blockColor, sunColor, invertedBlockColor, invertedSunColor, finalColor;
	int sunLevel, blockLevel;
	float blockLerp;
	cc_uint8 R, G, B;

	defaultBlockLight = PackedCol_Make(255, 235, 198, 255); /* A very mildly orange tinted light color */
	darkestShadow = PackedCol_Lerp(Env.ShadowCol, 0, 0.75f); /* Use a darkened version of shadow color as the darkest color in sun ramp */

	for (sunLevel = 0; sunLevel < MODERN_LIGHTING_LEVELS; sunLevel++) {
		for (blockLevel = 0; blockLevel < MODERN_LIGHTING_LEVELS; blockLevel++) {
			/* We want the brightest light level to be the sun env color, with all other 15 levels being interpolation */
			/* between shadow color and darkest shadow color */
			if (sunLevel == MODERN_LIGHTING_LEVELS - 1) {
				sunColor = Env.SunCol;
			} else if (sunLevel == MODERN_LIGHTING_LEVELS - 2) {
				sunColor = PackedCol_Lerp(Env.SunCol, Env.ShadowCol, 0.5F);
			} else {
				//sunColor = PackedCol_Lerp(darkestShadow, Env.ShadowCol, sunLevel / (float)(MODERN_LIGHTING_LEVELS - 3));
				blockLerp = sunLevel / (float)(MODERN_LIGHTING_LEVELS - 3);
				//blockLerp *= blockLerp;
				blockLerp *= (MATH_PI / 2);
				blockLerp = Math_Cos(blockLerp);
				sunColor = PackedCol_Lerp(darkestShadow, Env.ShadowCol, 1 - blockLerp);
			}

			blockLerp = blockLevel / (float)(MODERN_LIGHTING_LEVELS - 1);
			//blockLerp *= blockLerp;
			blockLerp *= (MATH_PI / 2);
			blockLerp = Math_Cos(blockLerp);
			blockColor = PackedCol_Lerp(0, defaultBlockLight, 1 - blockLerp);

			/* With Screen blend mode, the values of the pixels in the two layers are inverted, multiplied, and then inverted again. */
			R = 255 - PackedCol_R(sunColor);
			G = 255 - PackedCol_G(sunColor);
			B = 255 - PackedCol_B(sunColor);
			invertedSunColor = PackedCol_Make(R, G, B, 255);
			R = 255 - PackedCol_R(blockColor);
			G = 255 - PackedCol_G(blockColor);
			B = 255 - PackedCol_B(blockColor);
			invertedBlockColor = PackedCol_Make(R, G, B, 255);

			finalColor = PackedCol_Tint(invertedSunColor, invertedBlockColor);

			R = 255 - PackedCol_R(finalColor);
			G = 255 - PackedCol_G(finalColor);
			B = 255 - PackedCol_B(finalColor);
			palette[Modern_MakePaletteIndex(sunLevel, blockLevel)] =
				PackedCol_Scale(PackedCol_Make(R, G, B, 255), shaded);
			//	PackedCol_Scale(PackedCol_Make(R, G, B, 255),
			//		shaded + ((1-shaded) * ((MODERN_LIGHTING_LEVELS - sunLevel+1) / (float)MODERN_LIGHTING_LEVELS))
			//	);
		}
	}
}
static void ModernLighting_InitPalettes(void) {
	ModernLighting_InitPalette(modernLighting_palette, 1);
	ModernLighting_InitPalette(modernLighting_paletteX, PACKEDCOL_SHADE_X);
	ModernLighting_InitPalette(modernLighting_paletteZ, PACKEDCOL_SHADE_Z);
	ModernLighting_InitPalette(modernLighting_paletteY, PACKEDCOL_SHADE_YMIN);
}

static int chunksCount;
static void ModernLighting_AllocState(void) {
	Heightmap_Allocate();
	ModernLighting_InitPalettes();
	chunksCount = World.ChunksCount;

	chunkLightingDataFlags = (cc_uint8*)Mem_TryAllocCleared(chunksCount, sizeof(cc_uint8));
	chunkLightingData = (LightingChunk*)Mem_TryAllocCleared(chunksCount, sizeof(LightingChunk));
	LightQueue_Init(&lightQueue);

}

static void ModernLighting_FreeState(void) {
	Heightmap_Free();
	int i;
	/* This function can be called multiple times without calling ModernLighting_AllocState, so... */
	if (!chunkLightingDataFlags) return;

	for (i = 0; i < chunksCount; i++) {
		if (chunkLightingDataFlags[i] == CHUNK_UNCALCULATED) continue;
		Mem_Free(chunkLightingData[i]);
	}

	Mem_Free(chunkLightingDataFlags);
	Mem_Free(chunkLightingData);
	chunkLightingDataFlags = NULL;
	chunkLightingData      = NULL;
	LightQueue_Clear(&lightQueue);
}

/* Converts chunk x/y/z coordinates to the corresponding index in chunks array/list */
#define ChunkCoordsToIndex(cx, cy, cz) (((cy) * World.ChunksZ + (cz)) * World.ChunksX + (cx))
/* Converts local x/y/z coordinates to the corresponding index in a chunk */
#define LocalCoordsToIndex(lx, ly, lz) ((lx) | ((lz) << CHUNK_SHIFT) | ((ly) << (CHUNK_SHIFT * 2)))

static void SetBlocklight(cc_uint8 blockLight, int x, int y, int z, cc_bool sun) {
	cc_uint8 clearMask;
	cc_uint8 shift = sun ? MODERN_LIGHTING_SUN_SHIFT : 0;

	int cx = x >> CHUNK_SHIFT, lx = x & CHUNK_MASK;
	int cy = y >> CHUNK_SHIFT, ly = y & CHUNK_MASK;
	int cz = z >> CHUNK_SHIFT, lz = z & CHUNK_MASK;

	int chunkIndex = ChunkCoordsToIndex(cx, cy, cz);
	if (chunkLightingData[chunkIndex] == NULL) {
		chunkLightingData[chunkIndex] = (cc_uint8*)Mem_TryAllocCleared(CHUNK_SIZE_3, sizeof(cc_uint8));
	}

	int localIndex = LocalCoordsToIndex(lx, ly, lz);

	/* 00001111 if sun, otherwise 11110000*/
	clearMask = ~(MODERN_LIGHTING_MAX_LEVEL << shift);

	chunkLightingData[chunkIndex][localIndex] &= clearMask;
	chunkLightingData[chunkIndex][localIndex] |= blockLight << shift;
}
static cc_uint8 GetBlocklight(int x, int y, int z, cc_bool sun) {
	int cx = x >> CHUNK_SHIFT, lx = x & CHUNK_MASK;
	int cy = y >> CHUNK_SHIFT, ly = y & CHUNK_MASK;
	int cz = z >> CHUNK_SHIFT, lz = z & CHUNK_MASK;

	int chunkIndex = ChunkCoordsToIndex(cx, cy, cz);
	if (chunkLightingData[chunkIndex] == NULL) { return 0; }
	int localIndex = LocalCoordsToIndex(lx, ly, lz);

	return sun ?
		chunkLightingData[chunkIndex][localIndex] >> MODERN_LIGHTING_SUN_SHIFT :
		chunkLightingData[chunkIndex][localIndex] & MODERN_LIGHTING_MAX_LEVEL;
}

static cc_bool CanLightPass(BlockID thisBlock, Face face) {
	/* If it's not opaque and it doesn't block light, or it's fullbright, we can always pass through */
	if ((Blocks.Draw[thisBlock] > DRAW_OPAQUE && !Blocks.BlocksLight[thisBlock]) || Blocks.FullBright[thisBlock]) { return true; }
	/* Light can always pass through leaves and water */
	if (Blocks.Draw[thisBlock] == DRAW_TRANSPARENT_THICK || Blocks.Draw[thisBlock] == DRAW_TRANSLUCENT) { return true; }

	/* Light can never pass through a block that's full sized and blocks light */
	/* We can assume a block is full sized if none of the LightOffset flags are 0 */
	if (Blocks.BlocksLight[thisBlock] && Blocks.LightOffset[thisBlock] == 0xFF) { return false; }

	/* Is stone's face hidden by thisBlock? */
	return !Block_IsFaceHidden(BLOCK_STONE, thisBlock, face);
}

static void CalcBlockLight(cc_uint8 blockLight, int x, int y, int z) {
	SetBlocklight(blockLight, x, y, z, false);
	IVec3 entry = { x, y, z };
	LightQueue_Enqueue(&lightQueue, entry);

	//if (Blocks.BlocksLight[World_GetBlock(x, y, z)]) { return; }

	while (lightQueue.count > 0) {
		IVec3 curNode = LightQueue_Dequeue(&lightQueue);
		cc_uint8 curLight = GetBlocklight(curNode.X, curNode.Y, curNode.Z, false);
		if (curLight <= 0) {
			Platform_Log1("but there were still %i entries left...", &lightQueue.capacity);
			return;
		}

		BlockID thisBlock = World_GetBlock(curNode.X, curNode.Y, curNode.Z);
		curLight--; // 1 light level less in each neighbour
		if (curLight == 0) continue;


		curNode.X--;
		if (curNode.X > 0 &&
			CanLightPass(thisBlock, FACE_XMAX) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_XMIN) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, false) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, false);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.X += 2;
		if (curNode.X < World.MaxX &&
			CanLightPass(thisBlock, FACE_XMIN) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_XMAX) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, false) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, false);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.X--;

		curNode.Y--;
		if (curNode.Y > 0 &&
			CanLightPass(thisBlock, FACE_YMAX) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_YMIN) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, false) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, false);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.Y += 2;
		if (curNode.Y < World.MaxY &&
			CanLightPass(thisBlock, FACE_YMIN) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_YMAX) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, false) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, false);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.Y--;

		curNode.Z--;
		if (curNode.Z > 0 &&
			CanLightPass(thisBlock, FACE_ZMAX) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_ZMIN) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, false) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, false);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.Z += 2;
		if (curNode.Z < World.MaxZ &&
			CanLightPass(thisBlock, FACE_ZMIN) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_ZMAX) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, false) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, false);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
	}
}

static void CalcSkyLight(cc_uint8 blockLight, int x, int y, int z) {
	SetBlocklight(blockLight, x, y, z, true);
	IVec3 entry = { x, y, z };
	LightQueue_Enqueue(&lightQueue, entry);

	//if (Blocks.BlocksLight[World_GetBlock(x, y, z)]) { return; }

	while (lightQueue.count > 0) {
		IVec3 curNode = LightQueue_Dequeue(&lightQueue);
		cc_uint8 curLight = GetBlocklight(curNode.X, curNode.Y, curNode.Z, true);
		if (curLight <= 0) {
			Platform_Log1("but there were still %i entries left...", &lightQueue.capacity);
			return;
		}

		BlockID thisBlock = World_GetBlock(curNode.X, curNode.Y, curNode.Z);
		curLight--; // 1 light level less in each neighbour
		if (curLight == 0) continue;


		curNode.X--;
		if (curNode.X > 0 &&
			curNode.Y <= Heightmap_GetLightHeight(curNode.X, curNode.Z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_XMAX) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_XMIN) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, true) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, true);
			IVec3 entry = { curNode.X, curNode.Y, curNode.Z };
			LightQueue_Enqueue(&lightQueue, entry);
		}
		curNode.X += 2;
		if (curNode.X < World.MaxX &&
			curNode.Y <= Heightmap_GetLightHeight(curNode.X, curNode.Z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_XMIN) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_XMAX) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, true) < curLight)
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, true);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.X--;

		curNode.Y--;
		if (curNode.Y > 0 &&
			curNode.Y <= Heightmap_GetLightHeight(curNode.X, curNode.Z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_YMAX) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_YMIN) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, true) < curLight)
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, true);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.Y += 2;
		if (curNode.Y < World.MaxY &&
			curNode.Y <= Heightmap_GetLightHeight(curNode.X, curNode.Z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_YMIN) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_YMAX) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, true) < curLight)
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, true);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.Y--;

		curNode.Z--;
		if (curNode.Z > 0 &&
			curNode.Y <= Heightmap_GetLightHeight(curNode.X, curNode.Z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_ZMAX) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_ZMIN) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, true) < curLight)
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, true);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
		curNode.Z += 2;
		if (curNode.Z < World.MaxZ &&
			curNode.Y <= Heightmap_GetLightHeight(curNode.X, curNode.Z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_ZMIN) &&
			CanLightPass(World_GetBlock(curNode.X, curNode.Y, curNode.Z), FACE_ZMAX) &&
			GetBlocklight(curNode.X, curNode.Y, curNode.Z, true) < curLight) 
		{
			SetBlocklight(curLight, curNode.X, curNode.Y, curNode.Z, true);
			LightQueue_Enqueue(&lightQueue, curNode);
		}
	}
}


static void CalculateChunkLightingSelf(int chunkIndex, int cx, int cy, int cz) {
	int x, y, z;
	int chunkStartX, chunkStartY, chunkStartZ; //world coords
	int chunkEndX, chunkEndY, chunkEndZ; //world coords
	chunkStartX = cx * CHUNK_SIZE;
	chunkStartY = cy * CHUNK_SIZE;
	chunkStartZ = cz * CHUNK_SIZE;
	chunkEndX = chunkStartX + CHUNK_SIZE;
	chunkEndY = chunkStartY + CHUNK_SIZE;
	chunkEndZ = chunkStartZ + CHUNK_SIZE;
	if (chunkEndX > World.Width) { chunkEndX = World.Width; }
	if (chunkEndY > World.Height) { chunkEndY = World.Height; }
	if (chunkEndZ > World.Length) { chunkEndZ = World.Length; }

	//Platform_Log3("  calcing %i %i %i", &cx, &cy, &cz);

	cc_string msg; char msgBuffer[STRING_SIZE * 2];

	for (y = chunkStartY; y < chunkEndY; y++) {
		for (z = chunkStartZ; z < chunkEndZ; z++) {
			for (x = chunkStartX; x < chunkEndX; x++) {

				BlockID curBlock = World_GetBlock(x, y, z);
				if (Blocks.FullBright[curBlock]) {
					CalcBlockLight(MODERN_LIGHTING_MAX_LEVEL, x, y, z);
				}

				//this cell is exposed to sunlight
				if (y > Heightmap_GetLightHeight(x, z)) {
					CalcSkyLight(MODERN_LIGHTING_MAX_LEVEL, x, y, z);
				}
			}
		}
	}
	chunkLightingDataFlags[chunkIndex] = CHUNK_SELF_CALCULATED;
}

static void CalculateChunkLightingAll(int chunkIndex, int cx, int cy, int cz) {
	int x, y, z;
	int chunkStartX, chunkStartY, chunkStartZ; //chunk coords
	int chunkEndX, chunkEndY, chunkEndZ; //chunk coords

	chunkStartX = cx - 1;
	chunkStartY = cy - 1;
	chunkStartZ = cz - 1;
	chunkEndX = cx + 1;
	chunkEndY = cy + 1;
	chunkEndZ = cz + 1;

	if (chunkStartX == -1) { chunkStartX++; }
	if (chunkStartY == -1) { chunkStartY++; }
	if (chunkStartZ == -1) { chunkStartZ++; }
	if (chunkEndX == World.ChunksX) { chunkEndX--; }
	if (chunkEndY == World.ChunksY) { chunkEndY--; }
	if (chunkEndZ == World.ChunksZ) { chunkEndZ--; }

	cc_string msg; char msgBuffer[STRING_SIZE * 2];

	//cc_uint64 BEG = Stopwatch_Measure();

	for (y = chunkStartY; y <= chunkEndY; y++) {
		for (z = chunkStartZ; z <= chunkEndZ; z++) {
			for (x = chunkStartX; x <= chunkEndX; x++) {
				int curChunkIndex = ChunkCoordsToIndex(x, y, z);

				if (chunkLightingData[curChunkIndex] == NULL) {
					chunkLightingData[curChunkIndex] = (cc_uint8*)Mem_TryAllocCleared(CHUNK_SIZE_3, sizeof(cc_uint8));
				}

				if (chunkLightingDataFlags[curChunkIndex] == CHUNK_UNCALCULATED) {
					CalculateChunkLightingSelf(curChunkIndex, x, y, z);
				}
				//String_InitArray(msg, msgBuffer);

				//Chat_Add(&msg);
			}
		}
	}

	//cc_uint64 END = Stopwatch_Measure();

	//static float ELAPSED;
	//ELAPSED += Stopwatch_ElapsedMicroseconds(BEG, END) / 1000.0;
	//Platform_Log1("CALC TIME: %f3", &ELAPSED);
	chunkLightingDataFlags[chunkIndex] = CHUNK_ALL_CALCULATED;
}

static void ModernLighting_OnBlockChanged(void) { }
static void ModernLighting_Refresh(void) {
	ModernLighting_InitPalettes();
	/* Set all the chunk lighting data flags to CHUNK_UNCALCULATED? */
}
static cc_bool ModernLighting_IsLit(int x, int y, int z) { return true; }
static cc_bool ModernLighting_IsLit_Fast(int x, int y, int z) { return true; }

static PackedCol ModernLighting_Color_Core(int x, int y, int z, PackedCol* palette, PackedCol outOfBoundsColor) {
	if (!World_Contains(x, y, z)) return outOfBoundsColor;

	int cx = x >> CHUNK_SHIFT, lx = x & CHUNK_MASK;
	int cy = y >> CHUNK_SHIFT, ly = y & CHUNK_MASK;
	int cz = z >> CHUNK_SHIFT, lz = z & CHUNK_MASK;

	int chunkIndex = ChunkCoordsToIndex(cx, cy, cz);
	if (chunkLightingDataFlags[chunkIndex] < CHUNK_ALL_CALCULATED) {
		CalculateChunkLightingAll(chunkIndex, cx, cy, cz);
	}

	int localIndex = LocalCoordsToIndex(lx, ly, lz);
	cc_uint8 lightData = chunkLightingData[chunkIndex][localIndex];
	return palette[lightData];

	////palette test
	//cc_uint8 thing = y % MODERN_LIGHTING_LEVELS;
	//cc_uint8 thing2 = z % MODERN_LIGHTING_LEVELS;
	//return modernLighting_palette[thing | (thing2 << 4)];
}

static PackedCol ModernLighting_Color(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_palette, Env.SunCol);
}
static PackedCol ModernLighting_Color_XSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_paletteX, Env.SunXSide);
}
static PackedCol ModernLighting_Color_ZSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_paletteZ, Env.SunZSide);
}
static PackedCol ModernLighting_Color_YMinSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_paletteY, Env.SunYMin);
}

static void ModernLighting_LightHint(int startX, int startY, int startZ) {
	int cx, cy, cz, curX, curY, curZ;
	Heightmap_PreCalcColumns(startX, startZ);

	// precalculate lighting for this chunk and its neighbours
	cx = (startX + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;
	cy = (startY + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;
	cz = (startZ + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;

	for (curY = cy - 1; curY <= cy + 1; curY++)
		for (curZ = cz - 1; curZ <= cz + 1; curZ++)
			for (curX = cx - 1; curX <= cx + 1; curX++)
	{
		ModernLighting_Color(curX << CHUNK_SHIFT, curY << CHUNK_SHIFT, curZ << CHUNK_SHIFT);
	}
}

static void ModernLighting_SetActive(void) {
	Lighting.OnBlockChanged = ModernLighting_OnBlockChanged;
	Lighting.Refresh = ModernLighting_Refresh;
	Lighting.IsLit   = ModernLighting_IsLit;
	Lighting.Color   = ModernLighting_Color;
	Lighting.Color_XSide = ModernLighting_Color_XSide;

	Lighting.IsLit_Fast = ModernLighting_IsLit_Fast;
	Lighting.Color_Fast = ModernLighting_Color;
	Lighting.Color_YMin_Fast   = ModernLighting_Color_YMinSide;
	Lighting.Color_XSide_Fast  = ModernLighting_Color_XSide;
	Lighting.Color_ZSide_Fast  = ModernLighting_Color_ZSide;

	Lighting.FreeState  = ModernLighting_FreeState;
	Lighting.AllocState = ModernLighting_AllocState;
	Lighting.LightHint  = ModernLighting_LightHint;
}


/*########################################################################################################################*
*---------------------------------------------------Lighting component----------------------------------------------------*
*#########################################################################################################################*/
static void Lighting_ApplyActive(void) {
	if (Lighting_Modern) {
		ModernLighting_SetActive();
	} else {
		ClassicLighting_SetActive();
	}
}

void Lighting_SwitchActive(void) {
	Lighting.FreeState();
	Lighting_ApplyActive();
	Lighting.AllocState();
}

static void OnInit(void) {
	//if (!Game_ClassicMode) Lighting_Modern = Options_GetBool(OPT_MODERN_LIGHTING, false);
	Lighting_ApplyActive();
}
static void OnReset(void)        { Lighting.FreeState(); }
static void OnNewMapLoaded(void) { Lighting.AllocState(); }

struct IGameComponent Lighting_Component = {
	OnInit,  /* Init  */
	OnReset, /* Free  */
	OnReset, /* Reset */
	OnReset, /* OnNewMap */
	OnNewMapLoaded /* OnNewMapLoaded */
};