#include "MapRenderer.h"
#include "Block.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "EnvRenderer.h"
#include "World.h"
#include "Vectors.h"
#include "ChunkUpdater.h"
bool inTranslucent;

struct ChunkInfo* MapRenderer_GetChunk(Int32 cx, Int32 cy, Int32 cz) {
	return &MapRenderer_Chunks[MapRenderer_Pack(cx, cy, cz)];
}

void MapRenderer_RefreshChunk(Int32 cx, Int32 cy, Int32 cz) {
	if (cx < 0 || cy < 0 || cz < 0 || cx >= MapRenderer_ChunksX 
		|| cy >= MapRenderer_ChunksY || cz >= MapRenderer_ChunksZ) return;

	struct ChunkInfo* info = &MapRenderer_Chunks[MapRenderer_Pack(cx, cy, cz)];
	if (info->AllAir) return; /* do not recreate chunks completely air */
	info->Empty         = false;
	info->PendingDelete = true;
}

static void MapRenderer_CheckWeather(double delta) {
	Vector3 pos = Game_CurrentCameraPos;
	Vector3I coords;
	Vector3I_Floor(&coords, &pos);

	BlockID block = World_SafeGetBlock_3I(coords);
	bool outside = coords.X < 0 || coords.Y < 0 || coords.Z < 0 || coords.X >= World_Width || coords.Z >= World_Length;
	inTranslucent = Block_Draw[block] == DRAW_TRANSLUCENT || (pos.Y < Env_EdgeHeight && outside);

	/* If we are under water, render weather before to blend properly */
	if (!inTranslucent || Env_Weather == WEATHER_SUNNY) return;
	Gfx_SetAlphaBlending(true);
	EnvRenderer_RenderWeather(delta);
	Gfx_SetAlphaBlending(false);
}

static void MapRenderer_RenderNormalBatch(UInt32 batch) {
	UInt32 i, offset = MapRenderer_ChunksCount * batch;
	for (i = 0; i < MapRenderer_RenderChunksCount; i++) {
		struct ChunkInfo* info = MapRenderer_RenderChunks[i];
		if (!info->NormalParts) continue;

		struct ChunkPartInfo part = *(info->NormalParts + offset);
		if (part.Offset < 0) continue;
		MapRenderer_HasNormalParts[batch] = true;

#if !CC_BUILD_GL11
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

		Int32 offset = part.Offset + part.SpriteCount;
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
		Int32 count = part.SpriteCount >> 2; /* 4 per sprite */

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
	if (!MapRenderer_Chunks) return;
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);

	Int32 batch;
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

static void MapRenderer_RenderTranslucentBatch(UInt32 batch) {
	UInt32 i, offset = MapRenderer_ChunksCount * batch;
	for (i = 0; i < MapRenderer_RenderChunksCount; i++) {
		struct ChunkInfo* info = MapRenderer_RenderChunks[i];
		if (!info->TranslucentParts) continue;

		struct ChunkPartInfo part = *(info->TranslucentParts + offset);
		if (part.Offset < 0) continue;
		MapRenderer_HasTranslucentParts[batch] = true;

#if !CC_BUILD_GL11
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

		Int32 offset = part.Offset;
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
	if (!MapRenderer_Chunks) return;

	/* First fill depth buffer */
	UInt32 vertices = Game_Vertices;
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_SetTexturing(false);
	Gfx_SetAlphaBlending(false);
	Gfx_SetColourWriteMask(false, false, false, false);

	Int32 batch;
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
	Gfx_SetColourWriteMask(true, true, true, true);
	Gfx_SetDepthWrite(false); /* we already calculated depth values in depth pass */

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
