#include "MapRenderer.h"
#include "Block.h"
#include "Builder.h"
#include "Camera.h"
#include "Entity.h"
#include "EnvRenderer.h"
#include "Event.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "Graphics.h"
#include "Platform.h"
#include "TexturePack.h"
#include "Utils.h"
#include "World.h"
#include "Options.h"

int MapRenderer_1DUsedCount;
struct ChunkPartInfo* MapRenderer_PartsNormal;
struct ChunkPartInfo* MapRenderer_PartsTranslucent;

static cc_bool inTranslucent;
static IVec3 chunkPos;

/* The number of non-empty Normal/Translucent ChunkPartInfos (across entire world) for each 1D atlas batch. */
/* 1D atlas batches that do not have any ChunkPartInfos can be entirely skipped. */
static int normPartsCount[ATLAS1D_MAX_ATLASES], tranPartsCount[ATLAS1D_MAX_ATLASES];
/* Whether there are any visible Normal/Translucent ChunkPartInfos for each 1D atlas batch. */
/* 1D atlas batches that do not have any visible ChunkPartInfos can be skipped. */
static cc_bool hasNormParts[ATLAS1D_MAX_ATLASES], hasTranParts[ATLAS1D_MAX_ATLASES];
/* Whether renderer should check if there are any visible Normal/Translucent ChunkPartInfos for each 1D atlas batch. */
static cc_bool checkNormParts[ATLAS1D_MAX_ATLASES], checkTranParts[ATLAS1D_MAX_ATLASES];

/* Render info for all chunks in the world. Unsorted. */
static struct ChunkInfo* mapChunks;
/* Pointers to render info for all chunks in the world, sorted by distance from the camera. */
static struct ChunkInfo** sortedChunks;
/* Pointers to render info for all chunks in the world, sorted by distance from the camera. */
/* Only chunks that can be rendered (i.e. not empty and are visible) are included in this.  */
static struct ChunkInfo** renderChunks;
/* Number of actually used pointers in the renderChunks array. Entries past this are ignored and skipped. */
static int renderChunksCount;
/* Distance of each chunk from the camera. */
static cc_uint32* distances;
/* Maximum number of chunk updates that can be performed in one frame. */
static int maxChunkUpdates;
/* Cached number of chunks in the world */
static int chunksCount;

static void ChunkInfo_Reset(struct ChunkInfo* chunk, int x, int y, int z) {
	chunk->CentreX = x + HALF_CHUNK_SIZE; chunk->CentreY = y + HALF_CHUNK_SIZE; 
	chunk->CentreZ = z + HALF_CHUNK_SIZE;
#ifndef CC_BUILD_GL11
	chunk->Vb = 0;
#endif

	chunk->Visible = true;        chunk->Empty = false;
	chunk->PendingDelete = false; chunk->AllAir = false;
	chunk->DrawXMin = false; chunk->DrawXMax = false; chunk->DrawZMin = false;
	chunk->DrawZMax = false; chunk->DrawYMin = false; chunk->DrawYMax = false;

	chunk->NormalParts      = NULL;
	chunk->TranslucentParts = NULL;
}

/* Index of maximum used 1D atlas + 1 */
CC_NOINLINE static int MapRenderer_UsedAtlases(void) {
	TextureLoc maxLoc = 0;
	int i;

	for (i = 0; i < Array_Elems(Blocks.Textures); i++) {
		maxLoc = max(maxLoc, Blocks.Textures[i]);
	}
	return Atlas1D_Index(maxLoc) + 1;
}


