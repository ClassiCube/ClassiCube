#include "EnvRenderer.h"
#include "ExtMath.h"
#include "World.h"
#include "Funcs.h"
#include "Graphics.h"
#include "Physics.h"
#include "Block.h"
#include "Platform.h"
#include "Event.h"
#include "Utils.h"
#include "Game.h"
#include "Logger.h"
#include "Stream.h"
#include "Block.h"
#include "Event.h"
#include "TexturePack.h"
#include "Platform.h"
#include "Camera.h"
#include "Particle.h"

cc_bool EnvRenderer_Legacy, EnvRenderer_Minimal;

#define ENV_SMALL_VERTICES 4096
static float EnvRenderer_BlendFactor(float x) {
	/* return -0.05 + 0.22 * (Math_Log(x) * 0.25f); */
	double blend = -0.13 + 0.28 * (Math_Log(x) * 0.25);
	if (blend < 0.0) blend = 0.0;
	if (blend > 1.0) blend = 1.0;
	return (float)blend;
}

#define EnvRenderer_AxisSize() (EnvRenderer_Legacy ? 128 : 2048)
/* Returns the number of vertices needed to subdivide a quad */
static int EnvRenderer_Vertices(int axis1Len, int axis2Len) {
	int axisSize = EnvRenderer_AxisSize();
	return Math_CeilDiv(axis1Len, axisSize) * Math_CeilDiv(axis2Len, axisSize) * 4;
}


/*########################################################################################################################*
*------------------------------------------------------------Fog----------------------------------------------------------*
*#########################################################################################################################*/
static void EnvRenderer_CalcFog(float* density, PackedCol* col) {
	Vec3 pos;
	IVec3 coords;
	BlockID block;
	struct AABB blockBB;
	float blend;

	IVec3_Floor(&coords, &Camera.CurrentPos); /* coords = floor(camera_pos); */
	IVec3_ToVec3(&pos, &coords);              /* pos = coords; */

	block = World_SafeGetBlock(coords.X, coords.Y, coords.Z);
	Vec3_Add(&blockBB.Min, &pos, &Blocks.MinBB[block]);
	Vec3_Add(&blockBB.Max, &pos, &Blocks.MaxBB[block]);

	if (AABB_ContainsPoint(&blockBB, &Camera.CurrentPos) && Blocks.FogDensity[block]) {
		*density = Blocks.FogDensity[block];
		*col     = Blocks.FogCol[block];
	} else {
		*density = 0.0f;
		/* Blend fog and sky together */
		blend    = EnvRenderer_BlendFactor((float)Game_ViewDistance);
		*col     = PackedCol_Lerp(Env.FogCol, Env.SkyCol, blend);
	}
}

static void EnvRenderer_UpdateFogMinimal(float fogDensity) {
	int dist;
	/* TODO: rewrite this to avoid raising the event? want to avoid recreating vbos too many times often */

	if (fogDensity != 0.0f) {
		/* Exp fog mode: f = e^(-density*coord) */
		/* Solve coord for f = 0.05 (good approx for fog end) */
		/*   i.e. log(0.05) = -density * coord */
		#define LOG_005 -2.99573227355399

		dist = (int)(LOG_005 / -fogDensity);
		Game_SetViewDistance(min(dist, Game_UserViewDistance));
	} else {
		Game_SetViewDistance(Game_UserViewDistance);
	}
}

static void EnvRenderer_UpdateFogNormal(float fogDensity, PackedCol fogCol) {
	double density;

	if (fogDensity != 0.0f) {
		Gfx_SetFogMode(FOG_EXP);
		Gfx_SetFogDensity(fogDensity);
	} else if (Env.ExpFog) {
		Gfx_SetFogMode(FOG_EXP);
		/* f = 1-z/end   f = e^(-dz)
		   solve for f = 0.01 gives:
		   e^(-dz)=0.01 --> -dz=ln(0.01)
		   0.99=z/end   --> z=end*0.99
		     therefore
		  d = -ln(0.01)/(end*0.99) */
		#define LOG_001 -4.60517018598809

		density = -(LOG_001) / (Game_ViewDistance * 0.99);
		Gfx_SetFogDensity((float)density);
	} else {
		Gfx_SetFogMode(FOG_LINEAR);
		Gfx_SetFogEnd((float)Game_ViewDistance);
	}
	Gfx_SetFogCol(fogCol);
}

