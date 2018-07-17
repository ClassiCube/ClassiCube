#include "MapGenerator.h"
#include "BlockID.h"
#include "ErrorHandler.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Platform.h"

Int32 Gen_MaxX, Gen_MaxY, Gen_MaxZ, Gen_Volume;
#define Gen_Pack(x, y, z) (((y) * Gen_Length + (z)) * Gen_Width + (x))

static void Gen_Init(void) {
	Gen_MaxX = Gen_Width - 1; Gen_MaxY = Gen_Height - 1; Gen_MaxZ = Gen_Length - 1;
	Gen_Volume = Gen_Width * Gen_Length * Gen_Height;

	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState = "";

	Gen_Blocks = Platform_MemAlloc(Gen_Width * Gen_Height * Gen_Length, sizeof(BlockID));
	if (Gen_Blocks == NULL) {
		ErrorHandler_Fail("MapGen - failed to allocate Blocks array");
	}
	Gen_Done = false;
}


/*########################################################################################################################*
*-----------------------------------------------------Flatgrass gen-------------------------------------------------------*
*#########################################################################################################################*/
static void FlatgrassGen_MapSet(Int32 yStart, Int32 yEnd, BlockID block) {
	yStart = max(yStart, 0); yEnd = max(yEnd, 0);
	Int32 y, yHeight = (yEnd - yStart) + 1;
	BlockID* ptr = Gen_Blocks;
	UInt32 oneY = (UInt32)Gen_Width * (UInt32)Gen_Length;

	Gen_CurrentProgress = 0.0f;
	for (y = yStart; y <= yEnd; y++) {
		Platform_MemSet(ptr + y * oneY, block, oneY * (UInt32)sizeof(BlockID));
		Gen_CurrentProgress = (Real32)(y - yStart) / yHeight;
	}
}

void FlatgrassGen_Generate(void) {
	Gen_Init();

	Gen_CurrentState = "Setting air blocks";
	FlatgrassGen_MapSet(Gen_Height / 2, Gen_MaxY, BLOCK_AIR);

	Gen_CurrentState = "Setting dirt blocks";
	FlatgrassGen_MapSet(0, Gen_Height / 2 - 2, BLOCK_DIRT);

	Gen_CurrentState = "Setting grass blocks";
	FlatgrassGen_MapSet(Gen_Height / 2 - 1, Gen_Height / 2 - 1, BLOCK_GRASS);

	Gen_Done = true;
}


/*########################################################################################################################*
*----------------------------------------------------Notchy map gen-------------------------------------------------------*
*#########################################################################################################################*/
Int32 waterLevel, oneY, minHeight;
Int16* Heightmap;
Random rnd;

static void NotchyGen_FillOblateSpheroid(Int32 x, Int32 y, Int32 z, Real32 radius, BlockID block) {
	Int32 xStart = Math_Floor(max(x - radius, 0));
	Int32 xEnd = Math_Floor(min(x + radius, Gen_MaxX));
	Int32 yStart = Math_Floor(max(y - radius, 0));
	Int32 yEnd = Math_Floor(min(y + radius, Gen_MaxY));
	Int32 zStart = Math_Floor(max(z - radius, 0));
	Int32 zEnd = Math_Floor(min(z + radius, Gen_MaxZ));

	Real32 radiusSq = radius * radius;
	Int32 xx, yy, zz;

	for (yy = yStart; yy <= yEnd; yy++) {
		Int32 dy = yy - y;
		for (zz = zStart; zz <= zEnd; zz++) {
			Int32 dz = zz - z;
			for (xx = xStart; xx <= xEnd; xx++) {
				Int32 dx = xx - x;
				if ((dx * dx + 2 * dy * dy + dz * dz) < radiusSq) {
					Int32 index = Gen_Pack(xx, yy, zz);
					if (Gen_Blocks[index] == BLOCK_STONE)
						Gen_Blocks[index] = block;
				}
			}
		}
	}
}

#define Stack_Push(index)\
stack[stack_size++] = index;\
if (stack_size == 32768) {\
	ErrorHandler_Fail("NotchyGen_FloodFail - stack limit hit");\
}

