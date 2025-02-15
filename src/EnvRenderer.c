#include "EnvRenderer.h"
#include "String.h"
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
#include "Block.h"
#include "Event.h"
#include "TexturePack.h"
#include "Platform.h"
#include "Camera.h"
#include "Particle.h"
#include "Options.h"
#include "Entity.h"

cc_bool EnvRenderer_Legacy, EnvRenderer_Minimal;

static float CalcBlendFactor(float x) {
	float blend = -0.13f + 0.28f * ((float)Math_Log2(x) * 0.17329f);
	if (blend < 0.0f) blend = 0.0f;
	if (blend > 1.0f) blend = 1.0f;
	return blend;
}

#define EnvRenderer_AxisSize() (EnvRenderer_Legacy ? 128 : 2048)
/* Returns the number of vertices needed to subdivide a quad */
static int CalcNumVertices(int axis1Len, int axis2Len) {
	int axisSize = EnvRenderer_AxisSize();
	return Math_CeilDiv(axis1Len, axisSize) * Math_CeilDiv(axis2Len, axisSize) * 4;
}


/*########################################################################################################################*
*------------------------------------------------------------Fog----------------------------------------------------------*
*#########################################################################################################################*/
static cc_bool CameraInsideBlock(BlockID block, IVec3* coords) {
	struct AABB blockBB;
	Vec3 pos;
	IVec3_ToVec3(&pos, coords); /* pos = coords; */

	Vec3_Add(&blockBB.Min, &pos, &Blocks.MinBB[block]);
	Vec3_Add(&blockBB.Max, &pos, &Blocks.MaxBB[block]);
	return AABB_ContainsPoint(&blockBB, &Camera.CurrentPos);
}

static void CalcFog(float* density, PackedCol* color) {
	IVec3 coords;
	BlockID block;
	float blend;

	IVec3_Floor(&coords, &Camera.CurrentPos); /* coords = floor(camera_pos); */
	block = World_SafeGetBlock(coords.x, coords.y, coords.z);

	if (Blocks.FogDensity[block] && CameraInsideBlock(block, &coords)) {
		*density = Blocks.FogDensity[block];
		*color   = Blocks.FogCol[block];
	} else {
		*density = 0.0f;
		/* Blend fog and sky together */
		blend    = CalcBlendFactor((float)Game_ViewDistance);
		*color   = PackedCol_Lerp(Env.FogCol, Env.SkyCol, blend);
	}
}

static void UpdateFogMinimal(float fogDensity) {
	int dist;
	/* TODO: rewrite this to avoid raising the event? want to avoid recreating vbos too many times often */

	if (fogDensity != 0.0f) {
		/* Exp fog mode: f = e^(-density*coord) */
		/* Solve coord for f = 0.05 (good approx for fog end) */
		/*   i.e. log(0.05) = -density * coord */
		#define LOG_005 -2.99573227355399f

		dist = (int)(LOG_005 / -fogDensity);
		Game_SetViewDistance(min(dist, Game_UserViewDistance));
	} else {
		Game_SetViewDistance(Game_UserViewDistance);
	}
}

static void UpdateFogNormal(float fogDensity, PackedCol fogColor) {
	float density;

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
		#define LOG_001 -4.60517018598809f

		density = -LOG_001 / (Game_ViewDistance * 0.99f);
		Gfx_SetFogDensity(density);
	} else {
		Gfx_SetFogMode(FOG_LINEAR);
		Gfx_SetFogEnd((float)Game_ViewDistance);
	}
	Gfx_SetFogCol(fogColor);
	Game_SetViewDistance(Game_UserViewDistance);
}

void EnvRenderer_UpdateFog(void) {
	float fogDensity; 
	PackedCol fogColor;
	if (!World.Loaded) return;

	CalcFog(&fogDensity, &fogColor);
	Gfx_ClearColor(fogColor);

	if (EnvRenderer_Minimal) {
		UpdateFogMinimal(fogDensity);
	} else {
		UpdateFogNormal(fogDensity, fogColor);
	}
}