/*########################################################################################################################*
*-------------------------------------------------------Map rendering-----------------------------------------------------*
*#########################################################################################################################*/
static void CheckWeather(double delta) {
	IVec3 pos;
	BlockID block;
	cc_bool outside;
	IVec3_Floor(&pos, &Camera.CurrentPos);

	block   = World_SafeGetBlock(pos.X, pos.Y, pos.Z);
	outside = pos.Y < 0 || !World_ContainsXZ(pos.X, pos.Z);
	inTranslucent = Blocks.Draw[block] == DRAW_TRANSLUCENT || (pos.Y < Env.EdgeHeight && outside);

	/* If we are under water, render weather before to blend properly */
	if (!inTranslucent || Env.Weather == WEATHER_SUNNY) return;
	Gfx_SetAlphaBlending(true);
	EnvRenderer_RenderWeather(delta);
	Gfx_SetAlphaBlending(false);
}

#ifdef CC_BUILD_GL11
#define DrawFace(face, ign)    Gfx_BindVb(part.Vbs[face]); Gfx_DrawIndexedTris_T2fC4b(0, 0);
#define DrawFaces(f1, f2, ign) DrawFace(f1, ign); DrawFace(f2, ign);
#else
#define DrawFace(face, offset)    Gfx_DrawIndexedTris_T2fC4b(part.Counts[face], offset);
#define DrawFaces(f1, f2, offset) Gfx_DrawIndexedTris_T2fC4b(part.Counts[f1] + part.Counts[f2], offset);
#endif

#define DrawNormalFaces(minFace, maxFace) \
if (drawMin && drawMax) { \
	Gfx_SetFaceCulling(true); \
	DrawFaces(minFace, maxFace, offset); \
	Gfx_SetFaceCulling(false); \
	Game_Vertices += (part.Counts[minFace] + part.Counts[maxFace]); \
} else if (drawMin) { \
	DrawFace(minFace, offset); \
	Game_Vertices += part.Counts[minFace]; \
} else if (drawMax) { \
	DrawFace(maxFace, offset + part.Counts[minFace]); \
	Game_Vertices += part.Counts[maxFace]; \
}

static void RenderNormalBatch(int batch) {
	int batchOffset = chunksCount * batch;
	struct ChunkInfo* info;
	struct ChunkPartInfo part;
	cc_bool drawMin, drawMax;
	int i, offset, count;

	for (i = 0; i < renderChunksCount; i++) {
		info = renderChunks[i];
		if (!info->NormalParts) continue;

		part = info->NormalParts[batchOffset];
		if (part.Offset < 0) continue;
		hasNormParts[batch] = true;

#ifndef CC_BUILD_GL11
		Gfx_BindVb_Textured(info->Vb);
#endif

		offset  = part.Offset + part.SpriteCount;
		drawMin = info->DrawXMin && part.Counts[FACE_XMIN];
		drawMax = info->DrawXMax && part.Counts[FACE_XMAX];
		DrawNormalFaces(FACE_XMIN, FACE_XMAX);

		offset  += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];
		drawMin = info->DrawZMin && part.Counts[FACE_ZMIN];
		drawMax = info->DrawZMax && part.Counts[FACE_ZMAX];
		DrawNormalFaces(FACE_ZMIN, FACE_ZMAX);

		offset  += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];
		drawMin = info->DrawYMin && part.Counts[FACE_YMIN];
		drawMax = info->DrawYMax && part.Counts[FACE_YMAX];
		DrawNormalFaces(FACE_YMIN, FACE_YMAX);

		if (!part.SpriteCount) continue;
		offset = part.Offset;
		count  = part.SpriteCount >> 2; /* 4 per sprite */

		Gfx_SetFaceCulling(true);
		/* TODO: fix to not render them all */
#ifdef CC_BUILD_GL11
		Gfx_DrawIndexedTris_T2fC4b(part.Vbs[FACE_COUNT], 0);
		Game_Vertices += count * 4;
		Gfx_SetFaceCulling(false);
		continue;