static void NotchyGen_FloodFill(Int32 startIndex, BlockID block) {
	if (startIndex < 0) return; /* y below map, immediately ignore */
								/* This is way larger size than I actually have seen used, but we play it safe here.*/
	Int32 stack[32768];
	Int32 stack_size = 0;
	Stack_Push(startIndex);

	while (stack_size > 0) {
		stack_size--;
		Int32 index = stack[stack_size];
		if (Gen_Blocks[index] != 0) continue;
		Gen_Blocks[index] = block;

		Int32 x = index % Gen_Width;
		Int32 y = index / oneY;
		Int32 z = (index / Gen_Width) % Gen_Length;

		if (x > 0) { Stack_Push(index - 1); }
		if (x < Gen_MaxX) { Stack_Push(index + 1); }
		if (z > 0) { Stack_Push(index - Gen_Width); }
		if (z < Gen_MaxZ) { Stack_Push(index + Gen_Width); }
		if (y > 0) { Stack_Push(index - oneY); }
	}
}


static void NotchyGen_CreateHeightmap(void) {
	struct CombinedNoise n1, n2;
	CombinedNoise_Init(&n1, &rnd, 8, 8);
	CombinedNoise_Init(&n2, &rnd, 8, 8);
	struct OctaveNoise n3;
	OctaveNoise_Init(&n3, &rnd, 6);

	Int32 index = 0;
	Gen_CurrentState = "Building heightmap";

	Int32 x, z;
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (Real32)z / Gen_Length;
		for (x = 0; x < Gen_Width; x++) {
			Real32 hLow = CombinedNoise_Calc(&n1, x * 1.3f, z * 1.3f) / 6 - 4, height = hLow;

			if (OctaveNoise_Calc(&n3, (Real32)x, (Real32)z) <= 0) {
				Real32 hHigh = CombinedNoise_Calc(&n2, x * 1.3f, z * 1.3f) / 5 + 6;
				height = max(hLow, hHigh);
			}

			height *= 0.5f;
			if (height < 0) height *= 0.8f;

			Int32 adjHeight = (Int32)(height + waterLevel);
			minHeight = adjHeight < minHeight ? adjHeight : minHeight;
			Heightmap[index++] = adjHeight;
		}
	}
}

static Int32 NotchyGen_CreateStrataFast(void) {
	UInt32 oneY = (UInt32)Gen_Width * (UInt32)Gen_Length;
	Int32 y;
	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState = "Filling map";

	/* Make lava layer at bottom */
	Platform_MemSet(Gen_Blocks, BLOCK_LAVA, oneY * (UInt32)sizeof(BlockID));

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	Int32 stoneHeight = minHeight - 14;
	/* We can quickly fill in bottom solid layers */
	for (y = 1; y <= stoneHeight; y++) {
		Platform_MemSet(Gen_Blocks + y * oneY, BLOCK_STONE, oneY * (UInt32)sizeof(BlockID));
		Gen_CurrentProgress = (Real32)y / Gen_Height;
	}

	/* Fill in rest of map wih air */
	Int32 airHeight = max(0, stoneHeight) + 1;
	for (y = airHeight; y < Gen_Height; y++) {
		Platform_MemSet(Gen_Blocks + y * oneY, BLOCK_AIR, oneY * (UInt32)sizeof(BlockID));
		Gen_CurrentProgress = (Real32)y / Gen_Height;
	}

	/* if stoneHeight is <= 0, then no layer is fully stone */
	return max(stoneHeight, 1);
}

