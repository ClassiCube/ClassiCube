#ifndef CS_NOTCHY_GEN_H
#define CS_NOTCHY_GEN_H
#include "Compiler.h"
#include "Typedefs.h"

CLIENT_FUNC void NotchyGen_Init(Int32 width, Int32 height, Int32 length,
								Int32 seed, BlockID* blocks, Int16* heightmap);

CLIENT_FUNC void NotchyGen_CreateHeightmap();
CLIENT_FUNC void NotchyGen_CreateStrata();
Int32 NotchyGen_CreateStrataFast();
CLIENT_FUNC void NotchyGen_CreateSurfaceLayer();
CLIENT_FUNC void NotchyGen_PlantFlowers();
CLIENT_FUNC void NotchyGen_PlantMushrooms();
CLIENT_FUNC void NotchyGen_PlantTrees();
bool NotchyGen_CanGrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 treeHeight);
void NotchyGen_GrowTree(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 height);
#endif