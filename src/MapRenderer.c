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

static void ChunkInfo_Init(struct ChunkInfo* chunk, int x, int y, int z) {
	chunk->centreX = x + HALF_CHUNK_SIZE; 
	chunk->centreY = y + HALF_CHUNK_SIZE; 
	chunk->centreZ = z + HALF_CHUNK_SIZE;
#if CC_GFX_BACKEND != CC_GFX_BACKEND_GL11
	chunk->vb = 0;
#endif

	chunk->index     = World_ChunkPack(x >> CHUNK_SHIFT, y >> CHUNK_SHIFT, z >> CHUNK_SHIFT);
	chunk->flags     = CHUNK_FLAG_VISIBLE | CHUNK_FLAG_DIRTY; 
	chunk->drawFlags = 0;
}

static CC_INLINE void ChunkInfo_Refresh(struct ChunkInfo* chunk) {
	/* do not recreate chunks completely air */
	if (chunk->flags & CHUNK_FLAG_ALL_AIR) return;

	chunk->flags &= ~CHUNK_FLAG_EMPTY;
	chunk->flags |=  CHUNK_FLAG_DIRTY;
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
static void CheckWeather(float delta) {
	IVec3 pos;
	BlockID block;
	cc_bool outside;
	IVec3_Floor(&pos, &Camera.CurrentPos);

	block   = World_SafeGetBlock(pos.x, pos.y, pos.z);
	outside = pos.y < 0 || !World_ContainsXZ(pos.x, pos.z);
	inTranslucent = Blocks.Draw[block] == DRAW_TRANSLUCENT || (pos.y < Env.EdgeHeight && outside);

	/* If we are under water, render weather before to blend properly */
	if (!inTranslucent || Env.Weather == WEATHER_SUNNY) return;
	Gfx_SetAlphaBlending(true);
	EnvRenderer_RenderWeather(delta);
	Gfx_SetAlphaBlending(false);
}

#ifdef CC_CLIPPING_FLAGS
	#define DrawBatch(count, start) Gfx_DrawIndexedTris_T2fC4b(count, start, chunk->skipClip ? DRAW_HINT_NOCLIP : 0)
#else
	#define DrawBatch(count, start) Gfx_DrawIndexedTris_T2fC4b(count, start, 0)
#endif

#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
	#define DrawFace(face, ign)    Gfx_BindVb(part.vbs[face]); DrawBatch(0, 0);
	#define DrawFaces(f1, f2, ign) DrawFace(f1, ign); DrawFace(f2, ign);
#else
	#define DrawFace(face, offset)    DrawBatch(part.counts[face], offset);
	#define DrawFaces(f1, f2, offset) DrawBatch(part.counts[f1] + part.counts[f2], offset);
#endif

#define DrawNormalFaces(minFace, maxFace) \
if (drawMin && drawMax) { \
	Gfx_SetFaceCulling(true); \
	DrawFaces(minFace, maxFace, offset); \
	Gfx_SetFaceCulling(false); \
	Game_Vertices += (part.counts[minFace] + part.counts[maxFace]); \
} else if (drawMin) { \
	DrawFace(minFace, offset); \
	Game_Vertices += part.counts[minFace]; \
} else if (drawMax) { \
	DrawFace(maxFace, offset + part.counts[minFace]); \
	Game_Vertices += part.counts[maxFace]; \
}

static void RenderNormalBatch(int batch) {
	struct ChunkPartInfo* partsBase = MapRenderer_PartsNormal;
	int batchOffset = chunksCount * batch;
	struct ChunkInfo* chunk;
	struct ChunkPartInfo part;
	cc_bool drawMin, drawMax;
	int i, offset, count;
	cc_uint8 flags;

	for (i = 0; i < renderChunksCount; i++) 
	{
		chunk = renderChunks[i];
		if (!(chunk->flags & CHUNK_FLAG_NORM_PARTS)) continue;

		part = partsBase[chunk->index + batchOffset];
		if (part.offset < 0) continue;
		hasNormParts[batch] = true;

#if CC_GFX_BACKEND != CC_GFX_BACKEND_GL11
		Gfx_BindVb_Textured(chunk->vb);
#endif
		flags = chunk->drawFlags;

		offset  = part.offset + part.spriteCount;
		drawMin = (flags & FACE_BIT_XMIN) && part.counts[FACE_XMIN];
		drawMax = (flags & FACE_BIT_XMAX) && part.counts[FACE_XMAX];
		DrawNormalFaces(FACE_XMIN, FACE_XMAX);

		offset  += part.counts[FACE_XMIN] + part.counts[FACE_XMAX];
		drawMin = (flags & FACE_BIT_ZMIN) && part.counts[FACE_ZMIN];
		drawMax = (flags & FACE_BIT_ZMAX) && part.counts[FACE_ZMAX];
		DrawNormalFaces(FACE_ZMIN, FACE_ZMAX);

		offset  += part.counts[FACE_ZMIN] + part.counts[FACE_ZMAX];
		drawMin = (flags & FACE_BIT_YMIN) && part.counts[FACE_YMIN];
		drawMax = (flags & FACE_BIT_YMAX) && part.counts[FACE_YMAX];
		DrawNormalFaces(FACE_YMIN, FACE_YMAX);

		if (!part.spriteCount) continue;
		offset = part.offset;
		count  = part.spriteCount >> 2; /* 4 per sprite */

		Gfx_SetFaceCulling(true);
		/* TODO: fix to not render them all */
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
		Gfx_BindVb(part.vbs[FACE_COUNT]);
		DrawBatch(0, 0);
		Game_Vertices += count * 4;
		Gfx_SetFaceCulling(false);
		continue;
#endif
		if (flags & (FACE_BIT_XMAX | FACE_BIT_ZMIN)) {
			DrawBatch(count, offset); Game_Vertices += count;
		} offset += count;

		if (flags & (FACE_BIT_XMIN | FACE_BIT_ZMAX)) {
			DrawBatch(count, offset); Game_Vertices += count;
		} offset += count;

		if (flags & (FACE_BIT_XMIN | FACE_BIT_ZMIN)) {
			DrawBatch(count, offset); Game_Vertices += count;
		} offset += count;

		if (flags & (FACE_BIT_XMAX | FACE_BIT_ZMAX)) {
			DrawBatch(count, offset); Game_Vertices += count;
		}
		Gfx_SetFaceCulling(false);
	}
}

void MapRenderer_RenderNormal(float delta) {
	int batch;
	if (!mapChunks) return;

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_SetAlphaTest(true);
	
	Gfx_EnableMipmaps();
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) 
	{
		if (normPartsCount[batch] <= 0) continue;
		if (hasNormParts[batch] || checkNormParts[batch]) {
			Atlas1D_Bind(batch);
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
	Game_Vertices += (part.counts[minFace] + part.counts[maxFace]); \
} else if (drawMin) { \
	DrawFace(minFace, offset); \
	Game_Vertices += part.counts[minFace]; \
} else if (drawMax) { \
	DrawFace(maxFace, offset + part.counts[minFace]); \
	Game_Vertices += part.counts[maxFace]; \
}

static void RenderTranslucentBatch(int batch) {
	struct ChunkPartInfo* partsBase = MapRenderer_PartsTranslucent;
	int batchOffset = chunksCount * batch;
	struct ChunkInfo* chunk;
	struct ChunkPartInfo part;
	cc_bool drawMin, drawMax;
	int i, offset;
	cc_uint8 flags;

	for (i = 0; i < renderChunksCount; i++) 
	{
		chunk = renderChunks[i];
		if (!(chunk->flags & CHUNK_FLAG_TRNS_PARTS)) continue;

		part = partsBase[chunk->index + batchOffset];
		if (part.offset < 0) continue;
		hasTranParts[batch] = true;

#if CC_GFX_BACKEND != CC_GFX_BACKEND_GL11
		Gfx_BindVb_Textured(chunk->vb);
#endif
		flags = inTranslucent ? FACE_BITS_ALL : chunk->drawFlags;

		offset  = part.offset;
		drawMin = (flags & FACE_BIT_XMIN) && part.counts[FACE_XMIN];
		drawMax = (flags & FACE_BIT_XMAX) && part.counts[FACE_XMAX];
		DrawTranslucentFaces(FACE_XMIN, FACE_XMAX);

		offset  += part.counts[FACE_XMIN] + part.counts[FACE_XMAX];
		drawMin = (flags & FACE_BIT_ZMIN) && part.counts[FACE_ZMIN];
		drawMax = (flags & FACE_BIT_ZMAX) && part.counts[FACE_ZMAX];
		DrawTranslucentFaces(FACE_ZMIN, FACE_ZMAX);

		offset  += part.counts[FACE_ZMIN] + part.counts[FACE_ZMAX];
		drawMin = (flags & FACE_BIT_YMIN) && part.counts[FACE_YMIN];
		drawMax = (flags & FACE_BIT_YMAX) && part.counts[FACE_YMAX];
		DrawTranslucentFaces(FACE_YMIN, FACE_YMAX);
	}
}

void MapRenderer_RenderTranslucent(float delta) {
	int vertices, batch;
	if (!mapChunks) return;

	/* First fill depth buffer */
	vertices = Game_Vertices;
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_SetAlphaBlending(false);
	Gfx_DepthOnlyRendering(true);

	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) 
	{
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
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) 
	{
		if (tranPartsCount[batch] <= 0) continue;
		if (!hasTranParts[batch]) continue;

		Atlas1D_Bind(batch);
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
static void DeleteChunk(struct ChunkInfo* chunk) {
	struct ChunkPartInfo* ptr;
	int i;
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
	int j;
#else
	Gfx_DeleteVb(&chunk->vb);
#endif

	if (chunk->flags & CHUNK_FLAG_NORM_PARTS) {
		ptr = &MapRenderer_PartsNormal[chunk->index];
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) 
		{
			if (ptr->offset < 0) continue; 
			normPartsCount[i]--;
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
			for (j = 0; j < CHUNKPART_MAX_VBS; j++) Gfx_DeleteVb(&ptr->vbs[j]);
#endif
		}
	}

	if (chunk->flags & CHUNK_FLAG_TRNS_PARTS) {
		ptr = &MapRenderer_PartsTranslucent[chunk->index];
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) 
		{
			if (ptr->offset < 0) continue;
			tranPartsCount[i]--;
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL11
			for (j = 0; j < CHUNKPART_MAX_VBS; j++) Gfx_DeleteVb(&ptr->vbs[j]);
#endif
		}
	}

	chunk->flags &= ~CHUNK_FLAG_EMPTY;
	chunk->flags &= ~CHUNK_FLAG_ALL_AIR;
	chunk->flags &= ~CHUNK_FLAGS_ANY_PARTS;
	chunk->flags |=  CHUNK_FLAG_DIRTY;

#ifdef OCCLUSION
	chunk.OcclusionFlags = 0;
	chunk.OccludedFlags = 0;
#endif
}

/* Builds the mesh (hence vertex buffer) for the given chunk, and updates internal state */
static void BuildChunk(struct ChunkInfo* chunk, int* chunkUpdates) {
	struct ChunkPartInfo* ptr;
	int i;

	Game.ChunkUpdates++;
	(*chunkUpdates)++;
	if (!Builder_MakeChunk(chunk)) return;

	chunk->flags &= ~CHUNK_FLAG_DIRTY;
	chunk->flags &= ~CHUNK_FLAG_EMPTY;
	if (!(chunk->flags & CHUNK_FLAGS_ANY_PARTS)) chunk->flags |= CHUNK_FLAG_EMPTY;
	
	if (chunk->flags & CHUNK_FLAG_NORM_PARTS) {
		ptr = &MapRenderer_PartsNormal[chunk->index];
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) 
		{
			if (ptr->offset >= 0) normPartsCount[i]++;
		}
	}

	if (chunk->flags & CHUNK_FLAG_TRNS_PARTS) {
		ptr = &MapRenderer_PartsTranslucent[chunk->index];
		for (i = 0; i < MapRenderer_1DUsedCount; i++, ptr += chunksCount) 
		{
			if (ptr->offset >= 0) tranPartsCount[i]++;
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
				ChunkInfo_Init(&mapChunks[index], x, y, z);
				sortedChunks[index] = &mapChunks[index];
				renderChunks[index] = &mapChunks[index];
				distances[index]    = 0;
				index++;
			}
		}
	}
}

