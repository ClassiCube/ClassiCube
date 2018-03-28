#include "MapRenderer.h"
#include "Block.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "WeatherRenderer.h"
#include "World.h"
#include "Vectors.h"
#include "ChunkUpdater.h"
bool inTranslucent;

ChunkInfo* MapRenderer_GetChunk(Int32 cx, Int32 cy, Int32 cz) {
	return &MapRenderer_Chunks[MapRenderer_Pack(cx, cy, cz)];
}

void MapRenderer_RefreshChunk(Int32 cx, Int32 cy, Int32 cz) {
	if (cx < 0 || cy < 0 || cz < 0 || cx >= MapRenderer_ChunksX 
		|| cy >= MapRenderer_ChunksY || cz >= MapRenderer_ChunksZ) return;

	ChunkInfo* info = &MapRenderer_Chunks[MapRenderer_Pack(cx, cy, cz)];
	if (info->AllAir) return; /* do not recreate chunks completely air */
	info->Empty = false;
	info->PendingDelete = true;
}

void MapRenderer_Update(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;
	ChunkUpdater_UpdateSortOrder();
	ChunkUpdater_UpdateChunks(deltaTime);
}

void MapRenderer_CheckWeather(Real64 deltaTime) {
	Vector3 pos = Game_CurrentCameraPos;
	Vector3I coords;
	Vector3I_Floor(&coords, &pos);

	BlockID block = World_SafeGetBlock_3I(coords);
	bool outside = coords.X < 0 || coords.Y < 0 || coords.Z < 0 || coords.X >= World_Width || coords.Z >= World_Length;
	inTranslucent = Block_Draw[block] == DRAW_TRANSLUCENT || (pos.Y < WorldEnv_EdgeHeight && outside);

	/* If we are under water, render weather before to blend properly */
	if (!inTranslucent || WorldEnv_Weather == WEATHER_SUNNY) return;
	Gfx_SetAlphaBlending(true);
	WeatherRenderer_Render(deltaTime);
	Gfx_SetAlphaBlending(false);
}

void MapRenderer_RenderNormalBatch(UInt32 batch) {
	UInt32 i;
	for (i = 0; i < MapRenderer_RenderChunksCount; i++) {
		ChunkInfo* info = MapRenderer_RenderChunks[i];
		if (info->NormalParts == NULL) continue;

		ChunkPartInfo part = info->NormalParts[batch];
		if (!part.HasVertices) continue;
		MapRenderer_HasNormalParts[batch] = true;

		Gfx_BindVb(part.VbId);
		bool drawXMin = info->DrawXMin && part.XMinCount > 0;
		bool drawXMax = info->DrawXMax && part.XMaxCount > 0;
		bool drawYMin = info->DrawYMin && part.YMinCount > 0;
		bool drawYMax = info->DrawYMax && part.YMaxCount > 0;
		bool drawZMin = info->DrawZMin && part.ZMinCount > 0;
		bool drawZMax = info->DrawZMax && part.ZMaxCount > 0;

		UInt32 offset = part.SpriteCountDiv4 << 2;
		if (drawXMin && drawXMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.XMinCount + part.XMaxCount, offset);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.XMinCount + part.XMaxCount;
		} else if (drawXMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.XMinCount, offset);
			Game_Vertices += part.XMinCount;
		} else if (drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.XMaxCount, offset + part.XMinCount);
			Game_Vertices += part.XMaxCount;
		}
		offset += part.XMinCount + part.XMaxCount;

		if (drawZMin && drawZMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.ZMinCount + part.ZMaxCount, offset);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.ZMinCount + part.ZMaxCount;
		} else if (drawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.ZMinCount, offset);
			Game_Vertices += part.ZMinCount;
		} else if (drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.ZMaxCount, offset + part.ZMinCount);
			Game_Vertices += part.ZMaxCount;
		}
		offset += part.ZMinCount + part.ZMaxCount;

		if (drawYMin && drawYMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.YMinCount + part.YMaxCount, offset);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.YMaxCount + part.YMinCount;
		} else if (drawYMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.YMinCount, offset);
			Game_Vertices += part.YMinCount;
		} else if (drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.YMaxCount, offset + part.YMinCount);
			Game_Vertices += part.YMaxCount;
		}

		if (part.SpriteCountDiv4 == 0) continue;
		UInt32 count = part.SpriteCountDiv4; /* 4 per sprite */
		Gfx_SetFaceCulling(true);
		if (info->DrawXMax || info->DrawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, 0); Game_Vertices += count;
		}
		if (info->DrawXMin || info->DrawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, count); Game_Vertices += count;
		}
		if (info->DrawXMin || info->DrawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, count * 2); Game_Vertices += count;
		}
		if (info->DrawXMax || info->DrawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(count, count * 3); Game_Vertices += count;
		}
		Gfx_SetFaceCulling(false);
	}
}

