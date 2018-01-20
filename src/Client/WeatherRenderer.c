#include "WeatherRenderer.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Event.h"
#include "Game.h"
#include "GraphicsAPI.h"
#include "GraphicsEnums.h"
#include "GraphicsCommon.h"
#include "PackedCol.h"
#include "Platform.h"
#include "Vectors.h"
#include "VertexStructs.h"
#include "World.h"
#include "Particle.h"

GfxResourceID weather_rainTex;
GfxResourceID weather_snowTex;
GfxResourceID weather_vb;

#define weather_extent 4
#define weather_verticesCount 8 * (weather_extent * 2 + 1) * (weather_extent * 2 + 1)

Real64 weather_accumulator;
Vector3I weather_lastPos;

void WeatherRenderer_InitHeightmap(void) {
	Weather_Heightmap = Platform_MemAlloc(World_Width * World_Length * sizeof(Int16));
	if (Weather_Heightmap == NULL) {
		ErrorHandler_Fail("WeatherRenderer - Failed to allocate heightmap");
	}

	Int32 i;
	for (i = 0; i < World_Width * World_Length; i++) {
		Weather_Heightmap[i] = Int16_MaxValue;
	}
}

Int32 WeatherRenderer_CalcHeightAt(Int32 x, Int32 maxY, Int32 z, Int32 index) {
	Int32 mapIndex = (maxY * World_Length + z) * World_Width + x;
	Int32 y = maxY;
	for (y = maxY; y >= 0; y--) {
		UInt8 draw = Block_Draw[World_Blocks[mapIndex]];
		if (!(draw == DRAW_GAS || draw == DRAW_SPRITE)) {
			Weather_Heightmap[index] = (Int16)y;
			return y;
		}
		mapIndex -= World_OneY;
	}
	Weather_Heightmap[index] = -1;
	return -1;
}

Real32 WeatherRenderer_RainHeight(Int32 x, Int32 z) {
	if (x < 0 || z < 0 || x >= World_Width || z >= World_Length) {
		return (Real32)WorldEnv_EdgeHeight;
	}
	Int32 index = (x * World_Length) + z;
	Int32 height = Weather_Heightmap[index];

	Int32 y = height == Int16_MaxValue ? WeatherRenderer_CalcHeightAt(x, World_MaxY, z, index) : height;
	return y == -1 ? 0 : y + Block_MaxBB[World_GetBlock(x, y, z)].Y;
}

void WeatherRenderer_OnBlockChanged(Int32 x, Int32 y, Int32 z, BlockID oldBlock, BlockID newBlock) {
	bool didBlock = !(Block_Draw[oldBlock] == DRAW_GAS || Block_Draw[oldBlock] == DRAW_SPRITE);
	bool nowBlock = !(Block_Draw[newBlock] == DRAW_GAS || Block_Draw[newBlock] == DRAW_SPRITE);
	if (didBlock == nowBlock) return;

	Int32 index = (x * World_Length) + z;
	Int32 height = Weather_Heightmap[index];
	/* Two cases can be skipped here: */
	/* a) rain height was not calculated to begin with (height is short.MaxValue) */
	/* b) changed y is below current calculated rain height */
	if (y < height) return;

	if (nowBlock) {
		/* Simple case: Rest of column below is now not visible to rain. */
		Weather_Heightmap[index] = (Int16)y;
	} else {
		/* Part of the column is now visible to rain, we don't know how exactly how high it should be though. */
		/* However, we know that if the old block was above or equal to rain height, then the new rain height must be <= old block.y */
		WeatherRenderer_CalcHeightAt(x, y, z, index);
	}
}

void WeatherRenderer_ContextLost(void* obj) {
	Gfx_DeleteVb(&weather_vb);
}

void WeatherRenderer_ContextRecreated(void* obj) {
	weather_vb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, weather_verticesCount);
}

Real32 WeatherRenderer_AlphaAt(Real32 x) {
	/* Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9} */
	Real32 falloff = 0.05f * x * x - 7 * x;
	return 178 + falloff * WorldEnv_WeatherFade;
}

