#ifndef CS_WORLD_H
#define CS_WORLD_H
#include "Typedefs.h"
#include "String.h"
#include "Vectors.h"
#include "PackedCol.h"
/* Represents a fixed size 3D array of blocks.
   Also contains associated environment metadata.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Weather types a world can have. */
typedef Int32 Weather;
#define Weather_Sunny 0
#define Weather_Rainy 1
#define Weather_Snowy 2


/* Converts a single integer index into coordinates. */
#define World_Unpack(index, x, y, z)\
x = index % World_Width;\
z = (index / World_Width) % World_Length;\
y = (index / World_Width) / World_Length;

/* Packs a coordinate into a single integer index. */
#define World_Pack(x, y, z) (((y) * World_Length + (z)) * World_Width + (x))


/* Raw blocks of this world. */
BlockID* World_Blocks;

/* Size of blocks array. */
Int32 World_BlocksSize;

/* Length of world on X axis.*/
Int32 World_Width;

/* Length of world on Y axis (vertical).*/
Int32 World_Height;

/* Length of world on Z axis.*/
Int32 World_Length;

/* Largest valid X coordinate. */
Int32 World_MaxX;

/* Largest valid Y coordinate. */
Int32 World_MaxY;

/* Largest valid Z coordinate. */
Int32 World_MaxZ;

/* Amount a packed index must be changed by to advance Y coordinate. */
Int32 World_OneY;

/* Unique uuid/guid of this particular world. */
UInt8 World_Uuid[16];

/* Current terrain.png or texture pack url of this map. */
String World_TextureUrl;
/* TODO: how to initalise this string */


/* Resets all of the properties to their defaults. */
void World_Reset(void);

/* Updates the underlying block array, heightmap, and dimensions of this map. */
void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length);

BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z);


/* Sets the block at the given coordinates without bounds checking. */
#define World_SetBlock(x, y, z, blockId) World_Blocks[World_Pack(x, y, z)] = blockId

/* Sets the block at the given coordinates without bounds checking. */
#define World_SetBlock_3I(p, blockId) World_Blocks[World_Pack(p.X, p.Y, p.Z)] = blockId

/* Returns the block at the given coordinates without bounds checking. */
#define World_GetBlock(x, y, z) World_Blocks[World_Pack(x, y, z)]

/* Returns the block at the given world coordinates without bounds checking. */
#define World_GetBlock_3I(x, y, z) World_Blocks[World_Pack(p.X, p.Y, p.Z)]

/* Returns block at given coordinates if coordinates are inside the map, 0 if outside. */
BlockID World_SafeGetBlock(Int32 x, Int32 y, Int32 z);

/* Returns block at given coordinates if coordinates are inside the map, 0 if outside. */
BlockID World_SafeGetBlock_3I(Vector3I p);


/* Returns whether given coordinates are contained inside this map. */
bool World_IsValidPos(Int32 x, Int32 y, Int32 z);

/* Returns whether given coordinates are contained inside this map. */
bool World_IsValidPos_3I(Vector3I p);

/* Unpacks the given index into the map's block array into its original world coordinates. */
Vector3I World_GetCoords(Int32 index);


/* Block that surrounds map the map horizontally (default water) */
BlockID WorldEnv_EdgeBlock;

/* Block that surrounds the map that fills the bottom of the map horizontally,
fills part of the vertical sides of the map, and also surrounds map the map horizontally. (default bedrock) */
BlockID WorldEnv_SidesBlock;

/* Height of the map edge. */
Int32 WorldEnv_EdgeHeight;

/* Offset of height of map sides from height of map edge. */
Int32 WorldEnv_SidesOffset;

/* Height of the map sides. */
#define WorldEnv_SidesHeight (WorldEnv_EdgeHeight + WorldEnv_SidesOffset)

/* Height of the clouds. */
Int32 WorldEnv_CloudsHeight;