static void NotchyGen_CreateStrata(void) {
	/* Try to bulk fill bottom of the map if possible */
	Int32 minStoneY = NotchyGen_CreateStrataFast();

	struct OctaveNoise n;
	OctaveNoise_Init(&n, &rnd, 8);
	Gen_CurrentState = "Creating strata";
	Int32 hMapIndex = 0, maxY = Gen_MaxY, mapIndex = 0;

	Int32 x, y, z;
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (Real32)z / Gen_Length;
		for (x = 0; x < Gen_Width; x++) {
			Int32 dirtThickness = (Int32)(OctaveNoise_Calc(&n, (Real32)x, (Real32)z) / 24 - 4);
			Int32 dirtHeight = Heightmap[hMapIndex++];
			Int32 stoneHeight = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight  = min(dirtHeight,  maxY);

			mapIndex = Gen_Pack(x, minStoneY, z);
			for (y = minStoneY; y <= stoneHeight; y++) {
				Gen_Blocks[mapIndex] = BLOCK_STONE; mapIndex += oneY;
			}

			stoneHeight = max(stoneHeight, 0);
			mapIndex = Gen_Pack(x, (stoneHeight + 1), z);
			for (y = stoneHeight + 1; y <= dirtHeight; y++) {
				Gen_Blocks[mapIndex] = BLOCK_DIRT; mapIndex += oneY;
			}
		}
	}
}

static void NotchyGen_CarveCaves(void) {
	Int32 cavesCount = Gen_Volume / 8192;
	Gen_CurrentState = "Carving caves";

	Int32 i, j;
	for (i = 0; i < cavesCount; i++) {
		Gen_CurrentProgress = (Real32)i / cavesCount;
		Real32 caveX = (Real32)Random_Next(&rnd, Gen_Width);
		Real32 caveY = (Real32)Random_Next(&rnd, Gen_Height);
		Real32 caveZ = (Real32)Random_Next(&rnd, Gen_Length);

		Int32 caveLen = (Int32)(Random_Float(&rnd) * Random_Float(&rnd) * 200.0f);
		Real32 theta = Random_Float(&rnd) * 2.0f * MATH_PI, deltaTheta = 0.0f;
		Real32 phi   = Random_Float(&rnd) * 2.0f * MATH_PI, deltaPhi = 0.0f;
		Real32 caveRadius = Random_Float(&rnd) * Random_Float(&rnd);

		for (j = 0; j < caveLen; j++) {
			caveX += Math_SinF(theta) * Math_CosF(phi);
			caveZ += Math_CosF(theta) * Math_CosF(phi);
			caveY += Math_SinF(phi);

			theta = theta + deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);
			phi = phi * 0.5f + deltaPhi * 0.25f;
			deltaPhi = deltaPhi * 0.75f + Random_Float(&rnd) - Random_Float(&rnd);
			if (Random_Float(&rnd) < 0.25f) continue;

			Int32 cenX = (Int32)(caveX + (Random_Next(&rnd, 4) - 2) * 0.2f);
			Int32 cenY = (Int32)(caveY + (Random_Next(&rnd, 4) - 2) * 0.2f);
			Int32 cenZ = (Int32)(caveZ + (Random_Next(&rnd, 4) - 2) * 0.2f);

			Real32 radius = (Gen_Height - cenY) / (Real32)Gen_Height;
			radius = 1.2f + (radius * 3.5f + 1.0f) * caveRadius;
			radius = radius * Math_SinF(j * MATH_PI / caveLen);
			NotchyGen_FillOblateSpheroid(cenX, cenY, cenZ, radius, BLOCK_AIR);
		}
	}
}

static void NotchyGen_CarveOreVeins(Real32 abundance, const UChar* state, BlockID block) {
	Int32 numVeins = (Int32)(Gen_Volume * abundance / 16384);
	Gen_CurrentState = state;

	Int32 i, j;
	for (i = 0; i < numVeins; i++) {
		Gen_CurrentProgress = (Real32)i / numVeins;
		Real32 veinX = (Real32)Random_Next(&rnd, Gen_Width);
		Real32 veinY = (Real32)Random_Next(&rnd, Gen_Height);
		Real32 veinZ = (Real32)Random_Next(&rnd, Gen_Length);

		Int32 veinLen = (Int32)(Random_Float(&rnd) * Random_Float(&rnd) * 75 * abundance);
		Real32 theta = Random_Float(&rnd) * 2.0f * MATH_PI, deltaTheta = 0.0f;
		Real32 phi = Random_Float(&rnd) * 2.0f * MATH_PI, deltaPhi = 0.0f;

		for (j = 0; j < veinLen; j++) {
			veinX += Math_SinF(theta) * Math_CosF(phi);
			veinZ += Math_CosF(theta) * Math_CosF(phi);
			veinY += Math_SinF(phi);

			theta = deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);
			phi = phi * 0.5f + deltaPhi * 0.25f;
			deltaPhi = deltaPhi * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);

			Real32 radius = abundance * Math_SinF(j * MATH_PI / veinLen) + 1.0f;
			NotchyGen_FillOblateSpheroid((Int32)veinX, (Int32)veinY, (Int32)veinZ, radius, block);
		}
	}
}

