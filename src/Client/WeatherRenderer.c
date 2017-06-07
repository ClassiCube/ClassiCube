#include "WeatherRenderer.h"
#include "Block.h"
#include "BlockEnums.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "EventHandler.h"
#include "Game.h"
#include "GameProps.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "GraphicsCommon.h"
#include "MiscEvents.h"
#include "PackedCol.h"
#include "Platform.h"
#include "Vector3I.h"
#include "VertexStructs.h"
#include "World.h"
#include "WorldEnv.h"

GfxResourceID weather_rainTex;
GfxResourceID weather_snowTex;
GfxResourceID weather_vb;

#define weather_extent 4
#define weather_verticesCount 8 * (weather_extent * 2 + 1) * (weather_extent * 2 + 1)
VertexP3fT2fC4b weather_vertices[weather_verticesCount];

Int16* weather_heightmap;
Real64 weather_accumulator;
Vector3I weather_lastPos;

IGameComponent WeatherRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = WeatherRenderer_Init;
	comp.Free = WeatherRenderer_Free;
	comp.OnNewMap = WeatherRenderer_Reset;
	comp.Reset = WeatherRenderer_Reset;
	return comp;
}

void WeatherRenderer_Init(void) {
	EventHandler_RegisterStream(TextureEvents_FileChanged, &WeatherRenderer_FileChanged);
	weather_lastPos = Vector3I_Create1(Int32_MaxValue);

	WeatherRenderer_ContextRecreated();
	EventHandler_RegisterVoid(Gfx_ContextLost, &WeatherRenderer_ContextLost);
	EventHandler_RegisterVoid(Gfx_ContextRecreated, &WeatherRenderer_ContextRecreated);
}

