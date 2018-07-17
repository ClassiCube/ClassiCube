// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Events;
using BlockID = System.UInt16;

namespace ClassicalSharp.Map {

	public enum Weather { Sunny, Rainy, Snowy, }
	
	/// <summary> Contains the environment metadata for a world. </summary>
	public sealed class WorldEnv {
		
		public PackedCol SkyCol = DefaultSkyCol;
		public static readonly PackedCol DefaultSkyCol = new PackedCol(0x99, 0xCC, 0xFF);
		public const string DefaultSkyColHex = "99CCFF";

		public PackedCol FogCol = DefaultFogCol;
		public static readonly PackedCol DefaultFogCol = new PackedCol(0xFF, 0xFF, 0xFF);
		public const string DefaultFogColHex = "FFFFFF";
		
		public PackedCol CloudsCol = DefaultCloudsCol;
		public static readonly PackedCol DefaultCloudsCol = new PackedCol(0xFF, 0xFF, 0xFF);
		public const string DefaultCloudsColHex = "FFFFFF";
		
		public int CloudHeight;
		public float CloudsSpeed = 1;
		
		public float WeatherSpeed = 1;
		public float WeatherFade = 1;
		
		public PackedCol Sun, SunXSide, SunZSide, SunYBottom;
		public static readonly PackedCol DefaultSunlight = new PackedCol(0xFF, 0xFF, 0xFF);
		public const string DefaultSunlightHex = "FFFFFF";
		
		public PackedCol Shadow, ShadowXSide, ShadowZSide, ShadowYBottom;
		public static readonly PackedCol DefaultShadowlight = new PackedCol(0x9B, 0x9B, 0x9B);
		public const string DefaultShadowlightHex = "9B9B9B";
		
		public Weather Weather = Weather.Sunny;
		public BlockID EdgeBlock = Block.StillWater;
		public int EdgeHeight;
		
		public BlockID SidesBlock = Block.Bedrock;
		public int SidesHeight { get { return EdgeHeight + SidesOffset; } }
		public int SidesOffset = -2;
		
		public float SkyboxHorSpeed, SkyboxVerSpeed;
		public bool ExpFog;
		
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
			Shadow = DefaultShadowlight;
			PackedCol.GetShaded(Shadow, out ShadowXSide,
			                     out ShadowZSide, out ShadowYBottom);			
			Sun = DefaultSunlight;
			PackedCol.GetShaded(Sun, out SunXSide,
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
		
		public void SetSkyCol(PackedCol col) { Set(col, ref SkyCol, EnvVar.SkyCol); }
		public void SetFogCol(PackedCol col) { Set(col, ref FogCol, EnvVar.FogCol); }
		public void SetCloudsCol(PackedCol col) { Set(col, ref CloudsCol, EnvVar.CloudsCol); }

		public void SetSunCol(PackedCol col) {
			PackedCol.GetShaded(col, out SunXSide,
			                     out SunZSide, out SunYBottom);
			Set(col, ref Sun, EnvVar.SunCol);
		}

		public void SetShadowCol(PackedCol col) {
			PackedCol.GetShaded(col, out ShadowXSide,
			                     out ShadowZSide, out ShadowYBottom);
			Set(col, ref Shadow, EnvVar.ShadowCol);
		}
		
		bool Set<T>(T value, ref T target, EnvVar var) where T : IEquatable<T> {
			if (value.Equals(target)) return false;
			target = value;
			game.WorldEvents.RaiseEnvVariableChanged(var);
			return true;
		}
	}
}