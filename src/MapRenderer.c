#include "MapRenderer.h"
#include "Block.h"
#include "Game.h"
#include "Graphics.h"
#include "EnvRenderer.h"
#include "World.h"
#include "Camera.h"
#include "ChunkUpdater.h"
bool inTranslucent;

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

static void MapRenderer_RenderNormalBatch(int batch) {
	int batchOffset = MapRenderer_ChunksCount * batch;
	struct ChunkInfo* info;
	struct ChunkPartInfo part;
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
		bool drawXMin = info->DrawXMin && part.Counts[FACE_XMIN];
		bool drawXMax = info->DrawXMax && part.Counts[FACE_XMAX];
		bool drawYMin = info->DrawYMin && part.Counts[FACE_YMIN];
		bool drawYMax = info->DrawYMax && part.Counts[FACE_YMAX];
		bool drawZMin = info->DrawZMin && part.Counts[FACE_ZMIN];
		bool drawZMax = info->DrawZMax && part.Counts[FACE_ZMAX];

		offset = part.Offset + part.SpriteCount;
		if (drawXMin && drawXMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX], offset);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];
		} else if (drawXMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_XMIN], offset);
			Game_Vertices += part.Counts[FACE_XMIN];
		} else if (drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_XMAX], offset + part.Counts[FACE_XMIN]);
			Game_Vertices += part.Counts[FACE_XMAX];
		}
		offset += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];

		if (drawZMin && drawZMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX], offset);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];
		} else if (drawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_ZMIN], offset);
			Game_Vertices += part.Counts[FACE_ZMIN];
		} else if (drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_ZMAX], offset + part.Counts[FACE_ZMIN]);
			Game_Vertices += part.Counts[FACE_ZMAX];
		}
		offset += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];

		if (drawYMin && drawYMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_YMIN] + part.Counts[FACE_YMAX], offset);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.Counts[FACE_YMAX] + part.Counts[FACE_YMIN];
		} else if (drawYMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_YMIN], offset);
			Game_Vertices += part.Counts[FACE_YMIN];
		} else if (drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_YMAX], offset + part.Counts[FACE_YMIN]);
			Game_Vertices += part.Counts[FACE_YMAX];
		}

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

static void MapRenderer_RenderTranslucentBatch(int batch) {
	int batchOffset = MapRenderer_ChunksCount * batch;
	struct ChunkInfo* info;
	struct ChunkPartInfo part;
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
		bool drawXMin = (inTranslucent || info->DrawXMin) && part.Counts[FACE_XMIN];
		bool drawXMax = (inTranslucent || info->DrawXMax) && part.Counts[FACE_XMAX];
		bool drawYMin = (inTranslucent || info->DrawYMin) && part.Counts[FACE_YMIN];
		bool drawYMax = (inTranslucent || info->DrawYMax) && part.Counts[FACE_YMAX];
		bool drawZMin = (inTranslucent || info->DrawZMin) && part.Counts[FACE_ZMIN];
		bool drawZMax = (inTranslucent || info->DrawZMax) && part.Counts[FACE_ZMAX];

		offset = part.Offset;
		if (drawXMin && drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX], offset);
			Game_Vertices += (part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX]);
		} else if (drawXMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_XMIN], offset);
			Game_Vertices += part.Counts[FACE_XMIN];
		} else if (drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_XMAX], offset + part.Counts[FACE_XMIN]);
			Game_Vertices += part.Counts[FACE_XMAX];
		}
		offset += part.Counts[FACE_XMIN] + part.Counts[FACE_XMAX];

		if (drawZMin && drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX], offset);
			Game_Vertices += (part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX]);
		} else if (drawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_ZMIN], offset);
			Game_Vertices += part.Counts[FACE_ZMIN];
		} else if (drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_ZMAX], offset + part.Counts[FACE_ZMIN]);
			Game_Vertices += part.Counts[FACE_ZMAX];
		}
		offset += part.Counts[FACE_ZMIN] + part.Counts[FACE_ZMAX];

		if (drawYMin && drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_YMIN] + part.Counts[FACE_YMAX], offset);
			Game_Vertices += (part.Counts[FACE_YMIN] + part.Counts[FACE_YMAX]);
		} else if (drawYMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_YMIN], offset);
			Game_Vertices += part.Counts[FACE_YMIN];
		} else if (drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.Counts[FACE_YMAX], offset + part.Counts[FACE_YMIN]);
			Game_Vertices += part.Counts[FACE_YMAX];
		}
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
