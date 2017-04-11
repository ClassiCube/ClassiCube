#ifndef CS_NOTCHY_GEN_H
#define CS_NOTCHY_GEN_H
#include "Compiler.h"
#include "Typedefs.h"

CLIENT_API void NotchyGen_Init(Int32 width, Int32 height, Int32 length,
								Int32 seed, BlockID* blocks, Int16* heightmap);

CLIENT_API void NotchyGen_CreateHeightmap();
CLIENT_API void NotchyGen_CreateStrata();
Int32 NotchyGen_CreateStrataFast();
#endif