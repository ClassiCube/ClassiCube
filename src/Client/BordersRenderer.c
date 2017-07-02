#include "BordersRenderer.h"
#include "2DStructs.h"
#include "GameProps.h"
#include "World.h"
#include "WorldEnv.h"
#include "Block.h"
#include "BlockEnums.h"
#include "GraphicsAPI.h"
#include "GraphicsCommon.h"
#include "GraphicsEnums.h"
#include "MiscEvents.h"
#include "TerrainAtlas2D.h"
#include "ExtMath.h"
#include "Platform.h"
#include "Funcs.h"

GfxResourceID borders_sidesVb = -1, borders_edgesVb = -1;
GfxResourceID borders_edgeTexId, borders_sideTexId;
Int32 borders_sidesVertices, borders_edgesVertices;
bool borders_fullBrightSides, borders_fullBrightEdge;
Rectangle borders_rects[4];
TextureLoc borders_lastEdgeTexLoc, borders_lastSideTexLoc;

/* Creates game component implementation. */
IGameComponent BordersRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = BordersRenderer_Init;
	comp.Free = BordersRenderer_Free;
	comp.OnNewMap = BordersRenderer_Reset;
	comp.Reset = BordersRenderer_Reset;
	comp.OnNewMapLoaded = BordersRenderer_ResetSidesAndEdges;
	return comp;
}

void BordersRenderer_UseLegacyMode(bool legacy) {
	BordersRenderer_Legacy = legacy;
	BordersRenderer_ResetSidesAndEdges();
}

/* Avoid code duplication in sides and edge rendering */
#define BordersRenderer_SetupState(block, texId, vb) \
if (vb == -1) { return; }\
\
GfxCommon_SetupAlphaState(Block_Draw[block]);\
Gfx_SetTexturing(true);\
\
Gfx_BindTexture(texId);\
Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);\
Gfx_BindVb(vb);

#define BordersRenderer_ResetState(block) \
GfxCommon_RestoreAlphaState(Block_Draw[block]);\
Gfx_SetTexturing(false);

void BordersRenderer_RenderSides(Real64 delta) {
	BlockID block = WorldEnv_SidesBlock;
	BordersRenderer_SetupState(block, borders_sideTexId, borders_sidesVb)
	Gfx_DrawIndexedVb(DrawMode_Triangles, borders_sidesVertices * 6 / 4, 0);
	BordersRenderer_ResetState(block);
}

void BordersRenderer_RenderEdges(Real64 delta) {
	if (borders_edgesVb == -1) return;
	BlockID block = WorldEnv_EdgeBlock;
	BordersRenderer_SetupState(block, borders_sideTexId, borders_sidesVb)

	/* Do not draw water when we cannot see it. */
	/* Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps. */
	Vector3 camPos = Game_CurrentCameraPos;
	Int32 yVisible = min(0, WorldEnv_SidesHeight);
	if (camPos.Y >= yVisible) {
		Gfx_DrawIndexedVb(DrawMode_Triangles, borders_edgesVertices * 6 / 4, 0);
	}
	BordersRenderer_ResetState(block);
}


void BordersRenderer_Init(void) {
	EventHandler_RegisterInt32(WorldEvents_EnvVarChanged, BordersRenderer_EnvVariableChanged);
	EventHandler_RegisterVoid(GfxEvents_ViewDistanceChanged, BordersRenderer_ResetSidesAndEdges);
	EventHandler_RegisterVoid(TextureEvents_AtlasChanged, BordersRenderer_ResetTextures);
	EventHandler_RegisterVoid(Gfx_ContextLost, BordersRenderer_ContextLost);
	EventHandler_RegisterVoid(Gfx_ContextRecreated, BordersRenderer_ContextRecreated);
}

void BordersRenderer_Free(void) {
	BordersRenderer_ContextLost();
	EventHandler_UnregisterInt32(WorldEvents_EnvVarChanged, BordersRenderer_EnvVariableChanged);
	EventHandler_UnregisterVoid(GfxEvents_ViewDistanceChanged, BordersRenderer_ResetSidesAndEdges);
	EventHandler_UnregisterVoid(TextureEvents_AtlasChanged, BordersRenderer_ResetTextures);
	EventHandler_UnregisterVoid(Gfx_ContextLost, BordersRenderer_ContextLost);
	EventHandler_UnregisterVoid(Gfx_ContextRecreated, BordersRenderer_ContextRecreated);
}

