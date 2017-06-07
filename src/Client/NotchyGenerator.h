#ifndef CS_NOTCHY_GEN_H
#define CS_NOTCHY_GEN_H
#include "MapGenerator.h"
#include "Typedefs.h"
#include "String.h"
/* Implements original classic vanilla map generation
   Based on: https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
   Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
   I believe this process adheres to clean room reverse engineering.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Generates the map. */
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