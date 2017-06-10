#include "ChunkUpdater.h"
#include "MiscEvents.h"
#include "WorldEvents.h"
#include "GraphicsAPI.h"
#include "World.h"
#include "TerrainAtlas1D.h"
#include "MapRenderer.h"
#include "Constants.h"
#include "ExtMath.h"

void ChunkUpdater_Init(void) {
	EventHandler_RegisterVoid(TextureEvents_AtlasChanged, ChunkUpdater_TerrainAtlasChanged);
	EventHandler_RegisterVoid(WorldEvents_NewMap, ChunkUpdater_OnNewMap);
	EventHandler_RegisterVoid(WorldEvents_MapLoaded, ChunkUpdater_OnNewMapLoaded);
	EventHandler_RegisterVoid(WorldEvents_EnvVarChanged, ChunkUpdater_EnvVariableChanged);

	EventHandler_RegisterVoid(BlockEvents_BlockDefChanged, ChunkUpdater_BlockDefinitionChanged);
	EventHandler_RegisterVoid(GfxEvents_ViewDistanceChanged, ChunkUpdater_ViewDistanceChanged);
	EventHandler_RegisterVoid(GfxEvents_ProjectionChanged, ChunkUpdater_ProjectionChanged);
	EventHandler_RegisterVoid(Gfx_ContextLost, ChunkUpdater_ClearChunkCache);
	EventHandler_RegisterVoid(Gfx_ContextRecreated, ChunkUpdater_Refresh);

	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
	ChunkUpdater_ApplyMeshBuilder();
}

void ChunkUpdater_Free(void) {
	EventHandler_UnregisterVoid(TextureEvents_AtlasChanged, ChunkUpdater_TerrainAtlasChanged);
	EventHandler_UnregisterVoid(WorldEvents_NewMap, ChunkUpdater_OnNewMap);
	EventHandler_UnregisterVoid(WorldEvents_MapLoaded, ChunkUpdater_OnNewMapLoaded);
	EventHandler_UnregisterVoid(WorldEvents_EnvVarChanged, ChunkUpdater_EnvVariableChanged);

	EventHandler_UnregisterVoid(BlockEvents_BlockDefChanged, ChunkUpdater_BlockDefinitionChanged);
	EventHandler_UnregisterVoid(GfxEvents_ViewDistanceChanged, ChunkUpdater_ViewDistanceChanged);
	EventHandler_UnregisterVoid(GfxEvents_ProjectionChanged, ChunkUpdater_ProjectionChanged);
	EventHandler_UnregisterVoid(Gfx_ContextLost, ChunkUpdater_ClearChunkCache);
	EventHandler_UnregisterVoid(Gfx_ContextRecreated, ChunkUpdater_Refresh);
}

void ChunkUpdater_Refresh(void) {
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
	if (MapRenderer_Chunks != NULL && World_Blocks != NULL) {
		MapRenderer_ClearChunkCache();
		MapRenderer_ResetChunkCache();
	}
	ChunkUpdater_ResetPartCounts();
}

void ChunkUpdater_RefreshBorders(Int32 clipLevel) {
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
	if (MapRenderer_Chunks == NULL || World_Blocks == NULL) return;

	Int32 x, y, z, index = 0;
	for (z = 0; z < MapRenderer_ChunksZ; z++) {
		for (y = 0; y < MapRenderer_ChunksY; y++) {
			for (x = 0; x < MapRenderer_ChunksX; x++) {
				bool isBorder = x == 0 || z == 0 || x == (MapRenderer_ChunksX - 1) || z == (MapRenderer_ChunksZ - 1);
				if (isBorder && (y * CHUNK_SIZE) < clipLevel) {
					ChunkUpdater_DeleteChunk(&MapRenderer_Chunks[index]);
				}
				index++;
			}
		}
	}
}


void ChunkUpdater_ApplyMeshBuilder(void);


static void ChunkUpdater_EnvVariableChanged(Int32 envVar);

static void ChunkUpdater_TerrainAtlasChanged(void);

static void ChunkUpdater_BlockDefinitionChanged(void);

static void ChunkUpdater_ProjectionChanged(void);

static void ChunkUpdater_ViewDistanceChanged(void);

static void ChunkUpdater_OnNewMap(void);

static void ChunkUpdater_OnNewMapLoaded(void);


void ChunkUpdater_UpdateChunks(Real64 delta);

static Int32 ChunkUpdater_UpdateChunksAndVisibility(Int32* chunkUpdates);

static Int32 ChunkUpdater_UpdateChunksStill(Int32* chunkUpdates);


void ChunkUpdater_ResetPartFlags(void) {
	Int32 i;
	for (i = 0; i < Atlas1D_MaxAtlasesCount; i++) {
		MapRenderer_CheckingNormalParts[i] = true;
		MapRenderer_HasNormalParts[i] = false;
		MapRenderer_CheckingTranslucentParts[i] = true;
		MapRenderer_HasTranslucentParts[i] = false;
	}
}

void ChunkUpdater_ResetPartCounts(void) {
	Int32 i;
	for (i = 0; i < Atlas1D_MaxAtlasesCount; i++) {
		MapRenderer_NormalPartsCount[i] = 0;
		MapRenderer_TranslucentPartsCount[i] = 0;
	}
}

void ChunkUpdater_CreateChunkCache(void) {
	Int32 x, y, z, index = 0;
	for (z = 0; z < World_Length; z += CHUNK_SIZE) {
		for (y = 0; y < World_Height; y += CHUNK_SIZE) {
			for (x = 0; x < World_Width; x += CHUNK_SIZE) {
				ChunkInfo_Reset(&MapRenderer_Chunks[index], x, y, z);
				MapRenderer_SortedChunks[index] = &MapRenderer_Chunks[index];
				MapRenderer_RenderChunks[index] = &MapRenderer_Chunks[index];
				distances[index] = 0;
				index++;
			}
		}
	}
}

void ChunkUpdater_ResetChunkCache(void) {
	Int32 x, y, z, index = 0;
	for (z = 0; z < World_Length; z += CHUNK_SIZE) {
		for (y = 0; y < World_Height; y += CHUNK_SIZE) {
			for (x = 0; x < World_Width; x += CHUNK_SIZE) {
				ChunkInfo_Reset(&MapRenderer_Chunks[index], x, y, z);
				index++;
			}
		}
	}
}

void ChunkUpdater_ClearChunkCache(void) {
	if (MapRenderer_Chunks == NULL) return;
	Int32 i;

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkUpdater_DeleteChunk(&MapRenderer_Chunks[i]);
	}
	ChunkUpdater_ResetPartCounts();
}


void ChunkUpdater_DeleteChunk(ChunkInfo* info);

void ChunkUpdater_BuildChunk(ChunkInfo* info, Int32* chunkUpdates);

static Int32 AdjustViewDist(Real32 dist) {
	if (dist < CHUNK_SIZE) dist = CHUNK_SIZE;
	Int32 viewDist = Math_AdjViewDist(dist);
	return (viewDist + 24) * (viewDist + 24);
}