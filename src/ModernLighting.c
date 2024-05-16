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
#include "Queue.h"

struct LightNode {
	IVec3 coords;
	cc_uint8 brightness;
};

/*########################################################################################################################*
*----------------------------------------------------Modern lighting------------------------------------------------------*
*#########################################################################################################################*/

static struct Queue lightQueue;
static struct Queue unlightQueue;



/* Top face, X face, Z face, bottomY face*/
#define PALETTE_SHADES 4
/* One palette for sunlight, one palette for shadow */
#define PALETTE_TYPES 2
#define PALETTE_COUNT (PALETTE_SHADES * PALETTE_TYPES)

/* Index into palettes of light colors. */
/* There are 8 different palettes: Four block-face shades for shadowed areas and four block-face shades for sunlit areas. */
/* A palette is a 16x16 color array indexed by a byte where the leftmost 4 bits represent lamplight level and the rightmost 4 bits represent lavalight level */
/* E.G. myPalette[0b_0010_0001] will give us the color for lamp level 2 and lava level 1 (lowest level is 0) */
static PackedCol* palettes[PALETTE_COUNT];

#define PALETTE_YMAX_INDEX 0
#define PALETTE_XSIDE_INDEX 1
#define PALETTE_ZSIDE_INDEX 2
#define PALETTE_YMIN_INDEX 3


typedef cc_uint8* LightingChunk;
static cc_uint8* chunkLightingDataFlags;
#define CHUNK_UNCALCULATED 0
#define CHUNK_SELF_CALCULATED 1
#define CHUNK_ALL_CALCULATED 2
static LightingChunk* chunkLightingData;

#define Modern_MakePaletteIndex(sun, block) ((sun << MODERN_LIGHTING_SUN_SHIFT) | block)


