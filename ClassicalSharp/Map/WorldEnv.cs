// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using BlockID = System.UInt16;

namespace ClassicalSharp.Map {

	public enum Weather { Sunny, Rainy, Snowy, }
	
	/// <summary> Contains the environment metadata for a world. </summary>
	public sealed class WorldEnv {
		
		public FastColour SkyCol = DefaultSkyCol;
		public static readonly FastColour DefaultSkyCol = new FastColour(0x99, 0xCC, 0xFF);
		public const string DefaultSkyColHex = "99CCFF";

		public FastColour FogCol = DefaultFogCol;
		public static readonly FastColour DefaultFogCol = new FastColour(0xFF, 0xFF, 0xFF);
		public const string DefaultFogColHex = "FFFFFF";
		
		public FastColour CloudsCol = DefaultCloudsCol;
		public static readonly FastColour DefaultCloudsCol = new FastColour(0xFF, 0xFF, 0xFF);
		public const string DefaultCloudsColHex = "FFFFFF";
		
		public int CloudHeight;
		public float CloudsSpeed = 1;
		
		public float WeatherSpeed = 1;
		public float WeatherFade = 1;
		
		public FastColour Sunlight;
		public int Sun, SunXSide, SunZSide, SunYBottom;
		public static readonly FastColour DefaultSunlight = new FastColour(0xFF, 0xFF, 0xFF);
		public const string DefaultSunlightHex = "FFFFFF";
		
		public FastColour Shadowlight;
		public int Shadow, ShadowXSide, ShadowZSide, ShadowYBottom;
		public static readonly FastColour DefaultShadowlight = new FastColour(0x9B, 0x9B, 0x9B);
		public const string DefaultShadowlightHex = "9B9B9B";
		
		public Weather Weather = Weather.Sunny;
		public BlockID EdgeBlock = Block.StillWater;
		public int EdgeHeight;
		
		public BlockID SidesBlock = Block.Bedrock;
		public int SidesHeight { get { return EdgeHeight + SidesOffset; } }
		public int SidesOffset = -2;
		
		public float SkyboxHorSpeed, SkyboxVerSpeed;
		public bool ExpFog, SkyboxClouds;
		
		Game game;
		public WorldEnv(Game game) {
			this.game = game;
			ResetLight();
		}

		public void Reset() {
			EdgeHeight = -1; SidesOffset = -2; CloudHeight = -1;
			EdgeBlock = Block.StillWater; SidesBlock = Block.Bedrock;
			CloudsSpeed = 1; WeatherSpeed = 1; WeatherFade = 1;
			SkyboxHorSpeed = 0; SkyboxVerSpeed = 0;
			
			ResetLight();
			SkyCol = DefaultSkyCol;
			FogCol = DefaultFogCol;
			CloudsCol = DefaultCloudsCol;
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

		public void SetSidesBlock(BlockID block) {
			if (block == 255 && !BlockInfo.IsCustomDefined(255)) block = Block.Bedrock; // some server software wrongly uses this value
			if (block == SidesBlock) return;
			SidesBlock = block;
			game.WorldEvents.RaiseEnvVariableChanged(EnvVar.SidesBlock);
		}
		
		public void SetEdgeBlock(BlockID block) {
			if (block == 255 && !BlockInfo.IsCustomDefined(255)) block = Block.StillWater; // some server software wrongly uses this value
			if (block == EdgeBlock) return;
			EdgeBlock = block;
			game.WorldEvents.RaiseEnvVariableChanged(EnvVar.EdgeBlock);
		}

		public void SetCloudsLevel(int level) { Set(level, ref CloudHeight, EnvVar.CloudsLevel); }
		public void SetCloudsSpeed(float speed) { Set(speed, ref CloudsSpeed, EnvVar.CloudsSpeed); }

		public void SetWeatherSpeed(float speed) { Set(speed, ref WeatherSpeed, EnvVar.WeatherSpeed); }
		public void SetWeatherFade(float rate) { Set(rate, ref WeatherFade, EnvVar.WeatherFade); }

		public void SetEdgeLevel(int level) { Set(level, ref EdgeHeight, EnvVar.EdgeLevel); }
		public void SetSidesOffset(int level) { Set(level, ref SidesOffset, EnvVar.SidesOffset); }
		
		public void SetExpFog(bool expFog) { Set(expFog, ref ExpFog, EnvVar.ExpFog); }
		
		public void SetSkyboxHorSpeed(float speed) { Set(speed, ref SkyboxHorSpeed, EnvVar.SkyboxHorSpeed); }
		public void SetSkyboxVerSpeed(float speed) { Set(speed, ref SkyboxVerSpeed, EnvVar.SkyboxVerSpeed); }

		public void SetWeather(Weather weather) {
			if (weather == Weather) return;
			Weather = weather;
			game.WorldEvents.RaiseEnvVariableChanged(EnvVar.Weather);
		}
		
		public void SetSkyColour(FastColour col) { Set(col, ref SkyCol, EnvVar.SkyColour); }
		public void SetFogColour(FastColour col) { Set(col, ref FogCol, EnvVar.FogColour); }
		public void SetCloudsColour(FastColour col) { Set(col, ref CloudsCol, EnvVar.CloudsColour); }

		public void SetSunlight(FastColour col) {
			if (col == Sunlight) return;
			
			Sunlight = col;
			FastColour.GetShaded(col, out SunXSide,
			                     out SunZSide, out SunYBottom);
			Sun = Sunlight.Pack();
			game.WorldEvents.RaiseEnvVariableChanged(EnvVar.SunlightColour);
		}

		public void SetShadowlight(FastColour col) {
			if (col == Shadowlight) return;
			
			Shadowlight = col;
			FastColour.GetShaded(col, out ShadowXSide,
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