#ifndef CC_WORLD_H
#define CC_WORLD_H
#include "Typedefs.h"
#include "String.h"
#include "Vectors.h"
#include "PackedCol.h"
#include "AABB.h"
/* Represents a fixed size 3D array of blocks.
   Also contains associated environment metadata.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define World_Unpack(index, x, y, z)\
x = index % World_Width;\
z = (index / World_Width) % World_Length;\
y = (index / World_Width) / World_Length;

#define World_Pack(x, y, z) (((y) * World_Length + (z)) * World_Width + (x))

BlockID* World_Blocks;
Int32 World_BlocksSize;
Int32 World_Width, World_Height, World_Length;
Int32 World_MaxX, World_MaxY, World_MaxZ;
Int32 World_OneY;
UInt8 World_Uuid[16];
String World_TextureUrl;
/* TODO: how to initalise World_TextureUrl string */

void World_Reset(void);
void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length);
BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z);

#define World_SetBlock(x, y, z, blockId) World_Blocks[World_Pack(x, y, z)] = blockId
#define World_SetBlock_3I(p, blockId) World_Blocks[World_Pack(p.X, p.Y, p.Z)] = blockId
#define World_GetBlock(x, y, z) World_Blocks[World_Pack(x, y, z)]
#define World_GetBlock_3I(p) World_Blocks[World_Pack(p.X, p.Y, p.Z)]
BlockID World_SafeGetBlock(Int32 x, Int32 y, Int32 z);
BlockID World_SafeGetBlock_3I(Vector3I p);

bool World_IsValidPos(Int32 x, Int32 y, Int32 z);
bool World_IsValidPos_3I(Vector3I p);
Vector3I World_GetCoords(Int32 index);

BlockID WorldEnv_EdgeBlock;
BlockID WorldEnv_SidesBlock;
Int32 WorldEnv_EdgeHeight;
Int32 WorldEnv_SidesOffset;
#define WorldEnv_SidesHeight (WorldEnv_EdgeHeight + WorldEnv_SidesOffset)
Int32 WorldEnv_CloudsHeight;
Real32 WorldEnv_CloudsSpeed;

Real32 WorldEnv_WeatherSpeed;
Real32 WorldEnv_WeatherFade;
Int32 WorldEnv_Weather;
bool WorldEnv_ExpFog;
Real32 WorldEnv_SkyboxHorSpeed;
Real32 WorldEnv_SkyboxVerSpeed;
bool WorldEnv_SkyboxClouds;

PackedCol WorldEnv_SkyCol;
PackedCol WorldEnv_DefaultSkyCol;
PackedCol WorldEnv_FogCol;
PackedCol WorldEnv_DefaultFogCol;
PackedCol WorldEnv_CloudsCol;
PackedCol WorldEnv_DefaultCloudsCol;
PackedCol WorldEnv_SunCol;
PackedCol WorldEnv_SunXSide, WorldEnv_SunZSide, WorldEnv_SunYBottom;
PackedCol WorldEnv_DefaultSunCol;
PackedCol WorldEnv_ShadowCol;
PackedCol WorldEnv_ShadowXSide, WorldEnv_ShadowZSide, WorldEnv_ShadowYBottom;
PackedCol WorldEnv_DefaultShadowCol;

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

/* Finds the highest free Y coordinate in the given bounding box.*/
Real32 Respawn_HighestFreeY(AABB* bb);
/* Finds a suitable spawn position for the entity, by iterating downards from top of
the world until the ground is found. */
Vector3 Respawn_FindSpawnPosition(Real32 x, Real32 z, Vector3 modelSize);
#endif