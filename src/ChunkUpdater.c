#include "ChunkUpdater.h"
#include "Constants.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "Graphics.h"
#include "Entity.h"
#include "MapRenderer.h"
#include "Platform.h"
#include "TerrainAtlas.h"
#include "World.h"
#include "Builder.h"
#include "Utils.h"
#include "ErrorHandler.h"
#include "Camera.h"

Vector3I ChunkUpdater_ChunkPos;
uint32_t* ChunkUpdater_Distances;

void ChunkInfo_Reset(struct ChunkInfo* chunk, int x, int y, int z) {
	chunk->CentreX = x + 8; chunk->CentreY = y + 8; chunk->CentreZ = z + 8;
#ifndef CC_BUILD_GL11
	chunk->Vb = GFX_NULL;
#endif

	chunk->Visible = true;        chunk->Empty = false;
	chunk->PendingDelete = false; chunk->AllAir = false;
	chunk->DrawXMin = false; chunk->DrawXMax = false; chunk->DrawZMin = false;
	chunk->DrawZMax = false; chunk->DrawYMin = false; chunk->DrawYMax = false;

	chunk->NormalParts      = NULL;
	chunk->TranslucentParts = NULL;
}

int cu_chunksTarget = 12;
#define cu_targetTime ((1.0 / 30) + 0.01)
Vector3 cu_lastCamPos;
float cu_lastHeadY, cu_lastHeadX;
int cu_elementsPerBitmap;

static void ChunkUpdater_EnvVariableChanged(void* obj, int envVar) {
	if (envVar == ENV_VAR_SUN_COL || envVar == ENV_VAR_SHADOW_COL) {
		ChunkUpdater_Refresh();
	} else if (envVar == ENV_VAR_EDGE_HEIGHT || envVar == ENV_VAR_SIDES_OFFSET) {
		int oldClip        = Builder_EdgeLevel;
		Builder_SidesLevel = max(0, Env_SidesHeight);
		Builder_EdgeLevel  = max(0, Env_EdgeHeight);

		/* Only need to refresh chunks on map borders up to highest edge level.*/
		ChunkUpdater_RefreshBorders(max(oldClip, Builder_EdgeLevel));
	}
}

static void ChunkUpdater_TerrainAtlasChanged(void* obj) {
	if (MapRenderer_1DUsedCount) {
		bool refreshRequired = cu_elementsPerBitmap != Atlas1D_TilesPerAtlas;
		if (refreshRequired) ChunkUpdater_Refresh();
	}

	MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
	cu_elementsPerBitmap = Atlas1D_TilesPerAtlas;
	ChunkUpdater_ResetPartFlags();
}

static void ChunkUpdater_BlockDefinitionChanged(void* obj) {
	ChunkUpdater_Refresh();
	MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
	ChunkUpdater_ResetPartFlags();
}

static void ChunkUpdater_ProjectionChanged(void* obj) {
	cu_lastCamPos = Vector3_BigPos();
}

static void ChunkUpdater_ViewDistanceChanged(void* obj) {
	cu_lastCamPos = Vector3_BigPos();
}


static void ChunkUpdater_FreePartsAllocations(void) {
	Mem_Free(MapRenderer_PartsBuffer_Raw);
	MapRenderer_PartsBuffer_Raw  = NULL;
	MapRenderer_PartsNormal      = NULL;
	MapRenderer_PartsTranslucent = NULL;
}

static void ChunkUpdater_FreeAllocations(void) {
	if (!MapRenderer_Chunks) return;
	ChunkUpdater_FreePartsAllocations();

	Mem_Free(MapRenderer_Chunks);
	Mem_Free(MapRenderer_SortedChunks);
	Mem_Free(MapRenderer_RenderChunks);
	Mem_Free(ChunkUpdater_Distances);

	MapRenderer_Chunks       = NULL;
	MapRenderer_SortedChunks = NULL;
	MapRenderer_RenderChunks = NULL;
	ChunkUpdater_Distances   = NULL;
}

static void ChunkUpdater_PerformPartsAllocations(void) {
	uint32_t count = MapRenderer_ChunksCount * MapRenderer_1DUsedCount;
	MapRenderer_PartsBuffer_Raw  = Mem_AllocCleared(count * 2, sizeof(struct ChunkPartInfo), "chunk parts");
	MapRenderer_PartsNormal      = MapRenderer_PartsBuffer_Raw;
	MapRenderer_PartsTranslucent = MapRenderer_PartsBuffer_Raw + count;
}