#endif
		if (info->DrawXMax || info->DrawZMin) {
			Gfx_DrawIndexedTris_T2fC4b(count, offset); Game_Vertices += count;
		} offset += count;

		if (info->DrawXMin || info->DrawZMax) {
			Gfx_DrawIndexedTris_T2fC4b(count, offset); Game_Vertices += count;
		} offset += count;

		if (info->DrawXMin || info->DrawZMin) {
			Gfx_DrawIndexedTris_T2fC4b(count, offset); Game_Vertices += count;
		} offset += count;

		if (info->DrawXMax || info->DrawZMax) {
			Gfx_DrawIndexedTris_T2fC4b(count, offset); Game_Vertices += count;
		}
		Gfx_SetFaceCulling(false);
	}
}

void MapRenderer_RenderNormal(double delta) {
	int batch;
	if (!mapChunks) return;

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_SetAlphaTest(true);
	
	Gfx_EnableMipmaps();
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (normPartsCount[batch] <= 0) continue;
		if (hasNormParts[batch] || checkNormParts[batch]) {
			Gfx_BindTexture(Atlas1D.TexIds[batch]);
			RenderNormalBatch(batch);
			checkNormParts[batch] = false;
		}
	}
	Gfx_DisableMipmaps();

	CheckWeather(delta);
	Gfx_SetAlphaTest(false);
#if DEBUG_OCCLUSION
	DebugPickedPos();
#endif
}

#define DrawTranslucentFaces(minFace, maxFace) \
if (drawMin && drawMax) { \
	DrawFaces(minFace, maxFace, offset); \
	Game_Vertices += (part.Counts[minFace] + part.Counts[maxFace]); \
} else if (drawMin) { \
	DrawFace(minFace, offset); \
	Game_Vertices += part.Counts[minFace]; \
} else if (drawMax) { \
	DrawFace(maxFace, offset + part.Counts[minFace]); \
	Game_Vertices += part.Counts[maxFace]; \
}

static void RenderTranslucentBatch(int batch) {
	int batchOffset = chunksCount * batch;
	struct ChunkInfo* info;
	struct ChunkPartInfo part;
	cc_bool drawMin, drawMax;
	int i, offset;

	for (i = 0; i < renderChunksCount; i++) {
		info = renderChunks[i];
		if (!info->TranslucentParts) continue;

		part = info->TranslucentParts[batchOffset];
		if (part.Offset < 0) continue;
		hasTranParts[batch] = true;

#ifndef CC_BUILD_GL11
		Gfx_BindVb_Textured(info->Vb);
#endif

		offset  = part.Offset;
		drawMin = (inTranslucent || info->DrawXMin) && part.Counts[FACE_XMIN];
		drawMax = (inTranslucent || info->DrawXMax) && part.Counts[FACE_XMAX];
		DrawTranslucentFaces(FACE_XMIN, FACE_XMAX);

		offset  += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];
		drawMin = (inTranslucent || info->DrawZMin) && part.Counts[FACE_ZMIN];
		drawMax = (inTranslucent || info->DrawZMax) && part.Counts[FACE_ZMAX];
		DrawTranslucentFaces(FACE_ZMIN, FACE_ZMAX);

		offset  += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];
		drawMin = (inTranslucent || info->DrawYMin) && part.Counts[FACE_YMIN];
		drawMax = (inTranslucent || info->DrawYMax) && part.Counts[FACE_YMAX];
		DrawTranslucentFaces(FACE_YMIN, FACE_YMAX);
	}
}

