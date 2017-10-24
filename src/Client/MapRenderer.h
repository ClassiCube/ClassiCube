#ifndef CC_MAPRENDERER_H
#define CC_MAPRENDERER_H
#include "Typedefs.h"
#include "TerrainAtlas.h"
#include "ChunkUpdater.h"
/* Renders the blocks of the world by subdividing it into chunks.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Width of the world, in terms of number of chunks. */
Int32 MapRenderer_ChunksX;
/* Height of the world, in terms of number of chunks. */
Int32 MapRenderer_ChunksY;
/* Length of the world, in terms of number of chunks. */
Int32 MapRenderer_ChunksZ;
/* Packs a coordinate into a single integer index. */
#define MapRenderer_Pack(cx, cy, cz) (((cz) * MapRenderer_ChunksY + (cy)) * MapRenderer_ChunksX + (cx))
/* TODO: Swap Y and Z? Make sure to update ChunkUpdater's ResetChunkCache and ClearChunkCache methods! */

/* The count of actual used 1D atlases. (i.e. 1DIndex(maxTextureLoc) + 1*/
Int32 MapRenderer_1DUsedCount;
/* The number of non-empty Normal ChunkPartInfos (across entire world) for each 1D atlas batch.
1D atlas batches that do not have any ChunkPartInfos can be entirely skipped. */
Int32 MapRenderer_NormalPartsCount[ATLAS1D_MAX_ATLASES_COUNT];
/* The number of non-empty Translucent ChunkPartInfos (across entire world) for each 1D atlas batch.
1D atlas batches that do not have any ChunkPartInfos can be entirely skipped. */
Int32 MapRenderer_TranslucentPartsCount[ATLAS1D_MAX_ATLASES_COUNT];
/* Whether there are any visible Translucent ChunkPartInfos for each 1D atlas batch.
1D atlas batches that do not have any visible translucent ChunkPartInfos can be skipped. */
bool MapRenderer_HasTranslucentParts[ATLAS1D_MAX_ATLASES_COUNT];
/* Whether there are any visible Normal ChunkPartInfos for each 1D atlas batch.
1D atlas batches that do not have any visible normal ChunkPartInfos can be skipped. */
bool MapRenderer_HasNormalParts[ATLAS1D_MAX_ATLASES_COUNT];
/* Whether renderer should check if there are any visible Translucent ChunkPartInfos for each 1D atlas batch. */
bool MapRenderer_CheckingTranslucentParts[ATLAS1D_MAX_ATLASES_COUNT];
/* Whether renderer should check if there are any visible Normal ChunkPartInfos for each 1D atlas batch. */
bool MapRenderer_CheckingNormalParts[ATLAS1D_MAX_ATLASES_COUNT];

/* Render info for all chunks in the world. Unsorted.*/
ChunkInfo* MapRenderer_Chunks;
/* The number of chunks in the world, or ChunksX * ChunksY * ChunksZ */
Int32 MapRenderer_ChunksCount;
/* Pointers to render info for all chunks in the world, sorted by distance from the camera. */
ChunkInfo** MapRenderer_SortedChunks;
/* Pointers to render info for all chunks in the world, sorted by distance from the camera.
Chunks that can be rendered (not empty and are visible) are included in this array. */
ChunkInfo** MapRenderer_RenderChunks;
/* The number of actually used pointers in the RenderChunks array.
Entries past this count should be ignored and skipped. */
Int32 MapRenderer_RenderChunksCount;
/* Buffer for all chunk parts. 
There are MapRenderer_ChunksCount * Atlas1D_Count * 2 parts in the buffer. */
ChunkPartInfo* MapRenderer_PartsBuffer;
/* Offset of translucent chunk parts in MapRenderer_PartsBuffer. */
Int32 MapRenderer_TranslucentBufferOffset;

/* Retrieves the render info for the given chunk. */
ChunkInfo* MapRenderer_GetChunk(Int32 cx, Int32 cy, Int32 cz);
/* Marks the given chunk as needing to be deleted. */
void MapRenderer_RefreshChunk(Int32 cx, Int32 cy, Int32 cz);
/* Potentially generates meshes for several pending chunks. */
void MapRenderer_Update(Real64 deltaTime);
/* Renders all opaque and transparent blocks.
Pixels are either treated as fully replacing existing pixel, or skipped. */
void MapRenderer_RenderNormal(Real64 deltaTime);
/*Renders all translucent (e.g. water) blocks.
Pixels drawn blend into existing geometry.*/
void MapRenderer_RenderTranslucent(Real64 deltaTime);
#endif