static void ChunkUpdater_PerformAllocations(void) {
	MapRenderer_Chunks       = Mem_Alloc(MapRenderer_ChunksCount, sizeof(struct ChunkInfo), "chunk info");
	MapRenderer_SortedChunks = Mem_Alloc(MapRenderer_ChunksCount, sizeof(struct ChunkInfo*), "sorted chunk info");
	MapRenderer_RenderChunks = Mem_Alloc(MapRenderer_ChunksCount, sizeof(struct ChunkInfo*), "render chunk info");
	ChunkUpdater_Distances   = Mem_Alloc(MapRenderer_ChunksCount, 4, "chunk distances");
	ChunkUpdater_PerformPartsAllocations();
}

void ChunkUpdater_Refresh(void) {
	int oldCount;
	ChunkUpdater_ChunkPos = Vector3I_MaxValue();

	if (MapRenderer_Chunks && World_Blocks) {
		ChunkUpdater_ClearChunkCache();
		ChunkUpdater_ResetChunkCache();

		oldCount = MapRenderer_1DUsedCount;
		MapRenderer_1DUsedCount = Atlas1D_UsedAtlasesCount();
		/* Need to reallocate parts array in this case */
		if (MapRenderer_1DUsedCount != oldCount) {
			ChunkUpdater_FreePartsAllocations();
			ChunkUpdater_PerformPartsAllocations();
		}
	}
	ChunkUpdater_ResetPartCounts();
}
static void ChunkUpdater_Refresh_Handler(void* obj) {
	ChunkUpdater_Refresh();
}

void ChunkUpdater_RefreshBorders(int clipLevel) {
	int cx, cy, cz;
	bool onBorder;

	ChunkUpdater_ChunkPos = Vector3I_MaxValue();
	if (!MapRenderer_Chunks || !World_Blocks) return;

	for (cz = 0; cz < MapRenderer_ChunksZ; cz++) {
		for (cy = 0; cy < MapRenderer_ChunksY; cy++) {
			for (cx = 0; cx < MapRenderer_ChunksX; cx++) {
				onBorder = cx == 0 || cz == 0 || cx == (MapRenderer_ChunksX - 1) || cz == (MapRenderer_ChunksZ - 1);

				if (onBorder && (cy * CHUNK_SIZE) < clipLevel) {
					MapRenderer_RefreshChunk(cx, cy, cz);
				}
			}
		}
	}
}


void ChunkUpdater_ApplyMeshBuilder(void) {
	if (Game_SmoothLighting) {
		 /* TODO: Implement advanced lighting builder.*/
		AdvBuilder_SetActive();
	} else {
		NormalBuilder_SetActive();
	}
}

static void ChunkUpdater_OnNewMap(void* obj) {
	Game_ChunkUpdates = 0;
	ChunkUpdater_ClearChunkCache();
	ChunkUpdater_ResetPartCounts();
	ChunkUpdater_FreeAllocations();
	ChunkUpdater_ChunkPos = Vector3I_MaxValue();
}

