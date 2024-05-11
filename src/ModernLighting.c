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


/*########################################################################################################################*
*----------------------------------------------------Modern lighting------------------------------------------------------*
*#########################################################################################################################*/

static struct Queue lightQueue;

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

	chunkLightingDataFlags = (cc_uint8*)Mem_TryAllocCleared(chunksCount, sizeof(cc_uint8));
	chunkLightingData = (LightingChunk*)Mem_TryAllocCleared(chunksCount, sizeof(LightingChunk));
	Queue_Init(&lightQueue, sizeof(IVec3));

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
	/* If it's not opaque and it doesn't block light, or it is brighter than 0, we can always pass through */
	if ((Blocks.Draw[thisBlock] > DRAW_OPAQUE && !Blocks.BlocksLight[thisBlock]) || Blocks.Brightness[thisBlock]) { return true; }
	/* Light can always pass through leaves and water */
	if (Blocks.Draw[thisBlock] == DRAW_TRANSPARENT_THICK || Blocks.Draw[thisBlock] == DRAW_TRANSLUCENT) { return true; }

	/* Light can never pass through a block that's full sized and blocks light */
	/* We can assume a block is full sized if none of the LightOffset flags are 0 */
	if (Blocks.BlocksLight[thisBlock] && Blocks.LightOffset[thisBlock] == 0xFF) { return false; }

	/* Is stone's face hidden by thisBlock? TODO: Don't hardcode using stone */
	return !Block_IsFaceHidden(BLOCK_STONE, thisBlock, face);
}

static void CalcBlockLight(cc_uint8 blockLight, int x, int y, int z) {
	SetBlocklight(blockLight, x, y, z, false);
	IVec3 entry = { x, y, z };
	Queue_Enqueue(&lightQueue, &entry);

	//if (Blocks.BlocksLight[World_GetBlock(x, y, z)]) { return; }

	while (lightQueue.count > 0) {
		IVec3 curNode = *(IVec3*)(Queue_Dequeue(&lightQueue));
		cc_uint8 curLight = GetBlocklight(curNode.x, curNode.y, curNode.z, false);
		if (curLight <= 0) {
			Platform_Log1("but there were still %i entries left...", &lightQueue.capacity);
			return;
		}

		BlockID thisBlock = World_GetBlock(curNode.x, curNode.y, curNode.z);
		curLight--; // 1 light level less in each neighbour
		if (curLight == 0) continue;


		curNode.x--;
		if (curNode.x > 0 &&
			CanLightPass(thisBlock, FACE_XMAX) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_XMIN) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, false) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, false);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.x += 2;
		if (curNode.x < World.MaxX &&
			CanLightPass(thisBlock, FACE_XMIN) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_XMAX) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, false) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, false);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.x--;

		curNode.y--;
		if (curNode.y > 0 &&
			CanLightPass(thisBlock, FACE_YMAX) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_YMIN) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, false) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, false);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.y += 2;
		if (curNode.y < World.MaxY &&
			CanLightPass(thisBlock, FACE_YMIN) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_YMAX) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, false) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, false);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.y--;

		curNode.z--;
		if (curNode.z > 0 &&
			CanLightPass(thisBlock, FACE_ZMAX) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_ZMIN) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, false) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, false);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.z += 2;
		if (curNode.z < World.MaxZ &&
			CanLightPass(thisBlock, FACE_ZMIN) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_ZMAX) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, false) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, false);
			Queue_Enqueue(&lightQueue, &curNode);
		}
	}
}