void EnvRenderer_UpdateFog(void) {
	float fogDensity; PackedCol fogCol;
	if (!World.Blocks) return;

	EnvRenderer_CalcFog(&fogDensity, &fogCol);
	Gfx_ClearCol(fogCol);

	if (EnvRenderer_Minimal) {
		EnvRenderer_UpdateFogMinimal(fogDensity);
	} else {
		EnvRenderer_UpdateFogNormal(fogDensity, fogCol);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------Clouds---------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID clouds_vb, clouds_tex;
static int clouds_vertices;

void EnvRenderer_RenderClouds(double deltaTime) {
	float offset;
	struct Matrix m;

	if (!clouds_vb || Env.CloudsHeight < -2000) return;
	offset = (float)(Game.Time / 2048.0f * 0.6f * Env.CloudsSpeed);

	m = Matrix_Identity; m.Row3.X = offset; /* translate X axis */
	Gfx_LoadMatrix(MATRIX_TEXTURE, &m);

	Gfx_SetAlphaTest(true);
	Gfx_SetTexturing(true);
	Gfx_BindTexture(clouds_tex);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_BindVb(clouds_vb);
	Gfx_DrawVb_IndexedTris(clouds_vertices);
	Gfx_SetAlphaTest(false);
	Gfx_SetTexturing(false);

	Gfx_LoadIdentityMatrix(MATRIX_TEXTURE);
}

static void EnvRenderer_DrawCloudsY(int x1, int z1, int x2, int z2, int y, VertexP3fT2fC4b* v) {
	int endX = x2, endZ = z2, startZ = z1, axisSize = EnvRenderer_AxisSize();
	float u1, u2, v1, v2;
	float yy = (float)y + 0.1f; 
	PackedCol col = Env.CloudsCol;
	/* adjust range so that largest negative uv coordinate is shifted to 0 or above. */
	float offset = (float)Math_CeilDiv(-x1, 2048);

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;

		for (z1 = startZ; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			u1 = (float)x1 / 2048.0f + offset; u2 = (float)x2 / 2048.0f + offset;
			v1 = (float)z1 / 2048.0f + offset; v2 = (float)z2 / 2048.0f + offset;

			v->X = (float)x1; v->Y = yy; v->Z = (float)z1; v->Col = col; v->U = u1; v->V = v1; v++;
			v->X = (float)x1; v->Y = yy; v->Z = (float)z2; v->Col = col; v->U = u1; v->V = v2; v++;
			v->X = (float)x2; v->Y = yy; v->Z = (float)z2; v->Col = col; v->U = u2; v->V = v2; v++;
			v->X = (float)x2; v->Y = yy; v->Z = (float)z1; v->Col = col; v->U = u2; v->V = v1; v++;
		}
	}
}

static void EnvRenderer_UpdateClouds(void) {
	VertexP3fT2fC4b v[ENV_SMALL_VERTICES];
	VertexP3fT2fC4b* ptr;
	int extent;
	int x1, z1, x2, z2;
	
	if (!World.Blocks || Gfx.LostContext) return;
	Gfx_DeleteVb(&clouds_vb);
	if (EnvRenderer_Minimal) return;

	extent = Utils_AdjViewDist(Game_ViewDistance);
	x1 = -extent; x2 = World.Width  + extent;
	z1 = -extent; z2 = World.Length + extent;
	clouds_vertices = EnvRenderer_Vertices(x2 - x1, z2 - z1);

	ptr = v;
	if (clouds_vertices > ENV_SMALL_VERTICES) {
		ptr = (VertexP3fT2fC4b*)Mem_Alloc(clouds_vertices, sizeof(VertexP3fT2fC4b), "clouds vertices");
	}

	EnvRenderer_DrawCloudsY(x1, z1, x2, z2, Env.CloudsHeight, ptr);
	clouds_vb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FT2FC4B, clouds_vertices);

	if (clouds_vertices > ENV_SMALL_VERTICES) Mem_Free(ptr);
}


/*########################################################################################################################*
*------------------------------------------------------------Sky----------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID sky_vb;
static int sky_vertices;

void EnvRenderer_RenderSky(double deltaTime) {
	struct Matrix m;
	float skyY, normY, dy;
	if (!sky_vb || EnvRenderer_ShouldRenderSkybox()) return;

	normY = (float)World.Height + 8.0f;
	skyY  = max(Camera.CurrentPos.Y + 8.0f, normY);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FC4B);
	Gfx_BindVb(sky_vb);

	if (skyY == normY) {
		Gfx_DrawVb_IndexedTris(sky_vertices);
	} else {
		m  = Gfx.View;
		dy = skyY - normY; 
		/* inlined Y translation matrix multiply */
		m.Row3.X += dy * m.Row1.X; m.Row3.Y += dy * m.Row1.Y;
		m.Row3.Z += dy * m.Row1.Z; m.Row3.W += dy * m.Row1.W;

		Gfx_LoadMatrix(MATRIX_VIEW, &m);
		Gfx_DrawVb_IndexedTris(sky_vertices);
		Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	}
}

static void EnvRenderer_DrawSkyY(int x1, int z1, int x2, int z2, int y, VertexP3fC4b* v) {
	int endX = x2, endZ = z2, startZ = z1, axisSize = EnvRenderer_AxisSize();
	PackedCol col = Env.SkyCol;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;

		for (z1 = startZ; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			v->X = (float)x1; v->Y = (float)y; v->Z = (float)z1; v->Col = col; v++;
			v->X = (float)x1; v->Y = (float)y; v->Z = (float)z2; v->Col = col; v++;
			v->X = (float)x2; v->Y = (float)y; v->Z = (float)z2; v->Col = col; v++;
			v->X = (float)x2; v->Y = (float)y; v->Z = (float)z1; v->Col = col; v++;
		}
	}
}

