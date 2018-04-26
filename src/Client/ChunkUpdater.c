#include "ChunkUpdater.h"
#include "Constants.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "Entity.h"
#include "MapRenderer.h"
#include "Platform.h"
#include "TerrainAtlas.h"
#include "World.h"
#include "Builder.h"
#include "Utils.h"
#include "ErrorHandler.h"

void ChunkInfo_Reset(ChunkInfo* chunk, Int32 x, Int32 y, Int32 z) {
	chunk->CentreX = (UInt16)(x + 8);
	chunk->CentreY = (UInt16)(y + 8);
	chunk->CentreZ = (UInt16)(z + 8);

	chunk->Visible = true; chunk->Empty = false;
	chunk->PendingDelete = false; chunk->AllAir = false;
	chunk->DrawXMin = false; chunk->DrawXMax = false; chunk->DrawZMin = false;
	chunk->DrawZMax = false; chunk->DrawYMin = false; chunk->DrawYMax = false;

	chunk->NormalParts      = NULL;
	chunk->TranslucentParts = NULL;
}

Int32 cu_chunksTarget = 12;
#define cu_targetTime ((1.0 / 30) + 0.01)
Vector3 cu_lastCamPos;
Real32 cu_lastHeadY, cu_lastHeadX;
Int32 cu_elementsPerBitmap;

void ChunkUpdater_EnvVariableChanged(void* obj, Int32 envVar) {
	if (envVar == ENV_VAR_SUN_COL || envVar == ENV_VAR_SHADOW_COL) {
		ChunkUpdater_Refresh();
	} else if (envVar == ENV_VAR_EDGE_HEIGHT || envVar == ENV_VAR_SIDES_OFFSET) {
		Int32 oldClip = Builder_EdgeLevel;
		Builder_SidesLevel = max(0, WorldEnv_SidesHeight);
		Builder_EdgeLevel = max(0, WorldEnv_EdgeHeight);

		/* Only need to refresh chunks on map borders up to highest edge level.*/
		ChunkUpdater_RefreshBorders(max(oldClip, Builder_EdgeLevel));
	}
}

void ChunkUpdater_TerrainAtlasChanged(void* obj) {
	if (MapRenderer_1DUsedCount) {
		bool refreshRequired = cu_elementsPerBitmap != Atlas1D_ElementsPerBitmap;
		if (refreshRequired) ChunkUpdater_Refresh();
	}

	MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
	cu_elementsPerBitmap = Atlas1D_ElementsPerBitmap;
	ChunkUpdater_ResetPartFlags();
}

void ChunkUpdater_BlockDefinitionChanged(void* obj) {
	MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
	ChunkUpdater_ResetPartFlags();
	ChunkUpdater_Refresh();
}

void ChunkUpdater_ProjectionChanged(void* obj) {
	cu_lastCamPos = Vector3_BigPos();
}

void ChunkUpdater_ViewDistanceChanged(void* obj) {
	cu_lastCamPos = Vector3_BigPos();
}

void ChunkUpdater_Refresh(void) {
	ChunkUpdater_ChunkPos = Vector3I_MaxValue();
	if (MapRenderer_Chunks != NULL && World_Blocks != NULL) {
		ChunkUpdater_ClearChunkCache();
		ChunkUpdater_ResetChunkCache();
	}
	ChunkUpdater_ResetPartCounts();
}
void ChunkUpdater_Refresh_Handler(void* obj) {
	ChunkUpdater_Refresh();
}

void ChunkUpdater_RefreshBorders(Int32 clipLevel) {
	ChunkUpdater_ChunkPos = Vector3I_MaxValue();
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
		AdvLightingBuilder_SetActive();
	} else {
		NormalBuilder_SetActive();
	}
}


void ChunkUpdater_FreeAllocations(void) {
	if (MapRenderer_Chunks == NULL) return;
	Platform_MemFree(&MapRenderer_Chunks);
	Platform_MemFree(&MapRenderer_SortedChunks);
	Platform_MemFree(&MapRenderer_RenderChunks);
	Platform_MemFree(&ChunkUpdater_Distances);
	Platform_MemFree(&MapRenderer_PartsBuffer_Raw);

	MapRenderer_PartsNormal = NULL;
	MapRenderer_PartsTranslucent = NULL;
}