static PackedCol PackedCol_ScreenBlend(PackedCol a, PackedCol b) {
	PackedCol finalColor, aInverted, bInverted;
	cc_uint8 R, G, B;
	/* With Screen blend mode, the values of the pixels in the two layers are inverted, multiplied, and then inverted again. */
	R = 255 - PackedCol_R(a);
	G = 255 - PackedCol_G(a);
	B = 255 - PackedCol_B(a);
	aInverted = PackedCol_Make(R, G, B, 255);

	R = 255 - PackedCol_R(b);
	G = 255 - PackedCol_G(b);
	B = 255 - PackedCol_B(b);
	bInverted = PackedCol_Make(R, G, B, 255);

	finalColor = PackedCol_Tint(aInverted, bInverted);
	R = 255 - PackedCol_R(finalColor);
	G = 255 - PackedCol_G(finalColor);
	B = 255 - PackedCol_B(finalColor);
	return PackedCol_Make(R, G, B, 255);
}
/* Fill in a palette with values based on the current environment colors */
static void ModernLighting_InitPalette(PackedCol* palette, float shaded, PackedCol ambientColor) {
	PackedCol lavaColor, lampColor;
	int lampLevel, lavaLevel;
	float blockLerp;

	for (lampLevel = 0; lampLevel < MODERN_LIGHTING_LEVELS; lampLevel++) {
		for (lavaLevel = 0; lavaLevel < MODERN_LIGHTING_LEVELS; lavaLevel++) {
			if (lampLevel == MODERN_LIGHTING_LEVELS - 1) {
				lampColor = Env.LampLightCol;
			}
			else {
				blockLerp = max(lampLevel, MODERN_LIGHTING_LEVELS - SUN_LEVELS) / (float)(MODERN_LIGHTING_LEVELS - 1);
				blockLerp *= (MATH_PI / 2);
				blockLerp = Math_Cos(blockLerp);
				lampColor = PackedCol_Lerp(0, Env.LampLightCol, 1 - blockLerp);
			}

			blockLerp = lavaLevel / (float)(MODERN_LIGHTING_LEVELS - 1);
			//blockLerp *= blockLerp;
			blockLerp *= (MATH_PI / 2);
			blockLerp = Math_Cos(blockLerp);

			lavaColor = PackedCol_Lerp(0, Env.LavaLightCol, 1 - blockLerp);

			/* Blend the two light colors together, then blend that with the ambient color, then shade that by the face darkness */
			palette[Modern_MakePaletteIndex(lampLevel, lavaLevel)] =
				PackedCol_Scale(PackedCol_ScreenBlend(PackedCol_ScreenBlend(lampColor, lavaColor), ambientColor), shaded);
		}
	}
}
static void ModernLighting_InitPalettes(void) {
	int i;
	for (i = 0; i < PALETTE_COUNT; i++) {
		palettes[i] = (PackedCol*)Mem_Alloc(MODERN_LIGHTING_LEVELS * MODERN_LIGHTING_LEVELS, sizeof(PackedCol), "light color palette");
	}

	i = 0;
	ModernLighting_InitPalette(palettes[i + PALETTE_YMAX_INDEX],  1,                    Env.ShadowCol);
	ModernLighting_InitPalette(palettes[i + PALETTE_XSIDE_INDEX], PACKEDCOL_SHADE_X,    Env.ShadowCol);
	ModernLighting_InitPalette(palettes[i + PALETTE_ZSIDE_INDEX], PACKEDCOL_SHADE_Z,    Env.ShadowCol);
	ModernLighting_InitPalette(palettes[i + PALETTE_YMIN_INDEX],  PACKEDCOL_SHADE_YMIN, Env.ShadowCol);
	i += PALETTE_SHADES;
	ModernLighting_InitPalette(palettes[i + PALETTE_YMAX_INDEX],  1,                    Env.SunCol);
	ModernLighting_InitPalette(palettes[i + PALETTE_XSIDE_INDEX], PACKEDCOL_SHADE_X,    Env.SunCol);
	ModernLighting_InitPalette(palettes[i + PALETTE_ZSIDE_INDEX], PACKEDCOL_SHADE_Z,    Env.SunCol);
	ModernLighting_InitPalette(palettes[i + PALETTE_YMIN_INDEX],  PACKEDCOL_SHADE_YMIN, Env.SunCol);
}
static void ModernLighting_FreePalettes(void) {
	int i;

	for (i = 0; i < PALETTE_COUNT; i++) {
		Mem_Free(palettes[i]);
	}
}

static int chunksCount;
static void ModernLighting_AllocState(void) {
	ClassicLighting_AllocState();
	ModernLighting_InitPalettes();
	chunksCount = World.ChunksCount;

	chunkLightingDataFlags = (cc_uint8*)Mem_AllocCleared(chunksCount, sizeof(cc_uint8), "light flags");
	chunkLightingData = (LightingChunk*)Mem_AllocCleared(chunksCount, sizeof(LightingChunk), "light chunks");
	Queue_Init(&lightQueue, sizeof(struct LightNode));
	Queue_Init(&unlightQueue, sizeof(struct LightNode));
}

static void ModernLighting_FreeState(void) {
	int i;
	ClassicLighting_FreeState();
	
	/* This function can be called multiple times without calling ModernLighting_AllocState, so... */
	if (!chunkLightingDataFlags) return;

	ModernLighting_FreePalettes();

	for (i = 0; i < chunksCount; i++) {
		Mem_Free(chunkLightingData[i]);
	}

	Mem_Free(chunkLightingDataFlags);
	Mem_Free(chunkLightingData);
	chunkLightingDataFlags = NULL;
	chunkLightingData = NULL;
	Queue_Clear(&lightQueue);
	Queue_Clear(&unlightQueue);
}

/* Converts chunk x/y/z coordinates to the corresponding index in chunks array/list */
#define ChunkCoordsToIndex(cx, cy, cz) (((cy) * World.ChunksZ + (cz)) * World.ChunksX + (cx))
/* Converts local x/y/z coordinates to the corresponding index in a chunk */
#define LocalCoordsToIndex(lx, ly, lz) ((lx) | ((lz) << CHUNK_SHIFT) | ((ly) << (CHUNK_SHIFT * 2)))
/* Converts global x/y/z coordinates to the corresponding index in a chunk */
#define GlobalCoordsToChunkCoordsIndex(x, y, z) (LocalCoordsToIndex(x & CHUNK_MASK, y & CHUNK_MASK, z & CHUNK_MASK))

