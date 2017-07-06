#include "EnvRenderer.h"
#include "ExtMath.h"
#include "World.h"
#include "WorldEnv.h"
#include "GraphicsEnums.h"
#include "Funcs.h"
#include "GraphicsAPI.h"
#include "GameProps.h"
#include "Matrix.h"
#include "AABB.h"
#include "Block.h"
#include "Platform.h"

GfxResourceID env_cloudsVb = -1, env_skyVb = -1, env_cloudsTex = -1;
GfxResourceID env_cloudVertices, env_skyVertices;

IGameComponent EnvRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = EnvRenderer_Init;
	comp.Reset = EnvRenderer_Reset;
	comp.OnNewMapLoaded = EnvRenderer_OnNewMapLoaded;
	comp.Free = EnvRenderer_Free;
	return comp;
}

void EnvRenderer_UseLegacyMode(bool legacy) {
	EnvRenderer_Legacy = legacy;
	EnvRenderer_ContextRecreated();
}


BlockID EnvRenderer_BlockOn(Real32* fogDensity, PackedCol* fogCol) {
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
		*fogCol = Block_FogColour[block];
	} else {
		*fogDensity = 0.0f;
		/* Blend fog and sky together */
		Real32 blend = EnvRenderer_BlendFactor(Game_ViewDistance);
		*fogCol = PackedCol_Lerp(WorldEnv_FogCol, WorldEnv_SkyCol, blend);
	}
	return block;
}

Real32 EnvRenderer_BlendFactor(Real32 x) {
	/* return -0.05 + 0.22 * (Math_LogE(x) * 0.25f); */
	Real32 blend = -0.13f + 0.28f * (Math_LogE(x) * 0.25f);
	if (blend < 0.0f) blend = 0.0f;
	if (blend > 1.0f) blend = 1.0f;
	return blend;
}


void EnvRenderer_RenderClouds(Real64 delta) {
	if (WorldEnv_CloudsHeight < -2000) return;
	Real64 time = Game_Accumulator;
	Real32 offset = (Real32)(time / 2048.0f * 0.6f * WorldEnv_CloudsSpeed);

	Gfx_SetMatrixMode(MatrixType_Texture);
	Matrix matrix = Matrix_Identity; matrix.Row3.X = offset; /* translate X axis */
	Gfx_LoadMatrix(&matrix);
	Gfx_SetMatrixMode(MatrixType_Modelview);

	Gfx_SetAlphaTest(true);
	Gfx_SetTexturing(true);
	Gfx_BindTexture(env_cloudsTex);
	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	Gfx_BindVb(env_cloudsVb);
	Gfx_DrawVb_IndexedTris(env_cloudVertices * 6 / 4);
	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);

	Gfx_SetMatrixMode(MatrixType_Texture);
	Gfx_LoadIdentityMatrix();
	Gfx_SetMatrixMode(MatrixType_Modelview);
}

void EnvRenderer_UpdateFog(void) {
	if (World_Blocks == NULL) return;
	PackedCol fogCol = PackedCol_White;
	Real32 fogDensity = 0;
	BlockID block = EnvRenderer_BlockOn(&fogDensity, &fogCol);

	if (fogDensity != 0) {
		Gfx_SetFogMode(Fog_Exp);
		Gfx_SetFogDensity(fogDensity);
	} else if (WorldEnv_ExpFog) {
		Gfx_SetFogMode(Fog_Exp);
		/* f = 1-z/end   f = e^(-dz)
		   solve for f = 0.01 gives:
		   e^(-dz)=0.01 --> -dz=ln(0.01)
		   0.99=z/end   --> z=end*0.99
		     therefore
		  d = -ln(0.01)/(end*0.99) */
		Real32 density = -Math_LogE(0.01f) / (Game_ViewDistance * 0.99f);
		Gfx_SetFogDensity(density);
	} else {
		Gfx_SetFogMode(Fog_Linear);
		Gfx_SetFogEnd(Game_ViewDistance);
	}
	Gfx_ClearColour(fogCol);
	Gfx_SetFogColour(fogCol);
}

void EnvRenderer_ResetClouds(void) {
	if (World_Blocks == NULL || Gfx_LostContext) return;
	Gfx_DeleteVb(&env_cloudsVb);
	EnvRenderer_RebuildClouds((Int32)Game_ViewDistance, EnvRenderer_Legacy ? 128 : 65536);
}

void EnvRenderer_ResetSky(void) {
	if (World_Blocks == NULL || Gfx_LostContext) return;
	Gfx_DeleteVb(&env_skyVb);
	EnvRenderer_RebuildSky((Int32)Game_ViewDistance, EnvRenderer_Legacy ? 128 : 65536);
}

