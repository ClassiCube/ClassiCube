#ifndef CS_DRAWER_H
#define CS_DRAWER_H
#include "Typedefs.h"
/* Implements a simple chunk mesh builder, where each block face is a single colour. 
   (whatever lighting engine returns as light colour for given block face at given coordinates)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

static Int32 NormalBuilder_StretchXLiquid(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block);

static Int32 NormalBuilder_StretchX(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Int32 face);

static Int32 NormalBuilder_StretchZ(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Int32 face);

static bool NormalBuilder_CanStretch(BlockID initial, Int32 chunkIndex, Int32 x, Int32 y, Int32 z, Int32 face);

static Int32 NormalBuilder_LightCol(Int32 x, Int32 y, Int32 z, Int32 face, BlockID block);

static void NormalBuilder_PreStretchTiles(Int32 x1, Int32 y1, Int32 z1);

static void NormalBuilder_RenderTile(Int32 index);
#endif