/*########################################################################################################################*
*----------------------------------------------------------Clouds---------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID clouds_vb, clouds_tex;
static int clouds_vertices;

static void DrawCloudsY(int x1, int z1, int x2, int z2, int y, struct VertexTextured* v) {
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

			v->x = (float)x1; v->y = yy; v->z = (float)z1; v->Col = col; v->U = u1; v->V = v1; v++;
			v->x = (float)x1; v->y = yy; v->z = (float)z2; v->Col = col; v->U = u1; v->V = v2; v++;
			v->x = (float)x2; v->y = yy; v->z = (float)z2; v->Col = col; v->U = u2; v->V = v2; v++;
			v->x = (float)x2; v->y = yy; v->z = (float)z1; v->Col = col; v->U = u2; v->V = v1; v++;
		}
	}
}

static CC_NOINLINE void BuildClouds(void) {
	struct VertexTextured* data;
	int extent;
	int x1, z1, x2, z2;

	if (!World.Loaded || EnvRenderer_Minimal) return;

	extent = Utils_AdjViewDist(Game_ViewDistance);
	x1 = -extent; x2 = World.Width  + extent;
	z1 = -extent; z2 = World.Length + extent;
	clouds_vertices = CalcNumVertices(x2 - x1, z2 - z1);

	clouds_vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, clouds_vertices);
	data = (struct VertexTextured*)Gfx_LockVb(clouds_vb,
										VERTEX_FORMAT_TEXTURED, clouds_vertices);

	DrawCloudsY(x1, z1, x2, z2, Env.CloudsHeight, data);
	Gfx_UnlockVb(clouds_vb);
}

void EnvRenderer_RenderClouds(void) {
	float offset;
	if (Env.CloudsHeight < -2000) return;

	if (!clouds_vb) {
		BuildClouds();
		if (!clouds_vb) return;
	}
	
	offset = (float)(Game.Time / 2048.0f * 0.6f * Env.CloudsSpeed);

	Gfx_EnableTextureOffset(offset, 0);
	Gfx_SetAlphaTest(true);
	Gfx_BindTexture(clouds_tex);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindVb(clouds_vb);
	Gfx_DrawVb_IndexedTris(clouds_vertices);
	Gfx_SetAlphaTest(false);
	Gfx_DisableTextureOffset();
}


/*########################################################################################################################*
*------------------------------------------------------------Sky----------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID sky_vb;
static int sky_vertices;

static void DrawSkyY(int x1, int z1, int x2, int z2, int y, struct VertexColoured* v) {
	int endX = x2, endZ = z2, startZ = z1, axisSize = EnvRenderer_AxisSize();
	PackedCol col = Env.SkyCol;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;

		for (z1 = startZ; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			v->x = (float)x1; v->y = (float)y; v->z = (float)z1; v->Col = col; v++;
			v->x = (float)x1; v->y = (float)y; v->z = (float)z2; v->Col = col; v++;
			v->x = (float)x2; v->y = (float)y; v->z = (float)z2; v->Col = col; v++;
			v->x = (float)x2; v->y = (float)y; v->z = (float)z1; v->Col = col; v++;
		}
	}
}

static CC_NOINLINE void BuildSky(void) {
	struct VertexColoured* data;
	int extent, height;
	int x1, z1, x2, z2;

	if (!World.Loaded || EnvRenderer_Minimal) return;

	extent = Utils_AdjViewDist(Game_ViewDistance);
	x1 = -extent; x2 = World.Width  + extent;
	z1 = -extent; z2 = World.Length + extent;
	sky_vertices = CalcNumVertices(x2 - x1, z2 - z1);

	sky_vb = Gfx_CreateVb(VERTEX_FORMAT_COLOURED, sky_vertices);
	data   = (struct VertexColoured*)Gfx_LockVb(sky_vb,
										VERTEX_FORMAT_COLOURED, sky_vertices);

	height = max((World.Height + 2), Env.CloudsHeight) + 6;
	DrawSkyY(x1, z1, x2, z2, height, data);
	Gfx_UnlockVb(sky_vb);
}

void EnvRenderer_RenderSky(void) {
	struct Matrix m;
	float skyY, normY, dy;
	if (!sky_vb || EnvRenderer_ShouldRenderSkybox()) return;

	if (!sky_vb) {
		BuildSky();
		if (!sky_vb) return;
	}

	normY = (float)World.Height + 8.0f;
	skyY  = max(Camera.CurrentPos.y + 8.0f, normY);
	Gfx_SetVertexFormat(VERTEX_FORMAT_COLOURED);
	Gfx_BindVb(sky_vb);

	if (skyY == normY) {
		Gfx_DrawVb_IndexedTris(sky_vertices);
	} else {
		m  = Gfx.View;
		dy = skyY - normY; 
		/* inlined Y translation matrix multiply */
		m.row4.x += dy * m.row2.x; m.row4.y += dy * m.row2.y;
		m.row4.z += dy * m.row2.z; m.row4.w += dy * m.row2.w;

		Gfx_LoadMatrix(MATRIX_VIEW, &m);
		Gfx_DrawVb_IndexedTris(sky_vertices);
		Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	}
}