static void EnvRenderer_UpdateSky(void) {
	VertexP3fC4b v[ENV_SMALL_VERTICES];
	VertexP3fC4b* ptr;
	int extent, height;
	int x1, z1, x2, z2;

	if (!World.Blocks || Gfx.LostContext) return;
	Gfx_DeleteVb(&sky_vb);
	if (EnvRenderer_Minimal) return;

	extent = Utils_AdjViewDist(Game_ViewDistance);
	x1 = -extent; x2 = World.Width  + extent;
	z1 = -extent; z2 = World.Length + extent;
	sky_vertices = EnvRenderer_Vertices(x2 - x1, z2 - z1);

	ptr = v;
	if (sky_vertices > ENV_SMALL_VERTICES) {
		ptr = (VertexP3fC4b*)Mem_Alloc(sky_vertices, sizeof(VertexP3fC4b), "sky vertices");
	}

	height = max((World.Height + 2), Env.CloudsHeight) + 6;
	EnvRenderer_DrawSkyY(x1, z1, x2, z2, height, ptr);
	sky_vb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FC4B, sky_vertices);

	if (sky_vertices > ENV_SMALL_VERTICES) Mem_Free(ptr);
}

/*########################################################################################################################*
*----------------------------------------------------------Skybox---------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID skybox_tex, skybox_vb;
#define SKYBOX_COUNT (6 * 4)
cc_bool EnvRenderer_ShouldRenderSkybox(void) { return skybox_tex && !EnvRenderer_Minimal; }

void EnvRenderer_RenderSkybox(double deltaTime) {
	struct Matrix m, rotX, rotY, view;
	float rotTime;
	Vec3 pos;
	if (!skybox_vb) return;

	Gfx_SetDepthWrite(false);
	Gfx_SetTexturing(true);
	Gfx_BindTexture(skybox_tex);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);

	/* Base skybox rotation */
	rotTime = (float)(Game.Time * 2 * MATH_PI); /* So speed of 1 rotates whole skybox every second */
	Matrix_RotateY(&rotY, Env.SkyboxHorSpeed * rotTime);
	Matrix_RotateX(&rotX, Env.SkyboxVerSpeed * rotTime);
	Matrix_Mul(&m, &rotY, &rotX);

	/* Rotate around camera */
	pos = Camera.CurrentPos;
	Vec3_Set(Camera.CurrentPos, 0,0,0);
	Camera.Active->GetView(&view); Matrix_MulBy(&m, &view);
	Camera.CurrentPos = pos;

	Gfx_LoadMatrix(MATRIX_VIEW, &m);
	Gfx_BindVb(skybox_vb);
	Gfx_DrawVb_IndexedTris(SKYBOX_COUNT);

	Gfx_SetTexturing(false);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	Gfx_SetDepthWrite(true);
}

static void EnvRenderer_UpdateSkybox(void) {
	static VertexP3fT2fC4b vertices[SKYBOX_COUNT] = {
		/* Front quad */
		{ -1, -1, -1,  0, 0.25f, 1.00f }, {  1, -1, -1,  0, 0.50f, 1.00f },
		{  1,  1, -1,  0, 0.50f, 0.50f }, { -1,  1, -1,  0, 0.25f, 0.50f },
		/* Left quad */
		{ -1, -1,  1,  0, 0.00f, 1.00f }, { -1, -1, -1,  0, 0.25f, 1.00f },
		{ -1,  1, -1,  0, 0.25f, 0.50f }, { -1,  1,  1,  0, 0.00f, 0.50f },
		/* Back quad */
		{  1, -1,  1,  0, 0.75f, 1.00f }, { -1, -1,  1,  0, 1.00f, 1.00f },
		{ -1,  1,  1,  0, 1.00f, 0.50f }, {  1,  1,  1,  0, 0.75f, 0.50f },
		/* Right quad */
		{  1, -1, -1,  0, 0.50f, 1.00f }, {  1, -1,  1,  0, 0.75f, 1.00f },
		{  1,  1,  1,  0, 0.75f, 0.50f }, {  1,  1, -1,  0, 0.50f, 0.50f },
		/* Top quad */
		{  1,  1, -1,  0, 0.50f, 0.50f }, {  1,  1,  1,  0, 0.50f, 0.00f },
		{ -1,  1,  1,  0, 0.25f, 0.00f }, { -1,  1, -1,  0, 0.25f, 0.50f },
		/* Bottom quad */
		{  1, -1, -1,  0, 0.75f, 0.50f }, {  1, -1,  1,  0, 0.75f, 0.00f },
		{ -1, -1,  1,  0, 0.50f, 0.00f }, { -1, -1, -1,  0, 0.50f, 0.50f },
	};
	int i;

	if (Gfx.LostContext) return;
	Gfx_DeleteVb(&skybox_vb);
	if (EnvRenderer_Minimal) return;

	for (i = 0; i < SKYBOX_COUNT; i++) { vertices[i].Col = Env.SkyboxCol; }
	skybox_vb = Gfx_CreateVb(vertices, VERTEX_FORMAT_P3FT2FC4B, SKYBOX_COUNT);
}


/*########################################################################################################################*
*----------------------------------------------------------Weather--------------------------------------------------------*
*#########################################################################################################################*/
cc_int16* Weather_Heightmap;
static GfxResourceID rain_tex, snow_tex, weather_vb;
static double weather_accumulator;
static IVec3 lastPos;

#define WEATHER_EXTENT 4
#define WEATHER_VERTS_COUNT 8 * (WEATHER_EXTENT * 2 + 1) * (WEATHER_EXTENT * 2 + 1)
#define Weather_Pack(x, z) ((x) * World.Length + (z))

