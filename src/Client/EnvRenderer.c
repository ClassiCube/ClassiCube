#include "EnvRenderer.h"
#include "ExtMath.h"
#include "World.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "Physics.h"
#include "Block.h"
#include "Platform.h"
#include "SkyboxRenderer.h"
#include "Event.h"
#include "Utils.h"
#include "VertexStructs.h"
#include "Game.h"
#include "ErrorHandler.h"
#include "Stream.h"

GfxResourceID env_cloudsVb, env_skyVb, env_cloudsTex;
Int32 env_cloudVertices, env_skyVertices;

Real32 EnvRenderer_BlendFactor(Real32 x) {
	/* return -0.05 + 0.22 * (Math_Log(x) * 0.25f); */
	Real64 blend = -0.13 + 0.28 * (Math_Log(x) * 0.25);
	if (blend < 0.0) blend = 0.0;
	if (blend > 1.0) blend = 1.0;
	return (Real32)blend;
}

void EnvRenderer_BlockOn(Real32* fogDensity, PackedCol* fogCol) {
	Vector3 pos = Game_CurrentCameraPos;
	Vector3I coords;
	Vector3I_Floor(&coords, &pos);     /* coords = floor(pos); */
	Vector3I_ToVector3(&pos, &coords); /* pos = coords; */

	BlockID block = World_SafeGetBlock_3I(coords);
	AABB blockBB;
	Vector3_Add(&blockBB.Min, &pos, &Block_MinBB[block]);
	Vector3_Add(&blockBB.Max, &pos, &Block_MaxBB[block]);

	if (AABB_ContainsPoint(&blockBB, &pos) && Block_FogDensity[block] != 0.0f) {
		*fogDensity = Block_FogDensity[block];
		*fogCol = Block_FogCol[block];
	} else {
		*fogDensity = 0.0f;
		/* Blend fog and sky together */
		Real32 blend = EnvRenderer_BlendFactor((Real32)Game_ViewDistance);
		*fogCol = PackedCol_Lerp(WorldEnv_FogCol, WorldEnv_SkyCol, blend);
	}
}


void EnvRenderer_RenderMinimal(Real64 deltaTime) {
	if (World_Blocks == NULL) return;

	PackedCol fogCol = PACKEDCOL_WHITE;
	Real32 fogDensity = 0.0f;
	EnvRenderer_BlockOn(&fogDensity, &fogCol);
	Gfx_ClearColour(fogCol);

	/* TODO: rewrite this to avoid raising the event? want to avoid recreating vbos too many times often */
	if (fogDensity != 0.0f) {
		/* Exp fog mode: f = e^(-density*coord) */
		/* Solve coord for f = 0.05 (good approx for fog end) */
		/*   i.e. log(0.05) = -density * coord */

		#define LOG_005 -2.99573227355399
		Real64 dist = LOG_005 / -fogDensity;
		Game_SetViewDistance((Int32)dist, false);
	} else {
		Game_SetViewDistance(Game_UserViewDistance, false);
	}
}

void EnvRenderer_RenderClouds(Real64 deltaTime) {
	if (WorldEnv_CloudsHeight < -2000) return;
	Real64 time = Game_Accumulator;
	Real32 offset = (Real32)(time / 2048.0f * 0.6f * WorldEnv_CloudsSpeed);

	Gfx_SetMatrixMode(MATRIX_TYPE_TEXTURE);
	Matrix matrix = Matrix_Identity; matrix.Row3.X = offset; /* translate X axis */
	Gfx_LoadMatrix(&matrix);
	Gfx_SetMatrixMode(MATRIX_TYPE_VIEW);

	Gfx_SetAlphaTest(true);
	Gfx_SetTexturing(true);
	Gfx_BindTexture(env_cloudsTex);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_BindVb(env_cloudsVb);
	Gfx_DrawVb_IndexedTris(env_cloudVertices);
	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);

	Gfx_SetMatrixMode(MATRIX_TYPE_TEXTURE);
	Gfx_LoadIdentityMatrix();
	Gfx_SetMatrixMode(MATRIX_TYPE_VIEW);
}