void MapRenderer_RenderTranslucent(double delta) {
	int vertices, batch;
	if (!mapChunks) return;

	/* First fill depth buffer */
	vertices = Game_Vertices;
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_SetAlphaBlending(false);
	Gfx_DepthOnlyRendering(true);

	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (tranPartsCount[batch] <= 0) continue;
		if (hasTranParts[batch] || checkTranParts[batch]) {
			RenderTranslucentBatch(batch);
			checkTranParts[batch] = false;
		}
	}
	Game_Vertices = vertices;

	/* Then actually draw the transluscent blocks */
	Gfx_SetAlphaBlending(true);
	Gfx_DepthOnlyRendering(false);
	Gfx_SetDepthWrite(false); /* already calculated depth values in depth pass */

	Gfx_EnableMipmaps();
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (tranPartsCount[batch] <= 0) continue;
		if (!hasTranParts[batch]) continue;
		Gfx_BindTexture(Atlas1D.TexIds[batch]);
		RenderTranslucentBatch(batch);
	}
	Gfx_DisableMipmaps();

	Gfx_SetDepthWrite(true);
	/* If we weren't under water, render weather after to blend properly */
	if (!inTranslucent && Env.Weather != WEATHER_SUNNY) {
		Gfx_SetAlphaTest(true);
		EnvRenderer_RenderWeather(delta);
		Gfx_SetAlphaTest(false);
	}
	Gfx_SetAlphaBlending(false);
}


/*########################################################################################################################*
*---------------------------------------------------Chunk functionality---------------------------------------------------*
*#########################################################################################################################*/
/* Deletes vertex buffer associated with the given chunk and updates internal state */
static void DeleteChunk(struct ChunkInfo* info) {
	struct ChunkPartInfo* ptr;
	int i;
#ifdef CC_BUILD_GL11
	int j;
#else
	Gfx_DeleteVb(&info->Vb);
#endif

	info->Empty = false; info->AllAir = false;
#ifdef OCCLUSION
	info.OcclusionFlags = 0;
	info.OccludedFlags = 0;
#endif

	if (info->NormalParts) {
		ptr = info->NormalParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) {
			if (ptr->Offset < 0) continue; 
			normPartsCount[i]--;
#ifdef CC_BUILD_GL11
			for (j = 0; j < CHUNKPART_MAX_VBS; j++) Gfx_DeleteVb(&ptr->Vbs[j]);
#endif
		}
		info->NormalParts = NULL;
	}

	if (info->TranslucentParts) {
		ptr = info->TranslucentParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) {
			if (ptr->Offset < 0) continue;
			tranPartsCount[i]--;
#ifdef CC_BUILD_GL11
			for (j = 0; j < CHUNKPART_MAX_VBS; j++) Gfx_DeleteVb(&ptr->Vbs[j]);
#endif
		}
		info->TranslucentParts = NULL;
	}
}

/* Builds the mesh (hence vertex buffer) for the given chunk, and updates internal state */
static void BuildChunk(struct ChunkInfo* info, int* chunkUpdates) {
	struct ChunkPartInfo* ptr;
	int i;

	Game.ChunkUpdates++;
	(*chunkUpdates)++;
	info->PendingDelete = false;
	Builder_MakeChunk(info);

	if (!info->NormalParts && !info->TranslucentParts) {
		info->Empty = true; return;
	}
	
	if (info->NormalParts) {
		ptr = info->NormalParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) {
			if (ptr->Offset >= 0) normPartsCount[i]++;
		}
	}

	if (info->TranslucentParts) {
		ptr = info->TranslucentParts;
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) {
			if (ptr->Offset >= 0) tranPartsCount[i]++;
		}
	}
}


/*########################################################################################################################*
*----------------------------------------------------Chunks mangagement---------------------------------------------------*
*#########################################################################################################################*/
static void FreeParts(void) {
	Mem_Free(MapRenderer_PartsNormal);
	MapRenderer_PartsNormal      = NULL;
	MapRenderer_PartsTranslucent = NULL;
}

static void FreeChunks(void) {
	Mem_Free(mapChunks);
	Mem_Free(sortedChunks);
	Mem_Free(renderChunks);
	Mem_Free(distances);

	mapChunks    = NULL;
	sortedChunks = NULL;
	renderChunks = NULL;
	distances    = NULL;
}