static void EnvRenderer_InitWeatherHeightmap(void) {
	int i;
	Weather_Heightmap = (cc_int16*)Mem_Alloc(World.Width * World.Length, 2, "weather heightmap");
	
	for (i = 0; i < World.Width * World.Length; i++) {
		Weather_Heightmap[i] = Int16_MaxValue;
	}
}

#define EnvRenderer_RainCalcBody(get_block)\
for (y = maxY; y >= 0; y--, i -= World.OneY) {\
	draw = Blocks.Draw[get_block];\
\
	if (!(draw == DRAW_GAS || draw == DRAW_SPRITE)) {\
		Weather_Heightmap[hIndex] = y;\
		return y;\
	}\
}

static int EnvRenderer_CalcRainHeightAt(int x, int maxY, int z, int hIndex) {
	int i = World_Pack(x, maxY, z), y;
	cc_uint8 draw;

#ifndef EXTENDED_BLOCKS
	EnvRenderer_RainCalcBody(World.Blocks[i]);
#else
	if (World.IDMask <= 0xFF) {
		EnvRenderer_RainCalcBody(World.Blocks[i]);
	} else {
		EnvRenderer_RainCalcBody(World.Blocks[i] | (World.Blocks2[i] << 8));
	}
#endif

	Weather_Heightmap[hIndex] = -1;
	return -1;
}

static float EnvRenderer_RainHeight(int x, int z) {
	int hIndex, height;
	int y;
	if (!World_ContainsXZ(x, z)) return (float)Env.EdgeHeight;

	hIndex = Weather_Pack(x, z);
	height = Weather_Heightmap[hIndex];

	y = height == Int16_MaxValue ? EnvRenderer_CalcRainHeightAt(x, World.MaxY, z, hIndex) : height;
	return y == -1 ? 0 : y + Blocks.MaxBB[World_GetBlock(x, y, z)].Y;
}

void EnvRenderer_OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock) {
	cc_bool didBlock = !(Blocks.Draw[oldBlock] == DRAW_GAS || Blocks.Draw[oldBlock] == DRAW_SPRITE);
	cc_bool nowBlock = !(Blocks.Draw[newBlock] == DRAW_GAS || Blocks.Draw[newBlock] == DRAW_SPRITE);
	int hIndex, height;
	if (didBlock == nowBlock) return;

	hIndex = Weather_Pack(x, z);
	height = Weather_Heightmap[hIndex];
	/* Two cases can be skipped here: */
	/* a) rain height was not calculated to begin with (height is short.MaxValue) */
	/* b) changed y is below current calculated rain height */
	if (y < height) return;

	if (nowBlock) {
		/* Simple case: Rest of column below is now not visible to rain. */
		Weather_Heightmap[hIndex] = y;
	} else {
		/* Part of the column is now visible to rain, we don't know how exactly how high it should be though. */
		/* However, we know that if the old block was above or equal to rain height, then the new rain height must be <= old block.y */
		EnvRenderer_CalcRainHeightAt(x, y, z, hIndex);
	}
}

static float EnvRenderer_RainAlphaAt(float x) {
	/* Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9} */
	float falloff = 0.05f * x * x - 7 * x;
	return 178 + falloff * Env.WeatherFade;
}

void EnvRenderer_RenderWeather(double deltaTime) {
	VertexP3fT2fC4b vertices[WEATHER_VERTS_COUNT];
	VertexP3fT2fC4b* v;
	int weather, vCount;
	IVec3 pos;
	cc_bool moved, particles;
	float speed, vOffset;

	PackedCol col;
	int dist, dx, dz, x, z;
	float alpha, y, height;
	float worldV, v1, v2;
	float x1,y1,z1, x2,y2,z2;

	weather = Env.Weather;
	if (weather == WEATHER_SUNNY) return;
	if (!Weather_Heightmap) EnvRenderer_InitWeatherHeightmap();
	Gfx_BindTexture(weather == WEATHER_RAINY ? rain_tex : snow_tex);

	IVec3_Floor(&pos, &Camera.CurrentPos);
	moved   = pos.X != lastPos.X || pos.Y != lastPos.Y || pos.Z != lastPos.Z;
	lastPos = pos;

	/* Rain should extend up by 64 blocks, or to the top of the world. */
	pos.Y += 64;
	pos.Y = max(World.Height, pos.Y);

	speed     = (weather == WEATHER_RAINY ? 1.0f : 0.2f) * Env.WeatherSpeed;
	vOffset   = (float)Game.Time * speed;
	particles = weather == WEATHER_RAINY;
	weather_accumulator += deltaTime;

	v   = vertices;
	col = Env.SunCol;

	for (dx = -WEATHER_EXTENT; dx <= WEATHER_EXTENT; dx++) {
		for (dz = -WEATHER_EXTENT; dz <= WEATHER_EXTENT; dz++) {
			x = pos.X + dx; z = pos.Z + dz;

			y = EnvRenderer_RainHeight(x, z);
			height = pos.Y - y;
			if (height <= 0) continue;

			if (particles && (weather_accumulator >= 0.25 || moved)) {
				Vec3 particlePos = Vec3_Create3((float)x, y, (float)z);
				Particles_RainSnowEffect(particlePos);
			}

			dist  = dx * dx + dz * dz;
			alpha = EnvRenderer_RainAlphaAt((float)dist);
			Math_Clamp(alpha, 0.0f, 255.0f);
			col   = (col & PACKEDCOL_RGB_MASK) | PackedCol_A_Bits(alpha);

			worldV = vOffset + (z & 1) / 2.0f - (x & 0x0F) / 16.0f;
			v1 = y            / 6.0f + worldV; 
			v2 = (y + height) / 6.0f + worldV;
			x1 = (float)x;       y1 = (float)y;            z1 = (float)z;
			x2 = (float)(x + 1); y2 = (float)(y + height); z2 = (float)(z + 1);

			v->X = x1; v->Y = y1; v->Z = z1; v->Col = col; v->U = 0.0f; v->V = v1; v++;
			v->X = x1; v->Y = y2; v->Z = z1; v->Col = col; v->U = 0.0f; v->V = v2; v++;
			v->X = x2; v->Y = y2; v->Z = z2; v->Col = col; v->U = 1.0f; v->V = v2; v++;
			v->X = x2; v->Y = y1; v->Z = z2; v->Col = col; v->U = 1.0f; v->V = v1; v++;

			v->X = x2; v->Y = y1; v->Z = z1; v->Col = col; v->U = 1.0f; v->V = v1; v++;
			v->X = x2; v->Y = y2; v->Z = z1; v->Col = col; v->U = 1.0f; v->V = v2; v++;
			v->X = x1; v->Y = y2; v->Z = z2; v->Col = col; v->U = 0.0f; v->V = v2; v++;
			v->X = x1; v->Y = y1; v->Z = z2; v->Col = col; v->U = 0.0f; v->V = v1; v++;
		}
	}

	if (particles && (weather_accumulator >= 0.25f || moved)) {
		weather_accumulator = 0;
	}
	if (v == vertices) return;

	Gfx_SetAlphaTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaArgBlend(true);

	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	vCount = (int)(v - vertices);
	Gfx_UpdateDynamicVb_IndexedTris(weather_vb, vertices, vCount);

	Gfx_SetAlphaArgBlend(false);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaTest(false);
}


