#include "WorldEnv.h"
#include "EventHandler.h"
#include "WorldEvents.h"


/* Sets a value and potentially raises event. */
#define WorldEnv_Set(value, dst, envVar)\
if (value == dst) return;\
dst = value;\
WorldEvents_RaiseEnvVariableChanged(envVar);

/* Sets a colour and potentially raises event. */
#define WorldEnv_SetCol(value, dst, envVar)\
if (PackedCol_Equals(value, dst)) return;\
dst = value;\
WorldEvents_RaiseEnvVariableChanged(envVar);


void WorldEnv_Reset(void) {
	WorldEnv_DefaultSkyCol = PackedCol_Create3(0x99, 0xCC, 0xFF);
	WorldEnv_DefaultFogCol = PackedCol_Create3(0xFF, 0xFF, 0xFF);
	WorldEnv_DefaultCloudsCol = PackedCol_Create3(0xFF, 0xFF, 0xFF);

	WorldEnv_EdgeHeight = -1; 
	WorldEnv_SidesOffset = -2; 
	WorldEnv_CloudsHeight = -1;

	WorldEnv_EdgeBlock = BlockID_StillWater; 
	WorldEnv_SidesBlock = BlockID_Bedrock;

	WorldEnv_CloudsSpeed = 1.0f;
	WorldEnv_WeatherSpeed = 1.0f; 
	WorldEnv_WeatherFade = 1.0f;

	WorldEnv_ResetLight();
	WorldEnv_SkyCol = WorldEnv_DefaultSkyCol;
	WorldEnv_FogCol = WorldEnv_DefaultFogCol;
	WorldEnv_CloudsCol = WorldEnv_DefaultCloudsCol;
	WorldEnv_Weather = Weather_Sunny;
	WorldEnv_ExpFog = false;
}

void WorldEnv_ResetLight(void) {
	WorldEnv_DefaultShadowCol = PackedCol_Create3(0x9B, 0x9B, 0x9B);
	WorldEnv_DefaultSunCol = PackedCol_Create3(0xFF, 0xFF, 0xFF);

	WorldEnv_ShadowCol = WorldEnv_DefaultShadowCol;
	PackedCol_GetShaded(WorldEnv_ShadowCol, &WorldEnv_ShadowXSide,
		&WorldEnv_ShadowZSide, &WorldEnv_ShadowYBottom);

	WorldEnv_SunCol = WorldEnv_DefaultSunCol;
	PackedCol_GetShaded(WorldEnv_SunCol, &WorldEnv_SunXSide,
		&WorldEnv_SunZSide, &WorldEnv_SunYBottom);
}


void WorldEnv_SetEdgeBlock(BlockID block) {
	if (block == BlockID_Invalid) return;
	WorldEnv_Set(block, WorldEnv_EdgeBlock, EnvVar_EdgeBlock);
}

void WorldEnv_SetSidesBlock(BlockID block) {
	if (block == BlockID_Invalid) return;
	WorldEnv_Set(block, WorldEnv_SidesBlock, EnvVar_SidesBlock);
}

void WorldEnv_SetEdgeHeight(Int32 height) {
	WorldEnv_Set(height, WorldEnv_EdgeHeight, EnvVar_EdgeHeight);
}

void WorldEnv_SetSidesOffset(Int32 offset) {
	WorldEnv_Set(offset, WorldEnv_SidesOffset, EnvVar_SidesOffset);
}

void WorldEnv_SetCloudsHeight(Int32 height) {
	WorldEnv_Set(height, WorldEnv_CloudsHeight, EnvVar_CloudsHeight);
}

void WorldEnv_SetCloudsSpeed(Real32 speed) {
	WorldEnv_Set(speed, WorldEnv_CloudsSpeed, EnvVar_CloudsSpeed);
}


void WorldEnv_SetWeatherSpeed(Real32 speed) {
	WorldEnv_Set(speed, WorldEnv_WeatherSpeed, EnvVar_WeatherSpeed);
}

void WorldEnv_SetWeatherFade(Real32 rate) {
	WorldEnv_Set(rate, WorldEnv_WeatherFade, EnvVar_WeatherFade);
}

void WorldEnv_SetWeather(Int32 weather) {
	WorldEnv_Set(weather, WorldEnv_Weather, EnvVar_Weather);
}

void WorldEnv_SetExpFog(bool expFog) {
	WorldEnv_Set(expFog, WorldEnv_ExpFog, EnvVar_ExpFog);
}


void WorldEnv_SetSkyCol(PackedCol col) {
	WorldEnv_SetCol(col, WorldEnv_SkyCol, EnvVar_SkyCol);
}

void WorldEnv_SetFogCol(PackedCol col) {
	WorldEnv_SetCol(col, WorldEnv_FogCol, EnvVar_FogCol);
}

void WorldEnv_SetCloudsCol(PackedCol col) {
	WorldEnv_SetCol(col, WorldEnv_CloudsCol, EnvVar_CloudsCol);
}

void WorldEnv_SetSunCol(PackedCol col) {
	if (PackedCol_Equals(col, WorldEnv_SunCol)) return;

	WorldEnv_SunCol = col;
	PackedCol_GetShaded(WorldEnv_SunCol, &WorldEnv_SunXSide,
		&WorldEnv_SunZSide, &WorldEnv_SunYBottom);
	WorldEvents_RaiseEnvVariableChanged(EnvVar_SunCol);
}

void WorldEnv_SetShadowCol(PackedCol col) {
	if (PackedCol_Equals(col, WorldEnv_ShadowCol)) return;

	WorldEnv_ShadowCol = col;
	PackedCol_GetShaded(WorldEnv_ShadowCol, &WorldEnv_ShadowXSide,
		&WorldEnv_ShadowZSide, &WorldEnv_ShadowYBottom);
	WorldEvents_RaiseEnvVariableChanged(EnvVar_ShadowCol);
}