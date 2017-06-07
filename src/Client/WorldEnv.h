#ifndef CS_WORLDENV_H
#define CS_WORLDENV_H
#include "Typedefs.h"
#include "PackedCol.h"
#include "BlockID.h"
/* Contains environment metadata for a world.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Weather types a world can have. */
typedef Int32 Weather;
#define Weather_Sunny 0
#define Weather_Rainy 1
#define Weather_Snowy 2


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
void WorldEnv_SetWeather(Int32 weather);

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