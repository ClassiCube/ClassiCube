#ifndef CS_NORMALBUILDER_H
#define CS_NORMALBUILDER_H
#include "Builder.h"
#include "Typedefs.h"
#include "BlockEnums.h"
#include "PackedCol.h"
/* Implements a simple chunk mesh builder, where each block face is a single colour. 
   (whatever lighting engine returns as light colour for given block face at given coordinates)
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Replaces function pointers in Builder with function pointers in this file. */
void NormalBuilder_SetActive(void);

static Int32 NormalBuilder_StretchXLiquid(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block);

static Int32 NormalBuilder_StretchX(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);

static Int32 NormalBuilder_StretchZ(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);

static bool NormalBuilder_CanStretch(BlockID initial, Int32 chunkIndex, Int32 x, Int32 y, Int32 z, Face face);

static PackedCol NormalBuilder_LightCol(Int32 x, Int32 y, Int32 z, Int32 face, BlockID block);

static void NormalBuilder_RenderBlock(Int32 index);
#endif