static void AllocateParts(void) {
	struct ChunkPartInfo* ptr;
	cc_uint32 count = chunksCount * MapRenderer_1DUsedCount;

	ptr = (struct ChunkPartInfo*)Mem_AllocCleared(count * 2, sizeof(struct ChunkPartInfo), "chunk parts");
	MapRenderer_PartsNormal      = ptr;
	MapRenderer_PartsTranslucent = ptr + count;
}

static void AllocateChunks(void) {
	mapChunks    = (struct ChunkInfo*) Mem_Alloc(chunksCount, sizeof(struct ChunkInfo),  "chunk info");
	sortedChunks = (struct ChunkInfo**)Mem_Alloc(chunksCount, sizeof(struct ChunkInfo*), "sorted chunk info");
	renderChunks = (struct ChunkInfo**)Mem_Alloc(chunksCount, sizeof(struct ChunkInfo*), "render chunk info");
	distances    = (cc_uint32*)Mem_Alloc(chunksCount, 4, "chunk distances");
}

static void ResetPartFlags(void) {
	int i;
	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		checkNormParts[i] = true;
		hasNormParts[i]   = false;
		checkTranParts[i] = true;
		hasTranParts[i]   = false;
	}
}

static void ResetPartCounts(void) {
	int i;
	for (i = 0; i < ATLAS1D_MAX_ATLASES; i++) {
		normPartsCount[i] = 0;
		tranPartsCount[i] = 0;
	}
}

static void InitChunks(void) {
	int x, y, z, index = 0;
	for (z = 0; z < World.Length; z += CHUNK_SIZE) {
		for (y = 0; y < World.Height; y += CHUNK_SIZE) {
			for (x = 0; x < World.Width; x += CHUNK_SIZE) {
				ChunkInfo_Reset(&mapChunks[index], x, y, z);
				sortedChunks[index] = &mapChunks[index];
				renderChunks[index] = &mapChunks[index];
				distances[index]    = 0;
				index++;
			}
		}
	}
}

static void ResetChunks(void) {
	int x, y, z, index = 0;
	for (z = 0; z < World.Length; z += CHUNK_SIZE) {
		for (y = 0; y < World.Height; y += CHUNK_SIZE) {
			for (x = 0; x < World.Width; x += CHUNK_SIZE) {
				ChunkInfo_Reset(&mapChunks[index], x, y, z);
				index++;
			}
		}
	}
}

static void DeleteChunks(void) {
	int i;
	if (!mapChunks) return;

	for (i = 0; i < chunksCount; i++) {
		DeleteChunk(&mapChunks[i]);
	}
	ResetPartCounts();
}

void MapRenderer_Refresh(void) {
	int oldCount;
	chunkPos = IVec3_MaxValue();

	if (mapChunks && World.Blocks) {
		DeleteChunks();
		ResetChunks();

		oldCount = MapRenderer_1DUsedCount;
		MapRenderer_1DUsedCount = MapRenderer_UsedAtlases();
		/* Need to reallocate parts array in this case */
		if (MapRenderer_1DUsedCount != oldCount) {
			FreeParts();
			AllocateParts();
		}
	}
	ResetPartCounts();
}

/* Refreshes chunks on the border of the map whose y is less than 'maxHeight'. */
static void RefreshBorderChunks(int maxHeight) {
	int cx, cy, cz;
	cc_bool onBorder;

	chunkPos = IVec3_MaxValue();
	if (!mapChunks || !World.Blocks) return;

	for (cz = 0; cz < World.ChunksZ; cz++) {
		for (cy = 0; cy < World.ChunksY; cy++) {
			for (cx = 0; cx < World.ChunksX; cx++) {
				onBorder = cx == 0 || cz == 0 || cx == (World.ChunksX - 1) || cz == (World.ChunksZ - 1);

				if (onBorder && (cy * CHUNK_SIZE) < maxHeight) {
					MapRenderer_RefreshChunk(cx, cy, cz);
				}
			}
		}
	}
}