void WeatherRenderer_Render(Real64 deltaTime) {
	Int32 weather = WorldEnv_Weather;
	if (weather == WEATHER_SUNNY) return;
	if (Weather_Heightmap == NULL) WeatherRenderer_InitHeightmap();

	Gfx_BindTexture(weather == WEATHER_RAINY ? weather_rainTex : weather_snowTex);
	Vector3 camPos = Game_CurrentCameraPos;
	Vector3I pos;
	Vector3I_Floor(&pos, &camPos);
	bool moved = Vector3I_NotEquals(&pos, &weather_lastPos);
	weather_lastPos = pos;

	/* Rain should extend up by 64 blocks, or to the top of the world. */
	pos.Y += 64;
	pos.Y = max(World_Height, pos.Y);

	Real32 speed = (weather == WEATHER_RAINY ? 1.0f : 0.2f) * WorldEnv_WeatherSpeed;
	Real32 vOffset = (Real32)Game_Accumulator * speed;
	weather_accumulator += deltaTime;
	bool particles = weather == WEATHER_RAINY;

	PackedCol col = WorldEnv_SunCol;
	VertexP3fT2fC4b v;
	VertexP3fT2fC4b vertices[weather_verticesCount];
	VertexP3fT2fC4b* ptr = vertices;

	Int32 dx, dz;
	for (dx = -weather_extent; dx <= weather_extent; dx++) {
		for (dz = -weather_extent; dz <= weather_extent; dz++) {
			Int32 x = pos.X + dx, z = pos.Z + dz;
			Real32 y = WeatherRenderer_RainHeight(x, z);
			Real32 height = pos.Y - y;
			if (height <= 0) continue;

			if (particles && (weather_accumulator >= 0.25 || moved)) {
				Vector3 particlePos = Vector3_Create3((Real32)x, y, (Real32)z);
				Particles_RainSnowEffect(particlePos);
			}

			Real32 dist = (Real32)dx * (Real32)dx + (Real32)dz * (Real32)dz;
			Real32 alpha = WeatherRenderer_AlphaAt(dist);
			/* Clamp between 0 and 255 */
			alpha = alpha < 0.0f ? 0.0f : alpha;
			alpha = alpha > 255.0f ? 255.0f : alpha;
			col.A = (UInt8)alpha;

			/* NOTE: Making vertex is inlined since this is called millions of times. */
			v.Col = col;
			Real32 worldV = vOffset + (z & 1) / 2.0f - (x & 0x0F) / 16.0f;
			Real32 v1 = y / 6.0f + worldV, v2 = (y + height) / 6.0f + worldV;
			Real32 x1 = (Real32)x,       y1 = (Real32)y,            z1 = (Real32)z;
			Real32 x2 = (Real32)(x + 1), y2 = (Real32)(y + height), z2 = (Real32)(z + 1);

			v.X = x1; v.Y = y1; v.Z = z1; v.U = 0.0f; v.V = v1; *ptr = v; ptr++;
			          v.Y = y2;                      v.V = v2;  *ptr = v; ptr++;
			v.X = x2;           v.Z = z2; v.U = 1.0f; 	        *ptr = v; ptr++;
			          v.Y = y1;                      v.V = v1;  *ptr = v; ptr++;

			                    v.Z = z1;					    *ptr = v; ptr++;
			          v.Y = y2;                       v.V = v2; *ptr = v; ptr++;
			v.X = x1;           v.Z = z2; v.U = 0.0f;		    *ptr = v; ptr++;
			          v.Y = y1;                       v.V = v1; *ptr = v; ptr++;
		}
	}

	if (particles && (weather_accumulator >= 0.25f || moved)) {
		weather_accumulator = 0;
	}
	if (ptr == vertices) return;

	Gfx_SetAlphaTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaArgBlend(true);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	UInt32 vCount = (UInt32)(ptr - vertices) / VertexP3fT2fC4b_Size;
	GfxCommon_UpdateDynamicVb_IndexedTris(weather_vb, vertices, vCount);

	Gfx_SetAlphaArgBlend(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaTest(false);
}

void WeatherRenderer_FileChanged(void* obj, Stream* stream) {
	String snow = String_FromConst("snow.png");
	String rain = String_FromConst("rain.png");

	if (String_Equals(&stream->Name, &snow)) {
		Game_UpdateTexture(&weather_snowTex, stream, false);
	} else if (String_Equals(&stream->Name, &rain)) {
		Game_UpdateTexture(&weather_rainTex, stream, false);
	}
}

void WeatherRenderer_Init(void) {
	weather_lastPos = Vector3I_Create1(Int32_MaxValue);
	WeatherRenderer_ContextRecreated(NULL);

	Event_RegisterStream(&TextureEvents_FileChanged, NULL, &WeatherRenderer_FileChanged);
	Event_RegisterVoid(&GfxEvents_ContextLost,       NULL, &WeatherRenderer_ContextLost);
	Event_RegisterVoid(&GfxEvents_ContextRecreated,  NULL, &WeatherRenderer_ContextRecreated);
}

void WeatherRenderer_Reset(void) {
	if (Weather_Heightmap != NULL) Platform_MemFree(Weather_Heightmap);
	Weather_Heightmap = NULL;
	weather_lastPos = Vector3I_Create1(Int32_MaxValue);
}

void WeatherRenderer_Free(void) {
	Gfx_DeleteTexture(&weather_rainTex);
	Gfx_DeleteTexture(&weather_snowTex);
	WeatherRenderer_ContextLost(NULL);
	WeatherRenderer_Reset();

	Event_UnregisterStream(&TextureEvents_FileChanged, NULL, &WeatherRenderer_FileChanged);
	Event_UnregisterVoid(&GfxEvents_ContextLost,       NULL, &WeatherRenderer_ContextLost);
	Event_UnregisterVoid(&GfxEvents_ContextRecreated,  NULL, &WeatherRenderer_ContextRecreated);
}

IGameComponent WeatherRenderer_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init = WeatherRenderer_Init;
	comp.Free = WeatherRenderer_Free;
	comp.OnNewMap = WeatherRenderer_Reset;
	comp.Reset = WeatherRenderer_Reset;
	return comp;
}