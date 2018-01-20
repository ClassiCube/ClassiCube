#include "BordersRenderer.h"
#include "2DStructs.h"
#include "Game.h"
#include "World.h"
#include "Block.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "GraphicsEnums.h"
#include "Event.h"
#include "TerrainAtlas.h"
#include "ExtMath.h"
#include "Platform.h"
#include "Funcs.h"
#include "Utils.h"

GfxResourceID borders_sidesVb, borders_edgesVb;
GfxResourceID borders_edgeTexId, borders_sideTexId;
Int32 borders_sidesVertices, borders_edgesVertices;
bool borders_fullBrightSides, borders_fullBrightEdge;
Rectangle2D borders_rects[4];
TextureLoc borders_lastEdgeTexLoc, borders_lastSideTexLoc;


/* Avoid code duplication in sides and edge rendering */
#define BordersRenderer_SetupState(block, texId, vb) \
if (vb == NULL) { return; }\
\
Gfx_SetTexturing(true);\
GfxCommon_SetupAlphaState(Block_Draw[block]);\
Gfx_EnableMipmaps();\
\
Gfx_BindTexture(texId);\
Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);\
Gfx_BindVb(vb);

#define BordersRenderer_ResetState(block) \
Gfx_DisableMipmaps();\
GfxCommon_RestoreAlphaState(Block_Draw[block]);\
Gfx_SetTexturing(false);

void BordersRenderer_RenderSides(Real64 delta) {
	BlockID block = WorldEnv_SidesBlock;
	BordersRenderer_SetupState(block, borders_sideTexId, borders_sidesVb);
	Gfx_DrawVb_IndexedTris(borders_sidesVertices);
	BordersRenderer_ResetState(block);
}

void BordersRenderer_RenderEdges(Real64 delta) {
	if (borders_edgesVb == NULL) return;
	BlockID block = WorldEnv_EdgeBlock;
	BordersRenderer_SetupState(block, borders_sideTexId, borders_sidesVb);

	/* Do not draw water when we cannot see it. */
	/* Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps. */
	Vector3 camPos = Game_CurrentCameraPos;
	Int32 yVisible = min(0, WorldEnv_SidesHeight);
	if (camPos.Y >= yVisible) {
		Gfx_DrawVb_IndexedTris(borders_edgesVertices);
	}
	BordersRenderer_ResetState(block);
}


void BordersRenderer_MakeTexture(GfxResourceID* texId, TextureLoc* lastTexLoc, BlockID block) {
	TextureLoc texLoc = Block_GetTexLoc(block, FACE_YMAX);
	if (texLoc == *lastTexLoc || Gfx_LostContext) return;
	*lastTexLoc = texLoc;

	Gfx_DeleteTexture(texId);
	*texId = Atlas2D_LoadTextureElement(texLoc);
}

void BordersRenderer_CalculateRects(Int32 extent) {
	extent = Utils_AdjViewDist(extent);
	borders_rects[0] = Rectangle2D_Make(-extent, -extent, extent + World_Width + extent, extent);
	borders_rects[1] = Rectangle2D_Make(-extent, World_Length, extent + World_Width + extent, extent);

	borders_rects[2] = Rectangle2D_Make(-extent, 0, extent, World_Length);
	borders_rects[3] = Rectangle2D_Make(World_Width, 0, extent, World_Length);
}

void BordersRenderer_ResetTextures(void* obj) {
	borders_lastEdgeTexLoc = UInt8_MaxValue;
	borders_lastSideTexLoc = UInt8_MaxValue;
	BordersRenderer_MakeTexture(&borders_edgeTexId, &borders_lastEdgeTexLoc, WorldEnv_EdgeBlock);
	BordersRenderer_MakeTexture(&borders_sideTexId, &borders_lastSideTexLoc, WorldEnv_SidesBlock);
}



#define borders_HorOffset(block) (Block_RenderMinBB[block].X - Block_MinBB[block].X)
#define borders_YOffset(block) (Block_RenderMinBB[block].Y - Block_MinBB[block].Y)

