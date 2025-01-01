#ifndef CC_WORLD_H
#define CC_WORLD_H
#include "Vectors.h"
#include "PackedCol.h"
CC_BEGIN_HEADER

/* 
Represents a fixed size 3D array of blocks and associated metadata
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct AABB;
extern struct IGameComponent World_Component;

/* Unpacka an index into x,y,z (slow!) */
#define World_Unpack(idx, x, y, z) x = idx % World.Width; z = (idx / World.Width) % World.Length; y = (idx / World.Width) / World.Length;
/* Packs an x,y,z into a single index */
#define World_Pack(x, y, z) (((y) * World.Length + (z)) * World.Width + (x))
#define WORLD_UUID_LEN 16

#define World_ChunkPack(cx, cy, cz) (((cz) * World.ChunksY + (cy)) * World.ChunksX + (cx))
/* TODO: Swap Y and Z? Make sure to update MapRenderer's ResetChunkCache and ClearChunkCache methods! */


CC_VAR extern struct _WorldData {
	/* The blocks in the world. */
	BlockRaw* Blocks;
#ifdef EXTENDED_BLOCKS
	/* The upper 8 bit of blocks in the world. */
	/* If only 8 bit blocks are used, equals World_Blocks. */
	BlockRaw* Blocks2;
#endif
	/* Volume of the world. */
	int Volume;

	/* Dimensions of the world. */
	int Width, Height, Length;
	/* Maximum X/Y/Z coordinate in the world. */
	/* (i.e. Width - 1, Height - 1, Length - 1) */
	int MaxX, MaxY, MaxZ;
	/* Adds one Y coordinate to a packed index. */
	int OneY;
	/* Unique identifier for this world. */
	cc_uint8 Uuid[WORLD_UUID_LEN];

#ifdef EXTENDED_BLOCKS
	/* Masks access to World.Blocks/World.Blocks2 */
	/* e.g. this will be 255 if only 8 bit blocks are used */
	int IDMask;
#endif
	/* Whether the world has finished loading/generating. */
	/* NOTE: Blocks may still be NULL. (e.g. error during loading) */
	cc_bool Loaded;
	/* Point in time the current world was last saved at */
	double LastSave;
	/* Default name of the world when saving */
	cc_string Name;
	/* Number of chunks on each axis the world is subdivided into */
	int ChunksX, ChunksY, ChunksZ;
	/* Number of chunks in the world, or ChunksX * ChunksY * ChunksZ */
	int ChunksCount;
	/* Seed world was generated with. May be 0 (unknown) */
	int Seed;
} World;

/* Frees the blocks array, sets dimensions to 0, resets environment to default. */
void World_Reset(void);
/* Sets up state and raises WorldEvents.NewMap event */
/* NOTE: This implicitly calls World_Reset. */
CC_API void World_NewMap(void);
/* Sets blocks array/dimensions of the map and raises WorldEvents.MapLoaded event */
/* May also sets some environment settings like border/clouds height, if they are -1 */
CC_API void World_SetNewMap(BlockRaw* blocks, int width, int height, int length);
/* Sets the various dimension and max coordinate related variables. */
/* NOTE: This is an internal API. Use World_SetNewMap instead. */
CC_NOINLINE void World_SetDimensions(int width, int height, int length);
void World_OutOfMemory(void);

#ifdef EXTENDED_BLOCKS
/* Sets World.Blocks2 and updates internal state for more than 256 blocks. */
void World_SetMapUpper(BlockRaw* blocks);

#define World_GetRawBlock(idx) ((World.Blocks[idx] | (World.Blocks2[idx] << 8)) & World.IDMask)

/* Gets the block at the given coordinates. */
/* NOTE: Does NOT check that the coordinates are inside the map. */
static CC_INLINE BlockID World_GetBlock(int x, int y, int z) {
	int i = World_Pack(x, y, z);
	return (BlockID)World_GetRawBlock(i);
}
#else
#define World_GetBlock(x, y, z) World.Blocks[World_Pack(x, y, z)]
#define World_GetRawBlock(idx)  World.Blocks[idx]
#endif

