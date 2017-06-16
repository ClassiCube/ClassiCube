#include "ChunkUpdater.h"
#include "MiscEvents.h"
#include "WorldEvents.h"
#include "GraphicsAPI.h"
#include "World.h"
#include "TerrainAtlas1D.h"
#include "MapRenderer.h"
#include "Constants.h"
#include "ExtMath.h"
#include "Builder.h"
#include "Funcs.h"
#include "WorldEnv.h"
#include "GameProps.h"
#include "Platform.h"
#include "NormalBuilder.h"
#include "LocalPlayer.h"
#include "FrustumCulling.h"

Int32 cu_chunksTarget = 12;
#define cu_targetTime ((1.0 / 30) + 0.01)
Vector3 cu_lastCamPos;
Real32 cu_lastHeadY, cu_lastHeadX;
Int32 cu_atlas1DCount;

void ChunkUpdater_Init(void) {
	EventHandler_RegisterVoid(TextureEvents_AtlasChanged, ChunkUpdater_TerrainAtlasChanged);
	EventHandler_RegisterVoid(WorldEvents_NewMap, ChunkUpdater_OnNewMap);
	EventHandler_RegisterVoid(WorldEvents_MapLoaded, ChunkUpdater_OnNewMapLoaded);
	EventHandler_RegisterInt32(WorldEvents_EnvVarChanged, ChunkUpdater_EnvVariableChanged);

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
	EventHandler_UnregisterInt32(WorldEvents_EnvVarChanged, ChunkUpdater_EnvVariableChanged);

	EventHandler_UnregisterVoid(BlockEvents_BlockDefChanged, ChunkUpdater_BlockDefinitionChanged);
	EventHandler_UnregisterVoid(GfxEvents_ViewDistanceChanged, ChunkUpdater_ViewDistanceChanged);
	EventHandler_UnregisterVoid(GfxEvents_ProjectionChanged, ChunkUpdater_ProjectionChanged);
	EventHandler_UnregisterVoid(Gfx_ContextLost, ChunkUpdater_ClearChunkCache);
	EventHandler_UnregisterVoid(Gfx_ContextRecreated, ChunkUpdater_Refresh);

	ChunkUpdater_OnNewMap();
}