static void RefreshChunks(void) {
	int i;
	if (!mapChunks) return;

	for (i = 0; i < chunksCount; i++) 
	{
		ChunkInfo_Refresh(&mapChunks[i]);
	}
}

static void DeleteChunks(void) {
	int i;
	if (!mapChunks) return;

	for (i = 0; i < chunksCount; i++) 
	{
		DeleteChunk(&mapChunks[i]);
	}
	ResetPartCounts();
}

void MapRenderer_Refresh(void) {
	int oldCount;
	chunkPos = IVec3_MaxValue();

	if (mapChunks && World.Blocks) {
		DeleteChunks();

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
#define CHUNK_TARGET_TIME ((1.0f/30) + 0.01f)
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

	struct ChunkInfo* chunk;
	int i, j = 0, distSqr, res;

	for (i = 0; i < chunksCount; i++) 
	{
		chunk = sortedChunks[i];
		if (chunk->flags & CHUNK_FLAG_EMPTY) continue;
		distSqr = distances[i];
		
		/* Auto unload chunks far away chunks */
		if ((chunk->flags & CHUNK_FLAGS_ANY_PARTS) && distSqr >= buildDistSqr + 32 * 16) {
			DeleteChunk(chunk); continue;
		}

		/* Build chunks within (visible range + small buffer) that have changed */
		/* Building chunks ahead of time slightly past visible range reduces chunk pop-in when moving */
		if ((chunk->flags & CHUNK_FLAG_DIRTY) && distSqr <= buildDistSqr && *chunkUpdates < chunksTarget) {
			DeleteChunk(chunk);
			BuildChunk(chunk, chunkUpdates);

			if (!(chunk->flags & CHUNK_FLAGS_ANY_PARTS)) continue;
		}

		/* Skip over chunks definitely can't see */
		if (distSqr > renderDistSqr) {
			chunk->flags &= ~CHUNK_FLAG_VISIBLE; continue;
		}

		res = Frustum_TestSphere(chunk->centreX, chunk->centreY, chunk->centreZ, 14); /* 14 ~ sqrt(3 * 8^2) */
		chunk->flags &= ~CHUNK_FLAG_VISIBLE;
		if (res != FRUSTUM_OUTSIDE) chunk->flags |= CHUNK_FLAG_VISIBLE;

		chunk->flags &= ~CHUNK_FLAG_SKIP_CLIP;
		if (Gfx_CanSphereSkipClipping(chunk->centreX, chunk->centreY, chunk->centreZ, 14)) chunk->flags |= CHUNK_FLAG_SKIP_CLIP;

		if ((chunk->flags & CHUNK_FLAG_VISIBLE) && (chunk->flags & CHUNK_FLAGS_ANY_PARTS)) { 
			renderChunks[j] = chunk; j++;
		}
	}
	return j;
}

static int UpdateChunksStill(int* chunkUpdates) {
	int renderDistSqr = renderDistSquared;
	int buildDistSqr  = buildDistSquared;

	struct ChunkInfo* chunk;
	int i, j = 0, distSqr, res;

	for (i = 0; i < chunksCount; i++) 
	{
		chunk = sortedChunks[i];
		if (chunk->flags & CHUNK_FLAG_EMPTY) continue;
		distSqr = distances[i];

		/* Auto unload chunks far away chunks */
		if ((chunk->flags & CHUNK_FLAGS_ANY_PARTS) && distSqr >= buildDistSqr + 32 * 16) {
			DeleteChunk(chunk); continue;
		}

		/* Build chunks within (visible range + small buffer) that have changed */
		/* Building chunks ahead of time slightly past visible range reduces chunk pop-in when moving */
		if ((chunk->flags & CHUNK_FLAG_DIRTY) && distSqr <= buildDistSqr && *chunkUpdates < chunksTarget) {
			DeleteChunk(chunk);
			BuildChunk(chunk, chunkUpdates);
			if (!(chunk->flags & CHUNK_FLAGS_ANY_PARTS)) continue;

			/* only need to update the visibility of chunks in visible range */
			if (distSqr > renderDistSqr) {
				chunk->flags &= ~CHUNK_FLAG_VISIBLE; continue;
			}

			res = Frustum_TestSphere(chunk->centreX, chunk->centreY, chunk->centreZ, 14); /* 14 ~ sqrt(3 * 8^2) */
			chunk->flags &= ~CHUNK_FLAG_VISIBLE;
			if (res != FRUSTUM_OUTSIDE) chunk->flags |= CHUNK_FLAG_VISIBLE;

			chunk->flags &= ~CHUNK_FLAG_SKIP_CLIP;
			if (Gfx_CanSphereSkipClipping(chunk->centreX, chunk->centreY, chunk->centreZ, 14)) chunk->flags |= CHUNK_FLAG_SKIP_CLIP;
		}

		if ((chunk->flags & CHUNK_FLAG_VISIBLE) && (chunk->flags & CHUNK_FLAGS_ANY_PARTS)) { 
			renderChunks[j] = chunk; j++;
		}
	}
	return j;
}

static void UpdateChunks(float delta) {
	struct LocalPlayer* p;
	cc_bool samePos;
	int chunkUpdates = 0;

	/* Build more chunks if 30 FPS or over, otherwise slowdown */
	chunksTarget += delta < CHUNK_TARGET_TIME ? 1 : -1; 
	Math_Clamp(chunksTarget, 4, maxChunkUpdates);

	p = Entities.CurPlayer;
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
	struct ChunkInfo* chunk;
	IVec3 pos;
	int i, dx, dy, dz;

	/* pos is centre coordinate of chunk camera is in */
	IVec3_Floor(&pos, &Camera.CurrentPos);
	pos.x = (pos.x & ~CHUNK_MASK) + HALF_CHUNK_SIZE;
	pos.y = (pos.y & ~CHUNK_MASK) + HALF_CHUNK_SIZE;
	pos.z = (pos.z & ~CHUNK_MASK) + HALF_CHUNK_SIZE;

	/* If in same chunk, don't need to recalculate sort order */
	if (pos.x == chunkPos.x && pos.y == chunkPos.y && pos.z == chunkPos.z) return;
	chunkPos = pos;
	if (!chunksCount) return;

	for (i = 0; i < chunksCount; i++) 
	{
		chunk = sortedChunks[i];
		/* Calculate distance to chunk centre */
		dx = chunk->centreX - pos.x; dy = chunk->centreY - pos.y; dz = chunk->centreZ - pos.z;
		distances[i] = dx * dx + dy * dy + dz * dz;

		/* Consider these 3 chunks: */
		/* |       X-1      |        X        |       X+1      | */
		/* |################|########@########|################| */
		/* Assume the player is standing at @, then DrawXMin/XMax is calculated as this */
		/*    X-1: DrawXMin = false, DrawXMax = true  */
		/*    X  : DrawXMin = true,  DrawXMax = true  */
		/*    X+1: DrawXMin = true,  DrawXMax = false */
		chunk->drawFlags = 
			(dx >= 0 ? FACE_BIT_XMIN : 0) |
			(dx <= 0 ? FACE_BIT_XMAX : 0) |
			(dz >= 0 ? FACE_BIT_ZMIN : 0) |
			(dz <= 0 ? FACE_BIT_ZMAX : 0) |
			(dy >= 0 ? FACE_BIT_YMIN : 0) |
			(dy <= 0 ? FACE_BIT_YMAX : 0);
	}

	SortMapChunks(0, chunksCount - 1);
	ResetPartFlags();
	/*SimpleOcclusionCulling();*/
}

void MapRenderer_Update(float delta) {
	if (!mapChunks) return;
	UpdateSortOrder();
	UpdateChunks(delta);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void MapRenderer_RefreshChunk(int cx, int cy, int cz) {
	struct ChunkInfo* chunk;
	if (cx < 0 || cy < 0 || cz < 0 || cx >= World.ChunksX || cy >= World.ChunksY || cz >= World.ChunksZ) return;

	chunk = &mapChunks[World_ChunkPack(cx, cy, cz)];
	ChunkInfo_Refresh(chunk);
}

void MapRenderer_OnBlockChanged(int x, int y, int z, BlockID block) {
	int cx = x >> CHUNK_SHIFT, cy = y >> CHUNK_SHIFT, cz = z >> CHUNK_SHIFT;
	struct ChunkInfo* chunk;

	chunk = &mapChunks[World_ChunkPack(cx, cy, cz)];
	if (Blocks.Draw[block] != DRAW_GAS) chunk->flags &= ~CHUNK_FLAG_ALL_AIR;

	ChunkInfo_Refresh(chunk);
}

static void OnEnvVariableChanged(void* obj, int envVar) {
	if (envVar == ENV_VAR_SUN_COLOR || envVar == ENV_VAR_SHADOW_COLOR) {
		RefreshChunks();
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