static void NotchyGen_FloodFillWaterBorders(void) {
	Int32 waterY = waterLevel - 1;
	Int32 index1 = Gen_Pack(0, waterY, 0);
	Int32 index2 = Gen_Pack(0, waterY, Gen_Length - 1);
	Gen_CurrentState = "Flooding edge water";
	Int32 x, z;

	for (x = 0; x < Gen_Width; x++) {
		Gen_CurrentProgress = 0.0f + ((Real32)x / Gen_Width) * 0.5f;
		NotchyGen_FloodFill(index1, BLOCK_WATER);
		NotchyGen_FloodFill(index2, BLOCK_WATER);
		index1++; index2++;
	}

	index1 = Gen_Pack(0, waterY, 0);
	index2 = Gen_Pack(Gen_Width - 1, waterY, 0);
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = 0.5f + ((Real32)z / Gen_Length) * 0.5f;
		NotchyGen_FloodFill(index1, BLOCK_WATER);
		NotchyGen_FloodFill(index2, BLOCK_WATER);
		index1 += Gen_Width; index2 += Gen_Width;
	}
}

static void NotchyGen_FloodFillWater(void) {
	Int32 numSources = Gen_Width * Gen_Length / 800;
	Gen_CurrentState = "Flooding water";

	Int32 i;
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (Real32)i / numSources;
		Int32 x = Random_Next(&rnd, Gen_Width);
		Int32 z = Random_Next(&rnd, Gen_Length);
		Int32 y = waterLevel - Random_Range(&rnd, 1, 3);
		NotchyGen_FloodFill(Gen_Pack(x, y, z), BLOCK_WATER);
	}
}

static void NotchyGen_FloodFillLava(void) {
	Int32 numSources = Gen_Width * Gen_Length / 20000;
	Gen_CurrentState = "Flooding lava";

	Int32 i;
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (Real32)i / numSources;
		Int32 x = Random_Next(&rnd, Gen_Width);
		Int32 z = Random_Next(&rnd, Gen_Length);
		Int32 y = (Int32)((waterLevel - 3) * Random_Float(&rnd) * Random_Float(&rnd));
		NotchyGen_FloodFill(Gen_Pack(x, y, z), BLOCK_LAVA);
	}
}

static void NotchyGen_CreateSurfaceLayer(void) {
	struct OctaveNoise n1, n2;
	OctaveNoise_Init(&n1, &rnd, 8);
	OctaveNoise_Init(&n2, &rnd, 8);
	Gen_CurrentState = "Creating surface";
	/* TODO: update heightmap */

	Int32 hMapIndex = 0;
	Int32 x, z;
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (Real32)z / Gen_Length;
		for (x = 0; x < Gen_Width; x++) {
			Int32 y = Heightmap[hMapIndex++];
			if (y < 0 || y >= Gen_Height) continue;

			Int32 index = Gen_Pack(x, y, z);
			BlockID blockAbove = y >= Gen_MaxY ? BLOCK_AIR : Gen_Blocks[index + oneY];

			if (blockAbove == BLOCK_WATER && (OctaveNoise_Calc(&n2, (Real32)x, (Real32)z) > 12)) {
				Gen_Blocks[index] = BLOCK_GRAVEL;
			}
			else if (blockAbove == BLOCK_AIR) {
				Gen_Blocks[index] = (y <= waterLevel && (OctaveNoise_Calc(&n1, (Real32)x, (Real32)z) > 8)) ? BLOCK_SAND : BLOCK_GRASS;
			}
		}
	}
}