/*########################################################################################################################*
*--------------------------------------------------Chunks updating/sorting------------------------------------------------*
*#########################################################################################################################*/
#define CHUNK_TARGET_TIME ((1.0/30) + 0.01)
static int chunksTarget = 12;
static Vec3 lastCamPos;
static float lastYaw, lastPitch;
/* Max distance from camera that chunks are rendered within */
/* This may differ from the view distance configured by the user */
static int renderDistSquared;
/* Max distance from camera that chunks are built within */
/* Chunks past this distance are automatically unloaded */
static int buildDistSquared;

static int AdjustDist(int dist) {
	if (dist < CHUNK_SIZE) dist = CHUNK_SIZE;
	dist = Utils_AdjViewDist(dist);
	return (dist + 24) * (dist + 24);
}

static void CalcViewDists(void) {
	buildDistSquared  = AdjustDist(Game_UserViewDistance);
	renderDistSquared = AdjustDist(Game_ViewDistance);
}

static int UpdateChunksAndVisibility(int* chunkUpdates) {
	int renderDistSqr = renderDistSquared;
	int buildDistSqr  = buildDistSquared;

	struct ChunkInfo* info;
	int i, j = 0, distSqr;
	cc_bool noData;

	for (i = 0; i < chunksCount; i++) {
		info = sortedChunks[i];
		if (info->Empty) continue;

		distSqr = distances[i];
		noData  = !info->NormalParts && !info->TranslucentParts;
		
		/* Auto unload chunks far away chunks */
		if (!noData && distSqr >= buildDistSqr + 32 * 16) {
			DeleteChunk(info); continue;
		}
		noData |= info->PendingDelete;

		if (noData && distSqr <= buildDistSqr && *chunkUpdates < chunksTarget) {
			DeleteChunk(info);
			BuildChunk(info, chunkUpdates);
		}

		info->Visible = distSqr <= renderDistSqr &&
			FrustumCulling_SphereInFrustum(info->CentreX, info->CentreY, info->CentreZ, 14); /* 14 ~ sqrt(3 * 8^2) */
		if (info->Visible && !info->Empty) { renderChunks[j] = info; j++; }
	}
	return j;
}

static int UpdateChunksStill(int* chunkUpdates) {
	int renderDistSqr = renderDistSquared;
	int buildDistSqr  = buildDistSquared;

	struct ChunkInfo* info;
	int i, j = 0, distSqr;
	cc_bool noData;

	for (i = 0; i < chunksCount; i++) {
		info = sortedChunks[i];
		if (info->Empty) continue;

		distSqr = distances[i];
		noData  = !info->NormalParts && !info->TranslucentParts;

		/* Auto unload chunks far away chunks */
		if (!noData && distSqr >= buildDistSqr + 32 * 16) {
			DeleteChunk(info); continue;
		}
		noData |= info->PendingDelete;

		if (noData && distSqr <= buildDistSqr && *chunkUpdates < chunksTarget) {
			DeleteChunk(info);
			BuildChunk(info, chunkUpdates);

			/* only need to update the visibility of chunks in range. */
			info->Visible = distSqr <= renderDistSqr &&
				FrustumCulling_SphereInFrustum(info->CentreX, info->CentreY, info->CentreZ, 14); /* 14 ~ sqrt(3 * 8^2) */
			if (info->Visible && !info->Empty) { renderChunks[j] = info; j++; }
		} else if (info->Visible) {
			renderChunks[j] = info; j++;
		}
	}
	return j;
}

static void UpdateChunks(double delta) {
	struct LocalPlayer* p;
	cc_bool samePos;
	int chunkUpdates = 0;

	/* Build more chunks if 30 FPS or over, otherwise slowdown */
	chunksTarget += delta < CHUNK_TARGET_TIME ? 1 : -1; 
	Math_Clamp(chunksTarget, 4, maxChunkUpdates);

	p = &LocalPlayer_Instance;
	samePos = Vec3_Equals(&Camera.CurrentPos, &lastCamPos)
		&& p->Base.Pitch == lastPitch && p->Base.Yaw == lastYaw;

	renderChunksCount = samePos ?
		UpdateChunksStill(&chunkUpdates) :
		UpdateChunksAndVisibility(&chunkUpdates);

	lastCamPos = Camera.CurrentPos;
	lastPitch  = p->Base.Pitch;
	lastYaw    = p->Base.Yaw;

	if (!samePos || chunkUpdates) ResetPartFlags();
}