/*########################################################################################################################*
*----------------------------------------------------------Skybox---------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID skybox_tex, skybox_vb;
#define SKYBOX_COUNT (6 * 4)
cc_bool EnvRenderer_ShouldRenderSkybox(void) { return skybox_tex && !EnvRenderer_Minimal; }

static CC_NOINLINE void BuildSkybox(void) {
	static const struct VertexTextured vertices[SKYBOX_COUNT] = {
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
	struct VertexTextured* data;
	int i;

	skybox_vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, SKYBOX_COUNT);
	data = (struct VertexTextured*)Gfx_LockVb(skybox_vb,
										VERTEX_FORMAT_TEXTURED, SKYBOX_COUNT);

	Mem_Copy(data, vertices, sizeof(vertices));
	for (i = 0; i < SKYBOX_COUNT; i++) { data[i].Col = Env.SkyboxCol; }
	Gfx_UnlockVb(skybox_vb);
}

void EnvRenderer_RenderSkybox(void) {
	struct Matrix m, rotX, rotY, view;
	float rotTime;
	Vec3 pos;

	if (!skybox_vb) {
		BuildSkybox();
		if (!skybox_vb) return;
	}

	Gfx_SetDepthWrite(false);
	Gfx_BindTexture(skybox_tex);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);

	/* Base skybox rotation */
	rotTime = (float)(Game.Time * 2 * MATH_PI); /* So speed of 1 rotates whole skybox every second */
	Matrix_RotateY(&rotY, Env.SkyboxHorSpeed * rotTime);
	Matrix_RotateX(&rotX, Env.SkyboxVerSpeed * rotTime);
	Matrix_Mul(&m, &rotY, &rotX);

	/* Rotate around camera */
	pos = Camera.CurrentPos;
	Vec3_Set(Camera.CurrentPos, 0,0,0);
	Camera.Active->GetView(&view); 
	Matrix_MulBy(&m, &view);
	Camera.CurrentPos = pos;

	Gfx_LoadMatrix(MATRIX_VIEW, &m);
	Gfx_BindVb(skybox_vb);
	Gfx_DrawVb_IndexedTris(SKYBOX_COUNT);

	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);
	Gfx_SetDepthWrite(true);
}

/*########################################################################################################################*
*----------------------------------------------------------Weather--------------------------------------------------------*
*#########################################################################################################################*/
cc_int16* Weather_Heightmap;
static GfxResourceID rain_tex, snow_tex, weather_vb;
static float weather_accumulator;
static IVec3 lastPos;

#define WEATHER_EXTENT 4
#define WEATHER_VERTS  8 /* 2 quads per tile */
#define WEATHER_RANGE  (WEATHER_EXTENT * 2 + 1)

#define WEATHER_VERTS_COUNT WEATHER_RANGE * WEATHER_RANGE * WEATHER_VERTS
#define Weather_Pack(x, z) ((x) * World.Length + (z))

static void InitWeatherHeightmap(void) {
	int i;
	Weather_Heightmap = (cc_int16*)Mem_Alloc(World.Width * World.Length, 2, "weather heightmap");
	
	for (i = 0; i < World.Width * World.Length; i++) {
		Weather_Heightmap[i] = Int16_MaxValue;
	}
}

#define RainCalcBody(get_block)\
for (y = maxY; y >= 0; y--, i -= World.OneY) {\
	draw = Blocks.Draw[get_block];\
\
	if (!(draw == DRAW_GAS || draw == DRAW_SPRITE)) {\
		Weather_Heightmap[hIndex] = y;\
		return y;\
	}\
}

static int CalcRainHeightAt(int x, int maxY, int z, int hIndex) {
	int i = World_Pack(x, maxY, z), y;
	cc_uint8 draw;

#ifndef EXTENDED_BLOCKS
	RainCalcBody(World.Blocks[i]);
#else
	if (World.IDMask <= 0xFF) {
		RainCalcBody(World.Blocks[i]);
	} else {
		RainCalcBody(World.Blocks[i] | (World.Blocks2[i] << 8));
	}
#endif

	Weather_Heightmap[hIndex] = -1;
	return -1;
}

static float GetRainHeight(int x, int z) {
	int hIndex, height;
	int y;
	if (!World_ContainsXZ(x, z)) return (float)Env.EdgeHeight;

	hIndex = Weather_Pack(x, z);
	height = Weather_Heightmap[hIndex];

	y = height == Int16_MaxValue ? CalcRainHeightAt(x, World.MaxY, z, hIndex) : height;
	return y == -1 ? 0 : y + Blocks.MaxBB[World_GetBlock(x, y, z)].y;
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
		CalcRainHeightAt(x, y, z, hIndex);
	}
}