void ChunkUpdater_PerformAllocations(void) {
	MapRenderer_Chunks = Platform_MemAlloc(MapRenderer_ChunksCount, sizeof(ChunkInfo));
	if (MapRenderer_Chunks == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate chunk info");

	MapRenderer_SortedChunks = Platform_MemAlloc(MapRenderer_ChunksCount, sizeof(ChunkInfo*));
	if (MapRenderer_Chunks == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate sorted chunk info");

	MapRenderer_RenderChunks = Platform_MemAlloc(MapRenderer_ChunksCount, sizeof(ChunkInfo*));
	if (MapRenderer_RenderChunks == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate render chunk info");

	ChunkUpdater_Distances = Platform_MemAlloc(MapRenderer_ChunksCount, sizeof(Int32));
	if (ChunkUpdater_Distances == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate chunk distances");

	UInt32 partsCount = MapRenderer_ChunksCount * MapRenderer_1DUsedCount;
	MapRenderer_PartsBuffer_Raw = Platform_MemAlloc(partsCount * 2, sizeof(ChunkPartInfo));
	if (MapRenderer_PartsBuffer_Raw == NULL) ErrorHandler_Fail("ChunkUpdater - failed to allocate chunk parts buffer");

	UInt32 partsSize = partsCount * 2 * (UInt32)sizeof(ChunkPartInfo);
	Platform_MemSet(MapRenderer_PartsBuffer_Raw, 0, partsSize);
	MapRenderer_PartsNormal      = MapRenderer_PartsBuffer_Raw;
	MapRenderer_PartsTranslucent = MapRenderer_PartsBuffer_Raw + partsCount;
}

void ChunkUpdater_OnNewMap(void* obj) {
	Game_ChunkUpdates = 0;
	ChunkUpdater_ClearChunkCache();
	ChunkUpdater_ResetPartCounts();
	ChunkUpdater_FreeAllocations();
	ChunkUpdater_ChunkPos = Vector3I_MaxValue();
}

void ChunkUpdater_OnNewMapLoaded(void* obj) {
	MapRenderer_ChunksX = (World_Width + CHUNK_MAX) >> CHUNK_SHIFT;
	MapRenderer_ChunksY = (World_Height + CHUNK_MAX) >> CHUNK_SHIFT;
	MapRenderer_ChunksZ = (World_Length + CHUNK_MAX) >> CHUNK_SHIFT;

	Int32 count = MapRenderer_ChunksX * MapRenderer_ChunksY * MapRenderer_ChunksZ;
	/* TODO: Only perform reallocation when map volume has changed */
	/*if (MapRenderer_ChunksCount != count) { */
		MapRenderer_ChunksCount = count;
		ChunkUpdater_FreeAllocations();
		ChunkUpdater_PerformAllocations();
	/*}*/

	ChunkUpdater_CreateChunkCache();
	Builder_OnNewMapLoaded();
	cu_lastCamPos = Vector3_BigPos();
}


Int32 ChunkUpdater_AdjustViewDist(Int32 dist) {
	if (dist < CHUNK_SIZE) dist = CHUNK_SIZE;
	Int32 viewDist = Utils_AdjViewDist(dist);
	return (viewDist + 24) * (viewDist + 24);
}

Int32 ChunkUpdater_UpdateChunksAndVisibility(Int32* chunkUpdates) {
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

Int32 ChunkUpdater_UpdateChunksStill(Int32* chunkUpdates) {
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

void ChunkUpdater_UpdateChunks(Real64 delta) {
	Int32 chunkUpdates = 0;
	cu_chunksTarget += delta < cu_targetTime ? 1 : -1; /* build more chunks if 30 FPS or over, otherwise slowdown. */
	Math_Clamp(cu_chunksTarget, 4, Game_MaxChunkUpdates);

	LocalPlayer* p = &LocalPlayer_Instance;
	Vector3 camPos = Game_CurrentCameraPos;
	Real32 headX = p->Base.HeadX;
	Real32 headY = p->Base.HeadY;

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


void ChunkUpdater_ResetPartFlags(void) {
	Int32 i;
	for (i = 0; i < ATLAS1D_MAX_ATLASES_COUNT; i++) {
		MapRenderer_CheckingNormalParts[i] = true;
		MapRenderer_HasNormalParts[i] = false;
		MapRenderer_CheckingTranslucentParts[i] = true;
		MapRenderer_HasTranslucentParts[i] = false;
	}
}

void ChunkUpdater_ResetPartCounts(void) {
	Int32 i;
	for (i = 0; i < ATLAS1D_MAX_ATLASES_COUNT; i++) {
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
void ChunkUpdater_ClearChunkCache_Handler(void* obj) {
	ChunkUpdater_ClearChunkCache();
}


#define ChunkUpdater_DeleteParts(parts, partsCount)\
if (parts != NULL) {\
	ChunkPartInfo* ptr = parts;\
	for (i = 0; i < MapRenderer_1DUsedCount; i++) {\
		Gfx_DeleteVb(&ptr->VbId);\
		if (ptr->HasVertices) { partsCount[i]--; }\
		ptr += MapRenderer_ChunksCount;\
	}\
	parts = NULL;\
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
	for (i = 0; i < MapRenderer_1DUsedCount; i++) {\
		if (ptr->HasVertices) { partsCount[i]++; }\
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

void ChunkUpdater_QuickSort(Int32 left, Int32 right) {
	ChunkInfo** values = MapRenderer_SortedChunks; ChunkInfo* value;
	Int32* keys = ChunkUpdater_Distances;          Int32 key;
	while (left < right) {
		Int32 i = left, j = right;
		Int32 pivot = keys[(i + j) / 2];

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(ChunkUpdater_QuickSort)
	}
}

void ChunkUpdater_UpdateSortOrder(void) {
	Vector3 cameraPos = Game_CurrentCameraPos;
	Vector3I newChunkPos;
	Vector3I_Floor(&newChunkPos, &cameraPos);

	newChunkPos.X = (newChunkPos.X & ~CHUNK_MAX) + HALF_CHUNK_SIZE;
	newChunkPos.Y = (newChunkPos.Y & ~CHUNK_MAX) + HALF_CHUNK_SIZE;
	newChunkPos.Z = (newChunkPos.Z & ~CHUNK_MAX) + HALF_CHUNK_SIZE;
	/* Same chunk, therefore don't need to recalculate sort order. */
	if (Vector3I_Equals(&newChunkPos, &ChunkUpdater_ChunkPos)) return;

	Vector3I pPos = newChunkPos;
	ChunkUpdater_ChunkPos = pPos;
	if (MapRenderer_ChunksCount == 0) return;

	Int32 i = 0;
	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkInfo* info = MapRenderer_SortedChunks[i];

		/* Calculate distance to chunk centre*/
		Int32 dx = info->CentreX - pPos.X, dy = info->CentreY - pPos.Y, dz = info->CentreZ - pPos.Z;
		ChunkUpdater_Distances[i] = dx * dx + dy * dy + dz * dz; /* TODO: do we need to cast to unsigned for the mulitplies? */

																 /* Can work out distance to chunk faces as offset from distance to chunk centre on each axis. */
		Int32 dXMin = dx - HALF_CHUNK_SIZE, dXMax = dx + HALF_CHUNK_SIZE;
		Int32 dYMin = dy - HALF_CHUNK_SIZE, dYMax = dy + HALF_CHUNK_SIZE;
		Int32 dZMin = dz - HALF_CHUNK_SIZE, dZMax = dz + HALF_CHUNK_SIZE;

		/* Back face culling: make sure that the chunk is definitely entirely back facing. */
		info->DrawXMin = !(dXMin <= 0 && dXMax <= 0);
		info->DrawXMax = !(dXMin >= 0 && dXMax >= 0);
		info->DrawZMin = !(dZMin <= 0 && dZMax <= 0);
		info->DrawZMax = !(dZMin >= 0 && dZMax >= 0);
		info->DrawYMin = !(dYMin <= 0 && dYMax <= 0);
		info->DrawYMax = !(dYMin >= 0 && dYMax >= 0);
	}

	ChunkUpdater_QuickSort(0, MapRenderer_ChunksCount - 1);
	ChunkUpdater_ResetPartFlags();
	/*SimpleOcclusionCulling();*/
}

void ChunkUpdater_Update(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;
	ChunkUpdater_UpdateSortOrder();
	ChunkUpdater_UpdateChunks(deltaTime);
}

void ChunkUpdater_Init(void) {
	Event_RegisterVoid(&TextureEvents_AtlasChanged,    NULL, ChunkUpdater_TerrainAtlasChanged);
	Event_RegisterVoid(&WorldEvents_NewMap,            NULL, ChunkUpdater_OnNewMap);
	Event_RegisterVoid(&WorldEvents_MapLoaded,         NULL, ChunkUpdater_OnNewMapLoaded);
	Event_RegisterInt(&WorldEvents_EnvVarChanged,    NULL, ChunkUpdater_EnvVariableChanged);

	Event_RegisterVoid(&BlockEvents_BlockDefChanged,   NULL, ChunkUpdater_BlockDefinitionChanged);
	Event_RegisterVoid(&GfxEvents_ViewDistanceChanged, NULL, ChunkUpdater_ViewDistanceChanged);
	Event_RegisterVoid(&GfxEvents_ProjectionChanged,   NULL, ChunkUpdater_ProjectionChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,         NULL, ChunkUpdater_ClearChunkCache_Handler);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,    NULL, ChunkUpdater_Refresh_Handler);

	ChunkUpdater_ChunkPos = Vector3I_MaxValue();
	ChunkUpdater_ApplyMeshBuilder();
}

void ChunkUpdater_Free(void) {
	Event_UnregisterVoid(&TextureEvents_AtlasChanged,    NULL, ChunkUpdater_TerrainAtlasChanged);
	Event_UnregisterVoid(&WorldEvents_NewMap,            NULL, ChunkUpdater_OnNewMap);
	Event_UnregisterVoid(&WorldEvents_MapLoaded,         NULL, ChunkUpdater_OnNewMapLoaded);
	Event_UnregisterInt(&WorldEvents_EnvVarChanged,    NULL, ChunkUpdater_EnvVariableChanged);

	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,   NULL, ChunkUpdater_BlockDefinitionChanged);
	Event_UnregisterVoid(&GfxEvents_ViewDistanceChanged, NULL, ChunkUpdater_ViewDistanceChanged);
	Event_UnregisterVoid(&GfxEvents_ProjectionChanged,   NULL, ChunkUpdater_ProjectionChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,         NULL, ChunkUpdater_ClearChunkCache_Handler);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,    NULL, ChunkUpdater_Refresh_Handler);

	ChunkUpdater_OnNewMap(NULL);
}