void EnvRenderer_RenderSky(Real64 deltaTime) {
	Vector3 pos = Game_CurrentCameraPos;
	Real32 normalY = (Real32)World_Height + 8.0f;
	Real32 skyY = max(pos.Y + 8.0f, normalY);
	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FC4B);
	Gfx_BindVb(env_skyVb);

	if (skyY == normalY) {
		Gfx_DrawVb_IndexedTris(env_skyVertices);
	} else {
		Matrix m = Gfx_View;
		Real32 dy = skyY - normalY; /* inlined Y translation matrix multiply */
		m.Row3.X += dy * m.Row1.X; m.Row3.Y += dy * m.Row1.Y;
		m.Row3.Z += dy * m.Row1.Z; m.Row3.W += dy * m.Row1.W;

		Gfx_LoadMatrix(&m);
		Gfx_DrawVb_IndexedTris(env_skyVertices);
		Gfx_LoadMatrix(&Gfx_View);
	}
}

void EnvRenderer_UpdateFog(void) {
	if (World_Blocks == NULL || EnvRenderer_Minimal) return;
	PackedCol fogCol = PACKEDCOL_WHITE;
	Real32 fogDensity = 0.0f;
	EnvRenderer_BlockOn(&fogDensity, &fogCol);

	if (fogDensity != 0.0f) {
		Gfx_SetFogMode(FOG_EXP);
		Gfx_SetFogDensity(fogDensity);
	} else if (WorldEnv_ExpFog) {
		Gfx_SetFogMode(FOG_EXP);
		/* f = 1-z/end   f = e^(-dz)
		   solve for f = 0.01 gives:
		   e^(-dz)=0.01 --> -dz=ln(0.01)
		   0.99=z/end   --> z=end*0.99
		     therefore
		  d = -ln(0.01)/(end*0.99) */

		#define LOG_001 -4.60517018598809
		Real64 density = -(LOG_001) / (Game_ViewDistance * 0.99);
		Gfx_SetFogDensity((Real32)density);
	} else {
		Gfx_SetFogMode(FOG_LINEAR);
		Gfx_SetFogEnd((Real32)Game_ViewDistance);
	}
	Gfx_ClearColour(fogCol);
	Gfx_SetFogColour(fogCol);
}

void EnvRenderer_Render(Real64 deltaTime) {
	if (EnvRenderer_Minimal) {
		EnvRenderer_RenderMinimal(deltaTime);
	} else {
		if (env_skyVb == NULL || env_cloudsVb == NULL) return;
		if (!SkyboxRenderer_ShouldRender()) {
			EnvRenderer_RenderSky(deltaTime);
			EnvRenderer_RenderClouds(deltaTime);
		} else if (WorldEnv_SkyboxClouds) {
			EnvRenderer_RenderClouds(deltaTime);
		}
		EnvRenderer_UpdateFog();
	}
}

void EnvRenderer_DrawSkyY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Int32 y, Int32 axisSize, PackedCol col, VertexP3fC4b* vertices) {
	Int32 endX = x2, endZ = z2, startZ = z1;
	VertexP3fC4b v;
	v.Y = (Real32)y; v.Col = col;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		z1 = startZ;
		for (; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			v.X = (Real32)x1; v.Z = (Real32)z1; *vertices++ = v;
			                  v.Z = (Real32)z2; *vertices++ = v;
			v.X = (Real32)x2;                   *vertices++ = v;
			                  v.Z = (Real32)z1; *vertices++ = v;
		}
	}
}

