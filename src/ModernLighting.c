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

static struct LightNode {
	IVec3 coords;
	cc_uint8 brightness;
};

/*########################################################################################################################*
*----------------------------------------------------Modern lighting------------------------------------------------------*
*#########################################################################################################################*/

static struct Queue lightQueue;
static struct Queue unlightQueue;

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

#define Modern_MakePaletteIndex(sun, block) ((sun << MODERN_LIGHTING_SUN_SHIFT) | block)

/* Fill in modernLighting_palette with values based on the current environment colors in lieu of recieving a palette from the server */
static void ModernLighting_InitPalette(PackedCol* palette, float shaded) {
	PackedCol darkestShadow, defaultBlockLight, blockColor, sunColor, invertedBlockColor, invertedSunColor, finalColor;
	int sunLevel, blockLevel;
	float blockLerp;
	cc_uint8 R, G, B;

	defaultBlockLight = PackedCol_Make(255, 235, 198, 255); /* A very mildly orange tinted light color */
	//darkestShadow = PackedCol_Lerp(Env.ShadowCol, 0, 0.75f); /* Use a darkened version of shadow color as the darkest color in sun ramp */
	darkestShadow = Env.ShadowCol;

	for (sunLevel = 0; sunLevel < MODERN_LIGHTING_LEVELS; sunLevel++) {
		for (blockLevel = 0; blockLevel < MODERN_LIGHTING_LEVELS; blockLevel++) {
			if (sunLevel == MODERN_LIGHTING_LEVELS - 1) {
				sunColor = Env.SunCol;
			}
			else {
				//sunColor = PackedCol_Lerp(darkestShadow, Env.ShadowCol, sunLevel / (float)(MODERN_LIGHTING_LEVELS - 3));

				//used -2 before to go from sun to darkest shadow, thus making sun cloned twice at the start of the ramp
				blockLerp = max(sunLevel, MODERN_LIGHTING_LEVELS - SUN_LEVELS) / (float)(MODERN_LIGHTING_LEVELS - 1);
				//blockLerp *= blockLerp;
				blockLerp *= (MATH_PI / 2);
				blockLerp = Math_Cos(blockLerp);
				sunColor = PackedCol_Lerp(darkestShadow, Env.SunCol, 1 - blockLerp);
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
	ClassicLighting_AllocState();
	ModernLighting_InitPalettes();
	chunksCount = World.ChunksCount;

	chunkLightingDataFlags = (cc_uint8*)Mem_AllocCleared(chunksCount, sizeof(cc_uint8), "light flags");
	chunkLightingData = (LightingChunk*)Mem_AllocCleared(chunksCount, sizeof(LightingChunk), "light chunks");
	Queue_Init(&lightQueue, sizeof(struct LightNode));
	Queue_Init(&unlightQueue, sizeof(struct LightNode));
}

static void ModernLighting_FreeState(void) {
	ClassicLighting_FreeState();


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
	while (lightQueue.count > 0) {
		struct LightNode ln = *(struct LightNode*)(Queue_Dequeue(&lightQueue));

		cc_uint8 brightnessHere = GetBlocklight(ln.coords.x, ln.coords.y, ln.coords.z, isSun);

		/* If this cel is already just as or more lit, we can assume this cel and its neighbors have been accounted for */
		if (brightnessHere >= ln.brightness) { continue; }

		SetBlocklight(ln.brightness, ln.coords.x, ln.coords.y, ln.coords.z, isSun, refreshChunk);

		if (ln.brightness == 0) { continue; }

		BlockID thisBlock = World_GetBlock(ln.coords.x, ln.coords.y, ln.coords.z);
		ln.brightness--;
		if (ln.brightness == 0) continue;

		ln.coords.x--;
		Light_TrySpreadInto(x, X, > , 0, isSun, MAX, MIN)
		ln.coords.x += 2;
		Light_TrySpreadInto(x, X, < , World.MaxX, isSun, MIN, MAX)
		ln.coords.x--;

		//ln.coords.y--;
		//Light_TrySpreadInto(y, Y, >, 0, isSun, MAX, MIN)
		//ln.coords.y += 2;
		//Light_TrySpreadInto(y, Y, <, World.MaxY, isSun, MIN, MAX)
		//ln.coords.y--;

		ln.coords.z--;
		Light_TrySpreadInto(z, Z, > , 0, isSun, MAX, MIN)
		ln.coords.z += 2;
		Light_TrySpreadInto(z, Z, < , World.MaxZ, isSun, MIN, MAX)
	}
}

#define BlockBlockBrightness(curBlock) (curBlock == 201 ? 10 : (Blocks.Brightness[curBlock] & MODERN_LIGHTING_MAX_LEVEL))
#define BlockSunBrightness(curBlock) (Blocks.Brightness[curBlock] >> MODERN_LIGHTING_SUN_SHIFT)
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

	//Platform_Log3("  calcing %i %i %i", &cx, &cy, &cz);

	cc_string msg; char msgBuffer[STRING_SIZE * 2];

	for (y = chunkStartY; y < chunkEndY; y++) {
		for (z = chunkStartZ; z < chunkEndZ; z++) {
			for (x = chunkStartX; x < chunkEndX; x++) {

				curBlock = World_GetBlock(x, y, z);
				
				if (Blocks.Brightness[curBlock] > 0) {

					brightness = BlockBlockBrightness(curBlock);

					if (brightness > 0) {
						struct LightNode entry = { x, y, z, brightness };
						Queue_Enqueue(&lightQueue, &entry);
					}
					else {
						/* If no block brightness, it must use sun brightness */
						//TODO
					}
				}

				/* Note: This code only deals with generating light from block sources.
				Regular sun light is added on as a "post process" step when returning light color in the exposed API.
				This has the added benefit of being able to skip allocating chunk lighting data in regions that have no light-casting blocks*/
			}
		}
	}

	FlushLightQueue(false, false);

	chunkLightingDataFlags[chunkIndex] = CHUNK_SELF_CALCULATED;
}

static void CalculateChunkLightingAll(int chunkIndex, int cx, int cy, int cz, cc_bool recalculate) {
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


	if (recalculate) {
		for (y = chunkStartY; y <= chunkEndY; y++) {
			for (z = chunkStartZ; z <= chunkEndZ; z++) {
				for (x = chunkStartX; x <= chunkEndX; x++) {
					curChunkIndex = ChunkCoordsToIndex(x, y, z);

					//Delete all lighting from this chunk
					Mem_Free(chunkLightingData[curChunkIndex]);
					//TODO: Why does skipping this allocation cause a crash? It should automatically allocate in SetBlocklight
					chunkLightingData[curChunkIndex] = (cc_uint8*)Mem_TryAllocCleared(CHUNK_SIZE_3, sizeof(cc_uint8));
					
					chunkLightingDataFlags[curChunkIndex] = CHUNK_UNCALCULATED;
					MapRenderer_RefreshChunk(x, y, z);
				}
			}
		}
	}

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


#define Light_TryUnSpreadInto(axis, dir, limit, Type) \
		if (neighborCoords.axis dir ## = limit) { \
			neighborBrightness = GetBlocklight(neighborCoords.x, neighborCoords.y, neighborCoords.z, isSun); \
			neighborBlockBrightness = Block ## Type ## Brightness(World_GetBlock(neighborCoords.x, neighborCoords.y, neighborCoords.z)); \
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
				/* This spot is brighter or same, mark this spot as needing to be re-spread */ \
				else { \
					Platform_Log1("Spread for me. %i", &neighborBrightness); \
					struct LightNode entry = { neighborCoords.x, neighborCoords.y, neighborCoords.z, neighborBrightness }; \
					Queue_Enqueue(&lightQueue, &entry); \
				} \
			} \
		} \

static void CalcUnlight(int x, int y, int z, cc_uint8 brightness, cc_bool isSun) {
	SetBlocklight(0, x, y, z, isSun, true);
	struct LightNode sourceNode = { { x, y, z }, brightness };
	Queue_Enqueue(&unlightQueue, &sourceNode);

	while (unlightQueue.count > 0) {
		struct LightNode curNode = *(struct LightNode*)(Queue_Dequeue(&unlightQueue));
		cc_uint8 neighborBrightness;
		cc_uint8 neighborBlockBrightness;
		//Step 4: Look at all neighbouring voxels to that node.
		//         if their light level is nonzero and is
		//         less than the current lightLevel, then add them to the
		//         queue and set their light level to zero.
		//         Else if it is >= current node, then add it to
		//         the light propagation queue.

		IVec3 neighborCoords = curNode.coords;

		neighborCoords.x--;
		Light_TryUnSpreadInto(x, >, 0, Block)
		neighborCoords.x += 2;
		Light_TryUnSpreadInto(x, <, World.MaxX, Block)
		neighborCoords.x--;



		neighborCoords.z--;
		Light_TryUnSpreadInto(z, >, 0, Block)
		neighborCoords.z += 2;
		Light_TryUnSpreadInto(x, <, World.MaxZ, Block)
		neighborCoords.z--;
	}

	FlushLightQueue(isSun, true);
}
static void ModernLighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {
	cc_uint8 lightData;
	int cx, cy, cz, chunkIndex;
	int chunkCoordsIndex;
	BlockID thereBlock;
	cc_uint8 blockLightThere;
	cc_bool newBlockFullOpaque = IsFullOpaque(newBlock);

	/* For some reason this is a possible case */
	if (oldBlock == newBlock) { return; }

	ClassicLighting_OnBlockChanged(x, y, z, oldBlock, newBlock);

	cc_uint8 oldBlockLightLevel = BlockBlockBrightness(oldBlock);
	cc_uint8 newBlockLightLevel = BlockBlockBrightness(newBlock);

	cc_uint8 oldLightLevelHere = GetBlocklight(x, y, z, false);

	/* Cel has no lighting and new block doesn't cast light and blocks all light, no change */
	if (!oldLightLevelHere && !newBlockLightLevel && newBlockFullOpaque) {
		Platform_LogConst("No change");
		return;
	}


	/* Cel is darker than the new block, only brighter case */
	if (oldLightLevelHere < newBlockLightLevel) {
		/* brighten this spot, recalculate lighting */
		Platform_LogConst("Brightening");
		struct LightNode entry = { x, y, z, newBlockLightLevel };
		Queue_Enqueue(&lightQueue, &entry);
		FlushLightQueue(false, true);
		return;
	}


	/* Light passes through old and new, old block does not cast light, new block does not cast light; no change */
	if (IsFullTransparent(oldBlock) && IsFullTransparent(newBlock) && !oldBlockLightLevel && !newBlockLightLevel) {
		Platform_LogConst("Light passes through old and new and new block does not cast light, no change");
		return;
	}

	CalcUnlight(x, y, z, oldLightLevelHere, false);
	return;


	/* lazily recalculate all lighting in this chunkand neighbors, TODO optimise this */
	//CalculateChunkLightingAll(chunkIndex, cx, cy, cz, true);

}
/* Invalidates/Resets lighting state for all of the blocks in the world */
/*  (e.g. because a block changed whether it is full bright or not) */
static void ModernLighting_Refresh(void) {
	ModernLighting_InitPalettes();
	/* Set all the chunk lighting data flags to CHUNK_UNCALCULATED? */
}
static cc_bool ModernLighting_IsLit(int x, int y, int z) { return true; }
static cc_bool ModernLighting_IsLit_Fast(int x, int y, int z) { return true; }

#define RecalcForChunkIfNeeded(cx, cy, cz, chunkIndex) \
	if (chunkLightingDataFlags[chunkIndex] < CHUNK_ALL_CALCULATED) { \
		CalculateChunkLightingAll(chunkIndex, cx, cy, cz, false); \
	}

static PackedCol ModernLighting_Color_Core(int x, int y, int z, PackedCol* palette, PackedCol outOfBoundsColor) {
	cc_uint8 lightData;
	int cx, cy, cz, chunkIndex;
	int chunkCoordsIndex;

	if (!World_Contains(x, y, z)) return outOfBoundsColor;

	cx = x >> CHUNK_SHIFT;
	cy = y >> CHUNK_SHIFT;
	cz = z >> CHUNK_SHIFT;

	chunkIndex = ChunkCoordsToIndex(cx, cy, cz);
	RecalcForChunkIfNeeded(cx, cy, cz, chunkIndex);

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
		lightData |= MODERN_LIGHTING_SUN_MASK; /* Force the palette to use full sun color */
	}

	return palette[lightData];

	////palette test
	//cc_uint8 thing = y % MODERN_LIGHTING_LEVELS;
	//cc_uint8 thing2 = z % MODERN_LIGHTING_LEVELS;
	//return modernLighting_palette[thing | (thing2 << 4)];
}

static PackedCol ModernLighting_Color(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_palette, Env.SunCol);
}
static PackedCol ModernLighting_Color_YMaxSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_palette, Env.SunCol);
}
static PackedCol ModernLighting_Color_YMinSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_paletteY, Env.SunYMin);
}
static PackedCol ModernLighting_Color_XSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_paletteX, Env.SunXSide);
}
static PackedCol ModernLighting_Color_ZSide(int x, int y, int z) {
	return ModernLighting_Color_Core(x, y, z, modernLighting_paletteZ, Env.SunZSide);
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
	RecalcForChunkIfNeeded(cx, cy, cz, chunkIndex);
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
	if (envVar == ENV_VAR_SUN_COLOR || envVar == ENV_VAR_SHADOW_COLOR) {
		ModernLighting_InitPalettes();
	}
}