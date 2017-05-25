#include "MapRenderer.h"
#include "Block.h"
#include "BlockEnums.h"
#include "GameProps.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "WeatherRenderer.h"
#include "World.h"
#include "WorldEnv.h"
#include "Vectors.h"
#include "Vector3I.h"
bool inTranslucent;

ChunkInfo* MapRenderer_GetChunk(Int32 cx, Int32 cy, Int32 cz) {
	return &MapRenderer_Chunks[MapRender_Pack(cx, cy, cz)];
}

void MapRenderer_RefreshChunk(Int32 cx, Int32 cy, Int32 cz) {
	if (cx < 0 || cy < 0 || cz < 0 || cx >= MapRenderer_ChunksX 
		|| cy >= MapRenderer_ChunksY || cz >= MapRenderer_ChunksZ) return;

	ChunkInfo* info = &MapRenderer_Chunks[MapRender_Pack(cx, cy, cz)];
	if (info->AllAir) return; /* do not recreate chunks completely air */
	info->Empty = false;
	info->PendingDelete = true;
}

void MapRenderer_Update(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;
	ChunkSorter_UpdateSortOrder();
	ChunkUpdater_UpdateChunks(deltaTime);
}

void MapRenderer_RenderNormal(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	Gfx_SetTexturing(true);
	Gfx_SetAlphaTest(true);

	Int32 batch;
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (MapRenderer_PartsCount[batch] <= 0) continue;
		if (MapRenderer_HasNormalParts[batch] || MapRenderer_CheckingNormalParts[batch]) {
			Gfx_BindTexture(Atlas1D_TexIds[batch]);
			MapRenderer_RenderNormalBatch(batch);
			MapRenderer_CheckingNormalParts[batch] = false;
		}
	}

	CheckWeather(deltaTime);
	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);
#if DEBUG_OCCLUSION
	DebugPickedPos();
#endif
}

void MapRenderer_RenderTranslucent(Real64 deltaTime) {
	if (MapRenderer_Chunks == NULL) return;

	// First fill depth buffer
	Int64 vertices = Game_Vertices;
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	Gfx_SetTexturing(false);
	Gfx_SetAlphaBlending(false);
	Gfx_SetColourWrite(false);

	Int32 batch;
	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (MapRenderer_PartsCount[batch] <= 0) continue;
		if (MapRenderer_HasTranslucentParts[batch] || MapRenderer_CheckingTranslucentParts[batch]) {
			MapRenderer_RenderTranslucentBatch(batch);
			MapRenderer_CheckingTranslucentParts[batch] = false;
		}
	}
	Game_Vertices = vertices;

	// Then actually draw the transluscent blocks
	Gfx_SetAlphaBlending(true);
	Gfx_SetTexturing(true);
	Gfx_SetColourWrite(true);
	Gfx_SetDepthWrite(false); // we already calculated depth values in depth pass

	for (batch = 0; batch < MapRenderer_1DUsedCount; batch++) {
		if (MapRenderer_PartsCount[batch] <= 0) continue;
		if (!MapRenderer_HasTranslucentParts[batch]) continue;
		Gfx_BindTexture(Atlas1D_TexIds[batch]);
		MapRenderer_RenderTranslucentBatch(batch);
	}

	Gfx_SetDepthWrite(true);
	// If we weren't under water, render weather after to blend properly
	if (!inTranslucent && WorldEnv_Weather != Weather_Sunny) {
		Gfx_SetAlphaTest(true);
		WeatherRenderer_Render(deltaTime);
		Gfx_SetAlphaTest(false);
	}
	Gfx_SetAlphaBlending(false);
	Gfx_SetTexturing(false);
}

void MapRenderer_CheckWeather(Real64 deltaTime) {
	Vector3 pos = Game_CurrentCameraPos;
	Vector3I coords;
	Vector3I_Floor(&pos, &coords);

	BlockID block = World_SafeGetBlock_3I(coords);
	bool outside = !World_IsValidPos_3I(coords);
	inTranslucent = Block_Draw[block] == DrawType_Translucent || (pos.Y < WorldEnv_EdgeHeight && outside);

	// If we are under water, render weather before to blend properly
	if (!inTranslucent || WorldEnv_Weather == Weather_Sunny) return;
	Gfx_SetAlphaBlending(true);
	WeatherRenderer_Render(deltaTime);
	Gfx_SetAlphaBlending(false);
}