void BordersRenderer_DrawX(Int32 x, Int32 z1, Int32 z2, Int32 y1, Int32 y2, Int32 axisSize, PackedCol col, VertexP3fT2fC4b** vertices) {
	Int32 endZ = z2, endY = y2, startY = y1;
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v;
	v.X = (Real32)x; v.Col = col;

	for (; z1 < endZ; z1 += axisSize) {
		z2 = z1 + axisSize;
		if (z2 > endZ) z2 = endZ;
		y1 = startY;
		for (; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			Real32 u2 = (Real32)z2 - (Real32)z1, v2 = (Real32)y2 - (Real32)y1;
			v.Y = (Real32)y1; v.Z = (Real32)z1; v.U = 0.0f; v.V = v2;   *ptr = v; ptr++;
			v.Y = (Real32)y2;                               v.V = 0.0f; *ptr = v; ptr++;
			                  v.Z = (Real32)z2; v.U = u2;               *ptr = v; ptr++;
			v.Y = (Real32)y1;                               v.V = v2;   *ptr = v; ptr++;
		}
	}
	*vertices = ptr;
}

void BordersRenderer_DrawZ(Int32 z, Int32 x1, Int32 x2, Int32 y1, Int32 y2, Int32 axisSize, PackedCol col, VertexP3fT2fC4b** vertices) {
	Int32 endX = x2, endY = y2, startY = y1;
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v;
	v.Z = (Real32)z; v.Col = col;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		y1 = startY;
		for (; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			Real32 u2 = (Real32)x2 - (Real32)x1, v2 = (Real32)y2 - (Real32)y1;
			v.X = (Real32)x1; v.Y = (Real32)y1; v.U = 0.0f; v.V = v2;   *ptr = v; ptr++;
			                  v.Y = (Real32)y2;             v.V = 0.0f; *ptr = v; ptr++;
			v.X = (Real32)x2;                   v.U = u2;               *ptr = v; ptr++;
			                  v.Y = (Real32)y1;             v.V = v2;   *ptr = v; ptr++;
		}
	}
	*vertices = ptr;
}

void BordersRenderer_DrawY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Real32 y, Int32 axisSize, PackedCol col, Real32 offset, Real32 yOffset, VertexP3fT2fC4b** vertices) {
	Int32 endX = x2, endZ = z2, startZ = z1;
	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v;
	v.Y = y + yOffset; v.Col = col;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		z1 = startZ;
		for (; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			Real32 u2 = (Real32)x2 - (Real32)x1, v2 = (Real32)z2 - (Real32)z1;
			v.X = (Real32)x1 + offset; v.Z = (Real32)z1 + offset; v.U = 0.0f; v.V = 0.0f; *ptr = v; ptr++;
			                           v.Z = (Real32)z2 + offset;             v.V = v2;   *ptr = v; ptr++;
			v.X = (Real32)x2 + offset;                            v.U = u2;               *ptr = v; ptr++;
			                           v.Z = (Real32)z1 + offset;             v.V = 0.0f; *ptr = v; ptr++;
		}
	}
	*vertices = ptr;
}

