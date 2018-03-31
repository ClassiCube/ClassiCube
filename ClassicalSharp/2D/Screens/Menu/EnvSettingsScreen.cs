// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Map;
using ClassicalSharp.Renderers;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class EnvSettingsScreen : MenuOptionsScreen {
		
		public EnvSettingsScreen(Game game) : base(game) {
		}

		string[] defaultValues;
		int defaultIndex;
		
		public override void Init() {
			base.Init();
			ContextRecreated();
			MakeDefaultValues();
			MakeValidators();
		}
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnButtonClick;
			widgets = new Widget[] {
				MakeOpt(-1, -150, "Clouds col",  onClick, GetCloudsCol,    SetCloudsCol),
				MakeOpt(-1, -100, "Sky col",     onClick, GetSkyCol,       SetSkyCol),
				MakeOpt(-1, -50,  "Fog col",     onClick, GetFogCol,       SetFogCol),
				MakeOpt(-1, 0, "Clouds speed",   onClick, GetCloudsSpeed,  SetCloudsSpeed),
				MakeOpt(-1, 50, "Clouds height", onClick, GetCloudsHeight, SetCloudsHeight),

				MakeOpt(1, -150, "Sunlight col", onClick, GetSunCol,       SetSunCol),
				MakeOpt(1, -100, "Shadow col",   onClick, GetShadowCol,    SetShadowCol),
				MakeOpt(1, -50, "Weather",       onClick, GetWeather,      SetWeather),
				MakeOpt(1, 0, "Rain/Snow speed", onClick, GetWeatherSpeed, SetWeatherSpeed),
				MakeOpt(1, 50, "Water level",    onClick, GetEdgeHeight,   SetEdgeHeight),
				
				MakeBack(false, titleFont, SwitchOptions),
				null, null, null,
			};
		}
		
		static string GetCloudsCol(Game g) { return  g.World.Env.CloudsCol.ToRGBHexString(); }
		static void SetCloudsCol(Game g, string v) { g.World.Env.SetCloudsColour(FastColour.Parse(v)); }
		
		static string GetSkyCol(Game g) { return  g.World.Env.SkyCol.ToRGBHexString(); }
		static void SetSkyCol(Game g, string v) { g.World.Env.SetSkyColour(FastColour.Parse(v)); }
		
		static string GetFogCol(Game g) { return  g.World.Env.FogCol.ToRGBHexString(); }
		static void SetFogCol(Game g, string v) { g.World.Env.SetFogColour(FastColour.Parse(v)); }
		
		static string GetCloudsSpeed(Game g) { return  g.World.Env.CloudsSpeed.ToString("F2"); }
		static void SetCloudsSpeed(Game g, string v) { g.World.Env.SetCloudsSpeed(Utils.ParseDecimal(v)); }
				
		static string GetCloudsHeight(Game g) { return  g.World.Env.CloudHeight.ToString(); }
		static void SetCloudsHeight(Game g, string v) { g.World.Env.SetCloudsLevel(Int32.Parse(v)); }
		
		static string GetSunCol(Game g) { return  g.World.Env.Sunlight.ToRGBHexString(); }
		static void SetSunCol(Game g, string v) { g.World.Env.SetSunlight(FastColour.Parse(v)); }

		static string GetShadowCol(Game g) { return  g.World.Env.Shadowlight.ToRGBHexString(); }
		static void SetShadowCol(Game g, string v) { g.World.Env.SetShadowlight(FastColour.Parse(v)); }

		static string GetWeather(Game g) { return  g.World.Env.Weather.ToString(); }
		static void SetWeather(Game g, string v) { g.World.Env.SetWeather((Weather)Enum.Parse(typeof(Weather), v)); }

		static string GetWeatherSpeed(Game g) { return  g.World.Env.WeatherSpeed.ToString("F2"); }
		static void SetWeatherSpeed(Game g, string v) { g.World.Env.SetWeatherSpeed(Utils.ParseDecimal(v)); }

		static string GetEdgeHeight(Game g) { return  g.World.Env.EdgeHeight.ToString(); }
		static void SetEdgeHeight(Game g, string v) { g.World.Env.SetEdgeLevel(Int32.Parse(v)); }
		
		void MakeDefaultValues() {
			defaultIndex = widgets.Length - 3;			
			defaultValues = new string[] {
				WorldEnv.DefaultCloudsColour.ToRGBHexString(),
				WorldEnv.DefaultSkyColour.ToRGBHexString(),
				WorldEnv.DefaultFogColour.ToRGBHexString(),
				(1).ToString(),
				(game.World.Height + 2).ToString(),
				
				WorldEnv.DefaultSunlight.ToRGBHexString(),
				WorldEnv.DefaultShadowlight.ToRGBHexString(),
				null,
				(1).ToString(),
				(game.World.Height / 2).ToString(),
			};
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new HexColourValidator(),
				new HexColourValidator(),
				new HexColourValidator(),
				new RealValidator(0, 1000),
				new IntegerValidator(-10000, 10000),
				
				new HexColourValidator(),
				new HexColourValidator(),
				new EnumValidator(typeof(Weather)),
				new RealValidator(-100, 100),
				new IntegerValidator(-2048, 2048),
			};
		}
		
		protected override void InputClosed() {
			base.InputClosed();
			if (widgets[defaultIndex] != null)
				widgets[defaultIndex].Dispose();
			widgets[defaultIndex] = null;
		}
		
		protected override void InputOpened() {
			widgets[defaultIndex] = ButtonWidget.Create(game, 200, "Default value", titleFont, DefaultButtonClick)				
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 150);
		}
		
		void DefaultButtonClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			int index = IndexOfWidget(activeButton);
			string defValue = defaultValues[index];
			
			input.Clear();
			input.Append(defValue);
		}
	}
}