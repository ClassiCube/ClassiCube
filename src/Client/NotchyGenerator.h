#ifndef CS_NOTCHY_GEN_H
#define CS_NOTCHY_GEN_H
#include "Compiler.h"
#include "Typedefs.h"
/* Implements original classic vanilla map generation
   Based on: https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-map-generation-algorithm
   Thanks to Jerralish for originally reverse engineering classic's algorithm, then preparing a high level overview of the algorithm.
   I believe this process adheres to clean room reverse engineering.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/


/* Initalises state for map generation. */
CLIENT_FUNC void NotchyGen_Init(Int32 width, Int32 height, Int32 length,
								Int32 seed, BlockID* blocks, Int16* heightmap);

/* Creates the initial heightmap for the generated map. */
CLIENT_FUNC void NotchyGen_CreateHeightmap();

/* Creates the base stone layer and dirt overlay layer of the map. */
CLIENT_FUNC void NotchyGen_CreateStrata();

/* Quickly fills the base stone layer on tall maps. */
Int32 NotchyGen_CreateStrataFast();

/* Details the map by replacing some surface-layer dirt with grass, sand, or gravel. */
CLIENT_FUNC void NotchyGen_CreateSurfaceLayer();

/* Plants dandelion and rose bunches/groups on the surface of the map. */
CLIENT_FUNC void NotchyGen_PlantFlowers();

/* Plants mushroom bunches/groups in caves within the map. */
CLIENT_FUNC void NotchyGen_PlantMushrooms();

/* Plants trees on the surface layer of the map. */
CLIENT_FUNC void NotchyGen_PlantTrees();

/* Returns whether a tree can grow at the specified coordinates. */
bool NotchyGen_CanGrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 treeHeight);

/* Plants a tree of the given height at the given coordinates. */
void NotchyGen_GrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 height);
#endif