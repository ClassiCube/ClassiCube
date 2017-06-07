#include "NotchyGenerator.h"
#include "BlockID.h"
#include "Funcs.h"
#include "Noise.h"
#include "Random.h"
#include "ExtMath.h"
#include "Platform.h"
#include "ErrorHandler.h"
#include "TreeGen.h"

/* Internal variables */
Int32 waterLevel, oneY, volume, minHeight;
Int16* Heightmap;
Random rnd;

void NotchyGen_Generate(void) {
	Heightmap = Platform_MemAlloc(Gen_Width * Gen_Length * sizeof(Int16));
	if (Heightmap == NULL)
		ErrorHandler_Fail("NotchyGen_Heightmap - failed to allocate memory");
	Gen_Blocks = Platform_MemAlloc(Gen_Width * Gen_Height * Gen_Length * sizeof(BlockID));
	if (Gen_Blocks == NULL)
		ErrorHandler_Fail("NotchyGen_Blocks - failed to allocate memory");

	oneY = Gen_Width * Gen_Length;
	volume = Gen_Width * Gen_Length * Gen_Height;
	waterLevel = Gen_Height / 2;
	Random_Init(&rnd, Gen_Seed);
	minHeight = Gen_Height;

	Gen_CurrentProgress = 0.0f;
	Gen_CurrentState = String_FromConstant("");

	NotchyGen_CreateHeightmap();
	NotchyGen_CreateStrata();
	NotchyGen_CarveCaves();
	NotchyGen_CarveOreVeins(0.9f, "Carving coal ore", BlockID_CoalOre);
	NotchyGen_CarveOreVeins(0.7f, "Carving iron ore", BlockID_IronOre);
	NotchyGen_CarveOreVeins(0.5f, "Carving gold ore", BlockID_GoldOre);

	NotchyGen_FloodFillWaterBorders();
	NotchyGen_FloodFillWater();
	NotchyGen_FloodFillLava();

	NotchyGen_CreateSurfaceLayer();
	NotchyGen_PlantFlowers();
	NotchyGen_PlantMushrooms();
	NotchyGen_PlantTrees();

	Platform_MemFree(Heightmap);
}


void NotchyGen_CreateHeightmap(void) {
	CombinedNoise n1, n2;
	CombinedNoise_Init(&n1, &rnd, 8, 8);
	CombinedNoise_Init(&n2, &rnd, 8, 8);
	OctaveNoise n3;
	OctaveNoise_Init(&n3, &rnd, 6);

	Int32 index = 0;
	Gen_CurrentState = String_FromConstant("Building heightmap");

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

			Int16 adjHeight = (Int16)(height + waterLevel);
			minHeight = adjHeight < minHeight ? adjHeight : minHeight;
			Heightmap[index++] = adjHeight;
		}
	}
}

void NotchyGen_CreateStrata(void) {
	OctaveNoise n;
	OctaveNoise_Init(&n, &rnd, 8);
	Gen_CurrentState = String_FromConstant("Creating strata");
	Int32 hMapIndex = 0, maxY = Gen_Height - 1, mapIndex = 0;
	/* Try to bulk fill bottom of the map if possible */
	Int32 minStoneY = NotchyGen_CreateStrataFast();

	Int32 x, y, z;
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (Real32)z / Gen_Length;
		for (x = 0; x < Gen_Width; x++) {
			Int32 dirtThickness = (Int32)(OctaveNoise_Calc(&n, (Real32)x, (Real32)z) / 24 - 4);
			Int32 dirtHeight = Heightmap[hMapIndex++];
			Int32 stoneHeight = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight = min(dirtHeight, maxY);

			mapIndex = Gen_Pack(x, minStoneY, z);
			for (y = minStoneY; y <= stoneHeight; y++) {
				Gen_Blocks[mapIndex] = BlockID_Stone; mapIndex += oneY;
			}

			stoneHeight = max(stoneHeight, 0);
			mapIndex = Gen_Pack(x, (stoneHeight + 1), z);
			for (y = stoneHeight + 1; y <= dirtHeight; y++) {
				Gen_Blocks[mapIndex] = BlockID_Dirt; mapIndex += oneY;
			}
		}
	}
}

Int32 NotchyGen_CreateStrataFast(void) {
	/* Make lava layer at bottom */
	Int32 mapIndex = 0;
	Int32 x, y, z;

	for (z = 0; z < Gen_Length; z++) {
		for (x = 0; x < Gen_Width; x++) {
			Gen_Blocks[mapIndex++] = BlockID_Lava;
		}
	}

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	Int32 stoneHeight = minHeight - 14;
	if (stoneHeight <= 0) return 1; /* no layer is fully stone */

	/* We can quickly fill in bottom solid layers */
	for (y = 1; y <= stoneHeight; y++) {
		for (z = 0; z < Gen_Length; z++) {
			for (x = 0; x < Gen_Width; x++) {
				Gen_Blocks[mapIndex++] = BlockID_Stone;
			}
		}
	}
	return stoneHeight;
}