void MapRenderer_RenderNormal(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);

	UInt32 batch;
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

	MapRenderer_CheckWeather(deltaTime);
	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);
#if DEBUG_OCCLUSION
	DebugPickedPos();
#endif
}

void MapRenderer_RenderTranslucentBatch(UInt32 batch) {
	UInt32 i;
	for (i = 0; i < MapRenderer_RenderChunksCount; i++) {
		ChunkInfo* info = MapRenderer_RenderChunks[i];
		if (info->TranslucentParts == NULL) continue;

		ChunkPartInfo part = info->TranslucentParts[batch];
		if (!part.HasVertices) continue;
		MapRenderer_HasTranslucentParts[batch] = true;

		Gfx_BindVb(part.VbId);
		bool drawXMin = (inTranslucent || info->DrawXMin) && part.XMinCount > 0;
		bool drawXMax = (inTranslucent || info->DrawXMax) && part.XMaxCount > 0;
		bool drawYMin = (inTranslucent || info->DrawYMin) && part.YMinCount > 0;
		bool drawYMax = (inTranslucent || info->DrawYMax) && part.YMaxCount > 0;
		bool drawZMin = (inTranslucent || info->DrawZMin) && part.ZMinCount > 0;
		bool drawZMax = (inTranslucent || info->DrawZMax) && part.ZMaxCount > 0;

		UInt32 offset = 0;
		if (drawXMin && drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.XMinCount + part.XMaxCount, offset);
			Game_Vertices += (part.XMinCount + part.XMaxCount);
		} else if (drawXMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.XMinCount, offset);
			Game_Vertices += part.XMinCount;
		} else if (drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.XMaxCount, offset);
			Game_Vertices += part.XMaxCount;
		}
		offset += part.XMinCount + part.XMaxCount;

		if (drawZMin && drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.ZMinCount + part.ZMaxCount, offset);
			Game_Vertices += (part.ZMinCount + part.ZMaxCount);
		} else if (drawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.ZMinCount, offset);
			Game_Vertices += part.ZMinCount;
		} else if (drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.ZMaxCount, offset + part.ZMinCount);
			Game_Vertices += part.ZMaxCount;
		}
		offset += part.ZMinCount + part.ZMaxCount;

		if (drawYMin && drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.YMinCount + part.YMaxCount, offset);
			Game_Vertices += (part.YMinCount + part.YMaxCount);
		} else if (drawYMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.YMinCount, offset);
			Game_Vertices += part.YMinCount;
		} else if (drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.YMaxCount, offset + part.YMinCount);
			Game_Vertices += part.YMaxCount;
		}
	}
}

void MapRenderer_RenderTranslucent(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;

	/* First fill depth buffer */
	UInt32 vertices = Game_Vertices;
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_SetTexturing(false);
	Gfx_SetAlphaBlending(false);
	Gfx_SetColourWriteMask(false, false, false, false);

	UInt32 batch;
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
	if (!inTranslucent && WorldEnv_Weather != WEATHER_SUNNY) {
		Gfx_SetAlphaTest(true);
		WeatherRenderer_Render(deltaTime);
		Gfx_SetAlphaTest(false);
	}
	Gfx_SetAlphaBlending(false);
	Gfx_SetTexturing(false);
}