#ifndef CS_BUILDER_H
#define CS_BUILDER_H
#include "Typedefs.h"
#include "BlockEnums.h"
#include "PackedCol.h"
#include "ChunkInfo.h"
#include "BlockID.h"
/* Converts a 16x16x16 chunk into a mesh of vertices.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Current world coordinates being processed. */
Int32 Builder_X, Builder_Y, Builder_Z;

/* Current block being processed. */
BlockID Builder_Block;

/* Current chunk index being processed. */
Int32 Builder_ChunkIndex;


/* Pointer to current chunk on stack.*/
BlockID* Builder_Chunk;

/* Pointer to current counts on stack. */
UInt8* Builder_Counts;

/* Pointer to current bitflags on stack. */
Int32* Builder_BitFlags;

/* Whether BitFlags should actually be assigned and cleared. Default false. */
bool Builder_UseBitFlags;


/* Caches map edge and sides height. */
Int32 Builder_SidesLevel, Builder_EdgeLevel;

/* End coordinates of the chunk, as map may not be divisible by CHUNK_SIZE. */
Int32 Builder_ChunkEndX, Builder_ChunkEndZ;


/* Offset chunk indices for each face. */
Int32 Builder_Offsets[6];

/* Initalises state of this mesh builder. */
void Builder_Init();

/* Called when a new map is loaded. */
void Builder_OnNewMapLoaded();

/* Builds a mesh for the given chunk. */
void Builder_MakeChunk(ChunkInfo* info);


static bool BuildChunk(Int32 x1, Int32 y1, Int32 z1, bool* allAir);

static bool ReadChunkData(Int32 x1, Int32 y1, Int32 z1, bool* outAllAir);

static void Builder_SetPartInfo(DrawInfo part, Int32 i, ChunkPartInfo** parts);

static void Builder_Stretch(Int32 x1, Int32 y1, Int32 z1);


Int32 (*Builder_StretchXLiquid)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block);

Int32 (*Builder_StretchX)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);

Int32 (*Builder_StretchZ)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);

/* Returns whether a liquid block is occluded at the given index in the chunk. */
bool Builder_OccludedLiquid(Int32 chunkIndex);
#endif