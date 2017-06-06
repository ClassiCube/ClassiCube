#ifndef CS_MAP_GEN_H
#define CS_MAP_GEN_H
#include "Typedefs.h"
#include "String.h"
/* Common state for map generators.
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

#endif