static float CalcRainAlphaAt(float x) {
	/* Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9} */
	float falloff = 0.05f * x * x - 7 * x;
	return 178 + falloff * Env.WeatherFade;
}

struct RainCoord { int dx, dz; float y; };
static RNGState snowDirRng;

void EnvRenderer_RenderWeather(float delta) {
	struct RainCoord coords[WEATHER_RANGE * WEATHER_RANGE];
	int i, weather, numCoords = 0;
	struct VertexTextured* v;
	cc_bool moved, particles;
	float speed, vOffsetBase, vOffset;
	IVec3 pos;

	PackedCol color;
	int dist, dx, dz, x, z;
	float alpha, y, height;
	float uOffset1, uOffset2, uSpeed;
	float worldV, v1, v2, vPlane1Offset;
	float x1,y1,z1, x2,y2,z2;

	weather = Env.Weather;
	if (weather == WEATHER_SUNNY) return;

	if (!Weather_Heightmap) 
		InitWeatherHeightmap();
	if (!weather_vb)
		weather_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_TEXTURED, WEATHER_VERTS_COUNT);

	IVec3_Floor(&pos, &Camera.CurrentPos);
	moved   = pos.x != lastPos.x || pos.y != lastPos.y || pos.z != lastPos.z;
	lastPos = pos;

	/* Rain should extend up by 64 blocks, or to the top of the world. */
	pos.y += 64;
	pos.y = max(World.Height, pos.y);

	weather_accumulator += delta;
	particles = weather == WEATHER_RAINY && (weather_accumulator >= 0.25f || moved);

	for (dx = -WEATHER_EXTENT; dx <= WEATHER_EXTENT; dx++) {
		for (dz = -WEATHER_EXTENT; dz <= WEATHER_EXTENT; dz++) {
			x = pos.x + dx; z = pos.z + dz;

			y = GetRainHeight(x, z);
			if (pos.y <= y) continue;
			if (particles) Particles_RainSnowEffect((float)x, y, (float)z);

			coords[numCoords].dx = dx;
			coords[numCoords].y  = y;
			coords[numCoords].dz = dz;
			numCoords++;
		}
	}

	Gfx_BindTexture(weather == WEATHER_RAINY ? rain_tex : snow_tex);
	if (particles) weather_accumulator = 0;
	if (!numCoords) return;

	Gfx_SetAlphaTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaArgBlend(true);

	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	v = (struct VertexTextured*)Gfx_LockDynamicVb(weather_vb, 
										VERTEX_FORMAT_TEXTURED, numCoords * WEATHER_VERTS);

	color = Env.SunCol;
	speed = (weather == WEATHER_RAINY ? 1.0f : 0.2f) * Env.WeatherSpeed;

	vOffsetBase   = (float)Game.Time * speed;
	vPlane1Offset = weather == WEATHER_RAINY  ? 0 : 0.25f; /* Offset v on 1 plane while snowing to avoid the unnatural mirrored texture effect */

	for (i = 0; i < numCoords; i++)
	{
		dx = coords[i].dx;
		y  = coords[i].y;
		dz = coords[i].dz;

		height = pos.y - y;

		dist  = dx * dx + dz * dz;
		alpha = CalcRainAlphaAt((float)dist);
		Math_Clamp(alpha, 0.0f, 255.0f);
		color = (color & PACKEDCOL_RGB_MASK) | PackedCol_A_Bits(alpha);

		x = dx + pos.x;
		z = dz + pos.z;

		uOffset1 = 0;
		uOffset2 = 0;
		if (weather == WEATHER_SNOWY) {
			Random_Seed(&snowDirRng, (x + 1217 * z) & 0x7fffffff);

			/* Multiply horizontal speed by a random float from -1 to 1 */
			uSpeed   = (float)Game.Time * Env.WeatherSpeed * 0.5f;
			uOffset1 = uSpeed * (Random_Float(&snowDirRng) * 2 + -1);
			uOffset2 = uSpeed * (Random_Float(&snowDirRng) * 2 + -1);

			/* Multiply vertical speed by a random float from 1.0 to 0.25 */
			vOffset = vOffsetBase * (float)(Random_Float(&snowDirRng) * (1.0f - 0.25f) + 0.25f);
		} else {
			vOffset = vOffsetBase;
		}
		
		worldV = vOffset + (z & 1) / 2.0f - (x & 0x0F) / 16.0f;
		v1 = y            / 6.0f + worldV; 
		v2 = (y + height) / 6.0f + worldV;
		x1 = (float)x;       y1 = (float)y;            z1 = (float)z;
		x2 = (float)(x + 1); y2 = (float)(y + height); z2 = (float)(z + 1);

		v->x = x1; v->y = y1; v->z = z1; v->Col = color; v->U = uOffset1;        v->V = v1 + vPlane1Offset; v++;
		v->x = x1; v->y = y2; v->z = z1; v->Col = color; v->U = uOffset1;        v->V = v2 + vPlane1Offset; v++;
		v->x = x2; v->y = y2; v->z = z2; v->Col = color; v->U = uOffset1 + 1.0f; v->V = v2 + vPlane1Offset; v++;
		v->x = x2; v->y = y1; v->z = z2; v->Col = color; v->U = uOffset1 + 1.0f; v->V = v1 + vPlane1Offset; v++;

		v->x = x2; v->y = y1; v->z = z1; v->Col = color; v->U = uOffset2 + 1.0f; v->V = v1; v++;
		v->x = x2; v->y = y2; v->z = z1; v->Col = color; v->U = uOffset2 + 1.0f; v->V = v2; v++;
		v->x = x1; v->y = y2; v->z = z2; v->Col = color; v->U = uOffset2;        v->V = v2; v++;
		v->x = x1; v->y = y1; v->z = z2; v->Col = color; v->U = uOffset2;        v->V = v1; v++;
	}

	Gfx_UnlockDynamicVb(weather_vb);
	Gfx_DrawVb_IndexedTris(numCoords * WEATHER_VERTS);

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