/*########################################################################################################################*
*--------------------------------------------------------Sides/Edge-------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID sides_vb, edges_vb, sides_tex, edges_tex;
static int sides_vertices, edges_vertices;
static cc_bool sides_fullBright, edges_fullBright;
static TextureLoc edges_lastTexLoc, sides_lastTexLoc;

static void EnvRenderer_RenderBorders(BlockID block, GfxResourceID vb, GfxResourceID tex, int count) {
	if (!vb) return;

	Gfx_SetTexturing(true);
	Gfx_SetupAlphaState(Blocks.Draw[block]);
	Gfx_EnableMipmaps();

	Gfx_BindTexture(tex);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_BindVb(vb);
	Gfx_DrawVb_IndexedTris(count);

	Gfx_DisableMipmaps();
	Gfx_RestoreAlphaState(Blocks.Draw[block]);
	Gfx_SetTexturing(false);
}

void EnvRenderer_RenderMapSides(double delta) {
	EnvRenderer_RenderBorders(Env.SidesBlock, 
		sides_vb, sides_tex, sides_vertices);
}

void EnvRenderer_RenderMapEdges(double delta) {
	/* Do not draw water when player cannot see it */
	/* Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps */
	int yVisible = min(0, Env_SidesHeight);
	if (Camera.CurrentPos.Y < yVisible && sides_vb) return;

	EnvRenderer_RenderBorders(Env.EdgeBlock,
		edges_vb, edges_tex, edges_vertices);
}

static void EnvRenderer_MakeBorderTex(GfxResourceID* texId, BlockID block) {
	TextureLoc loc = Block_Tex(block, FACE_YMAX);
	if (Gfx.LostContext) return;

	Gfx_DeleteTexture(texId);
	*texId = Atlas2D_LoadTile(loc);
}

static Rect2D EnvRenderer_Rect(int x, int y, int width, int height) {
	Rect2D r;
	r.X = x; r.Y = y; r.Width = width; r.Height = height; 
	return r;
}

static void EnvRenderer_CalcBorderRects(Rect2D* rects) {
	int extent = Utils_AdjViewDist(Game_ViewDistance);
	rects[0] = EnvRenderer_Rect(-extent, -extent,      extent + World.Width + extent, extent);
	rects[1] = EnvRenderer_Rect(-extent, World.Length, extent + World.Width + extent, extent);

	rects[2] = EnvRenderer_Rect(-extent,     0, extent, World.Length);
	rects[3] = EnvRenderer_Rect(World.Width, 0, extent, World.Length);
}

static void EnvRenderer_UpdateBorderTextures(void) {
	EnvRenderer_MakeBorderTex(&edges_tex, Env.EdgeBlock);
	EnvRenderer_MakeBorderTex(&sides_tex, Env.SidesBlock);
}

#define Borders_HorOffset(block) (Blocks.RenderMinBB[block].X - Blocks.MinBB[block].X)
#define Borders_YOffset(block)   (Blocks.RenderMinBB[block].Y - Blocks.MinBB[block].Y)

