#ifndef CS_FLATGRASS_GEN_h
#define CS_FLATGRASS_GEN_h
#include "MapGenerator.h"
#include "Typedefs.h"
/* Implements flatgrass map generator.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Generates the map. */
void FlatgrassGen_Generate();

/* Sets a number of horizontal slices in the map to the given block. */
static void FlatgrassGen_MapSet(Int32 yStart, Int32 yEnd, BlockID block);
#endif