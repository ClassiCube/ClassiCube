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
Int32 World_BlocksSize;

Int32 World_Width, World_Height, World_Length;
Int32 World_MaxX, World_MaxY, World_MaxZ;
Int32 World_OneY;
UInt8 World_Uuid[16];
extern String World_TextureUrl;

void World_Reset(void);
void World_SetNewMap(BlockRaw* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length);

#ifdef EXTENDED_BLOCKS
extern Int32 Block_IDMask;
static inline BlockID World_GetBlock(Int32 x, Int32 y, Int32 z) {
	Int32 i = World_Pack(x, y, z);
	return (BlockID)((World_Blocks[i] | (World_Blocks2[i] << 8)) & Block_IDMask);
}
#else
#define World_GetBlock(x, y, z) World_Blocks[World_Pack(x, y, z)]
#endif

BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z);
void World_SetBlock(Int32 x, Int32 y, Int32 z, BlockID block);
BlockID World_SafeGetBlock_3I(Vector3I p);
bool World_IsValidPos(Int32 x, Int32 y, Int32 z);
bool World_IsValidPos_3I(Vector3I p);

enum ENV_VAR {
	ENV_VAR_EDGE_BLOCK, ENV_VAR_SIDES_BLOCK, ENV_VAR_EDGE_HEIGHT, ENV_VAR_SIDES_OFFSET,
	ENV_VAR_CLOUDS_HEIGHT, ENV_VAR_CLOUDS_SPEED, ENV_VAR_WEATHER_SPEED, ENV_VAR_WEATHER_FADE,
	ENV_VAR_WEATHER, ENV_VAR_EXP_FOG, ENV_VAR_SKYBOX_HOR_SPEED, ENV_VAR_SKYBOX_VER_SPEED,
	ENV_VAR_SKY_COL, ENV_VAR_CLOUDS_COL, ENV_VAR_FOG_COL, ENV_VAR_SUN_COL,
	ENV_VAR_SHADOW_COL,
};

BlockID Env_EdgeBlock, Env_SidesBlock;
Int32 Env_EdgeHeight;
Int32 Env_SidesOffset;
#define Env_SidesHeight (Env_EdgeHeight + Env_SidesOffset)
Int32 Env_CloudsHeight;
float Env_CloudsSpeed;

enum WEATHER { WEATHER_SUNNY, WEATHER_RAINY, WEATHER_SNOWY };
extern const char* Weather_Names[3];
float Env_WeatherSpeed;
float Env_WeatherFade;
Int32 Env_Weather;
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

void Env_Reset(void);
void Env_ResetLight(void);
void Env_SetEdgeBlock(BlockID block);
void Env_SetSidesBlock(BlockID block);
void Env_SetEdgeHeight(Int32 height);
void Env_SetSidesOffset(Int32 offset);
void Env_SetCloudsHeight(Int32 height);
void Env_SetCloudsSpeed(float speed);

void Env_SetWeatherSpeed(float speed);
void Env_SetWeatherFade(float rate);
void Env_SetWeather(Int32 weather);
void Env_SetExpFog(bool expFog);
void Env_SetSkyboxHorSpeed(float speed);
void Env_SetSkyboxVerSpeed(float speed);

void Env_SetSkyCol(PackedCol col);
void Env_SetFogCol(PackedCol col);
void Env_SetCloudsCol(PackedCol col);
void Env_SetSunCol(PackedCol col);
void Env_SetShadowCol(PackedCol col);

#define RESPAWN_NOT_FOUND -100000.0f
/* Finds the highest free Y coordinate in the given bounding box */
float Respawn_HighestFreeY(struct AABB* bb);
/* Finds a suitable spawn position for the entity, by iterating 
downwards from top of the world until the ground is found */
Vector3 Respawn_FindSpawnPosition(float x, float z, Vector3 modelSize);
#endif
