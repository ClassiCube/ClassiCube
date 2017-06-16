#ifndef CS_CHUNKUPDATER_H
#define CS_CHUNKUPDATER_H
#include "Typedefs.h"
#include "ChunkInfo.h"
#include "Vector3I.h"
#include "WorldEvents.h"
/* Manages the process of building/deleting chunk meshes, in addition to calculating the visibility of chunks
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Centre coordinates of chunk the camera is located in.*/
Vector3I ChunkUpdater_ChunkPos;

/* Distance of chunks from the camera. */
Int32* ChunkUpdater_Distances; /* TODO: Use UInt32s instead of Int32s? */


void ChunkUpdater_Init(void);

void ChunkUpdater_Free(void);

void ChunkUpdater_Refresh(void);

void ChunkUpdater_RefreshBorders(Int32 clipLevel);

void ChunkUpdater_ApplyMeshBuilder(void);


static void ChunkUpdater_EnvVariableChanged(EnvVar envVar);

static void ChunkUpdater_TerrainAtlasChanged(void);

static void ChunkUpdater_BlockDefinitionChanged(void);

static void ChunkUpdater_ProjectionChanged(void);

static void ChunkUpdater_ViewDistanceChanged(void);


static void ChunkUpdater_OnNewMap(void);

static void ChunkUpdater_OnNewMapLoaded(void);

static void ChunkUpdater_FreeAllocations(void);

static void ChunkUpdater_PerformAllocations(void);


void ChunkUpdater_UpdateChunks(Real64 delta);

static Int32 ChunkUpdater_UpdateChunksAndVisibility(Int32* chunkUpdates);

static Int32 ChunkUpdater_UpdateChunksStill(Int32* chunkUpdates);


void ChunkUpdater_ResetPartFlags(void);

void ChunkUpdater_ResetPartCounts(void);

void ChunkUpdater_CreateChunkCache(void);

void ChunkUpdater_ResetChunkCache(void);

void ChunkUpdater_ClearChunkCache(void);


void ChunkUpdater_DeleteChunk(ChunkInfo* info);

void ChunkUpdater_BuildChunk(ChunkInfo* info, Int32* chunkUpdates);

static Int32 ChunkUpdater_AdjustViewDist(Real32 dist);
#endif