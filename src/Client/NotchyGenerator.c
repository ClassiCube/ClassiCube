/* Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3 */
/* Based on:
 https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
 Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
 I believe this process adheres to clean room reverse engineering.*/
#include "NotchyGenerator.h"
#include "Block.h"
#include "Funcs.h"
#include "Noise.h"
#include "Random.h"

/* External variables */
/* TODO: how do they even work? */
Real32 CurrentProgress;

/* Internal variables */
Int32 Width, Height, Length;
Int32 waterLevel, oneY, minHeight;
BlockID* Blocks;
Int16* Heightmap;
Random rnd;

void NotchyGen_Init(Int32 width, Int32 height, Int32 length, 
					Int32 seed, BlockID* blocks, Int16* heightmap) {
	Width = width; Height = height; Length = length;
	Blocks = blocks; Heightmap = heightmap;

	oneY = Width * Length;
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
			Real32 hLow = CombinedNoise_Compute(&n1, x * 1.3f, z * 1.3f) / 6 - 4, height = hLow;

			if (OctaveNoise_Compute(&n3, x, z) <= 0) {
				Real32 hHigh = CombinedNoise_Compute(&n2, x * 1.3f, z * 1.3f) / 5 + 6;
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
			Int32 dirtThickness = (Int32)(OctaveNoise_Compute(&n, x, z) / 24 - 4);
			Int32 dirtHeight = Heightmap[hMapIndex++];
			Int32 stoneHeight = dirtHeight + dirtThickness;

			stoneHeight = min(stoneHeight, maxY);
			dirtHeight = min(dirtHeight, maxY);

			mapIndex = minStoneY * oneY + z * Width + x;
			for (Int32 y = minStoneY; y <= stoneHeight; y++) {
				Blocks[mapIndex] = Block_Stone; mapIndex += oneY;
			}

			stoneHeight = max(stoneHeight, 0);
			mapIndex = (stoneHeight + 1) * oneY + z * Width + x;
			for (Int32 y = stoneHeight + 1; y <= dirtHeight; y++) {
				Blocks[mapIndex] = Block_Dirt; mapIndex += oneY;
			}
		}
	}
}

Int32 NotchyGen_CreateStrataFast() {
	/* Make lava layer at bottom */
	Int32 mapIndex = 0;
	for (Int32 z = 0; z < Length; z++)
		for (int x = 0; x < Width; x++)
		{
			Blocks[mapIndex++] = Block_Lava;
		}

	/* Invariant: the lowest value dirtThickness can possible be is -14 */
	Int32 stoneHeight = minHeight - 14;
	if (stoneHeight <= 0) return 1; /* no layer is fully stone */

	/* We can quickly fill in bottom solid layers */
	for (Int32 y = 1; y <= stoneHeight; y++)
		for (Int32 z = 0; z < Length; z++)
			for (Int32 x = 0; x < Width; x++)
			{
				Blocks[mapIndex++] = Block_Stone;
			}
	return stoneHeight;
}