static void ChunkUpdater_OnNewMapLoaded(void* obj) {
	int count;
	MapRenderer_ChunksX = (World_Width  + CHUNK_MAX) >> CHUNK_SHIFT;
	MapRenderer_ChunksY = (World_Height + CHUNK_MAX) >> CHUNK_SHIFT;
	MapRenderer_ChunksZ = (World_Length + CHUNK_MAX) >> CHUNK_SHIFT;

	count = MapRenderer_ChunksX * MapRenderer_ChunksY * MapRenderer_ChunksZ;
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


static int ChunkUpdater_AdjustViewDist(int dist) {
	if (dist < CHUNK_SIZE) dist = CHUNK_SIZE;
	dist = Utils_AdjViewDist(dist);
	return (dist + 24) * (dist + 24);
}

static int ChunkUpdater_UpdateChunksAndVisibility(int* chunkUpdates) {
	int viewDistSqr = ChunkUpdater_AdjustViewDist(Game_ViewDistance);
	int userDistSqr = ChunkUpdater_AdjustViewDist(Game_UserViewDistance);

	struct ChunkInfo* info;
	int i, j = 0, distSqr;
	bool noData;

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		info = MapRenderer_SortedChunks[i];
		if (info->Empty) continue;

		distSqr = ChunkUpdater_Distances[i];
		noData  = !info->NormalParts && !info->TranslucentParts;
		
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

static int ChunkUpdater_UpdateChunksStill(int* chunkUpdates) {
	int viewDistSqr = ChunkUpdater_AdjustViewDist(Game_ViewDistance);
	int userDistSqr = ChunkUpdater_AdjustViewDist(Game_UserViewDistance);

	struct ChunkInfo* info;
	int i, j = 0, distSqr;
	bool noData;

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		info = MapRenderer_SortedChunks[i];
		if (info->Empty) continue;

		distSqr = ChunkUpdater_Distances[i];
		noData  = !info->NormalParts && !info->TranslucentParts;

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

void ChunkUpdater_UpdateChunks(double delta) {
	int chunkUpdates = 0;
	cu_chunksTarget += delta < cu_targetTime ? 1 : -1; /* build more chunks if 30 FPS or over, otherwise slowdown. */
	Math_Clamp(cu_chunksTarget, 4, Game_MaxChunkUpdates);

	struct LocalPlayer* p = &LocalPlayer_Instance;
	Vector3 pos = Camera_CurrentPos;
	float headX = p->Base.HeadX;
	float headY = p->Base.HeadY;

	bool samePos = Vector3_Equals(&pos, &cu_lastCamPos) && headX == cu_lastHeadX && headY == cu_lastHeadY;
	MapRenderer_RenderChunksCount = samePos ?
		ChunkUpdater_UpdateChunksStill(&chunkUpdates) :
		ChunkUpdater_UpdateChunksAndVisibility(&chunkUpdates);

	cu_lastCamPos = pos;
	cu_lastHeadX = headX; cu_lastHeadY = headY;

	if (!samePos || chunkUpdates != 0) {
		ChunkUpdater_ResetPartFlags();
	}
}


void ChunkUpdater_ResetPartFlags(void) {
	int i;
	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		MapRenderer_CheckingNormalParts[i] = true;
		MapRenderer_HasNormalParts[i] = false;
		MapRenderer_CheckingTranslucentParts[i] = true;
		MapRenderer_HasTranslucentParts[i] = false;
	}
}

void ChunkUpdater_ResetPartCounts(void) {
	int i;
	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		MapRenderer_NormalPartsCount[i] = 0;
		MapRenderer_TranslucentPartsCount[i] = 0;
	}
}

void ChunkUpdater_CreateChunkCache(void) {
	int x, y, z, index = 0;
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
	int x, y, z, index = 0;
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
	int i;
	if (!MapRenderer_Chunks) return;

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkUpdater_DeleteChunk(&MapRenderer_Chunks[i]);
	}
	ChunkUpdater_ResetPartCounts();
}
static void ChunkUpdater_ClearChunkCache_Handler(void* obj) {
	ChunkUpdater_ClearChunkCache();
}


void ChunkUpdater_DeleteChunk(struct ChunkInfo* info) {
	struct ChunkPartInfo* ptr;
	int i;

	info->Empty = false; info->AllAir = false;
#ifdef OCCLUSION
	info.OcclusionFlags = 0;
	info.OccludedFlags = 0;
#endif
#ifndef CC_BUILD_GL11
	Gfx_DeleteVb(&info->Vb);
#endif

	if (info->NormalParts) {
		ptr = info->NormalParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += MapRenderer_ChunksCount) {
			if (ptr->Offset < 0) continue; 
			MapRenderer_NormalPartsCount[i]--;
#ifdef CC_BUILD_GL11
			Gfx_DeleteVb(&ptr->Vb);
#endif
		}
		info->NormalParts = NULL;
	}

	if (info->TranslucentParts) {
		ptr = info->TranslucentParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += MapRenderer_ChunksCount) {
			if (ptr->Offset < 0) continue;
			MapRenderer_TranslucentPartsCount[i]--;
#ifdef CC_BUILD_GL11
			Gfx_DeleteVb(&ptr->Vb);
#endif
		}
		info->TranslucentParts = NULL;
	}
}

void ChunkUpdater_BuildChunk(struct ChunkInfo* info, int* chunkUpdates) {
	struct ChunkPartInfo* ptr;
	int i;

	Game_ChunkUpdates++;
	(*chunkUpdates)++;
	info->PendingDelete = false;
	Builder_MakeChunk(info);

	if (!info->NormalParts && !info->TranslucentParts) {
		info->Empty = true; return;
	}
	
	if (info->NormalParts) {
		ptr = info->NormalParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += MapRenderer_ChunksCount) {
			if (ptr->Offset >= 0) { MapRenderer_NormalPartsCount[i]++; }
		}
	}

	if (info->TranslucentParts) {
		ptr = info->TranslucentParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += MapRenderer_ChunksCount) {
			if (ptr->Offset >= 0) { MapRenderer_TranslucentPartsCount[i]++; }
		}
	}
}