void BordersRenderer_RebuildSides(Int32 y, Int32 axisSize) {
	BlockID block = WorldEnv_SidesBlock;
	borders_sidesVertices = 0;
	if (Block_Draw[block] == DRAW_GAS) return;

	Int32 i;
	for (i = 0; i < 4; i++) {
		Rectangle2D r = borders_rects[i];
		borders_sidesVertices += Math_CountVertices(r.Width, r.Height, axisSize); /* YQuads outside */
	}
	borders_sidesVertices += Math_CountVertices(World_Width, World_Length, axisSize); /* YQuads beneath map */
	borders_sidesVertices += 2 * Math_CountVertices(World_Width, Math_AbsI(y), axisSize); /* ZQuads */
	borders_sidesVertices += 2 * Math_CountVertices(World_Length, Math_AbsI(y), axisSize); /* XQuads */

	VertexP3fT2fC4b v[4096];
	VertexP3fT2fC4b* ptr = v;
	if (borders_sidesVertices > 4096) {
		ptr = Platform_MemAlloc(borders_sidesVertices * sizeof(VertexP3fT2fC4b));
		if (ptr == NULL) ErrorHandler_Fail("BordersRenderer_Sides - failed to allocate memory");
	}
	VertexP3fT2fC4b* temp = ptr;

	borders_fullBrightSides = Block_FullBright[block];
	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col = borders_fullBrightSides ? white : WorldEnv_ShadowCol;
	Block_Tint(col, block)

	for (i = 0; i < 4; i++) {
		Rectangle2D r = borders_rects[i];
		BordersRenderer_DrawY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, (Real32)y, axisSize, col,
			0, borders_YOffset(block), &temp);
	}

	/* Work properly for when ground level is below 0 */
	Int32 y1 = 0, y2 = y;
	if (y < 0) { y1 = y; y2 = 0; }
	BordersRenderer_DrawY(0, 0, World_Width, World_Length, 0, axisSize, col, 0, 0, &temp);
	BordersRenderer_DrawZ(0, 0, World_Width, y1, y2, axisSize, col, &temp);
	BordersRenderer_DrawZ(World_Length, 0, World_Width, y1, y2, axisSize, col, &temp);
	BordersRenderer_DrawX(0, 0, World_Length, y1, y2, axisSize, col, &temp);
	BordersRenderer_DrawX(World_Width, 0, World_Length, y1, y2, axisSize, col, &temp);

	borders_sidesVb = Gfx_CreateVb(v, VERTEX_FORMAT_P3FT2FC4B, borders_sidesVertices);
	if (borders_sidesVertices > 4096) Platform_MemFree(ptr);
}

void BordersRenderer_RebuildEdges(Int32 y, Int32 axisSize) {
	BlockID block = WorldEnv_EdgeBlock;
	borders_edgesVertices = 0;
	if (Block_Draw[block] == DRAW_GAS) return;

	Int32 i;
	for (i = 0; i < 4; i++) {
		Rectangle2D r = borders_rects[i];
		borders_edgesVertices += Math_CountVertices(r.Width, r.Height, axisSize); /* YPlanes outside */
	}

	VertexP3fT2fC4b v[4096];
	VertexP3fT2fC4b* ptr = v;
	if (borders_edgesVertices > 4096) {
		ptr = Platform_MemAlloc(borders_edgesVertices * sizeof(VertexP3fT2fC4b));
		if (ptr == NULL) ErrorHandler_Fail("BordersRenderer_Edges - failed to allocate memory");
	}
	VertexP3fT2fC4b* temp = ptr;

	borders_fullBrightEdge = Block_FullBright[block];
	PackedCol white = PACKEDCOL_WHITE;
	PackedCol col = borders_fullBrightEdge ? white : WorldEnv_SunCol;
	Block_Tint(col, block)

	for (i = 0; i < 4; i++) {
		Rectangle2D r = borders_rects[i];
		BordersRenderer_DrawY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, (Real32)y, axisSize, col,
			borders_HorOffset(block), borders_YOffset(block), &temp);
	}

	borders_edgesVb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FT2FC4B, borders_edgesVertices);
	if (borders_edgesVertices > 4096) Platform_MemFree(ptr);
}


void BordersRenderer_ResetSides(void) {
	if (World_Blocks == NULL || Gfx_LostContext) return;
	Gfx_DeleteVb(&borders_sidesVb);
	BordersRenderer_RebuildSides(WorldEnv_SidesHeight, BordersRenderer_Legacy ? 128 : 65536);
}

void BordersRenderer_ResetEdges(void) {
	if (World_Blocks == NULL || Gfx_LostContext) return;
	Gfx_DeleteVb(&borders_edgesVb);
	BordersRenderer_RebuildEdges(WorldEnv_EdgeHeight, BordersRenderer_Legacy ? 128 : 65536);
}

