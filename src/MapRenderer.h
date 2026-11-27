#ifndef CC_MAPRENDERER_H
#define CC_MAPRENDERER_H
#include "Core.h"
#include "Constants.h"
CC_BEGIN_HEADER

/* Renders the blocks of the world by subdividing it into chunks.
   Also manages the process of building/deleting chunk meshes.
   Also sorts chunks so nearest chunks are rendered first, and calculates chunk visibility.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent MapRenderer_Component;

/* Max used 1D atlases. (i.e. Atlas1D_Index(maxTextureLoc) + 1) */
extern int MapRenderer_1DUsedCount;

/* Buffer for all chunk parts. There are (MapRenderer_ChunksCount * Atlas1D_Count) parts in the buffer,
with parts for 'normal' buffer being in lower half. */
extern struct ChunkPartInfo* MapRenderer_PartsNormal; /* TODO: THAT DESC SUCKS */
extern struct ChunkPartInfo* MapRenderer_PartsTranslucent;

/* Describes a portion of the data needed for rendering a chunk. */
struct ChunkPartInfo {
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
	/* 1 VB per face, another VB for sprites */
	#define CHUNKPART_MAX_VBS (FACE_COUNT + 1)
	GfxResourceID vbs[CHUNKPART_MAX_VBS];
#endif
	int offset;      /* -1 if no vertices at all */
	int spriteCount; /* Sprite vertices count */
	cc_uint16 counts[FACE_COUNT]; /* Counts per face */
};

#define CHUNK_FLAG_VISIBLE    (1 << 0) /* Whether chunk is visible to the player */
#define CHUNK_FLAG_EMPTY      (1 << 1) /* Whether chunk is empty of data and is known to have no data */
#define CHUNK_FLAG_DIRTY      (1 << 2) /* Whether chunk is pending being rebuilt */
#define CHUNK_FLAG_ALL_AIR    (1 << 3) /* Whether chunk is completely air */
#define CHUNK_FLAG_SKIP_CLIP  (1 << 4) /* Whether chunk can skip GPU backend clipping (see CC_CLIPPING_FLAGS) */
#define CHUNK_FLAG_NORM_PARTS (1 << 5) /* Whether chunk has any normal parts */
#define CHUNK_FLAG_TRNS_PARTS (1 << 6) /* Whether chunk has any translucent parts */

#define CHUNK_FLAGS_ANY_PARTS (CHUNK_FLAG_NORM_PARTS | CHUNK_FLAG_TRNS_PARTS)

/* Describes data necessary for rendering a chunk. */
struct ChunkInfo {
	cc_uint16 centreX, centreY, centreZ; /* Centre coordinates of the chunk */
	cc_uint8 flags;     /* General flags about the chunk */
	cc_uint8 drawFlags; /* Which faces of chunk's blocks are front facing to the camera */

	int index; /* Index of the chunk in the world */
	cc_uint32 distSqr; /* Squared distance of chunk from the camera */

#ifdef OCCLUSION
	public cc_bool Visited = false, Occluded = false;
	public byte OcclusionFlags, OccludedFlags, DistanceFlags;
#endif
#if CC_GFX_BACKEND != CC_GFX_BACKEND_GL11
	GfxResourceID vb;
#endif
};

/* Renders the meshes of non-translucent blocks in visible chunks. */
void MapRenderer_RenderNormal(float delta);
/* Renders the meshes of translucent blocks in visible chunks. */
void MapRenderer_RenderTranslucent(float delta);
/* Potentially updates sort order of rendered chunks. */
/* Potentially builds meshes for several nearby chunks. */
/* NOTE: This should be called once per frame. */
void MapRenderer_Update(float delta);

/* Marks the given chunk as needing to be rebuilt/redrawn. */
/* NOTE: Coordinates outside the map are simply ignored. */
void MapRenderer_RefreshChunk(int cx, int cy, int cz);
/* Called when a block is changed, to update internal state. */
void MapRenderer_OnBlockChanged(int x, int y, int z, BlockID block);
/* Deletes all chunks and resets internal state. */
void MapRenderer_Refresh(void);

CC_END_HEADER
#endif
