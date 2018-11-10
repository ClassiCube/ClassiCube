#ifndef CC_WORLD_H
#define CC_WORLD_H
#include "Vectors.h"
#include "PackedCol.h"
/* Represents a fixed size 3D array of blocks.
   Also contains associated environment metadata.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct AABB;

#define World_Unpack(idx, x, y, z) x = idx % World_Width; z = (idx / World_Width) % World_Length; y = (idx / World_Width) / World_Length;
#define World_Pack(x, y, z) (((y) * World_Length + (z)) * World_Width + (x))

BlockRaw* World_Blocks;
#ifdef EXTENDED_BLOCKS
BlockRaw* World_Blocks2;
#endif
int World_BlocksSize;

int World_Width, World_Height, World_Length;
int World_MaxX, World_MaxY, World_MaxZ;
int World_OneY;
uint8_t World_Uuid[16];
extern String World_TextureUrl;

/* Frees the blocks array, sets dimensions to 0, resets environment to default. */
CC_EXPORT void World_Reset(void);
/* Sets the blocks array and dimensions of the map. */
/* May also sets some environment settings like border/clouds height, if they are -1 */
/* NOTE: Exits the game if size vs dimensions are inconsistent. */
CC_EXPORT void World_SetNewMap(BlockRaw* blocks, int blocksSize, int width, int height, int length);

#ifdef EXTENDED_BLOCKS
extern int Block_IDMask;
static inline BlockID World_GetBlock(int x, int y, int z) {
	int i = World_Pack(x, y, z);
	return (BlockID)((World_Blocks[i] | (World_Blocks2[i] << 8)) & Block_IDMask);
}
#else
#define World_GetBlock(x, y, z) World_Blocks[World_Pack(x, y, z)]
#endif

BlockID World_GetPhysicsBlock(int x, int y, int z);
void World_SetBlock(int x, int y, int z, BlockID block);
BlockID World_SafeGetBlock_3I(Vector3I p);
bool World_IsValidPos(int x, int y, int z);
bool World_IsValidPos_3I(Vector3I p);

enum EnvVar_ {
	ENV_VAR_EDGE_BLOCK, ENV_VAR_SIDES_BLOCK, ENV_VAR_EDGE_HEIGHT, ENV_VAR_SIDES_OFFSET,
	ENV_VAR_CLOUDS_HEIGHT, ENV_VAR_CLOUDS_SPEED, ENV_VAR_WEATHER_SPEED, ENV_VAR_WEATHER_FADE,
	ENV_VAR_WEATHER, ENV_VAR_EXP_FOG, ENV_VAR_SKYBOX_HOR_SPEED, ENV_VAR_SKYBOX_VER_SPEED,
	ENV_VAR_SKY_COL, ENV_VAR_CLOUDS_COL, ENV_VAR_FOG_COL, ENV_VAR_SUN_COL, ENV_VAR_SHADOW_COL
};

BlockID Env_EdgeBlock, Env_SidesBlock;
int Env_EdgeHeight;
int Env_SidesOffset;
#define Env_SidesHeight (Env_EdgeHeight + Env_SidesOffset)
int Env_CloudsHeight;
float Env_CloudsSpeed;

enum Weather_ { WEATHER_SUNNY, WEATHER_RAINY, WEATHER_SNOWY };
extern const char* Weather_Names[3];
float Env_WeatherSpeed;
float Env_WeatherFade;
int Env_Weather;
bool Env_ExpFog;
float Env_SkyboxHorSpeed, Env_SkyboxVerSpeed;

PackedCol Env_SkyCol, Env_FogCol, Env_CloudsCol;
extern PackedCol Env_DefaultSkyCol, Env_DefaultFogCol, Env_DefaultCloudsCol;
PackedCol Env_SunCol,    Env_SunXSide,    Env_SunZSide,    Env_SunYMin;
PackedCol Env_ShadowCol, Env_ShadowXSide, Env_ShadowZSide, Env_ShadowYMin;
extern PackedCol Env_DefaultSunCol, Env_DefaultShadowCol;

#define ENV_DEFAULT_SKYCOL_HEX "99CCFF"
#define ENV_DEFAULT_FOGCOL_HEX "FFFFFF"
#define ENV_DEFAULT_CLOUDSCOL_HEX "FFFFFF"
#define ENV_DEFAULT_SUNCOL_HEX "FFFFFF"
#define ENV_DEFAULT_SHADOWCOL_HEX "9B9B9B"

/* Resets all environment settings to default. */
/* NOTE: Unlike Env_Set functions, DOES NOT raise EnvVarChanged event. */
CC_EXPORT void Env_Reset(void);
/* Sets the edge/horizon block. (default water) */
CC_EXPORT void Env_SetEdgeBlock(BlockID block);
/* Sets the sides/border block. (default bedrock) */
CC_EXPORT void Env_SetSidesBlock(BlockID block);
/* Sets the edge/horizon height. (default height/2) */
CC_EXPORT void Env_SetEdgeHeight(int height);
/* Sets offset of sides/border from horizon. (default -2) */
CC_EXPORT void Env_SetSidesOffset(int offset);
/* Sets clouds height. (default height+2)*/
CC_EXPORT void Env_SetCloudsHeight(int height);
/* Sets how fast clouds move. (default 1) */
/* Negative speeds move in opposite direction. */
CC_EXPORT void Env_SetCloudsSpeed(float speed);

/* Sets how fast rain/snow falls. (default 1) */
/* Negative speeds makes rain/snow fall upwards. */
CC_EXPORT void Env_SetWeatherSpeed(float speed);
/* Sets how quickly rain/snow fades over distance. (default 1) */
CC_EXPORT void Env_SetWeatherFade(float rate);
/* Sets the weather of the map. (default sun) */
/* Can be sun/rain/snow, see WEATHER_ enum. */
CC_EXPORT void Env_SetWeather(int weather);
/* Sets whether exponential/smooth fog is used. (default false) */
CC_EXPORT void Env_SetExpFog(bool expFog);
/* Sets how quickly skybox rotates/spins horizontally. (default 0) */
/* speed is in rotations/second, so '2' completes two full spins per second. */
CC_EXPORT void Env_SetSkyboxHorSpeed(float speed);
/* Sets how quickly skybox rotates/spins vertically. (default 0) */
/* speed is in rotations/second, so '2' completes two full spins per second. */
CC_EXPORT void Env_SetSkyboxVerSpeed(float speed);

/* Sets colour of the sky above clouds. (default #99CCFF) */
CC_EXPORT void Env_SetSkyCol(PackedCol col);
/* Sets base colour of the horizon fog. (default #FFFFFF) */
/* Actual fog colour is blended between sky and fog colours, based on view distance. */
CC_EXPORT void Env_SetFogCol(PackedCol col);
/* Sets colour of the clouds and skybox. (default #FFFFFF) */
CC_EXPORT void Env_SetCloudsCol(PackedCol col);
/* Sets colour of sunlight. (default #FFFFFF) */
/* This is the colour used for lighting when not underground. */
CC_EXPORT void Env_SetSunCol(PackedCol col);
/* Sets colour of shadow. (default #9B9B9B) */
/* This is the colour used for lighting when underground. */
CC_EXPORT void Env_SetShadowCol(PackedCol col);

#define RESPAWN_NOT_FOUND -100000.0f
/* Finds the highest free Y coordinate in the given bounding box */
float Respawn_HighestFreeY(struct AABB* bb);
/* Finds a suitable spawn position for the entity. */
/* Works by iterating downwards from top of world until ground is found. */
Vector3 Respawn_FindSpawnPosition(float x, float z, Vector3 modelSize);
#endif