void BordersRenderer_Reset(void) {
	Gfx_DeleteVb(&borders_sidesVb);
	Gfx_DeleteVb(&borders_edgesVb);
	BordersRenderer_MakeTexture(&borders_edgeTexId, &borders_lastEdgeTexLoc, WorldEnv_EdgeBlock);
	BordersRenderer_MakeTexture(&borders_sideTexId, &borders_lastSideTexLoc, WorldEnv_SidesBlock);
}


void BordersRenderer_EnvVariableChanged(EnvVar envVar) {
	if (envVar == EnvVar_EdgeBlock) {
		BordersRenderer_MakeTexture(&borders_edgeTexId, &borders_lastEdgeTexLoc, WorldEnv_EdgeBlock);
		BordersRenderer_ResetEdges();
	} else if (envVar == EnvVar_SidesBlock) {
		BordersRenderer_MakeTexture(&borders_sideTexId, &borders_lastSideTexLoc, WorldEnv_SidesBlock);
		BordersRenderer_ResetSides();
	} else if (envVar == EnvVar_EdgeHeight || envVar == EnvVar_SidesOffset) {
		BordersRenderer_ResetSidesAndEdges();
	} else if (envVar == EnvVar_SunCol) {
		BordersRenderer_ResetEdges();
	} else if (envVar == EnvVar_ShadowCol) {
		BordersRenderer_ResetSides();
	}
}

void BordersRenderer_ContextLost(void) {
	Gfx_DeleteVb(&borders_sidesVb);
	Gfx_DeleteVb(&borders_edgesVb);
	Gfx_DeleteTexture(&borders_edgeTexId);
	Gfx_DeleteTexture(&borders_sideTexId);
}

void BordersRenderer_ContextRecreated(void) {
	BordersRenderer_ResetSides();
	BordersRenderer_ResetEdges();
	BordersRenderer_ResetTextures();
}


void BordersRenderer_ResetTextures(void) {
	borders_lastEdgeTexLoc = UInt8_MaxValue;
	borders_lastSideTexLoc = UInt8_MaxValue;
	BordersRenderer_MakeTexture(&borders_edgeTexId, &borders_lastEdgeTexLoc, WorldEnv_EdgeBlock);
	BordersRenderer_MakeTexture(&borders_sideTexId, &borders_lastSideTexLoc, WorldEnv_SidesBlock);
}

