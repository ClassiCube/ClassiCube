#ifndef CC_BUILDER_H
#define CC_BUILDER_H
#include "Typedefs.h"
#include "Block.h"
#include "PackedCol.h"
#include "ChunkUpdater.h"
#include "BlockID.h"
#include "TerrainAtlas.h"
#include "VertexStructs.h"
/* Converts a 16x16x16 chunk into a mesh of vertices.
NormalMeshBuilder:
   Implements a simple chunk mesh builder, where each block face is a single colour.
   (whatever lighting engine returns as light colour for given block face at given coordinates)

Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Contains state for vertices for a portion of a chunk mesh (vertices that are in a 1D atlas) */
typedef struct Builder1DPart_ {
	/* Pointers to offset within vertices, indexed by face. */
	VertexP3fT2fC4b* fVertices[Face_Count];
	/* Number of indices, indexed by face. */
	Int32 fCount[Face_Count];
	/* Number of indices, for sprites. */
	Int32 sCount;
	/* Current offset within vertices for sprites, delta between each sprite face. */
	Int32 sOffset, sAdvance;
	/* Pointer to vertex data. */
	VertexP3fT2fC4b* vertices;
	/* Number of elements in the vertices pointer. */
	Int32 verticesBufferCount;
} Builder1DPart;

/* Prepares the given part for building vertices. */
void Builder1DPart_Prepare(Builder1DPart* part);
/* Resets counts to zero for the given part.*/
void Builder1DPart_Reset(Builder1DPart* part);
/* Counts the total number of vertices in the given part. */
Int32 Builder1DPart_VerticesCount(Builder1DPart* part);

/* Current world coordinates being processed. */
Int32 Builder_X, Builder_Y, Builder_Z;
/* Current block being processed. */
BlockID Builder_Block;
/* Current chunk index being processed. */
Int32 Builder_ChunkIndex;
/* Whether current block being processed is full bright. */
bool Builder_FullBright;
/* Whether current block being processed is tinted. */
bool Builder_Tinted;

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
/* Part builder data, for both normal and translucent parts.
The first ATLAS1D_MAX_ATLASES_COUNT parts are for normal parts, remainder are for translucent parts. */
Builder1DPart Builder_Parts[ATLAS1D_MAX_ATLASES_COUNT * 2];

/* Initalises state of this mesh builder. */
void Builder_Init(void);
/* Sets function pointers and variables to default. */
void Builder_SetDefault(void);
/* Called when a new map is loaded. */
void Builder_OnNewMapLoaded(void);
/* Builds a mesh for the given chunk. */
void Builder_MakeChunk(ChunkInfo* info);

/* Returns whether a liquid block is occluded at the given index in the chunk. */
bool Builder_OccludedLiquid(Int32 chunkIndex);
/* Calculates how many blocks the current block face mesh can be stretched on X axis. */
Int32 (*Builder_StretchXLiquid)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block);
/* Calculates how many blocks the current block face mesh can be stretched on X axis. */
Int32 (*Builder_StretchX)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);
/* Calculates how many blocks the current block face mesh can be stretched on Z axis. */
Int32 (*Builder_StretchZ)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);
/* Renders the current block. */
void (*Builder_RenderBlock)(Int32 countsIndex);
/* Called just before Stretch(). */
void (*Builder_PreStretchTiles)(Int32 x1, Int32 y1, Int32 z1);
/* Called just after Stretch(). */
void (*Builder_PostStretchTiles)(Int32 x1, Int32 y1, Int32 z1);
void Builder_DefaultPreStretchTiles(Int32 x1, Int32 y1, Int32 z1);
void Builder_DefaultPostStretchTiles(Int32 x1, Int32 y1, Int32 z1);
/* Renders a sprite block. */
void Builder_DrawSprite(Int32 count);

/* Replaces function pointers in Builder with function pointers for normal mesh builder. */
void NormalBuilder_SetActive(void);
#endif