static void EnvRenderer_DrawBorderX(int x, int z1, int z2, int y1, int y2, PackedCol col, VertexP3fT2fC4b** vertices) {
	int endZ = z2, endY = y2, startY = y1, axisSize = EnvRenderer_AxisSize();
	float u2, v2;
	VertexP3fT2fC4b* v = *vertices;

	for (; z1 < endZ; z1 += axisSize) {
		z2 = z1 + axisSize;
		if (z2 > endZ) z2 = endZ;

		for (y1 = startY; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			u2   = (float)z2 - (float)z1;      v2   = (float)y2 - (float)y1;
			v->X = (float)x; v->Y = (float)y1; v->Z = (float)z1; v->Col = col; v->U = 0;  v->V = v2; v++;
			v->X = (float)x; v->Y = (float)y2; v->Z = (float)z1; v->Col = col; v->U = 0;  v->V = 0;  v++;
			v->X = (float)x; v->Y = (float)y2; v->Z = (float)z2; v->Col = col; v->U = u2; v->V = 0;  v++;
			v->X = (float)x; v->Y = (float)y1; v->Z = (float)z2; v->Col = col; v->U = u2; v->V = v2; v++;
		}
	}
	*vertices = v;
}

static void EnvRenderer_DrawBorderZ(int z, int x1, int x2, int y1, int y2, PackedCol col, VertexP3fT2fC4b** vertices) {
	int endX = x2, endY = y2, startY = y1, axisSize = EnvRenderer_AxisSize();
	float u2, v2;
	VertexP3fT2fC4b* v = *vertices;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;

		for (y1 = startY; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			u2   = (float)x2 - (float)x1;       v2   = (float)y2 - (float)y1;
			v->X = (float)x1; v->Y = (float)y1; v->Z = (float)z; v->Col = col; v->U = 0;  v->V = v2; v++;
			v->X = (float)x1; v->Y = (float)y2; v->Z = (float)z; v->Col = col; v->U = 0;  v->V = 0;  v++;
			v->X = (float)x2; v->Y = (float)y2; v->Z = (float)z; v->Col = col; v->U = u2; v->V = 0;  v++;
			v->X = (float)x2; v->Y = (float)y1; v->Z = (float)z; v->Col = col; v->U = u2; v->V = v2; v++;
		}
	}
	*vertices = v;
}

static void EnvRenderer_DrawBorderY(int x1, int z1, int x2, int z2, float y, PackedCol col, float offset, float yOffset, VertexP3fT2fC4b** vertices) {
	int endX = x2, endZ = z2, startZ = z1, axisSize = EnvRenderer_AxisSize();
	float u2, v2;
	VertexP3fT2fC4b* v = *vertices;
	float yy = y + yOffset;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		
		for (z1 = startZ; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			u2   = (float)x2 - (float)x1;         v2   = (float)z2 - (float)z1;
			v->X = (float)x1 + offset; v->Y = yy; v->Z = (float)z1 + offset; v->Col = col; v->U = 0;  v->V = 0;  v++;
			v->X = (float)x1 + offset; v->Y = yy; v->Z = (float)z2 + offset; v->Col = col; v->U = 0;  v->V = v2; v++;
			v->X = (float)x2 + offset; v->Y = yy; v->Z = (float)z2 + offset; v->Col = col; v->U = u2; v->V = v2; v++;
			v->X = (float)x2 + offset; v->Y = yy; v->Z = (float)z1 + offset; v->Col = col; v->U = u2; v->V = 0;  v++;
		}
	}
	*vertices = v;
}

static void EnvRenderer_UpdateMapSides(void) {
	Rect2D rects[4], r;
	BlockID block;
	PackedCol col, white = PACKEDCOL_WHITE;
	int y, y1, y2;
	int i;

	VertexP3fT2fC4b v[ENV_SMALL_VERTICES];
	VertexP3fT2fC4b* ptr;
	VertexP3fT2fC4b* cur;

	if (!World.Blocks || Gfx.LostContext) return;
	Gfx_DeleteVb(&sides_vb);
	block = Env.SidesBlock;

	if (Blocks.Draw[block] == DRAW_GAS) return;
	EnvRenderer_CalcBorderRects(rects);

	sides_vertices = 0;
	for (i = 0; i < 4; i++) {
		r = rects[i];
		sides_vertices += EnvRenderer_Vertices(r.Width, r.Height); /* YQuads outside */
	}

	y = Env_SidesHeight;
	sides_vertices += EnvRenderer_Vertices(World.Width, World.Length); /* YQuads beneath map */
	sides_vertices += 2 * EnvRenderer_Vertices(World.Width,  Math_AbsI(y)); /* ZQuads */
	sides_vertices += 2 * EnvRenderer_Vertices(World.Length, Math_AbsI(y)); /* XQuads */

	ptr = v;
	if (sides_vertices > ENV_SMALL_VERTICES) {
		ptr = (VertexP3fT2fC4b*)Mem_Alloc(sides_vertices, sizeof(VertexP3fT2fC4b), "sides vertices");
	}
	cur = ptr;

	sides_fullBright = Blocks.FullBright[block];
	col = sides_fullBright ? white : Env.ShadowCol;
	Block_Tint(col, block)

	for (i = 0; i < 4; i++) {
		r = rects[i];
		EnvRenderer_DrawBorderY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, (float)y, col,
			0, Borders_YOffset(block), &cur);
	}

	/* Work properly for when ground level is below 0 */
	y1 = 0; y2 = y;
	if (y < 0) { y1 = y; y2 = 0; }

	EnvRenderer_DrawBorderY(0, 0, World.Width, World.Length, 0, col, 0, 0, &cur);
	EnvRenderer_DrawBorderZ(0, 0, World.Width, y1, y2, col, &cur);
	EnvRenderer_DrawBorderZ(World.Length, 0, World.Width, y1, y2, col, &cur);
	EnvRenderer_DrawBorderX(0, 0, World.Length, y1, y2, col, &cur);
	EnvRenderer_DrawBorderX(World.Width, 0, World.Length, y1, y2, col, &cur);

	sides_vb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FT2FC4B, sides_vertices);
	if (sides_vertices > ENV_SMALL_VERTICES) Mem_Free(ptr);
}