void MapRenderer_RenderNormalBatch(Int32 batch) {
	Int32 i;
	for (Int32 i = 0; i < MapRenderer_RenderChunksCount; i++) {
		ChunkInfo* info = MapRenderer_RenderChunks[i];
		if (info->NormalParts == NULL) continue;

		ChunkPartInfo part = info->NormalParts[batch];
		if (part.IndicesCount == 0) continue;
		MapRenderer_HasNormalParts[batch] = true;

		Gfx_BindVb(part.VbId);
		bool drawXMin = info->DrawXMin && part.LeftCount > 0;
		bool drawXMax = info->DrawXMax && part.RightCount > 0;
		bool drawYMin = info->DrawYMin && part.BottomCount > 0;
		bool drawYMax = info->DrawYMax && part.TopCount > 0;
		bool drawZMin = info->DrawZMin && part.FrontCount > 0;
		bool drawZMax = info->DrawZMax && part.BackCount > 0;

		if (drawXMin && drawXMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.LeftCount + part.RightCount, part.LeftIndex);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.LeftCount + part.RightCount;
		} else if (drawXMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.LeftCount, part.LeftIndex);
			Game_Vertices += part.LeftCount;
		} else if (drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.RightCount, part.RightIndex);
			Game_Vertices += part.RightCount;
		}

		if (drawZMin && drawZMax) {
			Gfx_SetFaceCulling(true);
			Gfx_DrawIndexedVb_TrisT2fC4b(part.FrontCount + part.BackCount, part.FrontIndex);
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.FrontCount + part.BackCount;
		} else if (drawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.FrontCount, part.FrontIndex);
			Game_Vertices += part.FrontCount;
		} else if (drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.BackCount, part.BackIndex);
			Game_Vertices += part.BackCount;
		}

		// Special handling for top and bottom face, as these can go over 65536 vertices and we need to adjust the indices in this case.
		if (drawYMin && drawYMax) {
			Gfx_SetFaceCulling(true);
			if (part.IndicesCount > maxIndices) {
				Int32 part1Count = maxIndices - part.BottomIndex;
				Gfx_DrawIndexedVb_TrisT2fC4b(part1Count, part.BottomIndex);
				Gfx_DrawIndexedVb_TrisT2fC4b(part.BottomCount + part.TopCount - part1Count, maxVertex, 0);
			} else {
				Gfx_DrawIndexedVb_TrisT2fC4b(part.BottomCount + part.TopCount, part.BottomIndex);
			}
			Gfx_SetFaceCulling(false);
			Game_Vertices += part.TopCount + part.BottomCount;
		} else if (drawYMin) {
			Int32 part1Count;
			if (part.IndicesCount > Gfx_MaxIndices &&
				(part1Count = Gfx_MaxIndices - part.BottomIndex) < part.BottomCount) {
				Gfx_DrawIndexedVb_TrisT2fC4b(part1Count, part.BottomIndex);
				Gfx_DrawIndexedVb_TrisT2fC4b(part.BottomCount - part1Count, maxVertex, 0);
			} else {
				Gfx_DrawIndexedVb_TrisT2fC4b(part.BottomCount, part.BottomIndex);
			}
			Game_Vertices += part.BottomCount;
		} else if (drawYMax) {
			Int32 part1Count;
			if (part.IndicesCount > Gfx_MaxIndices &&
				(part1Count = Gfx_MaxIndices - part.TopIndex) < part.TopCount) {
				Gfx_DrawIndexedVb_TrisT2fC4b(part1Count, part.TopIndex);
				Gfx_DrawIndexedVb_TrisT2fC4b(part.TopCount - part1Count, maxVertex, 0);
			} else {
				Gfx_DrawIndexedVb_TrisT2fC4b(part.TopCount, part.TopIndex);
			}
			Game_Vertices += part.TopCount;
		}

		if (part.SpriteCount == 0) continue;
		Int32 count = part.SpriteCount / 4; // 4 per sprite
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

void RenderTranslucentBatch(Int32 batch) {
	Int32 i;
	for (Int32 i = 0; i < MapRenderer_RenderChunksCount; i++) {
		ChunkInfo* info = MapRenderer_RenderChunks[i];
		if (info->TranslucentParts == NULL) continue;

		ChunkPartInfo part = info->TranslucentParts[batch];
		if (part.IndicesCount == 0) continue;
		MapRenderer_HasTranslucentParts[batch] = true;

		Gfx_BindVb(part.VbId);
		bool drawXMin = (inTranslucent || info->DrawXMin) && part.LeftCount > 0;
		bool drawXMax = (inTranslucent || info->DrawXMax) && part.RightCount > 0;
		bool drawYMin = (inTranslucent || info->DrawYMin) && part.BottomCount > 0;
		bool drawYMax = (inTranslucent || info->DrawYMax) && part.TopCount > 0;
		bool drawZMin = (inTranslucent || info->DrawZMin) && part.FrontCount > 0;
		bool drawZMax = (inTranslucent || info->DrawZMax) && part.BackCount > 0;

		if (drawXMin && drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.LeftCount + part.RightCount, part.LeftIndex);
			Game_Vertices += (part.LeftCount + part.RightCount);
		} else if (drawXMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.LeftCount, part.LeftIndex);
			Game_Vertices += part.LeftCount;
		} else if (drawXMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.RightCount, part.RightIndex);
			Game_Vertices += part.RightCount;
		}

		if (drawZMin && drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.FrontCount + part.BackCount, part.FrontIndex);
			Game_Vertices += (part.FrontCount + part.BackCount);
		} else if (drawZMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.FrontCount, part.FrontIndex);
			Game_Vertices += part.FrontCount;
		} else if (drawZMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.BackCount, part.BackIndex);
			Game_Vertices += part.BackCount;
		}

		if (drawYMin && drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.BottomCount + part.TopCount, part.BottomIndex);
			Game_Vertices += (part.BottomCount + part.TopCount);
		} else if (drawYMin) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.BottomCount, part.BottomIndex);
			Game_Vertices += part.BottomCount;
		} else if (drawYMax) {
			Gfx_DrawIndexedVb_TrisT2fC4b(part.TopCount, part.TopIndex);
			Game_Vertices += part.TopCount;
		}
	}
}