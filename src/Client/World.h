#ifndef CC_WORLD_H
#define CC_WORLD_H
#include "Vectors.h"
#include "PackedCol.h"
/* Represents a fixed size 3D array of blocks.
   Also contains associated environment metadata.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct AABB_ AABB;

#define World_Unpack(idx, x, y, z) x = idx % World_Width; z = (idx / World_Width) % World_Length; y = (idx / World_Width) / World_Length;
#define World_Pack(x, y, z) (((y) * World_Length + (z)) * World_Width + (x))

BlockID* World_Blocks;
Int32 World_BlocksSize;
Int32 World_Width, World_Height, World_Length;
Int32 World_MaxX, World_MaxY, World_MaxZ;
Int32 World_OneY;
UInt8 World_Uuid[16];
extern String World_TextureUrl;

void World_Reset(void);
void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length);
BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z);

#define World_SetBlock(x, y, z, blockId) World_Blocks[World_Pack(x, y, z)] = blockId
#define World_SetBlock_3I(p, blockId) World_Blocks[World_Pack(p.X, p.Y, p.Z)] = blockId
#define World_GetBlock(x, y, z) World_Blocks[World_Pack(x, y, z)]
#define World_GetBlock_3I(p) World_Blocks[World_Pack(p.X, p.Y, p.Z)]
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

BlockID WorldEnv_EdgeBlock;
BlockID WorldEnv_SidesBlock;
Int32 WorldEnv_EdgeHeight;
Int32 WorldEnv_SidesOffset;
#define WorldEnv_SidesHeight (WorldEnv_EdgeHeight + WorldEnv_SidesOffset)
Int32 WorldEnv_CloudsHeight;
Real32 WorldEnv_CloudsSpeed;

enum WEATHER { WEATHER_SUNNY, WEATHER_RAINY, WEATHER_SNOWY };
extern const UInt8* Weather_Names[3];
Real32 WorldEnv_WeatherSpeed;
Real32 WorldEnv_WeatherFade;
Int32 WorldEnv_Weather;
bool WorldEnv_ExpFog;
Real32 WorldEnv_SkyboxHorSpeed;
Real32 WorldEnv_SkyboxVerSpeed;
bool WorldEnv_SkyboxClouds;

PackedCol WorldEnv_SkyCol;
extern PackedCol WorldEnv_DefaultSkyCol;
#define WORLDENV_DEFAULT_SKYCOL_HEX "99CCFF"
PackedCol WorldEnv_FogCol;
extern PackedCol WorldEnv_DefaultFogCol;
#define WORLDENV_DEFAULT_FOGCOL_HEX "FFFFFF"
PackedCol WorldEnv_CloudsCol;
extern PackedCol WorldEnv_DefaultCloudsCol;
#define WORLDENV_DEFAULT_CLOUDSCOL_HEX "FFFFFF"
PackedCol WorldEnv_SunCol, WorldEnv_SunXSide, WorldEnv_SunZSide, WorldEnv_SunYBottom;
extern PackedCol WorldEnv_DefaultSunCol;
#define WORLDENV_DEFAULT_SUNCOL_HEX "FFFFFF"
PackedCol WorldEnv_ShadowCol, WorldEnv_ShadowXSide, WorldEnv_ShadowZSide, WorldEnv_ShadowYBottom;
extern PackedCol WorldEnv_DefaultShadowCol;
#define WORLDENV_DEFAULT_SHADOWCOL_HEX "9B9B9B"

void WorldEnv_Reset(void);
void WorldEnv_ResetLight(void);
void WorldEnv_SetEdgeBlock(BlockID block);
void WorldEnv_SetSidesBlock(BlockID block);
void WorldEnv_SetEdgeHeight(Int32 height);
void WorldEnv_SetSidesOffset(Int32 offset);
void WorldEnv_SetCloudsHeight(Int32 height);
void WorldEnv_SetCloudsSpeed(Real32 speed);

void WorldEnv_SetWeatherSpeed(Real32 speed);
void WorldEnv_SetWeatherFade(Real32 rate);
void WorldEnv_SetWeather(Int32 weather);
void WorldEnv_SetExpFog(bool expFog);
void WorldEnv_SetSkyboxHorSpeed(Real32 speed);
void WorldEnv_SetSkyboxVerSpeed(Real32 speed);

void WorldEnv_SetSkyCol(PackedCol col);
void WorldEnv_SetFogCol(PackedCol col);
void WorldEnv_SetCloudsCol(PackedCol col);
void WorldEnv_SetSunCol(PackedCol col);
void WorldEnv_SetShadowCol(PackedCol col);

#define RESPAWN_NOT_FOUND -100000.0f
/* Finds the highest free Y coordinate in the given bounding box */
Real32 Respawn_HighestFreeY(AABB* bb);
/* Finds a suitable spawn position for the entity, by iterating 
downwards from top of the world until the ground is found */
Vector3 Respawn_FindSpawnPosition(Real32 x, Real32 z, Vector3 modelSize);
#endif