static void EnvRenderer_UpdateMapEdges(void) {
	Rect2D rects[4], r;
	BlockID block;
	PackedCol col, white = PACKEDCOL_WHITE;
	float y;
	int i;

	VertexP3fT2fC4b v[ENV_SMALL_VERTICES];
	VertexP3fT2fC4b* ptr;
	VertexP3fT2fC4b* cur;

	if (!World.Blocks || Gfx.LostContext) return;
	Gfx_DeleteVb(&edges_vb);
	block = Env.EdgeBlock;

	if (Blocks.Draw[block] == DRAW_GAS) return;
	EnvRenderer_CalcBorderRects(rects);

	edges_vertices = 0;
	for (i = 0; i < 4; i++) {
		r = rects[i];
		edges_vertices += EnvRenderer_Vertices(r.Width, r.Height); /* YPlanes outside */
	}

	ptr = v;
	if (edges_vertices > ENV_SMALL_VERTICES) {
		ptr = (VertexP3fT2fC4b*)Mem_Alloc(edges_vertices, sizeof(VertexP3fT2fC4b), "edge vertices");
	}
	cur = ptr;

	edges_fullBright = Blocks.FullBright[block];
	col = edges_fullBright ? white : Env.SunCol;
	Block_Tint(col, block)

	y = (float)Env.EdgeHeight;
	for (i = 0; i < 4; i++) {
		r = rects[i];
		EnvRenderer_DrawBorderY(r.X, r.Y, r.X + r.Width, r.Y + r.Height, y, col,
			Borders_HorOffset(block), Borders_YOffset(block), &cur);
	}

	edges_vb = Gfx_CreateVb(ptr, VERTEX_FORMAT_P3FT2FC4B, edges_vertices);
	if (edges_vertices > ENV_SMALL_VERTICES) Mem_Free(ptr);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void EnvRenderer_DeleteVbs(void) {
	Gfx_DeleteVb(&sky_vb);
	Gfx_DeleteVb(&clouds_vb);
	Gfx_DeleteVb(&skybox_vb);
	Gfx_DeleteVb(&sides_vb);
	Gfx_DeleteVb(&edges_vb);
	Gfx_DeleteVb(&weather_vb);
}

static void EnvRenderer_ContextLost(void* obj) {
	EnvRenderer_DeleteVbs();
	Gfx_DeleteTexture(&sides_tex);
	Gfx_DeleteTexture(&edges_tex);
}

static void EnvRenderer_UpdateAll(void) {
	EnvRenderer_DeleteVbs();
	EnvRenderer_UpdateMapSides();
	EnvRenderer_UpdateMapEdges();
	EnvRenderer_UpdateClouds();
	EnvRenderer_UpdateSky();
	EnvRenderer_UpdateSkybox();
	EnvRenderer_UpdateFog();

	if (Gfx.LostContext) return;
	/* TODO: Don't allocate unless used? */
	weather_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, WEATHER_VERTS_COUNT);
	/* TODO: Don't need to do this on every new map */
	EnvRenderer_UpdateBorderTextures();
}

static void EnvRenderer_ContextRecreated(void* obj) {
	Gfx_SetFog(!EnvRenderer_Minimal);
	EnvRenderer_UpdateAll();
}

void EnvRenderer_SetMode(int flags) {
	EnvRenderer_Legacy  = flags & ENV_LEGACY;
	EnvRenderer_Minimal = flags & ENV_MINIMAL;
	EnvRenderer_ContextRecreated(NULL);
}

int EnvRenderer_CalcFlags(const String* mode) {
	if (String_CaselessEqualsConst(mode, "legacyfast")) return ENV_LEGACY | ENV_MINIMAL;
	if (String_CaselessEqualsConst(mode, "legacy"))     return ENV_LEGACY;
	if (String_CaselessEqualsConst(mode, "normal"))     return 0;
	if (String_CaselessEqualsConst(mode, "normalfast")) return ENV_MINIMAL;

	return -1;
}


static void EnvRenderer_FileChanged(void* obj, struct Stream* src, const String* name) {
	if (String_CaselessEqualsConst(name, "clouds.png")) {
		Game_UpdateTexture(&clouds_tex, src, name, NULL);
	} else if (String_CaselessEqualsConst(name, "skybox.png")) {
		Game_UpdateTexture(&skybox_tex, src, name, NULL);
	} else if (String_CaselessEqualsConst(name, "snow.png")) {
		Game_UpdateTexture(&snow_tex, src, name, NULL);
	} else if (String_CaselessEqualsConst(name, "rain.png")) {
		Game_UpdateTexture(&rain_tex, src, name, NULL);
	}
}