static void SortMapChunks(int left, int right) {
	struct ChunkInfo** values = sortedChunks; struct ChunkInfo* value;
	cc_uint32* keys = distances; cc_uint32 key;

	while (left < right) {
		int i = left, j = right;
		cc_uint32 pivot = keys[(i + j) >> 1];

		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;
			QuickSort_Swap_KV_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(SortMapChunks)
	}
}

static void UpdateSortOrder(void) {
	struct ChunkInfo* info;
	IVec3 pos;
	int i, dx, dy, dz;

	/* pos is centre coordinate of chunk camera is in */
	IVec3_Floor(&pos, &Camera.CurrentPos);
	pos.X = (pos.X & ~CHUNK_MASK) + HALF_CHUNK_SIZE;
	pos.Y = (pos.Y & ~CHUNK_MASK) + HALF_CHUNK_SIZE;
	pos.Z = (pos.Z & ~CHUNK_MASK) + HALF_CHUNK_SIZE;

	/* If in same chunk, don't need to recalculate sort order */
	if (pos.X == chunkPos.X && pos.Y == chunkPos.Y && pos.Z == chunkPos.Z) return;
	chunkPos = pos;
	if (!chunksCount) return;

	for (i = 0; i < chunksCount; i++) {
		info = sortedChunks[i];
		/* Calculate distance to chunk centre */
		dx = info->CentreX - pos.X; dy = info->CentreY - pos.Y; dz = info->CentreZ - pos.Z;
		distances[i] = dx * dx + dy * dy + dz * dz;

		/* Consider these 3 chunks: */
		/* |       X-1      |        X        |       X+1      | */
		/* |################|########@########|################| */
		/* Assume the player is standing at @, then DrawXMin/XMax is calculated as this */
		/*    X-1: DrawXMin = false, DrawXMax = true  */
		/*    X  : DrawXMin = true,  DrawXMax = true  */
		/*    X+1: DrawXMin = true,  DrawXMax = false */

		info->DrawXMin = dx >= 0; info->DrawXMax = dx <= 0;
		info->DrawZMin = dz >= 0; info->DrawZMax = dz <= 0;
		info->DrawYMin = dy >= 0; info->DrawYMax = dy <= 0;
	}

	SortMapChunks(0, chunksCount - 1);
	ResetPartFlags();
	/*SimpleOcclusionCulling();*/
}

