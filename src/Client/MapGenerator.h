#if 0
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

/* Sets a number of horizontal slices in the map to the given block. */
static void FlatgrassGen_MapSet(Int32 yStart, Int32 yEnd, BlockID block);


/* Generates a notchy map. */
void NotchyGen_Generate(void);

/* Creates the initial heightmap for the generated map. */
static void NotchyGen_CreateHeightmap(void);

/* Creates the base stone layer and dirt overlay layer of the map. */
static void NotchyGen_CreateStrata(void);

/* Quickly fills the base stone layer on tall maps. */
static Int32 NotchyGen_CreateStrataFast(void);

/* Hollows out caves in the map. */
static void NotchyGen_CarveCaves(void);

/* Fills out ore veins in the map. */
static void NotchyGen_CarveOreVeins(Real32 abundance, const UInt8* state, BlockID block);

/* Floods water from the edges of the map. */
static void NotchyGen_FloodFillWaterBorders(void);

/* Floods water from random points in the map. */
static void NotchyGen_FloodFillWater(void);

/* Floods lava from random points in the map. */
static void NotchyGen_FloodFillLava(void);

/* Details the map by replacing some surface-layer dirt with grass, sand, or gravel. */
static void NotchyGen_CreateSurfaceLayer(void);

/* Plants dandelion and rose bunches/groups on the surface of the map. */
static void NotchyGen_PlantFlowers(void);

/* Plants mushroom bunches/groups in caves within the map. */
static void NotchyGen_PlantMushrooms(void);

/* Plants trees on the surface layer of the map. */
static void NotchyGen_PlantTrees(void);

/* Fills an oblate spheroid, but only replacing stone blocks. */
static void NotchyGen_FillOblateSpheroid(Int32 x, Int32 y, Int32 z, Real32 radius, BlockID block);

/* Floods a block, starting at the given coordinates. */
static void NotchyGen_FloodFill(Int32 startIndex, BlockID block);
#endif
#endif