static void MakeBorderTex(GfxResourceID* texId, BlockID block) {
	TextureLoc loc = Block_Tex(block, FACE_YMAX);
	if (Gfx.LostContext) return;

	Gfx_DeleteTexture(texId);
	*texId = Atlas2D_LoadTile(loc);
}

static Rect2D EnvRenderer_Rect(int x, int y, int width, int height) {
	Rect2D r;
	r.x = x; r.y = y; r.width = width; r.height = height; 
	return r;
}

static void CalcBorderRects(Rect2D* rects) {
	int extent = Utils_AdjViewDist(Game_ViewDistance);
	rects[0] = EnvRenderer_Rect(-extent, -extent,      extent + World.Width + extent, extent);
	rects[1] = EnvRenderer_Rect(-extent, World.Length, extent + World.Width + extent, extent);

	rects[2] = EnvRenderer_Rect(-extent,     0, extent, World.Length);
	rects[3] = EnvRenderer_Rect(World.Width, 0, extent, World.Length);
}

static void UpdateBorderTextures(void) {
	MakeBorderTex(&edges_tex, Env.EdgeBlock);
	MakeBorderTex(&sides_tex, Env.SidesBlock);
}

#define Borders_HorOffset(block) (Blocks.RenderMinBB[block].x - Blocks.MinBB[block].x)
#define Borders_YOffset(block)   (Blocks.RenderMinBB[block].y - Blocks.MinBB[block].y)

static void DrawBorderX(int x, int z1, int z2, int y1, int y2, PackedCol color, struct VertexTextured** vertices) {
	int endZ = z2, endY = y2, startY = y1, axisSize = EnvRenderer_AxisSize();
	float u2, v2;
	struct VertexTextured* v = *vertices;

	for (; z1 < endZ; z1 += axisSize) {
		z2 = z1 + axisSize;
		if (z2 > endZ) z2 = endZ;

		for (y1 = startY; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			u2   = (float)z2 - (float)z1;      v2   = (float)y2 - (float)y1;
			v->x = (float)x; v->y = (float)y1; v->z = (float)z1; v->Col = color; v->U = 0;  v->V = v2; v++;
			v->x = (float)x; v->y = (float)y2; v->z = (float)z1; v->Col = color; v->U = 0;  v->V = 0;  v++;
			v->x = (float)x; v->y = (float)y2; v->z = (float)z2; v->Col = color; v->U = u2; v->V = 0;  v++;
			v->x = (float)x; v->y = (float)y1; v->z = (float)z2; v->Col = color; v->U = u2; v->V = v2; v++;
		}
	}
	*vertices = v;
}

static void DrawBorderZ(int z, int x1, int x2, int y1, int y2, PackedCol color, struct VertexTextured** vertices) {
	int endX = x2, endY = y2, startY = y1, axisSize = EnvRenderer_AxisSize();
	float u2, v2;
	struct VertexTextured* v = *vertices;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;

		for (y1 = startY; y1 < endY; y1 += axisSize) {
			y2 = y1 + axisSize;
			if (y2 > endY) y2 = endY;

			u2   = (float)x2 - (float)x1;       v2   = (float)y2 - (float)y1;
			v->x = (float)x1; v->y = (float)y1; v->z = (float)z; v->Col = color; v->U = 0;  v->V = v2; v++;
			v->x = (float)x1; v->y = (float)y2; v->z = (float)z; v->Col = color; v->U = 0;  v->V = 0;  v++;
			v->x = (float)x2; v->y = (float)y2; v->z = (float)z; v->Col = color; v->U = u2; v->V = 0;  v++;
			v->x = (float)x2; v->y = (float)y1; v->z = (float)z; v->Col = color; v->U = u2; v->V = v2; v++;
		}
	}
	*vertices = v;
}