void NotchyGen_CarveCaves(void) {
	Int32 cavesCount = volume / 8192;
	Gen_CurrentState = String_FromConstant("Carving caves");

	Int32 i, j;
	for (i = 0; i < cavesCount; i++) {
		Gen_CurrentProgress = (Real32)i / cavesCount;
		Real32 caveX = (Real32)Random_Next(&rnd, Gen_Width);
		Real32 caveY = (Real32)Random_Next(&rnd, Gen_Height);
		Real32 caveZ = (Real32)Random_Next(&rnd, Gen_Length);

		Int32 caveLen = (Int32)(Random_Float(&rnd) * Random_Float(&rnd) * 200.0f);
		Real32 theta = Random_Float(&rnd) * 2.0f * MATH_PI, deltaTheta = 0.0f;
		Real32 phi   = Random_Float(&rnd) * 2.0f * MATH_PI, deltaPhi   = 0.0f;
		Real32 caveRadius = Random_Float(&rnd) * Random_Float(&rnd);

		for (j = 0; j < caveLen; j++) {
			caveX += Math_Sin(theta) * Math_Cos(phi);
			caveZ += Math_Cos(theta) * Math_Cos(phi);
			caveY += Math_Sin(phi);

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
			radius = radius * Math_Sin(j * MATH_PI / caveLen);
			NotchyGen_FillOblateSpheroid(cenX, cenY, cenZ, radius, BlockID_Air);
		}
	}
}

void NotchyGen_CarveOreVeins(Real32 abundance, const UInt8* state, BlockID block) {
	Int32 numVeins = (Int32)(volume * abundance / 16384);
	Gen_CurrentState = String_FromConstant(state);

	Int32 i, j;
	for (i = 0; i < numVeins; i++) {
		Gen_CurrentProgress = (Real32)i / numVeins;
		Real32 veinX = (Real32)Random_Next(&rnd, Gen_Width);
		Real32 veinY = (Real32)Random_Next(&rnd, Gen_Height);
		Real32 veinZ = (Real32)Random_Next(&rnd, Gen_Length);

		Int32 veinLen = (Int32)(Random_Float(&rnd) * Random_Float(&rnd) * 75 * abundance);
		Real32 theta = Random_Float(&rnd) * 2.0f * MATH_PI, deltaTheta = 0.0f;
		Real32 phi   = Random_Float(&rnd) * 2.0f * MATH_PI,   deltaPhi = 0.0f;

		for (j = 0; j < veinLen; j++) {
			veinX += Math_Sin(theta) * Math_Cos(phi);
			veinZ += Math_Cos(theta) * Math_Cos(phi);
			veinY += Math_Sin(phi);

			theta = deltaTheta * 0.2f;
			deltaTheta = deltaTheta * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);
			phi = phi * 0.5f + deltaPhi * 0.25f;
			deltaPhi = deltaPhi * 0.9f + Random_Float(&rnd) - Random_Float(&rnd);

			Real32 radius = abundance * Math_Sin(j * MATH_PI / veinLen) + 1.0f;
			NotchyGen_FillOblateSpheroid((Int32)veinX, (Int32)veinY, (Int32)veinZ, radius, block);
		}
	}
}

void NotchyGen_FloodFillWaterBorders(void) {
	Int32 waterY = waterLevel - 1;
	Int32 index1 = Gen_Pack(0, waterY, 0);
	Int32 index2 = Gen_Pack(0, waterY, Gen_Length - 1);
	Gen_CurrentState = String_FromConstant("Flooding edge water");
	Int32 x, z;

	for (x = 0; x < Gen_Width; x++) {
		Gen_CurrentProgress = 0.0f + ((Real32)x / Gen_Width) * 0.5f;
		NotchyGen_FloodFill(index1, BlockID_Water);
		NotchyGen_FloodFill(index2, BlockID_Water);
		index1++; index2++;
	}

	index1 = Gen_Pack(0, waterY, 0);
	index2 = Gen_Pack(Gen_Width - 1, waterY, 0);
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = 0.5f + ((Real32)z / Gen_Length) * 0.5f;
		NotchyGen_FloodFill(index1, BlockID_Water);
		NotchyGen_FloodFill(index2, BlockID_Water);
		index1 += Gen_Width; index2 += Gen_Width;
	}
}

void NotchyGen_FloodFillWater(void) {
	Int32 numSources = Gen_Width * Gen_Length / 800;
	Gen_CurrentState = String_FromConstant("Flooding water");

	Int32 i;
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (Real32)i / numSources;
		Int32 x = Random_Next(&rnd, Gen_Width);
		Int32 z = Random_Next(&rnd, Gen_Length);
		Int32 y = waterLevel - Random_Range(&rnd, 1, 3);
		NotchyGen_FloodFill(Gen_Pack(x, y, z), BlockID_Water);
	}
}