/* Sets the light level at this cell. Does NOT check that the cell is in bounds. */
static void SetBlocklight(cc_uint8 blockLight, int x, int y, int z, cc_bool sun, cc_bool refreshChunk) {
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

	if (refreshChunk) {
		cc_uint8 prevValue = chunkLightingData[chunkIndex][localIndex];

		chunkLightingData[chunkIndex][localIndex] &= clearMask;
		chunkLightingData[chunkIndex][localIndex] |= blockLight << shift;

		/* There is no reason to refresh current chunk as the builder does that automatically */
		if (prevValue != chunkLightingData[chunkIndex][localIndex]) {
			if (lx == CHUNK_MAX) MapRenderer_RefreshChunk(cx + 1, cy, cz);
			if (lx == 0)         MapRenderer_RefreshChunk(cx - 1, cy, cz);
			if (ly == CHUNK_MAX) MapRenderer_RefreshChunk(cx, cy + 1, cz);
			if (ly == 0)         MapRenderer_RefreshChunk(cx, cy - 1, cz);
			if (lz == CHUNK_MAX) MapRenderer_RefreshChunk(cx, cy, cz + 1);
			if (lz == 0)         MapRenderer_RefreshChunk(cx, cy, cz - 1);
		}
	}
	else {
		chunkLightingData[chunkIndex][localIndex] &= clearMask;
		chunkLightingData[chunkIndex][localIndex] |= blockLight << shift;
	}
}
/* Returns the light level at this cell. Does NOT check that the cell is in bounds. */
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


/* Light can never pass through a block that's full sized and blocks light */
/* We can assume a block is full sized if none of the LightOffset flags are 0 */
#define IsFullOpaque(thisBlock) (Blocks.BlocksLight[thisBlock] && Blocks.LightOffset[thisBlock] == 0xFF)

/* If it's not opaque and it doesn't block light, or it is brighter than 0, we can always pass through */
/* Light can always pass through leaves and water */
#define IsFullTransparent(thisBlock)\
(\
(Blocks.Draw[thisBlock] > DRAW_OPAQUE && !Blocks.BlocksLight[thisBlock]) || \
Blocks.Draw[thisBlock] == DRAW_TRANSPARENT_THICK || \
Blocks.Draw[thisBlock] == DRAW_TRANSLUCENT\
)

static cc_bool CanLightPass(BlockID thisBlock, Face face) {
	if (IsFullTransparent(thisBlock)) { return true; }
	if (Blocks.Brightness[thisBlock]) { return true; }
	if (IsFullOpaque(thisBlock)) { return false; }
	/* Is stone's face hidden by thisBlock? TODO: Don't hardcode using stone */
	return !Block_IsFaceHidden(BLOCK_STONE, thisBlock, face);
}