static void ChunkUpdater_QuickSort(int left, int right) {
	struct ChunkInfo** values = MapRenderer_SortedChunks; struct ChunkInfo* value;
	uint32_t* keys = ChunkUpdater_Distances; uint32_t key;

	while (left < right) {
		int i = left, j = right;
		uint32_t pivot = keys[(i + j) >> 1];

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

static void ChunkUpdater_UpdateSortOrder(void) {
	struct ChunkInfo* info;
	Vector3I pos;
	int dXMin, dXMax, dYMin, dYMax, dZMin, dZMax;
	int i, dx, dy, dz;

	/* pos is centre coordinate of chunk camera is in */
	Vector3I_Floor(&pos, &Camera_CurrentPos);
	pos.X = (pos.X & ~CHUNK_MASK) + HALF_CHUNK_SIZE;
	pos.Y = (pos.Y & ~CHUNK_MASK) + HALF_CHUNK_SIZE;
	pos.Z = (pos.Z & ~CHUNK_MASK) + HALF_CHUNK_SIZE;

	/* If in same chunk, don't need to recalculate sort order */
	if (Vector3I_Equals(&pos, &ChunkUpdater_ChunkPos)) return;
	ChunkUpdater_ChunkPos = pos;
	if (!MapRenderer_ChunksCount) return;

	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		info = MapRenderer_SortedChunks[i];
		/* Calculate distance to chunk centre */
		dx = info->CentreX - pos.X; dy = info->CentreY - pos.Y; dz = info->CentreZ - pos.Z;
		ChunkUpdater_Distances[i] = dx * dx + dy * dy + dz * dz;

		/* Can work out distance to chunk faces as offset from distance to chunk centre on each axis */
		dXMin = dx - HALF_CHUNK_SIZE; dXMax = dx + HALF_CHUNK_SIZE;
		dYMin = dy - HALF_CHUNK_SIZE; dYMax = dy + HALF_CHUNK_SIZE;
		dZMin = dz - HALF_CHUNK_SIZE; dZMax = dz + HALF_CHUNK_SIZE;

		/* Back face culling: make sure that the chunk is definitely entirely back facing */
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

void ChunkUpdater_Update(double deltaTime) {
	if (!MapRenderer_Chunks) return;
	ChunkUpdater_UpdateSortOrder();
	ChunkUpdater_UpdateChunks(deltaTime);
}

void ChunkUpdater_Init(void) {
	Event_RegisterVoid(&TextureEvents_AtlasChanged, NULL, ChunkUpdater_TerrainAtlasChanged);
	Event_RegisterVoid(&WorldEvents_NewMap,         NULL, ChunkUpdater_OnNewMap);
	Event_RegisterVoid(&WorldEvents_MapLoaded,      NULL, ChunkUpdater_OnNewMapLoaded);
	Event_RegisterInt(&WorldEvents_EnvVarChanged,   NULL, ChunkUpdater_EnvVariableChanged);

	Event_RegisterVoid(&BlockEvents_BlockDefChanged,   NULL, ChunkUpdater_BlockDefinitionChanged);
	Event_RegisterVoid(&GfxEvents_ViewDistanceChanged, NULL, ChunkUpdater_ViewDistanceChanged);
	Event_RegisterVoid(&GfxEvents_ProjectionChanged,   NULL, ChunkUpdater_ProjectionChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,         NULL, ChunkUpdater_ClearChunkCache_Handler);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,    NULL, ChunkUpdater_Refresh_Handler);

	/* This = 87 fixes map being invisible when no textures */
	MapRenderer_1DUsedCount = 87; /* Atlas1D_UsedAtlasesCount(); */
	ChunkUpdater_ChunkPos   = Vector3I_MaxValue();
	ChunkUpdater_ApplyMeshBuilder();
}

void ChunkUpdater_Free(void) {
	Event_UnregisterVoid(&TextureEvents_AtlasChanged, NULL, ChunkUpdater_TerrainAtlasChanged);
	Event_UnregisterVoid(&WorldEvents_NewMap,         NULL, ChunkUpdater_OnNewMap);
	Event_UnregisterVoid(&WorldEvents_MapLoaded,      NULL, ChunkUpdater_OnNewMapLoaded);
	Event_UnregisterInt(&WorldEvents_EnvVarChanged,   NULL, ChunkUpdater_EnvVariableChanged);

	Event_UnregisterVoid(&BlockEvents_BlockDefChanged,   NULL, ChunkUpdater_BlockDefinitionChanged);
	Event_UnregisterVoid(&GfxEvents_ViewDistanceChanged, NULL, ChunkUpdater_ViewDistanceChanged);
	Event_UnregisterVoid(&GfxEvents_ProjectionChanged,   NULL, ChunkUpdater_ProjectionChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,         NULL, ChunkUpdater_ClearChunkCache_Handler);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,    NULL, ChunkUpdater_Refresh_Handler);

	ChunkUpdater_OnNewMap(NULL);
}