void EnvRenderer_DrawCloudsY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Int32 y, Int32 axisSize, PackedCol col, VertexP3fT2fC4b* vertices) {
	Int32 endX = x2, endZ = z2, startZ = z1;
	/* adjust range so that largest negative uv coordinate is shifted to 0 or above. */
	Real32 offset = (Real32)Math_CeilDiv(-x1, 2048);
	VertexP3fT2fC4b v;
	v.Y = (Real32)y + 0.1f; v.Col = col;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		z1 = startZ;
		for (; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			Real32 u1 = (Real32)x1 / 2048.0f + offset, u2 = (Real32)x2 / 2048.0f + offset;
			Real32 v1 = (Real32)z1 / 2048.0f + offset, v2 = (Real32)z2 / 2048.0f + offset;
			v.X = (Real32)x1; v.Z = (Real32)z1; v.U = u1; v.V = v1; *vertices++ = v;
			                  v.Z = (Real32)z2;           v.V = v2; *vertices++ = v;
			v.X = (Real32)x2;                   v.U = u2;           *vertices++ = v;
			                  v.Z = (Real32)z1;           v.V = v1; *vertices++ = v;
		}
	}
}

void EnvRenderer_RebuildClouds(Int32 extent, Int32 axisSize) {
	extent = Utils_AdjViewDist(extent);
	Int32 x1 = -extent, x2 = World_Width + extent;
	Int32 z1 = -extent, z2 = World_Length + extent;
	env_cloudVertices = Math_CountVertices(x2 - x1, z2 - z1, axisSize);

	VertexP3fT2fC4b v[4096];
	VertexP3fT2fC4b* ptr = v;
	if (env_cloudVertices > 4096) {
		ptr = Platform_MemAlloc(env_cloudVertices, sizeof(VertexP3fT2fC4b));
		if (ptr == NULL) ErrorHandler_Fail("EnvRenderer_Clouds - failed to allocate memory");
	}

	EnvRenderer_DrawCloudsY(x1, z1, x2, z2, WorldEnv_CloudsHeight, axisSize, WorldEnv_CloudsCol, ptr);
	env_cloudsVb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FT2FC4B, env_cloudVertices);

	if (env_cloudVertices > 4096) Platform_MemFree(&ptr);
}

void EnvRenderer_RebuildSky(Int32 extent, Int32 axisSize) {
	extent = Utils_AdjViewDist(extent);
	Int32 x1 = -extent, x2 = World_Width + extent;
	Int32 z1 = -extent, z2 = World_Length + extent;
	env_skyVertices = Math_CountVertices(x2 - x1, z2 - z1, axisSize);

	VertexP3fC4b v[4096];
	VertexP3fC4b* ptr = v;
	if (env_skyVertices > 4096) {
		ptr = Platform_MemAlloc(env_skyVertices, sizeof(VertexP3fC4b));
		if (ptr == NULL) ErrorHandler_Fail("EnvRenderer_Sky - failed to allocate memory");
	}

	Int32 height = max((World_Height + 2) + 6, WorldEnv_CloudsHeight + 6);
	EnvRenderer_DrawSkyY(x1, z1, x2, z2, height, axisSize, WorldEnv_SkyCol, ptr);
	env_skyVb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FC4B, env_skyVertices);

	if (env_skyVertices > 4096) Platform_MemFree(&ptr);
}

void EnvRenderer_ResetClouds(void) {
	if (World_Blocks == NULL || Gfx_LostContext) return;
	Gfx_DeleteVb(&env_cloudsVb);
	EnvRenderer_RebuildClouds(Game_ViewDistance, EnvRenderer_Legacy ? 128 : 65536);
}

void EnvRenderer_ResetSky(void) {
	if (World_Blocks == NULL || Gfx_LostContext) return;
	Gfx_DeleteVb(&env_skyVb);
	EnvRenderer_RebuildSky(Game_ViewDistance, EnvRenderer_Legacy ? 128 : 65536);
}

void EnvRenderer_ContextLost(void* obj) {
	Gfx_DeleteVb(&env_skyVb);
	Gfx_DeleteVb(&env_cloudsVb);
}