static void DrawBorderY(int x1, int z1, int x2, int z2, float y, PackedCol color, float offset, float yOffset, struct VertexTextured** vertices) {
	int endX = x2, endZ = z2, startZ = z1, axisSize = EnvRenderer_AxisSize();
	float u2, v2;
	struct VertexTextured* v = *vertices;
	float yy = y + yOffset;

	for (; x1 < endX; x1 += axisSize) {
		x2 = x1 + axisSize;
		if (x2 > endX) x2 = endX;
		
		for (z1 = startZ; z1 < endZ; z1 += axisSize) {
			z2 = z1 + axisSize;
			if (z2 > endZ) z2 = endZ;

			u2   = (float)x2 - (float)x1;         v2   = (float)z2 - (float)z1;
			v->x = (float)x1 + offset; v->y = yy; v->z = (float)z1 + offset; v->Col = color; v->U = 0;  v->V = 0;  v++;
			v->x = (float)x1 + offset; v->y = yy; v->z = (float)z2 + offset; v->Col = color; v->U = 0;  v->V = v2; v++;
			v->x = (float)x2 + offset; v->y = yy; v->z = (float)z2 + offset; v->Col = color; v->U = u2; v->V = v2; v++;
			v->x = (float)x2 + offset; v->y = yy; v->z = (float)z1 + offset; v->Col = color; v->U = u2; v->V = 0;  v++;
		}
	}
	*vertices = v;
}

static CC_NOINLINE void BuildMapSides(void) {
	Rect2D rects[4], r;
	BlockID block;
	PackedCol color;
	int y, y1, y2;
	int i;
	struct VertexTextured* data;

	if (!World.Loaded) return;
	block = Env.SidesBlock;

	if (Blocks.Draw[block] == DRAW_GAS) return;
	CalcBorderRects(rects);

	sides_vertices = 0;
	for (i = 0; i < 4; i++) {
		r = rects[i];
		sides_vertices += CalcNumVertices(r.width, r.height); /* YQuads outside */
	}

	y = Env_SidesHeight;
	sides_vertices +=     CalcNumVertices(World.Width, World.Length);  /* YQuads beneath map */
	sides_vertices += 2 * CalcNumVertices(World.Width,  Math_AbsI(y)); /* ZQuads */
	sides_vertices += 2 * CalcNumVertices(World.Length, Math_AbsI(y)); /* XQuads */

	sides_vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, sides_vertices);
	data = (struct VertexTextured*)Gfx_LockVb(sides_vb,
										VERTEX_FORMAT_TEXTURED, sides_vertices);

	sides_fullBright = Blocks.Brightness[block];
	color = sides_fullBright ? PACKEDCOL_WHITE : Env.ShadowCol;
	Block_Tint(color, block)

	for (i = 0; i < 4; i++) {
		r = rects[i];
		DrawBorderY(r.x, r.y, r.x + r.width, r.y + r.height, (float)y, color,
			0, Borders_YOffset(block), &data);
	}

	/* Work properly for when ground level is below 0 */
	y1 = 0; y2 = y;
	if (y < 0) { y1 = y; y2 = 0; }

	DrawBorderY(0, 0, World.Width, World.Length, 0, color, 0, 0, &data);
	DrawBorderZ(0, 0, World.Width, y1, y2, color, &data);
	DrawBorderZ(World.Length, 0, World.Width, y1, y2, color, &data);
	DrawBorderX(0, 0, World.Length, y1, y2, color, &data);
	DrawBorderX(World.Width, 0, World.Length, y1, y2, color, &data);

	Gfx_UnlockVb(sides_vb);
}