void MapRenderer_Update(double delta) {
	if (!mapChunks) return;
	UpdateSortOrder();
	UpdateChunks(delta);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void MapRenderer_RefreshChunk(int cx, int cy, int cz) {
	struct ChunkInfo* info;
	if (cx < 0 || cy < 0 || cz < 0 || cx >= World.ChunksX || cy >= World.ChunksY || cz >= World.ChunksZ) return;

	info = &mapChunks[World_ChunkPack(cx, cy, cz)];
	if (info->AllAir) return; /* do not recreate chunks completely air */
	info->Empty         = false;
	info->PendingDelete = true;
}

void MapRenderer_OnBlockChanged(int x, int y, int z, BlockID block) {
	int cx = x >> CHUNK_SHIFT, cy = y >> CHUNK_SHIFT, cz = z >> CHUNK_SHIFT;
	struct ChunkInfo* chunk;

	chunk = &mapChunks[World_ChunkPack(cx, cy, cz)];
	chunk->AllAir &= Blocks.Draw[block] == DRAW_GAS;
	/* TODO: Don't lookup twice, refresh directly using chunk pointer */
	MapRenderer_RefreshChunk(cx, cy, cz);
}

static void OnEnvVariableChanged(void* obj, int envVar) {
	if (envVar == ENV_VAR_SUN_COLOR || envVar == ENV_VAR_SHADOW_COLOR) {
		MapRenderer_Refresh();
	} else if (envVar == ENV_VAR_EDGE_HEIGHT || envVar == ENV_VAR_SIDES_OFFSET) {
		int oldClip        = Builder_EdgeLevel;
		Builder_SidesLevel = max(0, Env_SidesHeight);
		Builder_EdgeLevel  = max(0, Env.EdgeHeight);

		/* Only need to refresh chunks on map borders up to highest edge level.*/
		RefreshBorderChunks(max(oldClip, Builder_EdgeLevel));
	}
}

static void OnTerrainAtlasChanged(void* obj) {
	static int tilesPerAtlas;
	/* e.g. If old atlas was 256x256 and new is 256x256, don't need to refresh */
	if (MapRenderer_1DUsedCount && tilesPerAtlas != Atlas1D.TilesPerAtlas) {
		MapRenderer_Refresh();
	}

	MapRenderer_1DUsedCount = MapRenderer_UsedAtlases();
	tilesPerAtlas = Atlas1D.TilesPerAtlas;
	ResetPartFlags();
}

static void OnBlockDefinitionChanged(void* obj) {
	MapRenderer_Refresh();
	MapRenderer_1DUsedCount = MapRenderer_UsedAtlases();
	ResetPartFlags();
}

static void OnVisibilityChanged(void* obj) {
	lastCamPos = Vec3_BigPos();
	CalcViewDists();
}
static void DeleteChunks_(void* obj) { DeleteChunks(); }
static void Refresh_(void* obj)      { MapRenderer_Refresh(); }

static void OnNewMap(void) {
	Game.ChunkUpdates = 0;
	DeleteChunks();
	ResetPartCounts();

	chunkPos = IVec3_MaxValue();
	FreeChunks();
	FreeParts();
}

static void OnNewMapLoaded(void) {
	chunksCount = World.ChunksCount;
	/* TODO: Only perform reallocation when map volume has changed */
	/*if (chunksCount != World.ChunksCount) { */
		/* chunksCount = World.ChunksCount; */
		FreeChunks();
		FreeParts();
		AllocateChunks();
		AllocateParts();
	/*}*/

	InitChunks();
	lastCamPos = Vec3_BigPos();
}

static void OnInit(void) {
	Event_Register_(&TextureEvents.AtlasChanged,  NULL, OnTerrainAtlasChanged);
	Event_Register_(&WorldEvents.EnvVarChanged,   NULL, OnEnvVariableChanged);
	Event_Register_(&BlockEvents.BlockDefChanged, NULL, OnBlockDefinitionChanged);

	Event_Register_(&GfxEvents.ViewDistanceChanged, NULL, OnVisibilityChanged);
	Event_Register_(&GfxEvents.ProjectionChanged,   NULL, OnVisibilityChanged);
	Event_Register_(&GfxEvents.ContextLost,         NULL, DeleteChunks_);
	Event_Register_(&GfxEvents.ContextRecreated,    NULL, Refresh_);

	/* This = 87 fixes map being invisible when no textures */
	MapRenderer_1DUsedCount = 87; /* Atlas1D_UsedAtlasesCount(); */
	chunkPos   = IVec3_MaxValue();
	maxChunkUpdates = Options_GetInt(OPT_MAX_CHUNK_UPDATES, 4, 1024, 30);
	CalcViewDists();
}

struct IGameComponent MapRenderer_Component = {
	OnInit, /* Init */
	OnNewMap, /* Free */
	OnNewMap, /* Reset */
	OnNewMap, /* OnNewMap */
	OnNewMapLoaded /* OnNewMapLoaded */
};