static void NotchyGen_PlantFlowers(void) {
	Int32 numPatches = Gen_Width * Gen_Length / 3000;
	Gen_CurrentState = "Planting flowers";

	Int32 i, j, k;
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (Real32)i / numPatches;
		BlockID type = (BlockID)(BLOCK_DANDELION + Random_Next(&rnd, 2));
		Int32 patchX = Random_Next(&rnd, Gen_Width), patchZ = Random_Next(&rnd, Gen_Length);
		for (j = 0; j < 10; j++) {
			Int32 flowerX = patchX, flowerZ = patchZ;
			for (k = 0; k < 5; k++) {
				flowerX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				flowerZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				if (flowerX < 0 || flowerZ < 0 || flowerX >= Gen_Width || flowerZ >= Gen_Length)
					continue;

				Int32 flowerY = Heightmap[flowerZ * Gen_Width + flowerX] + 1;
				if (flowerY <= 0 || flowerY >= Gen_Height) continue;

				Int32 index = Gen_Pack(flowerX, flowerY, flowerZ);
				if (Gen_Blocks[index] == BLOCK_AIR && Gen_Blocks[index - oneY] == BLOCK_GRASS)
					Gen_Blocks[index] = type;
			}
		}
	}
}

static void NotchyGen_PlantMushrooms(void) {
	Int32 numPatches = Gen_Volume / 2000;
	Gen_CurrentState = "Planting mushrooms";

	Int32 i, j, k;
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (Real32)i / numPatches;
		BlockID type = (BlockID)(BLOCK_BROWN_SHROOM + Random_Next(&rnd, 2));
		Int32 patchX = Random_Next(&rnd, Gen_Width);
		Int32 patchY = Random_Next(&rnd, Gen_Height);
		Int32 patchZ = Random_Next(&rnd, Gen_Length);

		for (j = 0; j < 20; j++) {
			Int32 mushX = patchX, mushY = patchY, mushZ = patchZ;
			for (k = 0; k < 5; k++) {
				mushX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				mushZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				if (mushX < 0 || mushZ < 0 || mushX >= Gen_Width || mushZ >= Gen_Length)
					continue;

				Int32 solidHeight = Heightmap[mushZ * Gen_Width + mushX];
				if (mushY >= (solidHeight - 1))
					continue;

				Int32 index = Gen_Pack(mushX, mushY, mushZ);
				if (Gen_Blocks[index] == BLOCK_AIR && Gen_Blocks[index - oneY] == BLOCK_STONE)
					Gen_Blocks[index] = type;
			}
		}
	}
}

static void NotchyGen_PlantTrees(void) {
	Int32 numPatches = Gen_Width * Gen_Length / 4000;
	Gen_CurrentState = "Planting trees";

	Tree_Width = Gen_Width; Tree_Height = Gen_Height; Tree_Length = Gen_Length;
	Tree_Blocks = Gen_Blocks;
	Tree_Rnd = &rnd;

	Int32 i, j, k, m;
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (Real32)i / numPatches;
		Int32 patchX = Random_Next(&rnd, Gen_Width), patchZ = Random_Next(&rnd, Gen_Length);

		for (j = 0; j < 20; j++) {
			Int32 treeX = patchX, treeZ = patchZ;
			for (k = 0; k < 20; k++) {
				treeX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				treeZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				if (treeX < 0 || treeZ < 0 || treeX >= Gen_Width ||
					treeZ >= Gen_Length || Random_Float(&rnd) >= 0.25)
					continue;

				Int32 treeY = Heightmap[treeZ * Gen_Width + treeX] + 1;
				if (treeY >= Gen_Height) continue;
				Int32 treeHeight = 5 + Random_Next(&rnd, 3);

				Int32 index = Gen_Pack(treeX, treeY, treeZ);
				BlockID blockUnder = treeY > 0 ? Gen_Blocks[index - oneY] : BLOCK_AIR;

				if (blockUnder == BLOCK_GRASS && TreeGen_CanGrow(treeX, treeY, treeZ, treeHeight)) {
					Vector3I coords[Tree_BufferCount];
					BlockID blocks[Tree_BufferCount];
					Int32 count = TreeGen_Grow(treeX, treeY, treeZ, treeHeight, coords, blocks);

					for (m = 0; m < count; m++) {
						index = Gen_Pack(coords[m].X, coords[m].Y, coords[m].Z);
						Gen_Blocks[index] = blocks[m];
					}
				}
			}
		}
	}
}