void ChunkUpdater_Refresh(void) {
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
	if (MapRenderer_Chunks != NULL && World_Blocks != NULL) {
		ChunkUpdater_ClearChunkCache();
		ChunkUpdater_ResetChunkCache();
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

void ChunkUpdater_ApplyMeshBuilder(void) {
	if (Game_SmoothLighting) {
		 /* TODO: Implement advanced lighting builder.*/
		NormalBuilder_SetActive();
	} else {
		NormalBuilder_SetActive();
	}
}


static void ChunkUpdater_EnvVariableChanged(EnvVar envVar) {
	if (envVar == EnvVar_SunCol || envVar == EnvVar_ShadowCol) {
		ChunkUpdater_Refresh();
	} else if (envVar == EnvVar_EdgeHeight || envVar == EnvVar_SidesOffset) {
		Int32 oldClip = Builder_EdgeLevel;
		Builder_SidesLevel = max(0, WorldEnv_SidesHeight);
		Builder_EdgeLevel  = max(0, WorldEnv_EdgeHeight);

		/* Only need to refresh chunks on map borders up to highest edge level.*/
		ChunkUpdater_RefreshBorders(max(oldClip, Builder_EdgeLevel));
	}
}

static void ChunkUpdater_TerrainAtlasChanged(void);

void ChunkUpdater_BlockDefinitionChanged(void) {
	MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
	ChunkUpdater_ResetPartFlags();
	ChunkUpdater_Refresh();
}

void ChunkUpdater_ProjectionChanged(void) {
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
}

void ChunkUpdater_ViewDistanceChanged(void) {
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
}


void ChunkUpdater_OnNewMap(void) {
	Game_ChunkUpdates = 0;
	ChunkUpdater_ClearChunkCache();
	ChunkUpdater_ResetPartCounts();
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
	ChunkUpdater_FreeAllocations();
}

void ChunkUpdater_OnNewMapLoaded(void) {
	MapRenderer_ChunksX = (World_Width + CHUNK_MAX) >> CHUNK_SHIFT;
	MapRenderer_ChunksY = (World_Height + CHUNK_MAX) >> CHUNK_SHIFT;
	MapRenderer_ChunksZ = (World_Length + CHUNK_MAX) >> CHUNK_SHIFT;

	Int32 count = MapRenderer_ChunksX * MapRenderer_ChunksY * MapRenderer_ChunksZ;
	if (MapRenderer_ChunksCount != count) {
		MapRenderer_ChunksCount = count;
		ChunkUpdater_FreeAllocations();
		ChunkUpdater_PerformAllocations();
	}

	ChunkUpdater_CreateChunkCache();
	Builder_OnNewMapLoaded();
	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
}

void ChunkUpdater_FreeAllocations(void) {
	if (MapRenderer_Chunks == NULL) return;
	Platform_MemFree(MapRenderer_Chunks); MapRenderer_Chunks = NULL;
	Platform_MemFree(MapRenderer_SortedChunks); MapRenderer_SortedChunks = NULL;
	Platform_MemFree(MapRenderer_RenderChunks); MapRenderer_RenderChunks = NULL;
	Platform_MemFree(ChunkUpdater_Distances); ChunkUpdater_Distances = NULL;
	Platform_MemFree(MapRenderer_PartsBuffer); MapRenderer_PartsBuffer = NULL;
}

void ChunkUpdater_PerformAllocations(void) {
	MapRenderer_Chunks = Platform_MemAlloc(MapRenderer_ChunksCount * sizeof(ChunkInfo));
	if (MapRenderer_Chunks == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate chunk info");

	MapRenderer_SortedChunks = Platform_MemAlloc(MapRenderer_ChunksCount * sizeof(ChunkInfo*));
	if (MapRenderer_Chunks == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate sorted chunk info");

	MapRenderer_RenderChunks = Platform_MemAlloc(MapRenderer_ChunksCount * sizeof(ChunkInfo*));
	if (MapRenderer_RenderChunks == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate render chunk info");

	ChunkUpdater_Distances = Platform_MemAlloc(MapRenderer_ChunksCount * sizeof(Int32));
	if (ChunkUpdater_Distances == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate chunk distances");

	UInt32 partsSize = MapRenderer_ChunksCount * (sizeof(ChunkPartInfo) * cu_atlas1DCount);
	MapRenderer_PartsBuffer = Platform_MemAlloc(partsSize);
	if (MapRenderer_PartsBuffer == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate chunk parts buffer");
	Platform_MemSet(MapRenderer_PartsBuffer, 0, partsSize);
}


void ChunkUpdater_UpdateChunks(Real64 delta) {
	Int32 chunkUpdates = 0;
	cu_chunksTarget += delta < cu_targetTime ? 1 : -1; /* build more chunks if 30 FPS or over, otherwise slowdown. */
	Math_Clamp(cu_chunksTarget, 4, 20);

	LocalPlayer* p = &LocalPlayer_Instance;
	Vector3 camPos = Game_CurrentCameraPos;
	Real32 headX = p->Base.Base.HeadX;
	Real32 headY = p->Base.Base.HeadY;

	bool samePos = Vector3_Equals(&camPos, &cu_lastCamPos) && headX == cu_lastHeadX && headY == cu_lastHeadY;
	MapRenderer_RenderChunksCount = samePos ? 
		ChunkUpdater_UpdateChunksStill(&chunkUpdates) :
		ChunkUpdater_UpdateChunksAndVisibility(&chunkUpdates);

	cu_lastCamPos = camPos;
	cu_lastHeadX = headX; cu_lastHeadY = headY;

	if (!samePos || chunkUpdates != 0) {
		ChunkUpdater_ResetPartFlags();
	}
}

static Int32 ChunkUpdater_UpdateChunksAndVisibility(Int32* chunkUpdates) {
	Int32 i, j = 0;
	Int32 viewDistSqr = ChunkUpdater_AdjustViewDist(Game_ViewDistance);
	Int32 userDistSqr = ChunkUpdater_AdjustViewDist(Game_UserViewDistance);

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkInfo* info = MapRenderer_SortedChunks[i];
		if (info->Empty) { continue; }
		Int32 distSqr = ChunkUpdater_Distances[i];
		bool noData = info->NormalParts == NULL && info->TranslucentParts == NULL;
		
		/* Unload chunks beyond visible range */
		if (!noData && distSqr >= userDistSqr + 32 * 16) {
			ChunkUpdater_DeleteChunk(info); continue;
		}
		noData |= info->PendingDelete;

		if (noData && distSqr <= viewDistSqr && *chunkUpdates < cu_chunksTarget) {
			ChunkUpdater_DeleteChunk(info);
			ChunkUpdater_BuildChunk(info, chunkUpdates);
		}

		info->Visible = distSqr <= viewDistSqr &&
			FrustumCulling_SphereInFrustum(info->CentreX, info->CentreY, info->CentreZ, 14); /* 14 ~ sqrt(3 * 8^2) */
		if (info->Visible && !info->Empty) { MapRenderer_RenderChunks[j] = info; j++; }
	}
	return j;
}

static Int32 ChunkUpdater_UpdateChunksStill(Int32* chunkUpdates) {
	Int32 i, j = 0;
	Int32 viewDistSqr = ChunkUpdater_AdjustViewDist(Game_ViewDistance);
	Int32 userDistSqr = ChunkUpdater_AdjustViewDist(Game_UserViewDistance);

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkInfo* info = MapRenderer_SortedChunks[i];
		if (info->Empty) { continue; }
		Int32 distSqr = ChunkUpdater_Distances[i];
		bool noData = info->NormalParts == NULL && info->TranslucentParts == NULL;

		/* Unload chunks beyond visible range */
		if (!noData && distSqr >= userDistSqr + 32 * 16) {
			ChunkUpdater_DeleteChunk(info); continue;
		}
		noData |= info->PendingDelete;

		if (noData && distSqr <= userDistSqr && *chunkUpdates < cu_chunksTarget) {
			ChunkUpdater_DeleteChunk(info);
			ChunkUpdater_BuildChunk(info, chunkUpdates);

			/* only need to update the visibility of chunks in range. */
			info->Visible = distSqr <= viewDistSqr &&
				FrustumCulling_SphereInFrustum(info->CentreX, info->CentreY, info->CentreZ, 14); /* 14 ~ sqrt(3 * 8^2) */
			if (info->Visible && !info->Empty) { MapRenderer_RenderChunks[j] = info; j++; }
		} else if (info->Visible) {
			MapRenderer_RenderChunks[j] = info; j++;
		}
	}
	return j;
}


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
				ChunkUpdater_Distances[index] = 0;
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


#define ChunkUpdater_DeleteParts(parts, partsCount)\
if (parts != NULL) {\
	ChunkPartInfo* ptr = parts;\
	for (i = 0; i < cu_atlas1DCount; i++) {\
		Gfx_DeleteVb(&ptr->VbId);\
		if (ptr->IndicesCount > 0) {partsCount[i]--; }\
		ptr += MapRenderer_ChunksCount;\
	}\
}

void ChunkUpdater_DeleteChunk(ChunkInfo* info) {
	info->Empty = false; info->AllAir = false;
#if OCCLUSION
	info.OcclusionFlags = 0;
	info.OccludedFlags = 0;
#endif

	Int32 i;
	ChunkUpdater_DeleteParts(info->NormalParts, MapRenderer_NormalPartsCount);
	ChunkUpdater_DeleteParts(info->TranslucentParts, MapRenderer_TranslucentPartsCount);
}

#define ChunkUpdater_AddParts(parts, partsCount)\
if (parts != NULL) {\
	ChunkPartInfo* ptr = parts;\
	for (i = 0; i < cu_atlas1DCount; i++) {\
		if (ptr->IndicesCount > 0) { partsCount[i]++; }\
		ptr += MapRenderer_ChunksCount;\
	}\
}

void ChunkUpdater_BuildChunk(ChunkInfo* info, Int32* chunkUpdates) {
	Game_ChunkUpdates++;
	(*chunkUpdates)++;
	info->PendingDelete = false;
	Builder_MakeChunk(info);

	if (info->NormalParts == NULL && info->TranslucentParts == NULL) {
		info->Empty = true;
		return;
	}

	Int32 i;
	ChunkUpdater_AddParts(info->NormalParts, MapRenderer_NormalPartsCount);
	ChunkUpdater_AddParts(info->TranslucentParts, MapRenderer_TranslucentPartsCount);
}

static Int32 ChunkUpdater_AdjustViewDist(Real32 dist) {
	if (dist < CHUNK_SIZE) dist = CHUNK_SIZE;
	Int32 viewDist = Math_AdjViewDist(dist);
	return (viewDist + 24) * (viewDist + 24);
}