void NotchyGen_FloodFillLava(void) {
	Int32 numSources = Gen_Width * Gen_Length / 20000;
	Gen_CurrentState = String_FromConstant("Flooding lava");

	Int32 i;
	for (i = 0; i < numSources; i++) {
		Gen_CurrentProgress = (float)i / numSources;
		Int32 x = Random_Next(&rnd, Gen_Width);
		Int32 z = Random_Next(&rnd, Gen_Length);
		Int32 y = (Int32)((waterLevel - 3) * Random_Float(&rnd) * Random_Float(&rnd));
		NotchyGen_FloodFill(Gen_Pack(x, y, z), BlockID_Lava);
	}
}

void NotchyGen_CreateSurfaceLayer(void) {
	OctaveNoise n1, n2;
	OctaveNoise_Init(&n1, &rnd, 8);
	OctaveNoise_Init(&n2, &rnd, 8);
	Gen_CurrentState = String_FromConstant("Creating surface");
	/* TODO: update heightmap */

	Int32 hMapIndex = 0;
	Int32 x, z;
	for (z = 0; z < Gen_Length; z++) {
		Gen_CurrentProgress = (Real32)z / Gen_Length;
		for (x = 0; x < Gen_Width; x++) {
			Int32 y = Heightmap[hMapIndex++];
			if (y < 0 || y >= Gen_Height) continue;

			Int32 index = Gen_Pack(x, y, z);
			BlockID blockAbove = y >= Gen_MaxY ? BlockID_Air : Gen_Blocks[index + oneY];

			if (blockAbove == BlockID_Water && (OctaveNoise_Calc(&n2, (Real32)x, (Real32)z) > 12)) {
				Gen_Blocks[index] = BlockID_Gravel;
			} else if (blockAbove == BlockID_Air) {
				Gen_Blocks[index] = (y <= waterLevel && (OctaveNoise_Calc(&n1, (Real32)x, (Real32)z) > 8)) ? BlockID_Sand : BlockID_Grass;
			}
		}
	}
}

void NotchyGen_PlantFlowers(void) {
	Int32 numPatches = Gen_Width * Gen_Length / 3000;
	Gen_CurrentState = String_FromConstant("Planting flowers");

	Int32 i, j, k;
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (Real32)i / numPatches;
		BlockID type = (BlockID)(BlockID_Dandelion + Random_Next(&rnd, 2));
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
				if (Gen_Blocks[index] == BlockID_Air && Gen_Blocks[index - oneY] == BlockID_Grass)
					Gen_Blocks[index] = type;
			}
		}
	}
}

void NotchyGen_PlantMushrooms(void) {
	Int32 numPatches = volume / 2000;
	Gen_CurrentState = String_FromConstant("Planting mushrooms");

	Int32 i, j, k;
	for (i = 0; i < numPatches; i++) {
		Gen_CurrentProgress = (Real32)i / numPatches;
		BlockID type = (BlockID)(BlockID_BrownMushroom + Random_Next(&rnd, 2));
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
				if (Gen_Blocks[index] == BlockID_Air && Gen_Blocks[index - oneY] == BlockID_Stone)
					Gen_Blocks[index] = type;
			}
		}
	}
}

void NotchyGen_PlantTrees(void) {
	Int32 numPatches = Gen_Width * Gen_Length / 4000;
	Gen_CurrentState = String_FromConstant("Planting trees");

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
				BlockID blockUnder = treeY > 0 ? Gen_Blocks[index - oneY] : BlockID_Air;

				if (blockUnder == BlockID_Grass && TreeGen_CanGrow(treeX, treeY, treeZ, treeHeight)) {
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


void NotchyGen_FillOblateSpheroid(Int32 x, Int32 y, Int32 z, Real32 radius, BlockID block) {
	Int32 xStart = Math_Floor(max(x - radius, 0));
	Int32 xEnd   = Math_Floor(min(x + radius, Gen_MaxX));
	Int32 yStart = Math_Floor(max(y - radius, 0));
	Int32 yEnd   = Math_Floor(min(y + radius, Gen_MaxY));
	Int32 zStart = Math_Floor(max(z - radius, 0));
	Int32 zEnd   = Math_Floor(min(z + radius, Gen_MaxZ));

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
					if (Gen_Blocks[index] == BlockID_Stone)
						Gen_Blocks[index] = block;
				}
			}
		}
	}
}

#define Stack_Push(index)\
stack[stack_size] = index;\
stack_size++;\
if (stack_size == 32768) {\
	ErrorHandler_Fail("NotchyGen_FloodFail - stack limit hit");\
}

void NotchyGen_FloodFill(Int32 startIndex, BlockID block) {
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

		if (x > 0)        { Stack_Push(index - 1); }
		if (x < Gen_MaxX) { Stack_Push(index + 1); }
		if (z > 0)        { Stack_Push(index - Gen_Width); }
		if (z < Gen_MaxZ) { Stack_Push(index + Gen_Width); }
		if (y > 0)        { Stack_Push(index - oneY); }
	}
}