#ifndef CS_BUILDER_H
#define CS_BUILDER_H
#include "Typedefs.h"
#include "BlockEnums.h"
#include "PackedCol.h"
#include "ChunkInfo.h"
#include "BlockID.h"
#include "Builder1DPart.h"
#include "TerrainAtlas1D.h"
/* Converts a 16x16x16 chunk into a mesh of vertices.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


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

/* Completely white colour. */
PackedCol Builder_WhiteCol;


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
The first Atlas1D_MaxAtlasesCount parts are for normal parts, remainder are for translucent parts. */
Builder1DPart Builder_Parts[Atlas1D_MaxAtlasesCount * 2];


/* Initalises state of this mesh builder. */
void Builder_Init(void);

/* Sets function pointers and variables to default. */
void Builder_SetDefault(void);

/* Called when a new map is loaded. */
void Builder_OnNewMapLoaded(void);

/* Builds a mesh for the given chunk. */
void Builder_MakeChunk(ChunkInfo* info);


static void Builder_AddSpriteVertices(BlockID block);

static void Builder_AddVertices(BlockID block, Face face);

/* Returns whether a liquid block is occluded at the given index in the chunk. */
bool Builder_OccludedLiquid(Int32 chunkIndex);


/* Calculates how many blocks the current block face mesh can be stretched on X axis. */
Int32(*Builder_StretchXLiquid)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block);

/* Calculates how many blocks the current block face mesh can be stretched on X axis. */
Int32(*Builder_StretchX)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);

/* Calculates how many blocks the current block face mesh can be stretched on Z axis. */
Int32(*Builder_StretchZ)(Int32 countIndex, Int32 x, Int32 y, Int32 z, Int32 chunkIndex, BlockID block, Face face);

/* Renders the current block. */
void(*Builder_RenderBlock)(Int32 countsIndex);

/* Called just before Stretch(). */
void(*Builder_PreStretchTiles)(Int32 x1, Int32 y1, Int32 z1);

/* Called just after Stretch(). */
void(*Builder_PostStretchTiles)(Int32 x1, Int32 y1, Int32 z1);


void Builder_DefaultPreStretchTiles(Int32 x1, Int32 y1, Int32 z1);

void Builder_DefaultPostStretchTiles(Int32 x1, Int32 y1, Int32 z1);


/* Renders a sprite block. */
void Builder_DrawSprite(Int32 count);


static bool Builder_BuildChunk(Int32 x1, Int32 y1, Int32 z1, bool* allAir);

static bool Builder_ReadChunkData(Int32 x1, Int32 y1, Int32 z1, bool* outAllAir);

static void Builder_SetPartInfo(Builder1DPart* part, Int32 i, Int32 partsIndex, bool* hasParts);

static void Builder_Stretch(Int32 x1, Int32 y1, Int32 z1);
#endif