#include "MapRenderer.h"
#include "Block.h"
#include "Game.h"
#include "Graphics.h"
#include "EnvRenderer.h"
#include "World.h"
#include "Camera.h"
#include "ChunkUpdater.h"
static bool inTranslucent;

struct ChunkInfo* MapRenderer_GetChunk(int cx, int cy, int cz) {
	return &MapRenderer_Chunks[MapRenderer_Pack(cx, cy, cz)];
}

void MapRenderer_RefreshChunk(int cx, int cy, int cz) {
	struct ChunkInfo* info;
	if (cx < 0 || cy < 0 || cz < 0 || cx >= MapRenderer_ChunksX 
		|| cy >= MapRenderer_ChunksY || cz >= MapRenderer_ChunksZ) return;

	info = &MapRenderer_Chunks[MapRenderer_Pack(cx, cy, cz)];
	if (info->AllAir) return; /* do not recreate chunks completely air */
	info->Empty         = false;
	info->PendingDelete = true;
}

static void MapRenderer_CheckWeather(double delta) {
	Vector3I pos;
	BlockID block;
	bool outside;
	Vector3I_Floor(&pos, &Camera_CurrentPos);

	block   = World_SafeGetBlock_3I(pos);
	outside = pos.X < 0 || pos.Y < 0 || pos.Z < 0 || pos.X >= World_Width || pos.Z >= World_Length;
	inTranslucent = Block_Draw[block] == DRAW_TRANSLUCENT || (pos.Y < Env_EdgeHeight && outside);

	/* If we are under water, render weather before to blend properly */
	if (!inTranslucent || Env_Weather == WEATHER_SUNNY) return;
	Gfx_SetAlphaBlending(true);
	EnvRenderer_RenderWeather(delta);
	Gfx_SetAlphaBlending(false);
}

#define MapRenderer_DrawNormalFaces(minFace, maxFace) \
if (drawMin && drawMax) { \
	Gfx_SetFaceCulling(true); \
	Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[minFace] + part.Counts[maxFace], offset); \
	Gfx_SetFaceCulling(false); \
	Game_Vertices += (part.Counts[minFace] + part.Counts[maxFace]); \
} else if (drawMin) { \
	Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[minFace], offset); \
	Game_Vertices += part.Counts[minFace]; \
} else if (drawMax) { \
	Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[maxFace], offset + part.Counts[minFace]); \
	Game_Vertices += part.Counts[maxFace]; \
}

static void MapRenderer_RenderNormalBatch(int batch) {
	int batchOffset = MapRenderer_ChunksCount * batch;
	struct ChunkInfo* info;
	struct ChunkPartInfo part;
	bool drawMin, drawMax;
	int i, offset, count;

	for (i = 0; i < MapRenderer_RenderChunksCount; i++) {
		info = MapRenderer_RenderChunks[i];
		if (!info->NormalParts) continue;

		part = info->NormalParts[batchOffset];
		if (part.Offset < 0) continue;
		MapRenderer_HasNormalParts[batch] = true;

#ifndef CC_BUILD_GL11
		Gfx_BindVb(info->Vb);
#else
		Gfx_BindVb(part.Vb);
#endif

		offset  = part.Offset + part.SpriteCount;
		drawMin = info->DrawXMin && part.Counts[FACE_XMIN];
		drawMax = info->DrawXMax && part.Counts[FACE_XMAX];
		MapRenderer_DrawNormalFaces(FACE_XMIN, FACE_XMAX);

		offset  += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];
		drawMin = info->DrawZMin && part.Counts[FACE_ZMIN];
		drawMax = info->DrawZMax && part.Counts[FACE_ZMAX];
		MapRenderer_DrawNormalFaces(FACE_ZMIN, FACE_ZMAX);

		offset  += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];
		drawMin = info->DrawYMin && part.Counts[FACE_YMIN];
		drawMax = info->DrawYMax && part.Counts[FACE_YMAX];
		MapRenderer_DrawNormalFaces(FACE_YMIN, FACE_YMAX);

		if (!part.SpriteCount) continue;
		offset = part.Offset;
		count  = part.SpriteCount >> 2; /* 4 per sprite */

		Gfx_SetFaceCulling(true);
		if (info->DrawXMax || info->DrawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, offset); Game_Vertices += count;
		} offset += count;

		if (info->DrawXMin || info->DrawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, offset); Game_Vertices += count;
		} offset += count;

		if (info->DrawXMin || info->DrawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, offset); Game_Vertices += count;
		} offset += count;

		if (info->DrawXMax || info->DrawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, offset); Game_Vertices += count;
		}
		Gfx_SetFaceCulling(false);
	}
}

