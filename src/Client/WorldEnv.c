#include "WorldEnv.h"

void WorldEnv_Reset() {
	WorldEnv_DefaultSkyCol = FastColour_Create3(0x99, 0xCC, 0xFF);
	WorldEnv_DefaultFogCol = FastColour_Create3(0xFF, 0xFF, 0xFF);
	WorldEnv_DefaultCloudsCol = FastColour_Create3(0xFF, 0xFF, 0xFF);

	WorldEnv_EdgeHeight = -1; 
	WorldEnv_SidesOffset = -2; 
	WorldEnv_CloudHeight = -1;

	WorldEnv_EdgeBlock = BlockID_StillWater; 
	WorldEnv_SidesBlock = BlockID_Bedrock;

	WorldEnv_CloudsSpeed = 1.0f;
	WorldEnv_WeatherSpeed = 1.0f; 
	WorldEnv_WeatherFade = 1.0f;

	ResetLight();
	WorldEnv_SkyCol = WorldEnv_DefaultSkyCol;
	WorldEnv_FogCol = WorldEnv_DefaultFogCol;
	WorldEnv_CloudsCol = WorldEnv_DefaultCloudsCol;
	WorldEnv_Weather = Weather_Sunny;
	WorldEnv_ExpFog = false;
}

void WorldEnv_ResetLight() {
	WorldEnv_DefaultShadow = FastColour_Create3(0x9B, 0x9B, 0x9B);
	WorldEnv_DefaultSun = FastColour_Create3(0xFF, 0xFF, 0xFF);

	WorldEnv_Shadow = WorldEnv_DefaultShadow;
	FastColour_GetShaded(WorldEnv_Shadow, &WorldEnv_ShadowXSide, 
						&WorldEnv_ShadowZSide, &WorldEnv_ShadowYBottom);

	WorldEnv_Sun = WorldEnv_DefaultSun;
	FastColour_GetShaded(WorldEnv_Sun, &WorldEnv_SunXSide,
						&WorldEnv_SunZSide, &WorldEnv_SunYBottom);
}