void NotchyGen_Generate(void) {
	Gen_Init();
	Heightmap = Platform_MemAlloc(Gen_Width * Gen_Length, sizeof(Int16));
	if (Heightmap == NULL) {
		ErrorHandler_Fail("NotchyGen - Failed to allocate Heightmap array");
	}

	Random_Init(&rnd, Gen_Seed);
	oneY = Gen_Width * Gen_Length;	
	waterLevel = Gen_Height / 2;	
	minHeight = Gen_Height;

	NotchyGen_CreateHeightmap();
	NotchyGen_CreateStrata();
	NotchyGen_CarveCaves();
	NotchyGen_CarveOreVeins(0.9f, "Carving coal ore", BLOCK_COAL_ORE);
	NotchyGen_CarveOreVeins(0.7f, "Carving iron ore", BLOCK_IRON_ORE);
	NotchyGen_CarveOreVeins(0.5f, "Carving gold ore", BLOCK_GOLD_ORE);

	NotchyGen_FloodFillWaterBorders();
	NotchyGen_FloodFillWater();
	NotchyGen_FloodFillLava();

	NotchyGen_CreateSurfaceLayer();
	NotchyGen_PlantFlowers();
	NotchyGen_PlantMushrooms();
	NotchyGen_PlantTrees();

	Platform_MemFree(&Heightmap);
	Gen_Done = true;
}


/*########################################################################################################################*
*---------------------------------------------------Noise generation------------------------------------------------------*
*#########################################################################################################################*/
void ImprovedNoise_Init(UInt8* p, Random* rnd) {
	/* shuffle randomly using fisher-yates */
	Int32 i;
	for (i = 0; i < 256; i++) { p[i] = i; }

	for (i = 0; i < 256; i++) {
		Int32 j = Random_Range(rnd, i, 256);
		UInt8 temp = p[i]; p[i] = p[j]; p[j] = temp;
	}

	for (i = 0; i < 256; i++) {
		p[i + 256] = p[i];
	}
}

Real32 ImprovedNoise_Calc(UInt8* p, Real32 x, Real32 y) {
	Int32 xFloor = x >= 0 ? (Int32)x : (Int32)x - 1;
	Int32 yFloor = y >= 0 ? (Int32)y : (Int32)y - 1;
	Int32 X = xFloor & 0xFF, Y = yFloor & 0xFF;
	x -= xFloor; y -= yFloor;

	Real32 u = x * x * x * (x * (x * 6 - 15) + 10); /* Fade(x) */
	Real32 v = y * y * y * (y * (y * 6 - 15) + 10); /* Fade(y) */
	Int32 A = p[X] + Y, B = p[X + 1] + Y;

	/* Normally, calculating Grad involves a function call. However, we can directly pack this table
	(since each value indicates either -1, 0 1) into a set of bit flags. This way we avoid needing
	to call another function that performs branching */
#define xFlags 0x46552222
#define yFlags 0x2222550A

	Int32 hash = (p[p[A]] & 0xF) << 1;
	Real32 g22 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * y; /* Grad(p[p[A], x, y) */
	hash = (p[p[B]] & 0xF) << 1;
	Real32 g12 = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * y; /* Grad(p[p[B], x - 1, y) */
	Real32 c1 = g22 + u * (g12 - g22);

	hash = (p[p[A + 1]] & 0xF) << 1;
	Real32 g21 = (((xFlags >> hash) & 3) - 1) * x + (((yFlags >> hash) & 3) - 1) * (y - 1); /* Grad(p[p[A + 1], x, y - 1) */
	hash = (p[p[B + 1]] & 0xF) << 1;
	Real32 g11 = (((xFlags >> hash) & 3) - 1) * (x - 1) + (((yFlags >> hash) & 3) - 1) * (y - 1); /* Grad(p[p[B + 1], x - 1, y - 1) */
	Real32 c2 = g21 + u * (g11 - g21);

	return c1 + v * (c2 - c1);
}


