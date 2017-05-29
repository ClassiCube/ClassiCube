#ifndef CS_CHUNKSORTER_H
#define CS_CHUNKSORTER_H
#include "Typedefs.h"
#include "ChunkInfo.h"
/* Sorts chunks so nearest chunks are ordered first, and calculates chunk visibility.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

void ChunkSorter_UpdateSortOrder(void);

static void ChunkSorter_QuickSort(Int32 left, Int32 right);
#endif