static void CalcSkyLight(cc_uint8 blockLight, int x, int y, int z) {
	SetBlocklight(blockLight, x, y, z, true);
	IVec3 entry = { x, y, z };
	Queue_Enqueue(&lightQueue, &entry);

	//if (Blocks.BlocksLight[World_GetBlock(x, y, z)]) { return; }

	while (lightQueue.count > 0) {
		IVec3 curNode = *(IVec3*)(Queue_Dequeue(&lightQueue));
		cc_uint8 curLight = GetBlocklight(curNode.x, curNode.y, curNode.z, true);
		if (curLight <= 0) {
			Platform_Log1("but there were still %i entries left...", &lightQueue.capacity);
			return;
		}

		BlockID thisBlock = World_GetBlock(curNode.x, curNode.y, curNode.z);
		curLight--; // 1 light level less in each neighbour
		if (curLight == MODERN_LIGHTING_LEVELS - SUN_LEVELS) continue;


		curNode.x--;
		if (curNode.x > 0 &&
			curNode.y <= ClassicLighting_GetLightHeight(curNode.x, curNode.z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_XMAX) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_XMIN) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, true) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, true);
			IVec3 entry = { curNode.x, curNode.y, curNode.z };
			Queue_Enqueue(&lightQueue, &entry);
		}
		curNode.x += 2;
		if (curNode.x < World.MaxX &&
			curNode.y <= ClassicLighting_GetLightHeight(curNode.x, curNode.z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_XMIN) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_XMAX) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, true) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, true);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.x--;

		curNode.y--;
		if (curNode.y > 0 &&
			curNode.y <= ClassicLighting_GetLightHeight(curNode.x, curNode.z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_YMAX) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_YMIN) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, true) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, true);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.y += 2;
		if (curNode.y < World.MaxY &&
			curNode.y <= ClassicLighting_GetLightHeight(curNode.x, curNode.z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_YMIN) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_YMAX) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, true) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, true);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.y--;

		curNode.z--;
		if (curNode.z > 0 &&
			curNode.y <= ClassicLighting_GetLightHeight(curNode.x, curNode.z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_ZMAX) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_ZMIN) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, true) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, true);
			Queue_Enqueue(&lightQueue, &curNode);
		}
		curNode.z += 2;
		if (curNode.z < World.MaxZ &&
			curNode.y <= ClassicLighting_GetLightHeight(curNode.x, curNode.z) && //don't propagate into full sunlight
			CanLightPass(thisBlock, FACE_ZMIN) &&
			CanLightPass(World_GetBlock(curNode.x, curNode.y, curNode.z), FACE_ZMAX) &&
			GetBlocklight(curNode.x, curNode.y, curNode.z, true) < curLight) {
			SetBlocklight(curLight, curNode.x, curNode.y, curNode.z, true);
			Queue_Enqueue(&lightQueue, &curNode);
		}
	}
}


static void CalculateChunkLightingSelf(int chunkIndex, int cx, int cy, int cz) {
	int x, y, z;
	int chunkStartX, chunkStartY, chunkStartZ; //world coords
	int chunkEndX, chunkEndY, chunkEndZ; //world coords
	cc_bool brightness;
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
				if (Blocks.Brightness[curBlock] > 0) {
					brightness = Blocks.Brightness[curBlock] & MODERN_LIGHTING_MAX_LEVEL; /* get block brightness */
					if (brightness > 0) { CalcBlockLight(brightness, x, y, z); }
					else {
						/* If no block brightness, it must use sun brightness */
						CalcSkyLight(Blocks.Brightness[curBlock] >> MODERN_LIGHTING_SUN_SHIFT, x, y, z);
					}
				}

				//this cell is exposed to sunlight
				if (y > ClassicLighting_GetLightHeight(x, z)) {
					//CalcSkyLight(MODERN_LIGHTING_MAX_LEVEL, x, y, z);
					/* Simply light this cell fully with sun, don't bother spreading */
					SetBlocklight(MODERN_LIGHTING_MAX_LEVEL, x, y, z, true);
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

static void ModernLighting_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {

}
/* Invalidates/Resets lighting state for all of the blocks in the world */
/*  (e.g. because a block changed whether it is full bright or not) */
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
	int cx, cy, cz, curX, curY, curZ;
	ClassicLighting_LightHint(startX, startZ);

	// precalculate lighting for this chunk and its neighbours
	cx = (startX + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;
	cy = (startY + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;
	cz = (startZ + HALF_CHUNK_SIZE) >> CHUNK_SHIFT;

	for (curY = cy - 1; curY <= cy + 1; curY++)
		for (curZ = cz - 1; curZ <= cz + 1; curZ++)
			for (curX = cx - 1; curX <= cx + 1; curX++) {
				ModernLighting_Color(curX << CHUNK_SHIFT, curY << CHUNK_SHIFT, curZ << CHUNK_SHIFT);
			}
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