/* If Y is above the map, returns BLOCK_AIR. */
/* If coordinates are outside the map, returns BLOCK_AIR. */
/* Otherwise returns the block at the given coordinates. */
BlockID World_GetPhysicsBlock(int x, int y, int z);
/* Sets the block at the given coordinates. */
/* NOTE: Does NOT check that the coordinates are inside the map. */
void World_SetBlock(int x, int y, int z, BlockID block);
/* If coordinates are outside the map, returns BLOCK_AIR. */
/* Otherwise returns the block at the given coordinates. */
BlockID World_SafeGetBlock(int x, int y, int z);

/* Whether the given coordinates lie inside the map. */
static CC_INLINE cc_bool World_Contains(int x, int y, int z) {
	return (unsigned)x < (unsigned)World.Width
		&& (unsigned)y < (unsigned)World.Height
		&& (unsigned)z < (unsigned)World.Length;
}
/* Whether the given coordinates lie horizontally inside the map. */
static CC_INLINE cc_bool World_ContainsXZ(int x, int z) {
	return (unsigned)x < (unsigned)World.Width 
		&& (unsigned)z < (unsigned)World.Length;
}

static CC_INLINE cc_bool World_CheckVolume(int width, int height, int length) {
	cc_uint64 volume = (cc_uint64)width * height * length;
	return volume <= Int32_MaxValue;
}

enum EnvVar {
	ENV_VAR_EDGE_BLOCK, ENV_VAR_SIDES_BLOCK, ENV_VAR_EDGE_HEIGHT, ENV_VAR_SIDES_OFFSET,
	ENV_VAR_CLOUDS_HEIGHT, ENV_VAR_CLOUDS_SPEED, ENV_VAR_WEATHER_SPEED, ENV_VAR_WEATHER_FADE,
	ENV_VAR_WEATHER, ENV_VAR_EXP_FOG, ENV_VAR_SKYBOX_HOR_SPEED, ENV_VAR_SKYBOX_VER_SPEED,
	ENV_VAR_SKY_COLOR, ENV_VAR_CLOUDS_COLOR, ENV_VAR_FOG_COLOR, 
	ENV_VAR_SUN_COLOR, ENV_VAR_SHADOW_COLOR, ENV_VAR_SKYBOX_COLOR,
	ENV_VAR_LAVALIGHT_COLOR, ENV_VAR_LAMPLIGHT_COLOR
};

CC_VAR extern struct _EnvData {
	BlockID EdgeBlock, SidesBlock;
	int EdgeHeight, SidesOffset;
	int CloudsHeight;
	float CloudsSpeed;

	float WeatherSpeed, WeatherFade;
	int Weather, ExpFog;
	float SkyboxHorSpeed, SkyboxVerSpeed;

	PackedCol SkyCol, FogCol, CloudsCol, SkyboxCol;
	PackedCol SunCol, SunXSide, SunZSide, SunYMin;
	PackedCol ShadowCol, ShadowXSide, ShadowZSide, ShadowYMin;
	PackedCol LavaLightCol, LampLightCol;
} Env;
#define Env_SidesHeight (Env.EdgeHeight + Env.SidesOffset)

enum Weather_ { WEATHER_SUNNY, WEATHER_RAINY, WEATHER_SNOWY };
extern const char* const Weather_Names[3];

#define ENV_DEFAULT_SKY_COLOR       PackedCol_Make(0x99, 0xCC, 0xFF, 0xFF)
#define ENV_DEFAULT_FOG_COLOR       PackedCol_Make(0xFF, 0xFF, 0xFF, 0xFF)
#define ENV_DEFAULT_CLOUDS_COLOR    PackedCol_Make(0xFF, 0xFF, 0xFF, 0xFF)
#define ENV_DEFAULT_SKYBOX_COLOR    PackedCol_Make(0xFF, 0xFF, 0xFF, 0xFF)
#define ENV_DEFAULT_SUN_COLOR       PackedCol_Make(0xFF, 0xFF, 0xFF, 0xFF)
#define ENV_DEFAULT_SHADOW_COLOR    PackedCol_Make(0x9B, 0x9B, 0x9B, 0xFF)
#define ENV_DEFAULT_LAVALIGHT_COLOR PackedCol_Make(0xFF, 0xEB, 0xC6, 0xFF)
#define ENV_DEFAULT_LAMPLIGHT_COLOR PackedCol_Make(0xFF, 0xFF, 0xFF, 0xFF)