#define Light_TrySpreadInto(axis, AXIS, dir, limit, isSun, thisFace, thatFace) \
	if (ln.coords.axis dir ## = limit && \
		CanLightPass(thisBlock, FACE_ ## AXIS ## thisFace) && \
		CanLightPass(World_GetBlock(ln.coords.x, ln.coords.y, ln.coords.z), FACE_ ## AXIS ## thatFace) && \
		GetBlocklight(ln.coords.x, ln.coords.y, ln.coords.z, isSun) < ln.brightness) { \
		Queue_Enqueue(&lightQueue, &ln); \
	} \

static void FlushLightQueue(cc_bool isSun, cc_bool refreshChunk) {
	int handled = 0;
	while (lightQueue.count > 0) {
		handled++;
		struct LightNode ln = *(struct LightNode*)(Queue_Dequeue(&lightQueue));

		cc_uint8 brightnessHere = GetBlocklight(ln.coords.x, ln.coords.y, ln.coords.z, isSun);

		/* If this cel is already more lit, we can assume this cel and its neighbors have been accounted for */
		if (brightnessHere >= ln.brightness) { continue; }
		if (ln.brightness == 0) { continue; }

		//Platform_Log4("Placing %i at %i %i %i", &ln.brightness, &ln.coords.x, &ln.coords.y, &ln.coords.z);
		SetBlocklight(ln.brightness, ln.coords.x, ln.coords.y, ln.coords.z, isSun, refreshChunk);


		BlockID thisBlock = World_GetBlock(ln.coords.x, ln.coords.y, ln.coords.z);
		ln.brightness--;
		if (ln.brightness == 0) continue;

		ln.coords.x--;
		Light_TrySpreadInto(x, X, > , 0, isSun, MAX, MIN)
		ln.coords.x += 2;
		Light_TrySpreadInto(x, X, < , World.MaxX, isSun, MIN, MAX)
		ln.coords.x--;

		ln.coords.y--;
		Light_TrySpreadInto(y, Y, >, 0, isSun, MAX, MIN)
		ln.coords.y += 2;
		Light_TrySpreadInto(y, Y, <, World.MaxY, isSun, MIN, MAX)
		ln.coords.y--;

		ln.coords.z--;
		Light_TrySpreadInto(z, Z, > , 0, isSun, MAX, MIN)
		ln.coords.z += 2;
		Light_TrySpreadInto(z, Z, < , World.MaxZ, isSun, MIN, MAX)
	}
}

cc_uint8 GetBlockBrightness(BlockID curBlock, cc_bool isSun) {
	if (isSun) return Blocks.Brightness[curBlock] >> MODERN_LIGHTING_SUN_SHIFT;
	return Blocks.Brightness[curBlock] & MODERN_LIGHTING_MAX_LEVEL;
}

static void CalculateChunkLightingSelf(int chunkIndex, int cx, int cy, int cz) {
	int x, y, z;
	int chunkStartX, chunkStartY, chunkStartZ; //world coords
	int chunkEndX, chunkEndY, chunkEndZ; //world coords
	cc_uint8 brightness;
	BlockID curBlock;
	chunkStartX = cx * CHUNK_SIZE;
	chunkStartY = cy * CHUNK_SIZE;
	chunkStartZ = cz * CHUNK_SIZE;
	chunkEndX = chunkStartX + CHUNK_SIZE;
	chunkEndY = chunkStartY + CHUNK_SIZE;
	chunkEndZ = chunkStartZ + CHUNK_SIZE;
	if (chunkEndX > World.Width) { chunkEndX = World.Width; }
	if (chunkEndY > World.Height) { chunkEndY = World.Height; }
	if (chunkEndZ > World.Length) { chunkEndZ = World.Length; }

	for (y = chunkStartY; y < chunkEndY; y++) {
		for (z = chunkStartZ; z < chunkEndZ; z++) {
			for (x = chunkStartX; x < chunkEndX; x++) {

				curBlock = World_GetBlock(x, y, z);
				
				if (Blocks.Brightness[curBlock] > 0) {

					brightness = GetBlockBrightness(curBlock, false);

					if (brightness > 0) {
						struct LightNode entry = { x, y, z, brightness };
						Queue_Enqueue(&lightQueue, &entry);
						FlushLightQueue(false, false);
					}
					else {
						/* If no block brightness, it must use sun brightness */
						brightness = Blocks.Brightness[curBlock] >> MODERN_LIGHTING_SUN_SHIFT;
						struct LightNode entry = { x, y, z, brightness };
						Queue_Enqueue(&lightQueue, &entry);
						FlushLightQueue(true, false);
					}
				}

				/* Note: This code only deals with generating light from block sources.
				Regular sun light is added on as a "post process" step when returning light color in the exposed API.
				This has the added benefit of being able to skip allocating chunk lighting data in regions that have no light-casting blocks*/
			}
		}
	}

	chunkLightingDataFlags[chunkIndex] = CHUNK_SELF_CALCULATED;
}

static void CalculateChunkLightingAll(int chunkIndex, int cx, int cy, int cz) {
	int x, y, z;
	int chunkStartX, chunkStartY, chunkStartZ; //chunk coords
	int chunkEndX, chunkEndY, chunkEndZ; //chunk coords
	int curChunkIndex;

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

	for (y = chunkStartY; y <= chunkEndY; y++) {
		for (z = chunkStartZ; z <= chunkEndZ; z++) {
			for (x = chunkStartX; x <= chunkEndX; x++) {
				curChunkIndex = ChunkCoordsToIndex(x, y, z);

				if (chunkLightingDataFlags[curChunkIndex] == CHUNK_UNCALCULATED) {
					CalculateChunkLightingSelf(curChunkIndex, x, y, z);
				}
			}
		}
	}
	chunkLightingDataFlags[chunkIndex] = CHUNK_ALL_CALCULATED;
}


#define Light_TryUnSpreadInto(axis, dir, limit, AXIS, thisFace, thatFace) \
		if (neighborCoords.axis dir ## = limit && \
			CanLightPass(thisBlock, FACE_ ## AXIS ## thisFace) && \
			CanLightPass(World_GetBlock(neighborCoords.x, neighborCoords.y, neighborCoords.z), FACE_ ## AXIS ## thatFace) \
		) \
		{ \
			neighborBrightness = GetBlocklight(neighborCoords.x, neighborCoords.y, neighborCoords.z, isSun); \
			neighborBlockBrightness = GetBlockBrightness(World_GetBlock(neighborCoords.x, neighborCoords.y, neighborCoords.z), isSun); \
			/* This spot is a light caster, mark this spot as needing to be re-spread */ \
			if (neighborBlockBrightness > 0) { \
				struct LightNode entry = { neighborCoords.x, neighborCoords.y, neighborCoords.z, neighborBlockBrightness }; \
				Queue_Enqueue(&lightQueue, &entry); \
			} \
			if (neighborBrightness > 0) { \
				/* This neighbor is darker than cur spot, darken it*/ \
				if (neighborBrightness < curNode.brightness) { \
					SetBlocklight(0, neighborCoords.x, neighborCoords.y, neighborCoords.z, isSun, true); \
					struct LightNode neighborNode = { { neighborCoords.x, neighborCoords.y, neighborCoords.z }, neighborBrightness }; \
					Queue_Enqueue(&unlightQueue, &neighborNode); \
				} \
				/* This neighbor is brighter or same, mark this spot as needing to be re-spread */ \
				else { \
					/* But only if the neighbor actually *can* spread to this block */ \
					if ( \
						CanLightPass(thisBlockTrue, FACE_ ## AXIS ## thisFace) && \
						CanLightPass(World_GetBlock(neighborCoords.x, neighborCoords.y, neighborCoords.z), FACE_ ## AXIS ## thatFace) \
					) \
					{ \
						struct LightNode entry = curNode; \
						entry.brightness = neighborBrightness-1; \
						Queue_Enqueue(&lightQueue, &entry); \
					} \
				} \
			} \
		} \

/* Spreads darkness out from this point and relights any necessary areas afterward */
static void CalcUnlight(int x, int y, int z, cc_uint8 brightness, cc_bool isSun) {
	int count = 0;
	SetBlocklight(0, x, y, z, isSun, true);
	struct LightNode sourceNode = { { x, y, z }, brightness };
	Queue_Enqueue(&unlightQueue, &sourceNode);

	while (unlightQueue.count > 0) {
		struct LightNode curNode = *(struct LightNode*)(Queue_Dequeue(&unlightQueue));
		cc_uint8 neighborBrightness;
		cc_uint8 neighborBlockBrightness;
		IVec3 neighborCoords = curNode.coords;

		BlockID thisBlockTrue = World_GetBlock(neighborCoords.x, neighborCoords.y, neighborCoords.z);
		/* For the original cell in the queue, assume this block is air
		so that light can unspread "out" of it in the case of a solid blocks. */
		BlockID thisBlock = count == 0 ? BLOCK_AIR : thisBlockTrue;

		count++;

		neighborCoords.x--;
		Light_TryUnSpreadInto(x, >, 0, X, MAX, MIN)
		neighborCoords.x += 2;
		Light_TryUnSpreadInto(x, <, World.MaxX, X, MIN, MAX)
		neighborCoords.x--;

		neighborCoords.y--;
		Light_TryUnSpreadInto(y, >, 0, Y, MAX, MIN)
		neighborCoords.y += 2;
		Light_TryUnSpreadInto(y, <, World.MaxY, Y, MIN, MAX)
		neighborCoords.y--;

		neighborCoords.z--;
		Light_TryUnSpreadInto(z, >, 0, Z, MAX, MIN)
		neighborCoords.z += 2;
		Light_TryUnSpreadInto(z, <, World.MaxZ, Z, MIN, MAX)
	}

	FlushLightQueue(isSun, true);
}
static void CalcBlockChange(int x, int y, int z, BlockID oldBlock, BlockID newBlock, cc_bool isSun) {
	cc_uint8 oldBlockLightLevel = GetBlockBrightness(oldBlock, isSun);
	cc_uint8 newBlockLightLevel = GetBlockBrightness(newBlock, isSun);

	cc_uint8 oldLightLevelHere = GetBlocklight(x, y, z, isSun);

	/* Cel has no lighting and new block doesn't cast light and blocks all light, no change */
	if (!oldLightLevelHere && !newBlockLightLevel && IsFullOpaque(newBlock)) return;


	/* Cel is darker than the new block, only brighter case */
	if (oldLightLevelHere < newBlockLightLevel) {
		/* brighten this spot, recalculate lighting */
		//Platform_LogConst("Brightening");
		struct LightNode entry = { x, y, z, newBlockLightLevel };
		Queue_Enqueue(&lightQueue, &entry);
		FlushLightQueue(isSun, true);
		return;
	}

	/* Light passes through old and new, old block does not cast light, new block does not cast light; no change */
	if (IsFullTransparent(oldBlock) && IsFullTransparent(newBlock) && !oldBlockLightLevel && !newBlockLightLevel) {
		//Platform_LogConst("Light passes through old and new and new block does not cast light, no change");
		return;
	}

	CalcUnlight(x, y, z, oldLightLevelHere, isSun);
}
static void ModernLighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {

	/* For some reason this is a possible case */
	if (oldBlock == newBlock) { return; }

	ClassicLighting_OnBlockChanged(x, y, z, oldBlock, newBlock);

	CalcBlockChange(x, y, z, oldBlock, newBlock, false);
	CalcBlockChange(x, y, z, oldBlock, newBlock, true);
}
/* Invalidates/Resets lighting state for all of the blocks in the world */
/*  (e.g. because a block changed whether it is full bright or not) */
static void ModernLighting_Refresh(void) {
	ClassicLighting_Refresh();
	ModernLighting_FreeState();
	ModernLighting_AllocState();
}
static cc_bool ModernLighting_IsLit(int x, int y, int z) { return true; }
static cc_bool ModernLighting_IsLit_Fast(int x, int y, int z) { return true; }

#define CalcForChunkIfNeeded(cx, cy, cz, chunkIndex) \
	if (chunkLightingDataFlags[chunkIndex] < CHUNK_ALL_CALCULATED) { \
		CalculateChunkLightingAll(chunkIndex, cx, cy, cz); \
	}

static PackedCol ModernLighting_Color_Core(int x, int y, int z, int paletteFace, PackedCol outOfBoundsColor) {
	cc_uint8 lightData;
	int cx, cy, cz, chunkIndex;
	int chunkCoordsIndex;

	if (!World_Contains(x, y, z)) return outOfBoundsColor;

	cx = x >> CHUNK_SHIFT;
	cy = y >> CHUNK_SHIFT;
	cz = z >> CHUNK_SHIFT;

	chunkIndex = ChunkCoordsToIndex(cx, cy, cz);
	CalcForChunkIfNeeded(cx, cy, cz, chunkIndex);

	/* There might be no light data in this chunk even after it was calculated */
	if (chunkLightingData[chunkIndex] == NULL) {
		/* 0, no sun or light (but it may appear as sun based on the 2D sun map) */
		lightData = 0;
	} else {
		chunkCoordsIndex = GlobalCoordsToChunkCoordsIndex(x, y, z);
		lightData = chunkLightingData[chunkIndex][chunkCoordsIndex];
	}

	/* This cell is exposed to sunlight */
	if (y > ClassicLighting_GetLightHeight(x, z)) {
		/* Push the pointer forward into the sun lit palette section */
		paletteFace += PALETTE_SHADES;
	}


	////palette test
	//cc_uint8 thing = y % MODERN_LIGHTING_LEVELS;
	//cc_uint8 thing2 = z % MODERN_LIGHTING_LEVELS;
	//return palettes[paletteFace][thing | (thing2 << 4)];

	return palettes[paletteFace][lightData];
}

static PackedCol ModernLighting_Color(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, PALETTE_YMAX_INDEX, Env.SunCol);
}
static PackedCol ModernLighting_Color_YMaxSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, PALETTE_YMAX_INDEX, Env.SunCol);
}
static PackedCol ModernLighting_Color_YMinSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, PALETTE_YMIN_INDEX, Env.SunYMin);
}
static PackedCol ModernLighting_Color_XSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, PALETTE_XSIDE_INDEX, Env.SunXSide);
}
static PackedCol ModernLighting_Color_ZSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, PALETTE_ZSIDE_INDEX, Env.SunZSide);
}

