// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
// Based on:
// https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
// Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
// I believe this process adheres to clean room reverse engineering.
#include "NotchyGenerator.h"
#include "Random.h"

// External variables
Real32 CurrentProgress;

// Internal variables
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