/* Resets all environment settings to default. */
/* NOTE: Unlike Env_Set functions, DOES NOT raise EnvVarChanged event. */
CC_API void Env_Reset(void);

/* Sets the edge/horizon block. (default water) */
CC_API void Env_SetEdgeBlock(BlockID block);
/* Sets the sides/border block. (default bedrock) */
CC_API void Env_SetSidesBlock(BlockID block);
/* Sets the edge/horizon height. (default height/2) */
CC_API void Env_SetEdgeHeight(int height);
/* Sets offset of sides/border from horizon. (default -2) */
CC_API void Env_SetSidesOffset(int offset);
/* Sets clouds height. (default height+2)*/
CC_API void Env_SetCloudsHeight(int height);
/* Sets how fast clouds move. (default 1) */
/* Negative speeds move in opposite direction. */
CC_API void Env_SetCloudsSpeed(float speed);

/* Sets how fast rain/snow falls. (default 1) */
/* Negative speeds makes rain/snow fall upwards. */
CC_API void Env_SetWeatherSpeed(float speed);
/* Sets how quickly rain/snow fades over distance. (default 1) */
CC_API void Env_SetWeatherFade(float rate);
/* Sets the weather of the map. (default sun) */
/* Can be sun/rain/snow, see WEATHER_ enum. */
CC_API void Env_SetWeather(int weather);
/* Sets whether exponential/smooth fog is used. (default false) */
CC_API void Env_SetExpFog(cc_bool expFog);
/* Sets how quickly skybox rotates/spins horizontally. (default 0) */
/* speed is in rotations/second, so '2' completes two full spins per second. */
CC_API void Env_SetSkyboxHorSpeed(float speed);
/* Sets how quickly skybox rotates/spins vertically. (default 0) */
/* speed is in rotations/second, so '2' completes two full spins per second. */
CC_API void Env_SetSkyboxVerSpeed(float speed);

/* Sets colour of the sky above clouds. (default #99CCFF) */
CC_API void Env_SetSkyCol(PackedCol color);
/* Sets base colour of the horizon fog. (default #FFFFFF) */
/* Actual fog colour is blended between sky and fog colours, based on view distance. */
CC_API void Env_SetFogCol(PackedCol color);
/* Sets colour of clouds. (default #FFFFFF) */
CC_API void Env_SetCloudsCol(PackedCol color);
/* Sets colour of the skybox. (default #FFFFFF) */
CC_API void Env_SetSkyboxCol(PackedCol color);
/* Sets colour of sunlight. (default #FFFFFF) */
/* This is the colour used for lighting when not underground. */
CC_API void Env_SetSunCol(PackedCol color);
/* Sets colour of shadow. (default #9B9B9B) */
/* This is the colour used for lighting when underground. */
CC_API void Env_SetShadowCol(PackedCol color);
/* Sets colour that bright natural blocks cast with fancy lighting. (default #FFEBC6) */
CC_API void Env_SetLavaLightCol(PackedCol color);
/* Sets colour that bright artificial blocks cast with fancy lighting. (default #FFFFFF) */
CC_API void Env_SetLampLightCol(PackedCol color);

#define RESPAWN_NOT_FOUND -100000.0f
/* Finds the highest Y coordinate of any solid block that intersects the given bounding box */
/* So essentially, means max(Y + Block_MaxBB[block].y) over all solid blocks the AABB touches */
/* Returns RESPAWN_NOT_FOUND when no intersecting solid blocks are found. */
float Respawn_HighestSolidY(struct AABB* bb);
/* Finds a suitable initial spawn position for the entity. */
/* Works by iterating downwards from top of world until solid ground is found. */
Vec3 Respawn_FindSpawnPosition(float x, float z, Vec3 modelSize);

CC_END_HEADER
#endif