static CC_NOINLINE void BuildMapEdges(void) {
	Rect2D rects[4], r;
	BlockID block;
	PackedCol color;
	float y;
	int i;
	struct VertexTextured* data;

	if (!World.Loaded) return;
	block = Env.EdgeBlock;

	if (Blocks.Draw[block] == DRAW_GAS) return;
	CalcBorderRects(rects);

	edges_vertices = 0;
	for (i = 0; i < 4; i++) {
		r = rects[i];
		edges_vertices += CalcNumVertices(r.width, r.height); /* YPlanes outside */
	}

	edges_vb = Gfx_CreateVb(VERTEX_FORMAT_TEXTURED, edges_vertices);
	data = (struct VertexTextured*)Gfx_LockVb(edges_vb,
										VERTEX_FORMAT_TEXTURED, edges_vertices);

	edges_fullBright = Blocks.Brightness[block];
	color = edges_fullBright ? PACKEDCOL_WHITE : Env.SunCol;
	Block_Tint(color, block)

	y = (float)Env.EdgeHeight;
	for (i = 0; i < 4; i++) {
		r = rects[i];
		DrawBorderY(r.x, r.y, r.x + r.width, r.y + r.height, y, color,
			Borders_HorOffset(block), Borders_YOffset(block), &data);
	}
	Gfx_UnlockVb(edges_vb);
}

static void RenderBorders(BlockID block, GfxResourceID vb, GfxResourceID tex, int count) {
	Gfx_SetupAlphaState(Blocks.Draw[block]);
	Gfx_EnableMipmaps();

	Gfx_BindTexture(tex);
	Gfx_SetVertexFormat(VERTEX_FORMAT_TEXTURED);
	Gfx_BindVb(vb);
	Gfx_DrawVb_IndexedTris(count);

	Gfx_DisableMipmaps();
	Gfx_RestoreAlphaState(Blocks.Draw[block]);
}

void EnvRenderer_RenderMapSides(void) {
	if (!sides_vb) {
		BuildMapSides();
		if (!sides_vb) return;
	}

	RenderBorders(Env.SidesBlock, sides_vb, sides_tex, sides_vertices);
}

void EnvRenderer_RenderMapEdges(void) {
	/* Do not draw water when player cannot see it */
	/* Fixes some 'depth bleeding through' issues with 16 bit depth buffers on large maps */
	int yVisible = min(0, Env_SidesHeight);
	if (Camera.CurrentPos.y < yVisible && sides_vb) return;

	if (!edges_vb) {
		BuildMapEdges();
		if (!edges_vb) return;
	}
	RenderBorders(Env.EdgeBlock, edges_vb, edges_tex, edges_vertices);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void CloudsPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&clouds_tex, stream, name, NULL, NULL);
}
static struct TextureEntry clouds_entry = { "clouds.png", CloudsPngProcess };

static void SkyboxPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&skybox_tex, stream, name, NULL, NULL);
}
static struct TextureEntry skybox_entry = { "skybox.png", SkyboxPngProcess };

static void SnowPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&snow_tex, stream, name, NULL, NULL);
}
static struct TextureEntry snow_entry = { "snow.png", SnowPngProcess };

static void RainPngProcess(struct Stream* stream, const cc_string* name) {
	Game_UpdateTexture(&rain_tex, stream, name, NULL, NULL);
}
static struct TextureEntry rain_entry = { "rain.png", RainPngProcess };


static void DeleteSkyVB(void)     { Gfx_DeleteVb(&sky_vb); }
static void DeleteCloudsVB(void)  { Gfx_DeleteVb(&clouds_vb); }
static void DeleteSkyboxVB(void)  { Gfx_DeleteVb(&skybox_vb); }
static void DeleteSidesVB(void)   { Gfx_DeleteVb(&sides_vb); }
static void DeleteEdgesVB(void)   { Gfx_DeleteVb(&edges_vb); }
static void DeleteWeatherVB(void) { Gfx_DeleteDynamicVb(&weather_vb); }

static void DeleteStaticVbs(void) {
	DeleteSkyVB();
	DeleteCloudsVB();
	DeleteSkyboxVB();
	DeleteSidesVB();
	DeleteEdgesVB();
}

static void OnContextLost(void* obj) {
	DeleteStaticVbs();
	DeleteWeatherVB();
	Gfx_DeleteTexture(&sides_tex);
	Gfx_DeleteTexture(&edges_tex);

	if (Gfx.ManagedTextures) return;
	Gfx_DeleteTexture(&clouds_tex);
	Gfx_DeleteTexture(&skybox_tex);
	Gfx_DeleteTexture(&rain_tex);
	Gfx_DeleteTexture(&snow_tex);
}

static void UpdateAll(void) {
	DeleteStaticVbs();
	EnvRenderer_UpdateFog();

	DeleteWeatherVB();
	/* TODO: Unnecessary to delete the weather VB? */
	if (Gfx.LostContext) return;
	/* TODO: Don't need to do this on every new map */
	UpdateBorderTextures();
}

static void OnContextRecreated(void* obj) {
	Gfx_SetFog(!EnvRenderer_Minimal);
	UpdateAll();
}

