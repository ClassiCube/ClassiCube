// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Map;

namespace ClassicalSharp.Gui.Screens {
	public class EnvSettingsScreen : MenuOptionsScreen {
		
		public EnvSettingsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			validators = new MenuInputValidator[widgets.Length];
			defaultValues = new string[widgets.Length];
			
			validators = new MenuInputValidator[widgets.Length];
			validators[0]    = new HexColourValidator();
			defaultValues[0] = WorldEnv.DefaultCloudsColHex;
			validators[1]    = new HexColourValidator();
			defaultValues[1] = WorldEnv.DefaultSkyColHex;
			validators[2]    = new HexColourValidator();
			defaultValues[2] = WorldEnv.DefaultFogColHex;
			validators[3]    = new RealValidator(0, 1000);
			defaultValues[3] = "1";
			validators[4]    = new IntegerValidator(-10000, 10000);
			defaultValues[4] = (game.World.Height + 2).ToString();
			
			validators[5]    = new HexColourValidator();
			defaultValues[5] = WorldEnv.DefaultSunlightHex;
			validators[6]    = new HexColourValidator();
			defaultValues[6] = WorldEnv.DefaultShadowlightHex;
			validators[7]    = new EnumValidator(typeof(Weather));
			validators[8]    = new RealValidator(-100, 100);
			defaultValues[8] = "1";
			validators[9]    = new IntegerValidator(-2048, 2048);
			defaultValues[9] = (game.World.Height / 2).ToString();
		}
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnInputClick;
			ClickHandler onEnum = OnEnumClick;

			widgets = new Widget[] {
				MakeOpt(-1, -150, "Clouds col",  onClick, GetCloudsCol,    SetCloudsCol),
				MakeOpt(-1, -100, "Sky col",     onClick, GetSkyCol,       SetSkyCol),
				MakeOpt(-1, -50,  "Fog col",     onClick, GetFogCol,       SetFogCol),
				MakeOpt(-1, 0, "Clouds speed",   onClick, GetCloudsSpeed,  SetCloudsSpeed),
				MakeOpt(-1, 50, "Clouds height", onClick, GetCloudsHeight, SetCloudsHeight),

				MakeOpt(1, -150, "Sunlight col", onClick, GetSunCol,       SetSunCol),
				MakeOpt(1, -100, "Shadow col",   onClick, GetShadowCol,    SetShadowCol),
				MakeOpt(1, -50, "Weather",       onEnum,  GetWeather,      SetWeather),
				MakeOpt(1, 0, "Rain/Snow speed", onClick, GetWeatherSpeed, SetWeatherSpeed),
				MakeOpt(1, 50, "Water level",    onClick, GetEdgeHeight,   SetEdgeHeight),
				
				MakeBack(false, titleFont, SwitchOptions),
				null, null, null,
			};
		}
		
		static string GetCloudsCol(Game g) { return  g.World.Env.CloudsCol.ToHex(); }
		static void SetCloudsCol(Game g, string v) { g.World.Env.SetCloudsColour(FastColour.Parse(v)); }
		
		static string GetSkyCol(Game g) { return  g.World.Env.SkyCol.ToHex(); }
		static void SetSkyCol(Game g, string v) { g.World.Env.SetSkyColour(FastColour.Parse(v)); }
		
		static string GetFogCol(Game g) { return  g.World.Env.FogCol.ToHex(); }
		static void SetFogCol(Game g, string v) { g.World.Env.SetFogColour(FastColour.Parse(v)); }
		
		static string GetCloudsSpeed(Game g) { return  g.World.Env.CloudsSpeed.ToString("F2"); }
		static void SetCloudsSpeed(Game g, string v) { g.World.Env.SetCloudsSpeed(Utils.ParseDecimal(v)); }
		
		static string GetCloudsHeight(Game g) { return  g.World.Env.CloudHeight.ToString(); }
		static void SetCloudsHeight(Game g, string v) { g.World.Env.SetCloudsLevel(Int32.Parse(v)); }
		
		static string GetSunCol(Game g) { return  g.World.Env.Sunlight.ToHex(); }
		static void SetSunCol(Game g, string v) { g.World.Env.SetSunlight(FastColour.Parse(v)); }

		static string GetShadowCol(Game g) { return  g.World.Env.Shadowlight.ToHex(); }
		static void SetShadowCol(Game g, string v) { g.World.Env.SetShadowlight(FastColour.Parse(v)); }

		static string GetWeather(Game g) { return  g.World.Env.Weather.ToString(); }
		static void SetWeather(Game g, string v) { g.World.Env.SetWeather((Weather)Enum.Parse(typeof(Weather), v)); }

		static string GetWeatherSpeed(Game g) { return  g.World.Env.WeatherSpeed.ToString("F2"); }
		static void SetWeatherSpeed(Game g, string v) { g.World.Env.SetWeatherSpeed(Utils.ParseDecimal(v)); }

		static string GetEdgeHeight(Game g) { return  g.World.Env.EdgeHeight.ToString(); }
		static void SetEdgeHeight(Game g, string v) { g.World.Env.SetEdgeLevel(Int32.Parse(v)); }
	}
}