#ifndef CS_WORLDENV_H
#define CS_WORLDENV_H
#include "Typedefs.h"
#include "FastColour.h"
#include "BlockID.h"
/* Contains environment metadata for a world.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


#define Weather_Sunny 0
#define Weather_Rainy 1
#define Weather_Snowy 2


/* Colour of the sky located behind / above clouds. */
FastColour WorldEnv_SkyCol;
FastColour WorldEnv_DefaultSkyCol;

/* Colour applied to the fog/horizon looking out horizontally.
Note the true horizon colour is a blend of this and sky colour. */
FastColour WorldEnv_FogCol;
FastColour WorldEnv_DefaultFogCol;

/* Colour applied to the clouds. */
FastColour WorldEnv_CloudsCol;
FastColour WorldEnv_DefaultCloudsCol;

/* Height of the clouds. */
Int32 WorldEnv_CloudHeight;

/* Modifier of how fast clouds travel across the world, defaults to 1. */
Real32 WorldEnv_CloudsSpeed = 1.0f;

/* Modifier of how fast rain/snow falls, defaults to 1. */
Real32 WorldEnv_WeatherSpeed = 1.0f;

/* Modifier of how fast rain/snow fades, defaults to 1. */
Real32 WorldEnv_WeatherFade = 1.0f;

/* Colour applied to blocks located in direct sunlight. */
FastColour WorldEnv_Sun;
FastColour WorldEnv_SunXSide, WorldEnv_SunZSide, WorldEnv_SunYBottom;
FastColour WorldEnv_DefaultSun;

/* Colour applied to blocks located in shadow / hidden from direct sunlight. */
FastColour WorldEnv_Shadow;
FastColour WorldEnv_ShadowXSide, WorldEnv_ShadowZSide, WorldEnv_ShadowYBottom;
FastColour WorldEnv_DefaultShadow;

/* Current weather for this particular map. */
Int32 WorldEnv_Weather = Weather_Sunny;

/* Block that surrounds map the map horizontally (default water) */
BlockID WorldEnv_EdgeBlock = BlockID_StillWater;

/* Height of the map edge. */
Int32 WorldEnv_EdgeHeight;

/* Block that surrounds the map that fills the bottom of the map horizontally,
fills part of the vertical sides of the map, and also surrounds map the map horizontally. (default bedrock) */
BlockID WorldEnv_SidesBlock = BlockID_Bedrock;

/* Offset of height of map sides from height of map edge. */
Int32 WorldEnv_SidesOffset = -2;

/* Whether exponential fog mode is used by default. */
bool WorldEnv_ExpFog = false;


/* Resets all of the environment properties to their defaults. */
void WorldEnv_Reset();

/*Resets sun and shadow environment properties to their defaults. */
void WorldEnv_ResetLight();

