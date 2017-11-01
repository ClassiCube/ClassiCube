#include "ChunkUpdater.h"
#include "Constants.h"
#include "Events.h"
#include "ExtMath.h"
#include "FrustumCulling.h"
#include "Funcs.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "Player.h"
#include "MapRenderer.h"
#include "Platform.h"
#include "TerrainAtlas.h"
#include "Vectors.h"
#include "World.h"
#include "Builder.h"
#include "Utils.h"

void ChunkInfo_Reset(ChunkInfo* chunk, Int32 x, Int32 y, Int32 z) {
	chunk->CentreX = (UInt16)(x + 8);
	chunk->CentreY = (UInt16)(y + 8);
	chunk->CentreZ = (UInt16)(z + 8);

	chunk->Visible = true; chunk->Empty = false;
	chunk->PendingDelete = false; chunk->AllAir = false;
	chunk->DrawXMin = false; chunk->DrawXMax = false; chunk->DrawZMin = false;
	chunk->DrawZMax = false; chunk->DrawYMin = false; chunk->DrawYMax = false;
}

Int32 cu_chunksTarget = 12;
#define cu_targetTime ((1.0 / 30) + 0.01)
Vector3 cu_lastCamPos;
Real32 cu_lastHeadY, cu_lastHeadX;
Int32 cu_atlas1DCount;
Int32 cu_elementsPerBitmap;

void ChunkUpdater_EnvVariableChanged(EnvVar envVar) {
	if (envVar == EnvVar_SunCol || envVar == EnvVar_ShadowCol) {
		ChunkUpdater_Refresh();
	} else if (envVar == EnvVar_EdgeHeight || envVar == EnvVar_SidesOffset) {
		Int32 oldClip = Builder_EdgeLevel;
		Builder_SidesLevel = max(0, WorldEnv_SidesHeight);
		Builder_EdgeLevel = max(0, WorldEnv_EdgeHeight);

		/* Only need to refresh chunks on map borders up to highest edge level.*/
		ChunkUpdater_RefreshBorders(max(oldClip, Builder_EdgeLevel));
	}
}

void ChunkUpdater_TerrainAtlasChanged(void) {
	if (MapRenderer_1DUsedCount != 0) {
		bool refreshRequired = cu_elementsPerBitmap != Atlas1D_ElementsPerBitmap;
		if (refreshRequired) ChunkUpdater_Refresh();
	}

	MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
	cu_elementsPerBitmap = Atlas1D_ElementsPerBitmap;
	ChunkUpdater_ResetPartFlags();
}

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


Int32 ChunkUpdater_AdjustViewDist(Real32 dist) {
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


#define ChunkUpdater_DeleteParts(parts, partsCount)\
if (parts != NULL) {\
	ChunkPartInfo* ptr = parts;\
	for (i = 0; i < cu_atlas1DCount; i++) {\
		Gfx_DeleteVb(&ptr->VbId);\
		if (ptr->HasVertices) { partsCount[i]--; }\
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
	ChunkInfo** values = MapRenderer_SortedChunks;
	Int32* keys = ChunkUpdater_Distances;
	while (left < right) {
		Int32 i = left, j = right;
		Int32 pivot = keys[(i + j) / 2];
		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;

			if (i <= j) {
				Int32 key = keys[i]; keys[i] = keys[j]; keys[j] = key;
				ChunkInfo* value = values[i]; values[i] = values[j]; values[j] = value;
				i++; j--;
			}
		}

		/* recurse into the smaller subset */
		if (j - left <= right - i) {
			if (left < j) ChunkUpdater_QuickSort(left, j);
			left = i;
		} else {
			if (i < right) ChunkUpdater_QuickSort(i, right);
			right = j;
		}
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

void ChunkUpdater_Init(void) {
	Event_RegisterVoid(&TextureEvents_AtlasChanged, ChunkUpdater_TerrainAtlasChanged);
	Event_RegisterVoid(&WorldEvents_NewMap, ChunkUpdater_OnNewMap);
	Event_RegisterVoid(&WorldEvents_MapLoaded, ChunkUpdater_OnNewMapLoaded);
	Event_RegisterInt32(&WorldEvents_EnvVarChanged, ChunkUpdater_EnvVariableChanged);

	Event_RegisterVoid(&BlockEvents_BlockDefChanged, ChunkUpdater_BlockDefinitionChanged);
	Event_RegisterVoid(&GfxEvents_ViewDistanceChanged, ChunkUpdater_ViewDistanceChanged);
	Event_RegisterVoid(&GfxEvents_ProjectionChanged, ChunkUpdater_ProjectionChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost, ChunkUpdater_ClearChunkCache);
	Event_RegisterVoid(&GfxEvents_ContextRecreated, ChunkUpdater_Refresh);

	ChunkUpdater_ChunkPos = Vector3I_Create1(Int32_MaxValue);
	ChunkUpdater_ApplyMeshBuilder();
}

void ChunkUpdater_Free(void) {
	Event_UnregisterVoid(&TextureEvents_AtlasChanged, ChunkUpdater_TerrainAtlasChanged);
	Event_UnregisterVoid(&WorldEvents_NewMap, ChunkUpdater_OnNewMap);
	Event_UnregisterVoid(&WorldEvents_MapLoaded, ChunkUpdater_OnNewMapLoaded);
	Event_UnregisterInt32(&WorldEvents_EnvVarChanged, ChunkUpdater_EnvVariableChanged);

	Event_UnregisterVoid(&BlockEvents_BlockDefChanged, ChunkUpdater_BlockDefinitionChanged);
	Event_UnregisterVoid(&GfxEvents_ViewDistanceChanged, ChunkUpdater_ViewDistanceChanged);
	Event_UnregisterVoid(&GfxEvents_ProjectionChanged, ChunkUpdater_ProjectionChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost, ChunkUpdater_ClearChunkCache);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated, ChunkUpdater_Refresh);

	ChunkUpdater_OnNewMap();
}