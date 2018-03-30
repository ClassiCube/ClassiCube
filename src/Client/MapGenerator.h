#ifndef CC_MAP_GEN_H
#define CC_MAP_GEN_H
#include "String.h"
/* Implements flatgrass map generator, and original classic vanilla map generation.
   Based on: https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
   Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

Real32 Gen_CurrentProgress;
String Gen_CurrentState;
bool Gen_Done;
Int32 Gen_Width, Gen_Height, Gen_Length;
Int32 Gen_MaxX, Gen_MaxY, Gen_MaxZ;
Int32 Gen_Seed;
BlockID* Gen_Blocks;
#define Gen_Pack(x, y, z) (((y) * Gen_Length + (z)) * Gen_Width + (x))

void FlatgrassGen_Generate(void);
void NotchyGen_Generate(void);
#endif