void EnvRenderer_ContextRecreated(void* obj) {
	EnvRenderer_ContextLost(NULL);
	Gfx_SetFog(!EnvRenderer_Minimal);

	if (EnvRenderer_Minimal) {
		Gfx_ClearColour(WorldEnv_SkyCol);
	} else {
		Gfx_SetFogStart(0.0f);
		EnvRenderer_ResetClouds();
		EnvRenderer_ResetSky();
	}
}

void EnvRenderer_ResetAllEnv(void* obj) {
	EnvRenderer_UpdateFog();
	EnvRenderer_ContextRecreated(NULL);
}

void EnvRenderer_FileChanged(void* obj, Stream* src) {
	if (String_CaselessEqualsConst(&src->Name, "cloud.png") || String_CaselessEqualsConst(&src->Name, "clouds.png")) {
		Game_UpdateTexture(&env_cloudsTex, src, false);
	}
}

void EnvRenderer_EnvVariableChanged(void* obj, Int32 envVar) {
	if (EnvRenderer_Minimal) return;

	if (envVar == ENV_VAR_SKY_COL) {
		EnvRenderer_ResetSky();
	} else if (envVar == ENV_VAR_FOG_COL) {
		EnvRenderer_UpdateFog();
	} else if (envVar == ENV_VAR_CLOUDS_COL) {
		EnvRenderer_ResetClouds();
	} else if (envVar == ENV_VAR_CLOUDS_HEIGHT) {
		EnvRenderer_ResetSky();
		EnvRenderer_ResetClouds();
	}
}


void EnvRenderer_UseLegacyMode(bool legacy) {
	EnvRenderer_Legacy = legacy;
	EnvRenderer_ContextRecreated(NULL);
}

void EnvRenderer_UseMinimalMode(bool minimal) {
	EnvRenderer_Minimal = minimal;
	EnvRenderer_ContextRecreated(NULL);
}

void EnvRenderer_Init(void) {
	EnvRenderer_ResetAllEnv(NULL);

	Event_RegisterStream(&TextureEvents_FileChanged,   NULL, EnvRenderer_FileChanged);
	Event_RegisterVoid(&GfxEvents_ViewDistanceChanged, NULL, EnvRenderer_ResetAllEnv);
	Event_RegisterVoid(&GfxEvents_ContextLost,         NULL, EnvRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,    NULL, EnvRenderer_ContextRecreated);
	Event_RegisterInt(&WorldEvents_EnvVarChanged,    NULL, EnvRenderer_EnvVariableChanged);
	Game_SetViewDistance(Game_UserViewDistance, false);
}

void EnvRenderer_OnNewMap(void) {
	Gfx_SetFog(false);
	EnvRenderer_ContextLost(NULL);
}

void EnvRenderer_OnNewMapLoaded(void) {
	Gfx_SetFog(!EnvRenderer_Minimal);
	EnvRenderer_ResetAllEnv(NULL);
}

void EnvRenderer_Free(void) {
	Gfx_DeleteTexture(&env_cloudsTex);
	EnvRenderer_ContextLost(NULL);

	Event_UnregisterStream(&TextureEvents_FileChanged,   NULL, EnvRenderer_FileChanged);
	Event_UnregisterVoid(&GfxEvents_ViewDistanceChanged, NULL, EnvRenderer_ResetAllEnv);
	Event_UnregisterVoid(&GfxEvents_ContextLost,         NULL, EnvRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,    NULL, EnvRenderer_ContextRecreated);
	Event_UnregisterInt(&WorldEvents_EnvVarChanged,    NULL, EnvRenderer_EnvVariableChanged);
}

IGameComponent EnvRenderer_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = EnvRenderer_Init;
	comp.Reset = EnvRenderer_OnNewMap;
	comp.OnNewMap = EnvRenderer_OnNewMap;
	comp.OnNewMapLoaded = EnvRenderer_OnNewMapLoaded;
	comp.Free = EnvRenderer_Free;
	return comp;
}
