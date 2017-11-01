#ifndef CC_CHUNKUPDATER_H
#define CC_CHUNKUPDATER_H
#include "Typedefs.h"
#include "Vectors.h"
#include "Events.h"
/* Manages the process of building/deleting chunk meshes.
   Also sorts chunks so nearest chunks are ordered first, and calculates chunk visibility.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Describes a portion of the data needed for rendering a chunk. */
typedef struct ChunkPartInfo_ {
	GfxResourceID VbId;
	UInt16 HasVertices;     /* Does this chunk have any vertices at all? */
	UInt16 SpriteCountDiv4; /* Sprite vertices count, divided by 4 */
	UInt16 XMinCount, XMaxCount, ZMinCount, ZMaxCount, YMinCount, YMaxCount; /* Counts per face */
} ChunkPartInfo;

/* Describes data necessary for rendering a chunk. */
typedef struct ChunkInfo_ {	
	UInt16 CentreX, CentreY, CentreZ; /* Centre coordinates of the chunk */

	UInt8 Visible : 1;       /* Whether chunk is visibile to the player */
	UInt8 Empty : 1;         /* Whether the chunk is empty of data */
	UInt8 PendingDelete : 1; /* Whether chunk is pending deletion*/	
	UInt8 AllAir : 1;        /* Whether chunk is completely air */
	UInt8 : 0;               /* pad to next byte*/

	UInt8 DrawXMin : 1;
	UInt8 DrawXMax : 1;
	UInt8 DrawZMin : 1;
	UInt8 DrawZMax : 1;
	UInt8 DrawYMin : 1;
	UInt8 DrawYMax : 1;
	UInt8 : 0;          /* pad to next byte */
#if OCCLUSION
	public bool Visited = false, Occluded = false;
	public byte OcclusionFlags, OccludedFlags, DistanceFlags;
#endif
	ChunkPartInfo* NormalParts;
	ChunkPartInfo* TranslucentParts;
} ChunkInfo;

/* Resets contents of given chunk render info structure. */
void ChunkInfo_Reset(ChunkInfo* chunk, Int32 x, Int32 y, Int32 z);

/* Centre coordinates of chunk the camera is located in.*/
Vector3I ChunkUpdater_ChunkPos;
/* Distance of chunks from the camera. */
Int32* ChunkUpdater_Distances; /* TODO: Use UInt32s instead of Int32s? */

void ChunkUpdater_Init(void);
void ChunkUpdater_Free(void);
void ChunkUpdater_Refresh(void);
void ChunkUpdater_RefreshBorders(Int32 clipLevel);
void ChunkUpdater_ApplyMeshBuilder(void);
void ChunkUpdater_UpdateChunks(Real64 delta);

void ChunkUpdater_ResetPartFlags(void);
void ChunkUpdater_ResetPartCounts(void);
void ChunkUpdater_CreateChunkCache(void);
void ChunkUpdater_ResetChunkCache(void);
void ChunkUpdater_ClearChunkCache(void);

void ChunkUpdater_DeleteChunk(ChunkInfo* info);
void ChunkUpdater_BuildChunk(ChunkInfo* info, Int32* chunkUpdates);
void ChunkUpdater_UpdateSortOrder(void);
#endif