void BordersRenderer_ResetSidesAndEdges(void) {
	BordersRenderer_CalculateRects((Int32)Game_ViewDistance);
	BordersRenderer_ContextRecreated();
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

#define borders_HorOffset(block) (Block_RenderMinBB[block].X - Block_MinBB[block].X)
#define borders_YOffset(block) (Block_RenderMinBB[block].Y - Block_MinBB[block].Y)

void BordersRenderer_RebuildSides(Int32 y, Int32 axisSize) {
	BlockID block = WorldEnv_SidesBlock;
	borders_sidesVertices = 0;
	if (Block_Draw[block] == DrawType_Gas) return;

	Int32 i;
	for (i = 0; i < 4; i++) {
		Rectangle r = borders_rects[i];
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
	PackedCol col = borders_fullBrightSides ? PackedCol_White : WorldEnv_ShadowCol;
	Block_Tint(col, block)

	for (i = 0; i < 4; i++) {
		Rectangle r = borders_rects[i];
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

	borders_sidesVb = Gfx_CreateVb(v, VertexFormat_P3fT2fC4b, borders_sidesVertices);
	if (borders_sidesVertices > 4096) Platform_MemFree(ptr);
}

void BordersRenderer_RebuildEdges(Int32 y, Int32 axisSize) {
	BlockID block = WorldEnv_EdgeBlock;
	borders_edgesVertices = 0;
	if (Block_Draw[block] == DrawType_Gas) return;

	Int32 i;
	for (i = 0; i < 4; i++) {
		Rectangle r = borders_rects[i];
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
	PackedCol col = borders_fullBrightEdge ? PackedCol_White : WorldEnv_SunCol;
	Block_Tint(col, block)

	for (i = 0; i < 4; i++) {
		Rectangle r = borders_rects[i];
		BordersRenderer_DrawY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, (Real32)y, axisSize, col,
			borders_HorOffset(block), borders_YOffset(block), &temp);
	}

	borders_edgesVb = Gfx_CreateVb(ptr, VertexFormat_P3fT2fC4b, borders_edgesVertices);
	if (borders_edgesVertices > 4096) Platform_MemFree(ptr);
}

void BordersRenderer_DrawX(Int32 x, Int32 z1, Int32 z2, Int32 y1, Int32 y2, Int32 axisSize, PackedCol col, VertexP3fT2fC4b** v) {
	Int32 endZ = z2, endY = y2, startY = y1;
	VertexP3fT2fC4b* ptr = *v;

	for (; z1 < endZ; z1 += axisSize) {
		z2 = z1 + axisSize;
		if (z2 > endZ) z2 = endZ;
		y1 = startY;
		for (; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			TextureRec rec = TextureRec_FromPoints(0, 0, (Real32)z2 - (Real32)z1, (Real32)y2 - (Real32)y1);
			VertexP3fT2fC4b_Set(ptr, (Real32)x, (Real32)y1, (Real32)z1, rec.U1, rec.V2, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x, (Real32)y2, (Real32)z1, rec.U1, rec.V1, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x, (Real32)y2, (Real32)z2, rec.U2, rec.V1, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x, (Real32)y1, (Real32)z2, rec.U2, rec.V2, col); ptr++;
		}
	}
	*v = ptr;
}

void BordersRenderer_DrawZ(Int32 z, Int32 x1, Int32 x2, Int32 y1, Int32 y2, Int32 axisSize, PackedCol col, VertexP3fT2fC4b** v) {
	Int32 endX = x2, endY = y2, startY = y1;
	VertexP3fT2fC4b* ptr = *v;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		y1 = startY;
		for (; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			TextureRec rec = TextureRec_FromPoints(0, 0, (Real32)x2 - (Real32)x1, (Real32)y2 - (Real32)y1);
			VertexP3fT2fC4b_Set(ptr, (Real32)x1, (Real32)y1, (Real32)z, rec.U1, rec.V2, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x1, (Real32)y2, (Real32)z, rec.U1, rec.V1, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x2, (Real32)y2, (Real32)z, rec.U2, rec.V1, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x2, (Real32)y1, (Real32)z, rec.U2, rec.V2, col); ptr++;
		}
	}
	*v = ptr;
}

void BordersRenderer_DrawY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Real32 y, Int32 axisSize, PackedCol col, Real32 offset, Real32 yOffset, VertexP3fT2fC4b** v) {
	Int32 endX = x2, endZ = z2, startZ = z1;
	VertexP3fT2fC4b* ptr = *v;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		z1 = startZ;
		for (; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			TextureRec rec = TextureRec_FromPoints(0, 0, (Real32)x2 - (Real32)x1, (Real32)z2 - (Real32)z1);
			VertexP3fT2fC4b_Set(ptr, (Real32)x1 + offset, y + yOffset, (Real32)z1 + offset, rec.U1, rec.V1, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x1 + offset, y + yOffset, (Real32)z2 + offset, rec.U1, rec.V2, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x2 + offset, y + yOffset, (Real32)z2 + offset, rec.U2, rec.V2, col); ptr++;
			VertexP3fT2fC4b_Set(ptr, (Real32)x2 + offset, y + yOffset, (Real32)z1 + offset, rec.U2, rec.V1, col); ptr++;
		}
	}
	*v = ptr;
}


void BordersRenderer_CalculateRects(Int32 extent) {
	extent = Math_AdjViewDist(extent);
	borders_rects[0] = Rectangle_Make(-extent, -extent, extent + World_Width + extent, extent);
	borders_rects[1] = Rectangle_Make(-extent, World_Length, extent + World_Width + extent, extent);

	borders_rects[2] = Rectangle_Make(-extent, 0, extent, World_Length);
	borders_rects[3] = Rectangle_Make(World_Width, 0, extent, World_Length);
}

void BordersRenderer_MakeTexture(GfxResourceID* texId, TextureLoc* lastTexLoc, BlockID block) {
	TextureLoc texLoc = Block_GetTexLoc(block, Face_YMax);
	if (texLoc == *lastTexLoc || Gfx_LostContext) return;
	*lastTexLoc = texLoc;

	Gfx_DeleteTexture(texId);
	*texId = Atlas2D_LoadTextureElement(texLoc);
}