#ifndef CC_MAPRENDERER_H
#define CC_MAPRENDERER_H
#include "Core.h"
#include "Constants.h"
/* Renders the blocks of the world by subdividing it into chunks.
   Also manages the process of building/deleting chunk meshes.
   Also sorts chunks so nearest chunks are rendered first, and calculates chunk visibility.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent MapRenderer_Component;

extern int MapRenderer_ChunksX, MapRenderer_ChunksY, MapRenderer_ChunksZ;
#define MapRenderer_Pack(cx, cy, cz) (((cz) * MapRenderer_ChunksY + (cy)) * MapRenderer_ChunksX + (cx))
/* TODO: Swap Y and Z? Make sure to update ChunkUpdater's ResetChunkCache and ClearChunkCache methods! */

/* Count of actual used 1D atlases. (i.e. 1DIndex(maxTextureLoc) + 1 */
extern int MapRenderer_1DUsedCount;
/* Number of chunks in the world, or ChunksX * ChunksY * ChunksZ */
extern int MapRenderer_ChunksCount;

/* Buffer for all chunk parts. There are (MapRenderer_ChunksCount * Atlas1D_Count) parts in the buffer,
with parts for 'normal' buffer being in lower half. */
extern struct ChunkPartInfo* MapRenderer_PartsNormal; /* TODO: THAT DESC SUCKS */
extern struct ChunkPartInfo* MapRenderer_PartsTranslucent;

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
/* Gets the chunk at the given chunk coordinates in the world. */
/* NOTE: Does NOT check coordinates are within bounds. */
struct ChunkInfo* MapRenderer_GetChunk(int cx, int cy, int cz);
/* Renders the meshes of non-translucent blocks in visible chunks. */
void MapRenderer_RenderNormal(double delta);
/* Renders the meshes of translucent blocks in visible chunks. */
void MapRenderer_RenderTranslucent(double delta);
/* Potentially updates sort order of rendered chunks. */
/* Potentially builds meshes for several nearby chunks. */
/* NOTE: This should be called once per frame. */
void MapRenderer_Update(double deltaTime);

/* Marks the given chunk as needing to be rebuilt/redrawn. */
/* NOTE: Coordinates outside the map are simply ignored. */
void MapRenderer_RefreshChunk(int cx, int cy, int cz);
/* Deletes the vertex buffer associated with the given chunk. */
/* NOTE: This method also adjusts internal state, so do not bypass this. */
void MapRenderer_DeleteChunk(struct ChunkInfo* info);
/* Builds the mesh (and hence vertex buffer) for the given chunk. */
/* NOTE: This method also adjusts internal state, so do not bypass this. */
void MapRenderer_BuildChunk(struct ChunkInfo* info, int* chunkUpdates);

/* Refreshes chunks on the border of the map. */
/* NOTE: Only refreshes chunks whose y is less than 'maxHeight'. */
void MapRenderer_RefreshBorders(int maxHeight);
/* Deletes all chunks and resets internal state. */
void MapRenderer_Refresh(void);
void MapRenderer_ApplyMeshBuilder(void);
#endif