void WeatherRenderer_Render(Real64 deltaTime) {
	Weather weather = WorldEnv_Weather;
	if (weather == Weather_Sunny) return;
	if (weather_heightmap == NULL) WeatherRenderer_InitHeightmap();

	Gfx_BindTexture(weather == Weather_Rainy ? weather_rainTex : weather_snowTex);
	Vector3 camPos = Game_CurrentCameraPos;
	Vector3I pos;
	Vector3I_Floor(&pos, &camPos);
	bool moved = Vector3I_NotEquals(&pos, &weather_lastPos);
	weather_lastPos = pos;

	/* Rain should extend up by 64 blocks, or to the top of the world. */
	pos.Y += 64;
	pos.Y = max(World_Height, pos.Y);

	Real32 speed = (weather == Weather_Rainy ? 1.0f : 0.2f) * WorldEnv_WeatherSpeed;
	Real32 vOffset = (Real32)Game_Accumulator * speed;
	weather_accumulator += deltaTime;
	bool particles = weather == Weather_Rainy;

	Int32 index = 0; // TODO: SET THIS VALUE!!!!
	PackedCol col = WorldEnv_SunCol;
	VertexP3fT2fC4b v;
	VertexP3fT2fC4b* ptr = weather_vertices;

	for (Int32 dx = -weather_extent; dx <= weather_extent; dx++)
		for (Int32 dz = -weather_extent; dz <= weather_extent; dz++)
		{
			Int32 x = pos.X + dx, z = pos.Z + dz;
			Real32 y = WeatherRenderer_RainHeight(x, z);
			Real32 height = pos.Y - y;
			if (height <= 0) continue;

			if (particles && (weather_accumulator >= 0.25 || moved))
				ParticleManager_AddRainParticle(x, y, z);

			Real32 alpha = WeatherRenderer_AlphaAt(dx * dx + dz * dz);
			/* Clamp between 0 and 255 */
			alpha = alpha < 0.0f ? 0.0f : alpha;
			alpha = alpha > 255.0f ? 255.0f : alpha;
			col.A = (UInt8)alpha;

			/* NOTE: Making vertex is inlined since this is called millions of times. */
			v.Colour = col;
			Real32 worldV = vOffset + (z & 1) / 2.0f - (x & 0x0F) / 16.0f;
			Real32 v1 = y / 6.0f + worldV, v2 = (y + height) / 6.0f + worldV;
#define AddVertex *ptr = v; ptr++;

			v.X = x; v.Y = y; v.Z = z; v.U = 0.0f; v.V = v1; AddVertex
			/* (x, y, z)                  (0, v1) */
			v.Y = y + height; v.V = v2; 					 AddVertex
			/* (x, y + height, z)         (0, v2) */
			v.X = x + 1; v.Z = z + 1; v.U = 1.0f;			 AddVertex
			/* (x + 1, y + height, z + 1) (1, v2) */
			v.Y = y; v.V = v1; 								 AddVertex
			/* (x + 1, y, z + 1)          (1, v1) */

			v.Z = z;										 AddVertex
			/* (x + 1, y, z)              (1, v1) */
			v.Y = y + height; v.V = v2; 					 AddVertex
			/* (x + 1, y + height, z)     (1, v2) */
			v.X = x; v.Z = z + 1; v.U = 0.0f;				 AddVertex
			/* (x, y + height, z + 1)     (0, v2) */
			v.Y = y; v.V = v1; 								 AddVertex
			/* (x y, z + 1)               (0, v1) */
		}

	if (particles && (weather_accumulator >= 0.25 || moved))
		weather_accumulator = 0;
	if (index == 0) return;

	Gfx_SetAlphaTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaArgBlend(true);

	Gfx_SetBatchFormat(VertexFormat_P3fT2fC4b);
	GfxCommon_UpdateDynamicIndexedVb(DrawMode_Triangles, weather_vb, weather_vertices, index);

	Gfx_SetAlphaArgBlend(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaTest(false);
}

Real32 WeatherRenderer_AlphaAt(Real32 x) {
	/* Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9} */
	Real32 falloff = 0.05f * x * x - 7 * x;
	return 178 + falloff * WorldEnv_WeatherFade;
}

void WeatherRenderer_Reset(void) {
	if (weather_heightmap != NULL) Platform_MemFree(weather_heightmap);
	weather_heightmap = NULL;
	weather_lastPos = Vector3I_Create1(Int32_MaxValue);
}

void WeatherRenderer_FileChanged(Stream* stream) {
	String snow = String_FromConstant("snow.png");
	String rain = String_FromConstant("rain.png");

	if (String_Equals(&stream->Name, &snow)) {
		Game_UpdateTexture(&weather_snowTex, stream, false);
	} else if (String_Equals(&stream->Name, &rain)) {
		Game_UpdateTexture(&weather_rainTex, stream, false);
	}
}

void WeatherRenderer_Free(void) {
	Gfx_DeleteTexture(&weather_rainTex);
	Gfx_DeleteTexture(&weather_snowTex);
	WeatherRenderer_ContextLost();
	WeatherRenderer_Reset();

	EventHandler_UnregisterStream(TextureEvents_FileChanged, &WeatherRenderer_FileChanged);
	EventHandler_UnregisterVoid(Gfx_ContextLost, &WeatherRenderer_ContextLost);
	EventHandler_UnregisterVoid(Gfx_ContextRecreated, &WeatherRenderer_ContextRecreated);
}

void WeatherRenderer_InitHeightmap(void) {
	weather_heightmap = Platform_MemAlloc(World_Width * World_Length * sizeof(Int16));
	if (weather_heightmap == NULL) {
		ErrorHandler_Fail("WeatherRenderer - Failed to allocate heightmap");
	}

	Int32 i;
	for (i = 0; i < World_Width * World_Length; i++) {
		weather_heightmap[i] = Int16_MaxValue;
	}
}

Real32 WeatherRenderer_RainHeight(Int32 x, Int32 z) {
	if (x < 0 || z < 0 || x >= World_Width || z >= World_Length) {
		return (Real32)WorldEnv_EdgeHeight;
	}
	Int32 index = (x * World_Length) + z;
	Int32 height = weather_heightmap[index];

	Int32 y = height == Int16_MaxValue ? WeatherRenderer_CalcHeightAt(x, World_MaxY, z, index) : height;
	return y == -1 ? 0 : y + Block_MaxBB[World_GetBlock(x, y, z)].Y;
}

Int32 WeatherRenderer_CalcHeightAt(Int32 x, Int32 maxY, Int32 z, Int32 index) {
	Int32 mapIndex = (maxY * World_Length + z) * World_Width + x;
	Int32 y = maxY;
	for (y = maxY; y >= 0; y--) {
		UInt8 draw = Block_Draw[World_Blocks[mapIndex]];
		if (!(draw == DrawType_Gas || draw == DrawType_Sprite)) {
			weather_heightmap[index] = (Int16)y;
			return y;
		}
		mapIndex -= World_OneY;
	}
	weather_heightmap[index] = -1;
	return -1;
}

void WeatherRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock) {
	bool didBlock = !(Block_Draw[oldBlock] == DrawType_Gas || Block_Draw[oldBlock] == DrawType_Sprite);
	bool nowBlock = !(Block_Draw[newBlock] == DrawType_Gas || Block_Draw[newBlock] == DrawType_Sprite);
	if (didBlock == nowBlock) return;

	Int32 index = (x * World_Length) + z;
	Int32 height = weather_heightmap[index];
	/* Two cases can be skipped here: */
	/* a) rain height was not calculated to begin with (height is short.MaxValue) */
	/* b) changed y is below current calculated rain height */
	if (y < height) return;

	if (nowBlock) {
		/* Simple case: Rest of column below is now not visible to rain. */
		weather_heightmap[index] = (Int16)y;
	} else {
		/* Part of the column is now visible to rain, we don't know how exactly how high it should be though. */
		/* However, we know that if the old block was above or equal to rain height, then the new rain height must be <= old block.y */
		WeatherRenderer_CalcHeightAt(x, y, z, index);
	}
}

void WeatherRenderer_ContextLost(void) {
	Gfx_DeleteVb(&weather_vb);
}

void WeatherRenderer_ContextRecreated(void) {
	weather_vb = Gfx_CreateDynamicVb(VertexFormat_P3fT2fC4b, weather_verticesCount);
}