void MapRenderer_RenderNormal(double delta) {
	int batch;
	if (!MapRenderer_Chunks) return;

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);
	
	Gfx_EnableMipmaps();
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (MapRenderer_NormalPartsCount[batch] <= 0) continue;
		if (MapRenderer_HasNormalParts[batch] || MapRenderer_CheckingNormalParts[batch]) {
			Gfx_BindTexture(Atlas1D_TexIds[batch]);
			MapRenderer_RenderNormalBatch(batch);
			MapRenderer_CheckingNormalParts[batch] = false;
		}
	}
	Gfx_DisableMipmaps();

	MapRenderer_CheckWeather(delta);
	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);
#if DEBUG_OCCLUSION
	DebugPickedPos();
#endif
}

#define MapRenderer_DrawTranslucentFaces(minFace, maxFace) \
if (drawMin && drawMax) { \
	Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[minFace] + part.Counts[maxFace], offset); \
	Game_Vertices += (part.Counts[minFace] + part.Counts[maxFace]); \
} else if (drawMin) { \
	Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[minFace], offset); \
	Game_Vertices += part.Counts[minFace]; \
} else if (drawMax) { \
	Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[maxFace], offset + part.Counts[minFace]); \
	Game_Vertices += part.Counts[maxFace]; \
}

static void MapRenderer_RenderTranslucentBatch(int batch) {
	int batchOffset = MapRenderer_ChunksCount * batch;
	struct ChunkInfo* info;
	struct ChunkPartInfo part;
	bool drawMin, drawMax;
	int i, offset;

	for (i = 0; i < MapRenderer_RenderChunksCount; i++) {
		info = MapRenderer_RenderChunks[i];
		if (!info->TranslucentParts) continue;

		part = info->TranslucentParts[batchOffset];
		if (part.Offset < 0) continue;
		MapRenderer_HasTranslucentParts[batch] = true;

#ifndef CC_BUILD_GL11
		Gfx_BindVb(info->Vb);
#else
		Gfx_BindVb(part.Vb);
#endif

		offset  = part.Offset;
		drawMin = (inTranslucent || info->DrawXMin) && part.Counts[FACE_XMIN];
		drawMax = (inTranslucent || info->DrawXMax) && part.Counts[FACE_XMAX];
		MapRenderer_DrawTranslucentFaces(FACE_XMIN, FACE_XMAX);

		offset  += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];
		drawMin = (inTranslucent || info->DrawZMin) && part.Counts[FACE_ZMIN];
		drawMax = (inTranslucent || info->DrawZMax) && part.Counts[FACE_ZMAX];
		MapRenderer_DrawTranslucentFaces(FACE_ZMIN, FACE_ZMAX);

		offset  += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];
		drawMin = (inTranslucent || info->DrawYMin) && part.Counts[FACE_YMIN];
		drawMax = (inTranslucent || info->DrawYMax) && part.Counts[FACE_YMAX];
		MapRenderer_DrawTranslucentFaces(FACE_YMIN, FACE_YMAX);
	}
}

void MapRenderer_RenderTranslucent(double delta) {
	int vertices, batch;
	if (!MapRenderer_Chunks) return;

	/* First fill depth buffer */
	vertices = Game_Vertices;
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_SetTexturing(false);
	Gfx_SetAlphaBlending(false);
	Gfx_SetColWriteMask(false, false, false, false);

	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (MapRenderer_TranslucentPartsCount[batch] <= 0) continue;
		if (MapRenderer_HasTranslucentParts[batch] || MapRenderer_CheckingTranslucentParts[batch]) {
			MapRenderer_RenderTranslucentBatch(batch);
			MapRenderer_CheckingTranslucentParts[batch] = false;
		}
	}
	Game_Vertices = vertices;

	/* Then actually draw the transluscent blocks */
	Gfx_SetAlphaBlending(true);
	Gfx_SetTexturing(true);
	Gfx_SetColWriteMask(true, true, true, true);
	Gfx_SetDepthWrite(false); /* already calculated depth values in depth pass */

	Gfx_EnableMipmaps();
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (MapRenderer_TranslucentPartsCount[batch] <= 0) continue;
		if (!MapRenderer_HasTranslucentParts[batch]) continue;
		Gfx_BindTexture(Atlas1D_TexIds[batch]);
		MapRenderer_RenderTranslucentBatch(batch);
	}
	Gfx_DisableMipmaps();

	Gfx_SetDepthWrite(true);
	/* If we weren't under water, render weather after to blend properly */
	if (!inTranslucent && Env_Weather != WEATHER_SUNNY) {
		Gfx_SetAlphaTest(true);
		EnvRenderer_RenderWeather(delta);
		Gfx_SetAlphaTest(false);
	}
	Gfx_SetAlphaBlending(false);
	Gfx_SetTexturing(false);
}
