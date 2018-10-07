#ifndef CC_CHUNKUPDATER_H
#define CC_CHUNKUPDATER_H
#include "Core.h"
#include "Constants.h"
/* Manages the process of building/deleting chunk meshes.
   Also sorts chunks so nearest chunks are ordered first, and calculates chunk visibility.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Describes a portion of the data needed for rendering a chunk. */
struct ChunkPartInfo {
#ifdef CC_BUILD_GL11
	GfxResourceID Vb;
#endif
	int Offset;      /* -1 if no vertices at all */
	int SpriteCount; /* Sprite vertices count */
	uint16_t Counts[FACE_COUNT]; /* Counts per face */
};

/* Describes data necessary for rendering a chunk. */
struct ChunkInfo {	
	uint16_t CentreX, CentreY, CentreZ; /* Centre coordinates of the chunk */

	uint8_t Visible : 1;       /* Whether chunk is visibile to the player */
	uint8_t Empty : 1;         /* Whether the chunk is empty of data */
	uint8_t PendingDelete : 1; /* Whether chunk is pending deletion*/
	uint8_t AllAir : 1;        /* Whether chunk is completely air */
	uint8_t : 0;               /* pad to next byte*/

	uint8_t DrawXMin : 1;
	uint8_t DrawXMax : 1;
	uint8_t DrawZMin : 1;
	uint8_t DrawZMax : 1;
	uint8_t DrawYMin : 1;
	uint8_t DrawYMax : 1;
	uint8_t : 0;          /* pad to next byte */
#ifdef OCCLUSION
	public bool Visited = false, Occluded = false;
	public byte OcclusionFlags, OccludedFlags, DistanceFlags;
#endif
#ifndef CC_BUILD_GL11
	GfxResourceID Vb;
#endif
	struct ChunkPartInfo* NormalParts;
	struct ChunkPartInfo* TranslucentParts;
};

void ChunkInfo_Reset(struct ChunkInfo* chunk, int x, int y, int z);

void ChunkUpdater_Init(void);
void ChunkUpdater_Free(void);
void ChunkUpdater_Refresh(void);
void ChunkUpdater_RefreshBorders(int clipLevel);
void ChunkUpdater_ApplyMeshBuilder(void);
void ChunkUpdater_Update(double deltaTime);

void ChunkUpdater_ResetPartFlags(void);
void ChunkUpdater_ResetPartCounts(void);
void ChunkUpdater_CreateChunkCache(void);
void ChunkUpdater_ResetChunkCache(void);
void ChunkUpdater_ClearChunkCache(void);

void ChunkUpdater_DeleteChunk(struct ChunkInfo* info);
void ChunkUpdater_BuildChunk(struct ChunkInfo* info, int* chunkUpdates);
#endif