void BordersRenderer_ContextLost(void* obj) {
	Gfx_DeleteVb(&borders_sidesVb);
	Gfx_DeleteVb(&borders_edgesVb);
	Gfx_DeleteTexture(&borders_edgeTexId);
	Gfx_DeleteTexture(&borders_sideTexId);
}

void BordersRenderer_ContextRecreated(void* obj) {
	BordersRenderer_ResetSides();
	BordersRenderer_ResetEdges();
	BordersRenderer_ResetTextures(NULL);
}

void BordersRenderer_ResetSidesAndEdges(void) {
	BordersRenderer_CalculateRects((Int32)Game_ViewDistance);
	BordersRenderer_ContextRecreated(NULL);
}
void BordersRenderer_ResetSidesAndEdges_Handler(void* obj) {
	BordersRenderer_ResetSidesAndEdges();
}

void BordersRenderer_EnvVariableChanged(void* obj, Int32 envVar) {
	if (envVar == ENV_VAR_EDGE_BLOCK) {
		BordersRenderer_MakeTexture(&borders_edgeTexId, &borders_lastEdgeTexLoc, WorldEnv_EdgeBlock);
		BordersRenderer_ResetEdges();
	} else if (envVar == ENV_VAR_SIDES_BLOCK) {
		BordersRenderer_MakeTexture(&borders_sideTexId, &borders_lastSideTexLoc, WorldEnv_SidesBlock);
		BordersRenderer_ResetSides();
	} else if (envVar == ENV_VAR_EDGE_HEIGHT || envVar == ENV_VAR_SIDES_OFFSET) {
		BordersRenderer_ResetSidesAndEdges();
	} else if (envVar == ENV_VAR_SUN_COL) {
		BordersRenderer_ResetEdges();
	} else if (envVar == ENV_VAR_SHADOW_COL) {
		BordersRenderer_ResetSides();
	}
}

void BordersRenderer_UseLegacyMode(bool legacy) {
	BordersRenderer_Legacy = legacy;
	BordersRenderer_ResetSidesAndEdges();
}

void BordersRenderer_Init(void) {
	Event_RegisterInt32(&WorldEvents_EnvVarChanged,    NULL, BordersRenderer_EnvVariableChanged);
	Event_RegisterVoid(&GfxEvents_ViewDistanceChanged, NULL, BordersRenderer_ResetSidesAndEdges_Handler);
	Event_RegisterVoid(&TextureEvents_AtlasChanged,    NULL, BordersRenderer_ResetTextures);
	Event_RegisterVoid(&GfxEvents_ContextLost,         NULL, BordersRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,    NULL, BordersRenderer_ContextRecreated);
}

void BordersRenderer_Free(void) {
	BordersRenderer_ContextLost(NULL);
	Event_UnregisterInt32(&WorldEvents_EnvVarChanged,    NULL, BordersRenderer_EnvVariableChanged);
	Event_UnregisterVoid(&GfxEvents_ViewDistanceChanged, NULL, BordersRenderer_ResetSidesAndEdges_Handler);
	Event_UnregisterVoid(&TextureEvents_AtlasChanged,    NULL, BordersRenderer_ResetTextures);
	Event_UnregisterVoid(&GfxEvents_ContextLost,         NULL, BordersRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,    NULL, BordersRenderer_ContextRecreated);
}

void BordersRenderer_Reset(void) {
	Gfx_DeleteVb(&borders_sidesVb);
	Gfx_DeleteVb(&borders_edgesVb);
	BordersRenderer_MakeTexture(&borders_edgeTexId, &borders_lastEdgeTexLoc, WorldEnv_EdgeBlock);
	BordersRenderer_MakeTexture(&borders_sideTexId, &borders_lastSideTexLoc, WorldEnv_SidesBlock);
}


IGameComponent BordersRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = BordersRenderer_Init;
	comp.Free = BordersRenderer_Free;
	comp.OnNewMap = BordersRenderer_Reset;
	comp.Reset = BordersRenderer_Reset;
	comp.OnNewMapLoaded = BordersRenderer_ResetSidesAndEdges;
	return comp;
}