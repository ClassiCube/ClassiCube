
#include "NotchyGenerator.h"
#include "BlockID.h"
#include "Funcs.h"
#include "Noise.h"
#include "Random.h"

/* External variables */
/* TODO: how do they even work? */
Real32 CurrentProgress;

/* Internal variables */
Int32 Width, Height, Length;
Int32 waterLevel, oneY, volume, minHeight;
BlockID* Blocks;
Int16* Heightmap;
Random rnd;

void NotchyGen_Init(Int32 width, Int32 height, Int32 length, 
					Int32 seed, BlockID* blocks, Int16* heightmap) {
	Width = width; Height = height; Length = length;
	Blocks = blocks; Heightmap = heightmap;

	oneY = Width * Length;
	volume = Width * Length * Height;
	waterLevel = Height / 2;
	Random_Init(&rnd, seed);
	minHeight = Height;

	CurrentProgress = 0.0f;
}


void NotchyGen_CreateHeightmap() {
	CombinedNoise n1, n2;
	CombinedNoise_Init(&n1, &rnd, 8, 8);
	CombinedNoise_Init(&n2, &rnd, 8, 8);
	OctaveNoise n3;
	OctaveNoise_Init(&n3, &rnd, 6);

	Int32 index = 0;
	//CurrentState = "Building heightmap";

	for (Int32 z = 0; z < Length; z++) {
		CurrentProgress = (Real32)z / Length;
		for (Int32 x = 0; x < Width; x++) {
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
	//CurrentState = "Creating strata";
	Int32 hMapIndex = 0, maxY = Height - 1, mapIndex = 0;
	/* Try to bulk fill bottom of the map if possible */
	Int32 minStoneY = NotchyGen_CreateStrataFast();

	for (Int32 z = 0; z < Length; z++) {
		CurrentProgress = (Real32)z / Length;
		for (Int32 x = 0; x < Width; x++) {
			Int32 dirtThickness = (Int32)(OctaveNoise_Calc(&n, x, z) / 24 - 4);
			Int32 dirtHeight = Heightmap[hMapIndex++];
			Int32 stoneHeight = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight = min(dirtHeight, maxY);

			mapIndex = minStoneY * oneY + z * Width + x;
			for (Int32 y = minStoneY; y <= stoneHeight; y++) {
				Blocks[mapIndex] = BlockID_Stone; mapIndex += oneY;
			}

			stoneHeight = max(stoneHeight, 0);
			mapIndex = (stoneHeight + 1) * oneY + z * Width + x;
			for (Int32 y = stoneHeight + 1; y <= dirtHeight; y++) {
				Blocks[mapIndex] = BlockID_Dirt; mapIndex += oneY;
			}
		}
	}
}

Int32 NotchyGen_CreateStrataFast() {
	/* Make lava layer at bottom */
	Int32 mapIndex = 0;
	for (Int32 z = 0; z < Length; z++)
		for (Int32 x = 0; x < Width; x++)
		{
			Blocks[mapIndex++] = BlockID_Lava;
		}

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	Int32 stoneHeight = minHeight - 14;
	if (stoneHeight <= 0) return 1; /* no layer is fully stone */

	/* We can quickly fill in bottom solid layers */
	for (Int32 y = 1; y <= stoneHeight; y++)
		for (Int32 z = 0; z < Length; z++)
			for (Int32 x = 0; x < Width; x++)
			{
				Blocks[mapIndex++] = BlockID_Stone;
			}
	return stoneHeight;
}

void NotchyGen_CreateSurfaceLayer() {
	OctaveNoise n1, n2;
	OctaveNoise_Init(&n1, &rnd, 8);
	OctaveNoise_Init(&n2, &rnd, 8);
	//CurrentState = "Creating surface";
	/* TODO: update heightmap */

	Int32 hMapIndex = 0;
	for (Int32 z = 0; z < Length; z++) {
		CurrentProgress = (Real32)z / Length;
		for (Int32 x = 0; x < Width; x++) {
			Int32 y = Heightmap[hMapIndex++];
			if (y < 0 || y >= Height) continue;

			Int32 index = (y * Length + z) * Width + x;
			BlockID blockAbove = y >= (Height - 1) ? BlockID_Air : Blocks[index + oneY];
			if (blockAbove == BlockID_Water && (OctaveNoise_Calc(&n2, x, z) > 12)) {
				Blocks[index] = BlockID_Gravel;
			} else if (blockAbove == BlockID_Air) {
				Blocks[index] = (y <= waterLevel && (OctaveNoise_Calc(&n1, x, z) > 8)) ? BlockID_Sand : BlockID_Grass;
			}
		}
	}
}

void NotchyGen_PlantFlowers() {
	Int32 numPatches = Width * Length / 3000;
	//CurrentState = "Planting flowers";

	for (Int32 i = 0; i < numPatches; i++) {
		CurrentProgress = (Real32)i / numPatches;
		BlockID type = (BlockID)(BlockID_Dandelion + Random_Next(&rnd, 2));
		Int32 patchX = Random_Next(&rnd, Width), patchZ = Random_Next(&rnd, Length);
		for (Int32 j = 0; j < 10; j++) {
			Int32 flowerX = patchX, flowerZ = patchZ;
			for (Int32 k = 0; k < 5; k++) {
				flowerX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				flowerZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				if (flowerX < 0 || flowerZ < 0 || flowerX >= Width || flowerZ >= Length)
					continue;

				Int32 flowerY = Heightmap[flowerZ * Width + flowerX] + 1;
				if (flowerY <= 0 || flowerY >= Height) continue;

				Int32 index = (flowerY * Length + flowerZ) * Width + flowerX;
				if (Blocks[index] == BlockID_Air && Blocks[index - oneY] == BlockID_Grass)
					Blocks[index] = type;
			}
		}
	}
}

void NotchyGen_PlantMushrooms() {
	Int32 numPatches = volume / 2000;
	//CurrentState = "Planting mushrooms";

	for (Int32 i = 0; i < numPatches; i++) {
		CurrentProgress = (Real32)i / numPatches;
		BlockID type = (BlockID)(BlockID_BrownMushroom + Random_Next(&rnd, 2));
		Int32 patchX = Random_Next(&rnd, Width);
		Int32 patchY = Random_Next(&rnd, Height);
		Int32 patchZ = Random_Next(&rnd, Length);

		for (Int32 j = 0; j < 20; j++) {
			Int32 mushX = patchX, mushY = patchY, mushZ = patchZ;
			for (Int32 k = 0; k < 5; k++) {
				mushX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				mushZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				if (mushX < 0 || mushZ < 0 || mushX >= Width || mushZ >= Length)
					continue;

				Int32 solidHeight = Heightmap[mushZ * Width + mushX];
				if (mushY >= (solidHeight - 1))
					continue;

				Int32 index = (mushY * Length + mushZ) * Width + mushX;
				if (Blocks[index] == BlockID_Air && Blocks[index - oneY] == BlockID_Stone)
					Blocks[index] = type;
			}
		}
	}
}

void NotchyGen_PlantTrees() {
	Int32 numPatches = Width * Length / 4000;
	//CurrentState = "Planting trees";

	for (Int32 i = 0; i < numPatches; i++) {
		CurrentProgress = (Real32)i / numPatches;
		Int32 patchX = Random_Next(&rnd, Width), patchZ = Random_Next(&rnd, Length);

		for (Int32 j = 0; j < 20; j++) {
			Int32 treeX = patchX, treeZ = patchZ;
			for (Int32 k = 0; k < 20; k++) {
				treeX += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				treeZ += Random_Next(&rnd, 6) - Random_Next(&rnd, 6);
				if (treeX < 0 || treeZ < 0 || treeX >= Width ||
					treeZ >= Length || Random_Float(&rnd) >= 0.25)
					continue;

				Int32 treeY = Heightmap[treeZ * Width + treeX] + 1;
				Int32 treeHeight = 5 + Random_Next(&rnd, 3);

				Int32 index = (treeY * Length + treeZ) * Width + treeX;
				BlockID blockUnder = treeY > 0 ? Blocks[index - oneY] : BlockID_Air;

				if (blockUnder == BlockID_Grass && NotchyGen_CanGrowTree(treeX, treeY, treeZ, treeHeight)) {
					NotchyGen_GrowTree(treeX, treeY, treeZ, treeHeight);
				}
			}
		}
	}
}

bool NotchyGen_CanGrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 treeHeight) {
	// check tree base
	Int32 baseHeight = treeHeight - 4;
	for (Int32 y = treeY; y < treeY + baseHeight; y++)
		for (Int32 z = treeZ - 1; z <= treeZ + 1; z++)
			for (Int32 x = treeX - 1; x <= treeX + 1; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= Width || y >= Height || z >= Length)
					return false;
				Int32 index = (y * Length + z) * Width + x;
				if (Blocks[index] != 0) return false;
			}

	// and also check canopy
	for (Int32 y = treeY + baseHeight; y < treeY + treeHeight; y++)
		for (Int32 z = treeZ - 2; z <= treeZ + 2; z++)
			for (Int32 x = treeX - 2; x <= treeX + 2; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= Width || y >= Height || z >= Length)
					return false;
				Int32  index = (y * Length + z) * Width + x;
				if (Blocks[index] != 0) return false;
			}
	return true;
}