void OctaveNoise_Init(struct OctaveNoise* n, Random* rnd, Int32 octaves) {
	n->octaves = octaves;
	Int32 i;
	for (i = 0; i < octaves; i++) {
		ImprovedNoise_Init(n->p[i], rnd);
	}
}

Real32 OctaveNoise_Calc(struct OctaveNoise* n, Real32 x, Real32 y) {
	Real32 amplitude = 1, freq = 1;
	Real32 sum = 0;
	Int32 i;

	for (i = 0; i < n->octaves; i++) {
		sum += ImprovedNoise_Calc(n->p[i], x * freq, y * freq) * amplitude;
		amplitude *= 2.0f;
		freq *= 0.5f;
	}
	return sum;
}


void CombinedNoise_Init(struct CombinedNoise* n, Random* rnd, Int32 octaves1, Int32 octaves2) {
	OctaveNoise_Init(&n->noise1, rnd, octaves1);
	OctaveNoise_Init(&n->noise2, rnd, octaves2);
}

Real32 CombinedNoise_Calc(struct CombinedNoise* n, Real32 x, Real32 y) {
	Real32 offset = OctaveNoise_Calc(&n->noise2, x, y);
	return OctaveNoise_Calc(&n->noise1, x + offset, y);
}


/*########################################################################################################################*
*--------------------------------------------------Tree generation----------------------------------------------------*
*#########################################################################################################################*/
#define Tree_Pack(x, y, z) (((y) * Tree_Length + (z)) * Tree_Width + (x))

bool TreeGen_CanGrow(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 treeHeight) {
	/* check tree base */
	Int32 baseHeight = treeHeight - 4;
	Int32 x, y, z;

	for (y = treeY; y < treeY + baseHeight; y++) {
		for (z = treeZ - 1; z <= treeZ + 1; z++) {
			for (x = treeX - 1; x <= treeX + 1; x++) {
				if (x < 0 || y < 0 || z < 0 || x >= Tree_Width || y >= Tree_Height || z >= Tree_Length)
					return false;
				Int32 index = Tree_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}

	/* and also check canopy */
	for (y = treeY + baseHeight; y < treeY + treeHeight; y++) {
		for (z = treeZ - 2; z <= treeZ + 2; z++) {
			for (x = treeX - 2; x <= treeX + 2; x++) {
				if (x < 0 || y < 0 || z < 0 || x >= Tree_Width || y >= Tree_Height || z >= Tree_Length)
					return false;
				Int32 index = Tree_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}
	return true;
}

#define TreeGen_Place(x, y, z, block)\
coords[count].X = (x); coords[count].Y = (y); coords[count].Z = (z);\
blocks[count] = block; count++;

Int32 TreeGen_Grow(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 height, Vector3I* coords, BlockID* blocks) {
	Int32 baseHeight = height - 4;
	Int32 count = 0;
	Int32 xx, y, zz;

	/* leaves bottom layer */
	for (y = treeY + baseHeight; y < treeY + baseHeight + 2; y++) {
		for (zz = -2; zz <= 2; zz++) {
			for (xx = -2; xx <= 2; xx++) {
				Int32 x = treeX + xx, z = treeZ + zz;

				if (Math_AbsI(xx) == 2 && Math_AbsI(zz) == 2) {
					if (Random_Float(Tree_Rnd) >= 0.5f) {
						TreeGen_Place(x, y, z, BLOCK_LEAVES);
					}
				} else {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				}
			}
		}
	}

	/* leaves top layer */
	Int32 bottomY = treeY + baseHeight + 2;
	for (y = treeY + baseHeight + 2; y < treeY + height; y++) {
		for (zz = -1; zz <= 1; zz++) {
			for (xx = -1; xx <= 1; xx++) {
				Int32 x = xx + treeX, z = zz + treeZ;

				if (xx == 0 || zz == 0) {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				} else if (y == bottomY && Random_Float(Tree_Rnd) >= 0.5f) {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				}
			}
		}
	}

	/* then place trunk */
	for (y = 0; y < height - 1; y++) {
		TreeGen_Place(treeX, treeY + y, treeZ, BLOCK_LOG);
	}
	return count;
}
