#ifndef CS_NOTCHY_GEN_H
#define CS_NOTCHY_GEN_H
#include "Typedefs.h"
#include "Noise.h"
#include "Funcs.h"
#include "Block.h"

void NotchyGen_Init(Int32 width, Int32 height, Int32 length,
					Int32 seed, BlockID* blocks, Int16* heightmap);

void NotchyGen_CreateHeightmap();
void NotchyGen_CreateStrata();
Int32 NotchyGen_CreateStrataFast();
#endif