static void ModernLighting_LightHint(int startX, int startY, int startZ) {
	int cx, cy, cz, chunkIndex;
	ClassicLighting_LightHint(startX, startY, startZ);
	/* Add 1 to startX/Z, as coordinates are for the extended chunk (18x18x18) */
	startX++; startY++; startZ++;

	// precalculate lighting for this chunk and its neighbours
	cx = (startX + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;
	cy = (startY + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;
	cz = (startZ + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;

	chunkIndex = ChunkCoordsToIndex(cx, cy, cz);
	CalcForChunkIfNeeded(cx, cy, cz, chunkIndex);
}

void ModernLighting_SetActive(void) {
	Lighting.OnBlockChanged = ModernLighting_OnBlockChanged;
	Lighting.Refresh = ModernLighting_Refresh;
	Lighting.IsLit = ModernLighting_IsLit;
	Lighting.Color = ModernLighting_Color;
	Lighting.Color_XSide = ModernLighting_Color_XSide;

	Lighting.IsLit_Fast = ModernLighting_IsLit_Fast;
	Lighting.Color_Sprite_Fast = ModernLighting_Color;
	Lighting.Color_YMax_Fast = ModernLighting_Color;
	Lighting.Color_YMin_Fast = ModernLighting_Color_YMinSide;
	Lighting.Color_XSide_Fast = ModernLighting_Color_XSide;
	Lighting.Color_ZSide_Fast = ModernLighting_Color_ZSide;

	Lighting.FreeState = ModernLighting_FreeState;
	Lighting.AllocState = ModernLighting_AllocState;
	Lighting.LightHint = ModernLighting_LightHint;
}

void ModernLighting_OnEnvVariableChanged(void* obj, int envVar) {
	/* This is always called, but should only do anything if modern lighting is on */
	if (!Lighting_Modern) { return; }

	if (envVar == ENV_VAR_SUN_COLOR || envVar == ENV_VAR_SHADOW_COLOR || envVar == ENV_VAR_LAVALIGHT_COLOR || envVar == ENV_VAR_LAMPLIGHT_COLOR) {
		ModernLighting_InitPalettes();
	}
	if (envVar == ENV_VAR_LAVALIGHT_COLOR || envVar == ENV_VAR_LAMPLIGHT_COLOR) MapRenderer_Refresh();
}