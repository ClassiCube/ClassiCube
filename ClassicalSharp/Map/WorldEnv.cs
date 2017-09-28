// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Map {

	public enum Weather { Sunny, Rainy, Snowy, }
	
	/// <summary> Contains the environment metadata for a world. </summary>
	public sealed class WorldEnv {
		
		/// <summary> Colour of the sky located behind/above clouds. </summary>
		public FastColour SkyCol = DefaultSkyColour;
		public static readonly FastColour DefaultSkyColour = new FastColour(0x99, 0xCC, 0xFF);
		
		/// <summary> Colour applied to the fog/horizon looking out horizontally.
		/// Note the true horizon colour is a blend of this and sky colour. </summary>
		public FastColour FogCol = DefaultFogColour;
		public static readonly FastColour DefaultFogColour = new FastColour(0xFF, 0xFF, 0xFF);
		
		/// <summary> Colour applied to the clouds. </summary>
		public FastColour CloudsCol = DefaultCloudsColour;
		public static readonly FastColour DefaultCloudsColour =  new FastColour(0xFF, 0xFF, 0xFF);
		
		/// <summary> Height of the clouds in world space. </summary>
		public int CloudHeight;
		
		/// <summary> Modifier of how fast clouds travel across the world, defaults to 1. </summary>
		public float CloudsSpeed = 1;
		
		/// <summary> Modifier of how fast rain/snow falls, defaults to 1. </summary>
		public float WeatherSpeed = 1;
		
		/// <summary> Modifier of how fast rain/snow fades, defaults to 1. </summary>
		public float WeatherFade = 1;
		
		/// <summary> Colour applied to blocks located in direct sunlight. </summary>
		public FastColour Sunlight;
		public int Sun, SunXSide, SunZSide, SunYBottom;
		public static readonly FastColour DefaultSunlight = new FastColour(0xFF, 0xFF, 0xFF);
		
		/// <summary> Colour applied to blocks located in shadow / hidden from direct sunlight. </summary>
		public FastColour Shadowlight;
		public int Shadow, ShadowXSide, ShadowZSide, ShadowYBottom;
		public static readonly FastColour DefaultShadowlight = new FastColour(0x9B, 0x9B, 0x9B);
		
		/// <summary> Current weather for this particular map. </summary>
		public Weather Weather = Weather.Sunny;
		
		/// <summary> Block that surrounds map the map horizontally (default water) </summary>
		public BlockID EdgeBlock = Block.StillWater;
		
		/// <summary> Height of the map edge in world space. </summary>
		public int EdgeHeight;
		
		/// <summary> Block that surrounds the map that fills the bottom of the map horizontally,
		/// fills part of the vertical sides of the map, and also surrounds map the map horizontally. (default bedrock) </summary>
		public BlockID SidesBlock = Block.Bedrock;
		
		/// <summary> Maximum height of the various parts of the map sides, in world space. </summary>
		public int SidesHeight { get { return EdgeHeight + SidesOffset; } }
		
		/// <summary> Offset of height of map sides from height of map edge. </summary>
		public int SidesOffset = -2;
		
		/// <summary> Whether exponential fog mode is used by default. </summary>
		public bool ExpFog;
		
		/// <summary> Horizontal skybox rotation speed. </summary>
		public float SkyboxHorSpeed;
		
		/// <summary> Vertical skybox rotation speed. </summary>
		public float SkyboxVerSpeed;
		
		/// <summary> Whether clouds still render, even with skybox. </summary>
		public bool SkyboxClouds;
		
		Game game;
		public WorldEnv(Game game) {
			this.game = game;
			ResetLight();
		}

		/// <summary> Resets all of the environment properties to their defaults. </summary>
		public void Reset() {
			EdgeHeight = -1; SidesOffset = -2; CloudHeight = -1;
			EdgeBlock = Block.StillWater; SidesBlock = Block.Bedrock;
			CloudsSpeed = 1; WeatherSpeed = 1; WeatherFade = 1;
			SkyboxHorSpeed = 0; SkyboxVerSpeed = 0;
			
			ResetLight();
			SkyCol = DefaultSkyColour;
			FogCol = DefaultFogColour;
			CloudsCol = DefaultCloudsColour;
			Weather = Weather.Sunny;
			ExpFog = false;
		}
		
		void ResetLight() {
			Shadowlight = DefaultShadowlight;
			Shadow = Shadowlight.Pack();
			FastColour.GetShaded(Shadowlight, out ShadowXSide,
			                     out ShadowZSide, out ShadowYBottom);
			
			Sunlight = DefaultSunlight;
			Sun = Sunlight.Pack();
			FastColour.GetShaded(Sunlight, out SunXSide,
			                     out SunZSide, out SunYBottom);
		}
		
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
		
		/// <summary> Sets horizontal speed of skybox rotation, and raises
		/// EnvVariableChanged event with variable 'SkyboxHorSpeed'. </summary>
		public void SetSkyboxHorSpeed(float speed) { Set(speed, ref SkyboxHorSpeed, EnvVar.SkyboxHorSpeed); }
		
		/// <summary> Sets vertical speed of skybox rotation, and raises
		/// EnvVariableChanged event with variable 'SkyboxVerSpeed'. </summary>
		public void SetSkyboxVerSpeed(float speed) { Set(speed, ref SkyboxVerSpeed, EnvVar.SkyboxVerSpeed); }

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
		
		bool Set<T>(T value, ref T target, EnvVar var) where T : IEquatable<T> {
			if (value.Equals(target)) return false;
			target = value;
			game.WorldEvents.RaiseEnvVariableChanged(var);
			return true;
		}
	}
}