void NotchyGen_GrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 height) {
	Int32 baseHeight = height - 4;
	Int32 index = 0;

	// leaves bottom layer
	for (Int32 y = treeY + baseHeight; y < treeY + baseHeight + 2; y++)
		for (Int32 zz = -2; zz <= 2; zz++)
			for (Int32 xx = -2; xx <= 2; xx++)
			{
				Int32 x = xx + treeX, z = zz + treeZ;
				index = (y * Length + z) * Width + x;

				if (abs(xx) == 2 && abs(zz) == 2) {
					if (Random_Float(&rnd) >= 0.5f)
						Blocks[index] = BlockID_Leaves;
				} else {
					Blocks[index] = BlockID_Leaves;
				}
			}

	// leaves top layer
	Int32 bottomY = treeY + baseHeight + 2;
	for (Int32 y = treeY + baseHeight + 2; y < treeY + height; y++)
		for (Int32 zz = -1; zz <= 1; zz++)
			for (Int32 xx = -1; xx <= 1; xx++)
			{
				Int32 x = xx + treeX, z = zz + treeZ;
				index = (y * Length + z) * Width + x;

				if (xx == 0 || zz == 0) {
					Blocks[index] = BlockID_Leaves;
				} else if (y == bottomY && Random_Float(&rnd) >= 0.5f) {
					Blocks[index] = BlockID_Leaves;
				}
			}

	// then place trunk
	index = (treeY * Length + treeZ) * Width + treeX;
	for (Int32 y = 0; y < height - 1; y++) {
		Blocks[index] = BlockID_Log;
		index += oneY;
	}
}