/* Modifier of how fast clouds travel across the world, defaults to 1. */
Real32 WorldEnv_CloudsSpeed;


/* Modifier of how fast rain/snow falls, defaults to 1. */
Real32 WorldEnv_WeatherSpeed;

/* Modifier of how fast rain/snow fades, defaults to 1. */
Real32 WorldEnv_WeatherFade;

/* Current weather for this particular map. */
Weather WorldEnv_Weather;

/* Whether exponential fog mode is used by default. */
bool WorldEnv_ExpFog;


/* Colour of the sky located behind / above clouds. */
PackedCol WorldEnv_SkyCol;
PackedCol WorldEnv_DefaultSkyCol;

/* Colour applied to the fog/horizon looking out horizontally.
Note the true horizon colour is a blend of this and sky colour. */
PackedCol WorldEnv_FogCol;
PackedCol WorldEnv_DefaultFogCol;

/* Colour applied to the clouds. */
PackedCol WorldEnv_CloudsCol;
PackedCol WorldEnv_DefaultCloudsCol;

/* Colour applied to blocks located in direct sunlight. */
PackedCol WorldEnv_SunCol;
PackedCol WorldEnv_SunXSide, WorldEnv_SunZSide, WorldEnv_SunYBottom;
PackedCol WorldEnv_DefaultSunCol;

/* Colour applied to blocks located in shadow / hidden from direct sunlight. */
PackedCol WorldEnv_ShadowCol;
PackedCol WorldEnv_ShadowXSide, WorldEnv_ShadowZSide, WorldEnv_ShadowYBottom;
PackedCol WorldEnv_DefaultShadowCol;


/* Resets all of the environment properties to their defaults. */
void WorldEnv_Reset(void);

/*Resets sun and shadow environment properties to their defaults. */
void WorldEnv_ResetLight(void);


/* Sets edge block to the given block, and raises event with variable 'EdgeBlock'. */
void WorldEnv_SetEdgeBlock(BlockID block);

/* Sets sides block to the given block, and raises event with variable 'SidesBlock'. */
void WorldEnv_SetSidesBlock(BlockID block);

/* Sets height of the map edges, raises event with variable 'EdgeHeight'. */
void WorldEnv_SetEdgeHeight(Int32 height);

/* Sets offset of the height of the map sides from map edges, raises event with variable 'SidesLevel'. */
void WorldEnv_SetSidesOffset(Int32 offset);

/* Sets clouds height in world space, raises event with variable 'CloudsHeight'. */
void WorldEnv_SetCloudsHeight(Int32 height);

/* Sets clouds speed, raises event with variable 'CloudsSpeed'. */
void WorldEnv_SetCloudsSpeed(Real32 speed);


/* Sets weather speed, raises event with variable 'WeatherSpeed'. */
void WorldEnv_SetWeatherSpeed(Real32 speed);

/* Sets weather fade rate, raises event with variable 'WeatherFade'. */
void WorldEnv_SetWeatherFade(Real32 rate);

/* Sets weather, raises event with variable 'Weather'. */
void WorldEnv_SetWeather(Weather weather);

/* Sets whether exponential fog is used, raises event with variable 'ExpFog'. */
void WorldEnv_SetExpFog(bool expFog);


/* Sets sky colour, raises event with variable 'SkyCol'. */
void WorldEnv_SetSkyCol(PackedCol col);

/* Sets fog colour, raises event with variable 'FogCol'. */
void WorldEnv_SetFogCol(PackedCol col);

/* Sets clouds colour, and raises event with variable 'CloudsCol'. */
void WorldEnv_SetCloudsCol(PackedCol col);

/* Sets sun colour, and raises event with variable 'SunCol'. */
void WorldEnv_SetSunCol(PackedCol col);

/* Sets shadow colour, and raises event with variable 'ShadowCol'. */
void WorldEnv_SetShadowCol(PackedCol col);
#endif