void EnvRenderer_SetMode(int flags) {
	EnvRenderer_Legacy  = flags & ENV_LEGACY;
	EnvRenderer_Minimal = flags & ENV_MINIMAL;
	OnContextRecreated(NULL);
}

int EnvRenderer_CalcFlags(const cc_string* mode) {
	if (String_CaselessEqualsConst(mode, "normal")) return 0;
	if (String_CaselessEqualsConst(mode, "legacy")) return ENV_LEGACY;
	if (String_CaselessEqualsConst(mode, "fast"))   return ENV_MINIMAL;
	/* backwards compatibility */
	if (String_CaselessEqualsConst(mode, "normalfast")) return ENV_MINIMAL;
	if (String_CaselessEqualsConst(mode, "legacyfast")) return ENV_LEGACY | ENV_MINIMAL;

	return -1;
}


static void OnTexturePackChanged(void* obj) {
	/* TODO: Find better way, really should delete them all here */
	Gfx_DeleteTexture(&skybox_tex);
}
static void OnTerrainAtlasChanged(void* obj) { UpdateBorderTextures(); }
static void OnViewDistanceChanged(void* obj) { UpdateAll(); }

static void OnEnvVariableChanged(void* obj, int envVar) {
	if (envVar == ENV_VAR_EDGE_BLOCK) {
		MakeBorderTex(&edges_tex, Env.EdgeBlock);
		DeleteEdgesVB();
	} else if (envVar == ENV_VAR_SIDES_BLOCK) {
		MakeBorderTex(&sides_tex, Env.SidesBlock);
		DeleteSidesVB();
	} else if (envVar == ENV_VAR_EDGE_HEIGHT || envVar == ENV_VAR_SIDES_OFFSET) {
		DeleteEdgesVB();
		DeleteSidesVB();
	} else if (envVar == ENV_VAR_SUN_COLOR) {
		DeleteEdgesVB();
	} else if (envVar == ENV_VAR_SHADOW_COLOR) {
		DeleteSidesVB();
	} else if (envVar == ENV_VAR_SKY_COLOR) {
		DeleteSkyVB();
	} else if (envVar == ENV_VAR_FOG_COLOR) {
		EnvRenderer_UpdateFog();
	} else if (envVar == ENV_VAR_CLOUDS_COLOR) {
		DeleteCloudsVB();
	} else if (envVar == ENV_VAR_CLOUDS_HEIGHT) {
		DeleteSkyVB();
		DeleteCloudsVB();
	} else if (envVar == ENV_VAR_SKYBOX_COLOR) {
		DeleteSkyboxVB();
	}
}


/*########################################################################################################################*
*--------------------------------------------------EnvRenderer component--------------------------------------------------*
*#########################################################################################################################*/
static void OnInit(void) {
	cc_string renderType;
	int flags;
	Options_UNSAFE_Get(OPT_RENDER_TYPE, &renderType);

	flags = EnvRenderer_CalcFlags(&renderType);
	if (flags == -1) flags = 0;
	EnvRenderer_Legacy  = flags & ENV_LEGACY;
	EnvRenderer_Minimal = flags & ENV_MINIMAL;

	TextureEntry_Register(&clouds_entry);
	TextureEntry_Register(&skybox_entry);
	TextureEntry_Register(&snow_entry);
	TextureEntry_Register(&rain_entry);

	Event_Register_(&TextureEvents.PackChanged,  NULL, OnTexturePackChanged);
	Event_Register_(&TextureEvents.AtlasChanged, NULL, OnTerrainAtlasChanged);

	Event_Register_(&GfxEvents.ViewDistanceChanged, NULL, OnViewDistanceChanged);
	Event_Register_(&WorldEvents.EnvVarChanged,     NULL, OnEnvVariableChanged);
	Event_Register_(&GfxEvents.ContextLost,         NULL, OnContextLost);
	Event_Register_(&GfxEvents.ContextRecreated,    NULL, OnContextRecreated);

	Game_SetViewDistance(Game_UserViewDistance);
}

static void OnFree(void) {
	OnContextLost(NULL);
	Mem_Free(Weather_Heightmap);
	Weather_Heightmap = NULL;
}

static void OnReset(void) {
	Gfx_SetFog(false);
	DeleteStaticVbs();
	DeleteWeatherVB();

	Mem_Free(Weather_Heightmap);
	Weather_Heightmap = NULL;
	lastPos = IVec3_MaxValue();
}

static void OnNewMapLoaded(void) { OnContextRecreated(NULL); }

struct IGameComponent EnvRenderer_Component = {
	OnInit,  /* Init  */
	OnFree,  /* Free  */
	OnReset, /* Reset */
	OnReset, /* OnNewMap */
	OnNewMapLoaded /* OnNewMapLoaded */
};