// TODO: all the missing events !!!

	/// <summary> Sets sides block to the given block, and raises
	/// EnvVariableChanged event with variable 'SidesBlock'. </summary>
	public void SetSidesBlock(BlockID block) {
		if (block == SidesBlock) return;
		if (block == Block.MaxDefinedBlock) {
			Utils.LogDebug("Tried to set sides block to an invalid block: " + block);
			block = Block.Bedrock;
		}
		SidesBlock = block;
		game.WorldEvents.RaiseEnvVariableChanged(EnvVar.SidesBlock);
	}

	/// <summary> Sets edge block to the given block, and raises
	/// EnvVariableChanged event with variable 'EdgeBlock'. </summary>
	public void SetEdgeBlock(BlockID block) {
		if (block == EdgeBlock) return;
		if (block == Block.MaxDefinedBlock) {
			Utils.LogDebug("Tried to set edge block to an invalid block: " + block);
			block = Block.StillWater;
		}
		EdgeBlock = block;
		game.WorldEvents.RaiseEnvVariableChanged(EnvVar.EdgeBlock);
	}

	/// <summary> Sets clouds height in world space, and raises
	/// EnvVariableChanged event with variable 'CloudsLevel'. </summary>
	public void SetCloudsLevel(int level) { Set(level, ref CloudHeight, EnvVar.CloudsLevel); }

	/// <summary> Sets clouds speed, and raises EnvVariableChanged
	/// event with variable 'CloudsSpeed'. </summary>
	public void SetCloudsSpeed(float speed) { Set(speed, ref CloudsSpeed, EnvVar.CloudsSpeed); }

	/// <summary> Sets weather speed, and raises EnvVariableChanged
	/// event with variable 'WeatherSpeed'. </summary>
	public void SetWeatherSpeed(float speed) { Set(speed, ref WeatherSpeed, EnvVar.WeatherSpeed); }

	/// <summary> Sets weather fade rate, and raises EnvVariableChanged
	/// event with variable 'WeatherFade'. </summary>
	public void SetWeatherFade(float rate) { Set(rate, ref WeatherFade, EnvVar.WeatherFade); }

	/// <summary> Sets height of the map edges in world space, and raises
	/// EnvVariableChanged event with variable 'EdgeLevel'. </summary>
	public void SetEdgeLevel(int level) { Set(level, ref EdgeHeight, EnvVar.EdgeLevel); }

	/// <summary> Sets offset of the height of the map sides from map edges in world space, and raises
	/// EnvVariableChanged event with variable 'SidesLevel'. </summary>
	public void SetSidesOffset(int level) { Set(level, ref SidesOffset, EnvVar.SidesOffset); }

	/// <summary> Sets whether exponential fog is used, and raises
	/// EnvVariableChanged event with variable 'ExpFog'. </summary>
	public void SetExpFog(bool expFog) { Set(expFog, ref ExpFog, EnvVar.ExpFog); }

	/// <summary> Sets weather, and raises
	/// EnvVariableChanged event with variable 'Weather'. </summary>
	public void SetWeather(Weather weather) {
		if (weather == Weather) return;
		Weather = weather;
		game.WorldEvents.RaiseEnvVariableChanged(EnvVar.Weather);
	}


	/// <summary> Sets tsky colour, and raises
	/// EnvVariableChanged event with variable 'SkyColour'. </summary>
	public void SetSkyColour(FastColour col) { Set(col, ref SkyCol, EnvVar.SkyColour); }

	/// <summary> Sets fog colour, and raises
	/// EnvVariableChanged event with variable 'FogColour'. </summary>
	public void SetFogColour(FastColour col) { Set(col, ref FogCol, EnvVar.FogColour); }

	/// <summary> Sets clouds colour, and raises
	/// EnvVariableChanged event with variable 'CloudsColour'. </summary>
	public void SetCloudsColour(FastColour col) { Set(col, ref CloudsCol, EnvVar.CloudsColour); }

	/// <summary> Sets sunlight colour, and raises
	/// EnvVariableChanged event with variable 'SunlightColour'. </summary>
	public void SetSunlight(FastColour col) {
		if (!Set(col, ref Sunlight, EnvVar.SunlightColour)) return;

		FastColour.GetShaded(Sunlight, out SunXSide,
			out SunZSide, out SunYBottom);
		Sun = Sunlight.Pack();
		game.WorldEvents.RaiseEnvVariableChanged(EnvVar.SunlightColour);
	}

	/// <summary> Sets current shadowlight colour, and raises
	/// EnvVariableChanged event with variable 'ShadowlightColour'. </summary>
	public void SetShadowlight(FastColour col) {
		if (!Set(col, ref Shadowlight, EnvVar.ShadowlightColour)) return;

		FastColour.GetShaded(Shadowlight, out ShadowXSide,
			out ShadowZSide, out ShadowYBottom);
		Shadow = Shadowlight.Pack();
		game.WorldEvents.RaiseEnvVariableChanged(EnvVar.ShadowlightColour);
	}

	bool Set<T>(T value, ref T target, EnvVar var) where T : IEquatable<T>{
		if (value.Equals(target)) return false;
	target = value;
	game.WorldEvents.RaiseEnvVariableChanged(var);
	return true;
	}
}
}
#endif