static void EnvRenderer_TexturePackChanged(void* obj) {
	/* TODO: Find better way, really should delete them all here */
	Gfx_DeleteTexture(&skybox_tex);
}
static void EnvRenderer_TerrainAtlasChanged(void* obj) {
	EnvRenderer_UpdateBorderTextures();
}

static void EnvRenderer_ViewDistanceChanged(void* obj) {
	EnvRenderer_UpdateAll();
}

static void EnvRenderer_EnvVariableChanged(void* obj, int envVar) {
	if (envVar == ENV_VAR_EDGE_BLOCK) {
		EnvRenderer_MakeBorderTex(&edges_tex, Env.EdgeBlock);
		EnvRenderer_UpdateMapEdges();
	} else if (envVar == ENV_VAR_SIDES_BLOCK) {
		EnvRenderer_MakeBorderTex(&sides_tex, Env.SidesBlock);
		EnvRenderer_UpdateMapSides();
	} else if (envVar == ENV_VAR_EDGE_HEIGHT || envVar == ENV_VAR_SIDES_OFFSET) {
		EnvRenderer_UpdateMapEdges();
		EnvRenderer_UpdateMapSides();
	} else if (envVar == ENV_VAR_SUN_COL) {
		EnvRenderer_UpdateMapEdges();
	} else if (envVar == ENV_VAR_SHADOW_COL) {
		EnvRenderer_UpdateMapSides();
	} else if (envVar == ENV_VAR_SKY_COL) {
		EnvRenderer_UpdateSky();
	} else if (envVar == ENV_VAR_FOG_COL) {
		EnvRenderer_UpdateFog();
	} else if (envVar == ENV_VAR_CLOUDS_COL) {
		EnvRenderer_UpdateClouds();
	} else if (envVar == ENV_VAR_CLOUDS_HEIGHT) {
		EnvRenderer_UpdateSky();
		EnvRenderer_UpdateClouds();
	} else if (envVar == ENV_VAR_SKYBOX_COL) {
		EnvRenderer_UpdateSkybox();
	}
}


/*########################################################################################################################*
*--------------------------------------------------EnvRenderer component--------------------------------------------------*
*#########################################################################################################################*/
static void EnvRenderer_Init(void) {
	String renderType;
	int flags;
	Options_UNSAFE_Get(OPT_RENDER_TYPE, &renderType);

	flags = EnvRenderer_CalcFlags(&renderType);
	if (flags == -1) flags = 0;
	EnvRenderer_Legacy  = flags & ENV_LEGACY;
	EnvRenderer_Minimal = flags & ENV_MINIMAL;

	Event_RegisterEntry(&TextureEvents.FileChanged, NULL, EnvRenderer_FileChanged);
	Event_RegisterVoid(&TextureEvents.PackChanged,  NULL, EnvRenderer_TexturePackChanged);
	Event_RegisterVoid(&TextureEvents.AtlasChanged, NULL, EnvRenderer_TerrainAtlasChanged);

	Event_RegisterVoid(&GfxEvents.ViewDistanceChanged, NULL, EnvRenderer_ViewDistanceChanged);
	Event_RegisterInt(&WorldEvents.EnvVarChanged,      NULL, EnvRenderer_EnvVariableChanged);
	Event_RegisterVoid(&GfxEvents.ContextLost,         NULL, EnvRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents.ContextRecreated,    NULL, EnvRenderer_ContextRecreated);

	Game_SetViewDistance(Game_UserViewDistance);
}

static void EnvRenderer_Free(void) {
	Event_UnregisterEntry(&TextureEvents.FileChanged, NULL, EnvRenderer_FileChanged);
	Event_UnregisterVoid(&TextureEvents.PackChanged,  NULL, EnvRenderer_TexturePackChanged);
	Event_UnregisterVoid(&TextureEvents.AtlasChanged, NULL, EnvRenderer_TerrainAtlasChanged);

	Event_UnregisterVoid(&GfxEvents.ViewDistanceChanged, NULL, EnvRenderer_ViewDistanceChanged);
	Event_UnregisterInt(&WorldEvents.EnvVarChanged,      NULL, EnvRenderer_EnvVariableChanged);
	Event_UnregisterVoid(&GfxEvents.ContextLost,         NULL, EnvRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents.ContextRecreated,    NULL, EnvRenderer_ContextRecreated);

	EnvRenderer_ContextLost(NULL);
	Mem_Free(Weather_Heightmap);
	Weather_Heightmap = NULL;

	Gfx_DeleteTexture(&clouds_tex);
	Gfx_DeleteTexture(&skybox_tex);
	Gfx_DeleteTexture(&rain_tex);
	Gfx_DeleteTexture(&snow_tex);
}

static void EnvRenderer_Reset(void) {
	Gfx_SetFog(false);
	EnvRenderer_DeleteVbs();

	Mem_Free(Weather_Heightmap);
	Weather_Heightmap = NULL;
	lastPos = IVec3_MaxValue();
}

static void EnvRenderer_OnNewMapLoaded(void) {
	EnvRenderer_ContextRecreated(NULL);
}

struct IGameComponent EnvRenderer_Component = {
	EnvRenderer_Init,  /* Init  */
	EnvRenderer_Free,  /* Free  */
	EnvRenderer_Reset, /* Reset */
	EnvRenderer_Reset, /* OnNewMap */
	EnvRenderer_OnNewMapLoaded /* OnNewMapLoaded */
};