void EnvRenderer_RebuildClouds(Int32 extent, Int32 axisSize) {
	extent = Math_AdjViewDist(extent);
	Int32 x1 = -extent, x2 = World_Width + extent;
	Int32 z1 = -extent, z2 = World_Length + extent;
	env_cloudVertices = Math_CountVertices(x2 - x1, z2 - z1, axisSize);

	VertexP3fT2fC4b v[4096];
	VertexP3fT2fC4b* ptr = v;
	if (env_cloudVertices > 4096) {
		ptr = Platform_MemAlloc(env_cloudVertices * sizeof(VertexP3fT2fC4b));
		if (ptr == NULL) ErrorHandler_Fail("EnvRenderer_Clouds - failed to allocate memory");
	}

	EnvRenderer_DrawCloudsY(x1, z1, x2, z2, WorldEnv_CloudsHeight, axisSize, WorldEnv_CloudsCol, ptr);
	env_cloudsVb = Gfx_CreateVb(ptr, VertexFormat_P3fT2fC4b, env_cloudVertices);

	if (env_cloudVertices > 4096) Platform_MemFree(ptr);
}

void EnvRenderer_RebuildSky(Int32 extent, Int32 axisSize) {
	extent = Math_AdjViewDist(extent);
	Int32 x1 = -extent, x2 = World_Width + extent;
	Int32 z1 = -extent, z2 = World_Length + extent;
	env_skyVertices = Math_CountVertices(x2 - x1, z2 - z1, axisSize);

	VertexP3fC4b v[4096];
	VertexP3fC4b* ptr = v;
	if (env_skyVertices > 4096) {
		ptr = Platform_MemAlloc(env_skyVertices * sizeof(VertexP3fC4b));
		if (ptr == NULL) ErrorHandler_Fail("EnvRenderer_Sky - failed to allocate memory");
	}

	Int32 height = max((World_Height + 2) + 6, WorldEnv_CloudsHeight + 6);
	EnvRenderer_DrawSkyY(x1, z1, x2, z2, height, axisSize, WorldEnv_SkyCol, ptr);
	env_skyVb = Gfx_CreateVb(ptr, VertexFormat_P3fC4b, env_skyVertices);

	if (env_skyVertices > 4096) Platform_MemFree(ptr);
}

void EnvRenderer_DrawSkyY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Int32 y, Int32 axisSize, PackedCol col, VertexP3fC4b* vertices) {
	Int32 endX = x2, endZ = z2, startZ = z1;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		z1 = startZ;
		for (; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			VertexP3fC4b_Set(vertices, (Real32)x1, (Real32)y, (Real32)z1, col); vertices++;
			VertexP3fC4b_Set(vertices, (Real32)x1, (Real32)y, (Real32)z2, col); vertices++;
			VertexP3fC4b_Set(vertices, (Real32)x2, (Real32)y, (Real32)z2, col); vertices++;
			VertexP3fC4b_Set(vertices, (Real32)x2, (Real32)y, (Real32)z1, col); vertices++;
		}
	}
}

void EnvRenderer_DrawCloudsY(Int32 x1, Int32 z1, Int32 x2, Int32 z2, Int32 y, Int32 axisSize, PackedCol col, VertexP3fT2fC4b* vertices) {
	Int32 endX = x2, endZ = z2, startZ = z1;
	/* adjust range so that largest negative uv coordinate is shifted to 0 or above. */
	Real32 offset = (Real32)Math_CeilDiv(-x1, 2048);

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		z1 = startZ;
		for (; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			VertexP3fT2fC4b_Set(vertices, (Real32)x1, (Real32)y + 0.1f, (Real32)z1, 
				(Real32)x1 / 2048.0f + offset, (Real32)z1 / 2048.0f + offset, col); vertices++;
			VertexP3fT2fC4b_Set(vertices, (Real32)x1, (Real32)y + 0.1f, (Real32)z2,
				(Real32)x1 / 2048.0f + offset, (Real32)z2 / 2048.0f + offset, col); vertices++;
			VertexP3fT2fC4b_Set(vertices, (Real32)x2, (Real32)y + 0.1f, (Real32)z2,
				(Real32)x2 / 2048.0f + offset, (Real32)z2 / 2048.0f + offset, col); vertices++;
			VertexP3fT2fC4b_Set(vertices, (Real32)x2, (Real32)y + 0.1f, (Real32)z1,
				(Real32)x2 / 2048.0f + offset, (Real32)z1 / 2048.0f + offset, col); vertices++;
		}
	}
}