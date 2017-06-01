#include "NotchyGenerator.h"
#include "BlockID.h"
#include "Funcs.h"
#include "Noise.h"
#include "Random.h"
#include "ExtMath.h"
#include "Platform.h"
#include "ErrorHandler.h"

/* Internal variables */
Int32 waterLevel, oneY, volume, minHeight;
Int16* Heightmap;
Random rnd;

void NotchyGen_Generate() {
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
}


void NotchyGen_CreateHeightmap() {
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

			if (OctaveNoise_Calc(&n3, x, z) <= 0) {
				Real32 hHigh = CombinedNoise_Calc(&n2, x * 1.3f, z * 1.3f) / 5 + 6;
				height = max(hLow, hHigh);
			}

			height *= 0.5;
			if (height < 0) height *= 0.8f;

			Int16 adjHeight = (Int16)(height + waterLevel);
			minHeight = adjHeight < minHeight ? adjHeight : minHeight;
			Heightmap[index++] = adjHeight;
		}
	}
}

void NotchyGen_CreateStrata() {
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
			Int32 dirtThickness = (Int32)(OctaveNoise_Calc(&n, x, z) / 24 - 4);
			Int32 dirtHeight = Heightmap[hMapIndex++];
			Int32 stoneHeight = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight = min(dirtHeight, maxY);

			mapIndex = minStoneY * oneY + z * Gen_Width + x;
			for (y = minStoneY; y <= stoneHeight; y++) {
				Gen_Blocks[mapIndex] = BlockID_Stone; mapIndex += oneY;
			}

			stoneHeight = max(stoneHeight, 0);
			mapIndex = (stoneHeight + 1) * oneY + z * Gen_Width + x;
			for (y = stoneHeight + 1; y <= dirtHeight; y++) {
				Gen_Blocks[mapIndex] = BlockID_Dirt; mapIndex += oneY;
			}
		}
	}
}

Int32 NotchyGen_CreateStrataFast() {
	/* Make lava layer at bottom */
	Int32 mapIndex = 0;
	Int32 x, y, z;

	for (z = 0; z < Gen_Length; z++)
		for (x = 0; x < Gen_Width; x++)
		{
			Gen_Blocks[mapIndex++] = BlockID_Lava;
		}

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	Int32 stoneHeight = minHeight - 14;
	if (stoneHeight <= 0) return 1; /* no layer is fully stone */

	/* We can quickly fill in bottom solid layers */
	for (y = 1; y <= stoneHeight; y++)
		for (z = 0; z < Gen_Length; z++)
			for (x = 0; x < Gen_Width; x++)
			{
				Gen_Blocks[mapIndex++] = BlockID_Stone;
			}
	return stoneHeight;
}

void NotchyGen_CreateSurfaceLayer() {
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

			Int32 index = (y * Gen_Length + z) * Gen_Width + x;
			BlockID blockAbove = y >= (Gen_Height - 1) ? BlockID_Air : Gen_Blocks[index + oneY];
			if (blockAbove == BlockID_Water && (OctaveNoise_Calc(&n2, x, z) > 12)) {
				Gen_Blocks[index] = BlockID_Gravel;
			} else if (blockAbove == BlockID_Air) {
				Gen_Blocks[index] = (y <= waterLevel && (OctaveNoise_Calc(&n1, x, z) > 8)) ? BlockID_Sand : BlockID_Grass;
			}
		}
	}
}

void NotchyGen_PlantFlowers() {
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

				Int32 index = (flowerY * Gen_Length + flowerZ) * Gen_Width + flowerX;
				if (Gen_Blocks[index] == BlockID_Air && Gen_Blocks[index - oneY] == BlockID_Grass)
					Gen_Blocks[index] = type;
			}
		}
	}
}

void NotchyGen_PlantMushrooms() {
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

				Int32 index = (mushY * Gen_Length + mushZ) * Gen_Width + mushX;
				if (Gen_Blocks[index] == BlockID_Air && Gen_Blocks[index - oneY] == BlockID_Stone)
					Gen_Blocks[index] = type;
			}
		}
	}
}

void NotchyGen_PlantTrees() {
	Int32 numPatches = Gen_Width * Gen_Length / 4000;
	Gen_CurrentState = String_FromConstant("Planting trees");

	Int32 i, j, k;
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

				Int32 index = (treeY * Gen_Length + treeZ) * Gen_Width + treeX;
				BlockID blockUnder = treeY > 0 ? Gen_Blocks[index - oneY] : BlockID_Air;

				if (blockUnder == BlockID_Grass && NotchyGen_CanGrowTree(treeX, treeY, treeZ, treeHeight)) {
					NotchyGen_GrowTree(treeX, treeY, treeZ, treeHeight);
				}
			}
		}
	}
}

bool NotchyGen_CanGrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 treeHeight) {
	/* check tree base */
	Int32 baseHeight = treeHeight - 4;
	Int32 x, y, z;

	for (y = treeY; y < treeY + baseHeight; y++)
		for (z = treeZ - 1; z <= treeZ + 1; z++)
			for (x = treeX - 1; x <= treeX + 1; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= Gen_Width || y >= Gen_Height || z >= Gen_Length)
					return false;
				Int32 index = (y * Gen_Length + z) * Gen_Width + x;
				if (Gen_Blocks[index] != 0) return false;
			}

	/* and also check canopy */
	for (y = treeY + baseHeight; y < treeY + treeHeight; y++)
		for (z = treeZ - 2; z <= treeZ + 2; z++)
			for (x = treeX - 2; x <= treeX + 2; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= Gen_Width || y >= Gen_Height || z >= Gen_Length)
					return false;
				Int32  index = (y * Gen_Length + z) * Gen_Width + x;
				if (Gen_Blocks[index] != 0) return false;
			}
	return true;
}

void NotchyGen_GrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 height) {
	Int32 baseHeight = height - 4;
	Int32 index = 0;
	Int32 xx, y, zz;

	/* leaves bottom layer */
	for (y = treeY + baseHeight; y < treeY + baseHeight + 2; y++)
		for (zz = -2; zz <= 2; zz++)
			for (xx = -2; xx <= 2; xx++)
			{
				Int32 x = xx + treeX, z = zz + treeZ;
				index = (y * Gen_Length + z) * Gen_Width + x;

				if (Math_Abs(xx) == 2 && Math_Abs(zz) == 2) {
					if (Random_Float(&rnd) >= 0.5f)
						Gen_Blocks[index] = BlockID_Leaves;
				} else {
					Gen_Blocks[index] = BlockID_Leaves;
				}
			}

	/* leaves top layer */
	Int32 bottomY = treeY + baseHeight + 2;
	for (y = treeY + baseHeight + 2; y < treeY + height; y++)
		for (zz = -1; zz <= 1; zz++)
			for (xx = -1; xx <= 1; xx++)
			{
				Int32 x = xx + treeX, z = zz + treeZ;
				index = (y * Gen_Length + z) * Gen_Width + x;

				if (xx == 0 || zz == 0) {
					Gen_Blocks[index] = BlockID_Leaves;
				} else if (y == bottomY && Random_Float(&rnd) >= 0.5f) {
					Gen_Blocks[index] = BlockID_Leaves;
				}
			}

	/* then place trunk */
	index = (treeY * Gen_Length + treeZ) * Gen_Width + treeX;
	for (y = 0; y < height - 1; y++) {
		Gen_Blocks[index] = BlockID_Log;
		index += oneY;
	}
}