#ifndef CS_MAP_GEN_H
#define CS_MAP_GEN_H
#include "Typedefs.h"
#include "String.h"
/* Implements flatgrass map generator, and original classic vanilla map generation.
   Based on: https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
   Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Progress of current operation. */
Real32 Gen_CurrentProgress;
/* Name of current operation being performed. */
String Gen_CurrentState;
/* Whether the generation has completed all operations. */
bool Gen_Done;
/* Dimensions of the map to be generated. */
Int32 Gen_Width, Gen_Height, Gen_Length;
/* Maxmimum coordinate of the map. */
Int32 Gen_MaxX, Gen_MaxY, Gen_MaxZ;
/* Seed used for generating the map. */
Int32 Gen_Seed;
/* Blocks of the map generated. */
BlockID* Gen_Blocks;
/* Packs a coordinate into a single integer index. */
#define Gen_Pack(x, y, z) (((y) * Gen_Length + (z)) * Gen_Width + (x))

/* Generates a flatgrass map. */
void FlatgrassGen_Generate(void);
/* Generates a notchy map. */
void NotchyGen_Generate(void);
#endif