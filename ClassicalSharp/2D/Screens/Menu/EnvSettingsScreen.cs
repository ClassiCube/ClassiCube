// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Map;
using ClassicalSharp.Renderers;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public class EnvSettingsScreen : MenuOptionsScreen {
		
		string[] defaultValues;
		int defaultIndex;
		public EnvSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				MakeOpt( -1, -150, "Clouds col", OnWidgetClick,
				     g => g.World.Env.CloudsCol.ToRGBHexString(),
				     (g, v) => g.World.Env.SetCloudsColour( FastColour.Parse( v ) ) ),
				
				MakeOpt( -1, -100, "Sky col", OnWidgetClick,
				     g => g.World.Env.SkyCol.ToRGBHexString(),
				     (g, v) => g.World.Env.SetSkyColour( FastColour.Parse( v ) ) ),
				
				MakeOpt( -1, -50, "Fog col", OnWidgetClick,
				     g => g.World.Env.FogCol.ToRGBHexString(),
				     (g, v) => g.World.Env.SetFogColour( FastColour.Parse( v ) ) ),
				
				MakeOpt( -1, 0, "Clouds speed", OnWidgetClick,
				     g => g.World.Env.CloudsSpeed.ToString( "F2" ),
				     (g, v) => g.World.Env.SetCloudsSpeed( Single.Parse( v ) ) ),
				
				MakeOpt( -1, 50, "Clouds height", OnWidgetClick,
				     g => g.World.Env.CloudHeight.ToString(),
				     (g, v) => g.World.Env.SetCloudsLevel( Int32.Parse( v ) ) ),
				
				// Column 2
				MakeOpt( 1, -150, "Sunlight col", OnWidgetClick,
				     g => g.World.Env.Sunlight.ToRGBHexString(),
				     (g, v) => g.World.Env.SetSunlight( FastColour.Parse( v ) ) ),
				
				MakeOpt( 1, -100, "Shadow col", OnWidgetClick,
				     g => g.World.Env.Shadowlight.ToRGBHexString(),
				     (g, v) => g.World.Env.SetShadowlight( FastColour.Parse( v ) ) ),
				
				MakeOpt( 1, -50, "Weather", OnWidgetClick,
				     g => g.World.Env.Weather.ToString(),
				     (g, v) => g.World.Env.SetWeather( (Weather)Enum.Parse( typeof(Weather), v ) ) ),
				
				MakeOpt( 1, 0, "Rain/Snow speed", OnWidgetClick,
				     g => g.World.Env.WeatherSpeed.ToString( "F2" ),
				     (g, v) => g.World.Env.SetWeatherSpeed( Single.Parse( v ) ) ),
				
				MakeOpt( 1, 50, "Water level", OnWidgetClick,
				     g => g.World.Env.EdgeHeight.ToString(),
				     (g, v) => g.World.Env.SetEdgeLevel( Int32.Parse( v ) ) ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				null, null, null,
			};
			MakeDefaultValues();
			MakeValidators();
		}
		
		void MakeDefaultValues() {
			defaultIndex = widgets.Length - 3;			
			defaultValues = new [] {
				WorldEnv.DefaultCloudsColour.ToRGBHexString(),
				WorldEnv.DefaultSkyColour.ToRGBHexString(),
				WorldEnv.DefaultFogColour.ToRGBHexString(),
				(1).ToString(),
				(game.World.Height + 2).ToString(),
				
				WorldEnv.DefaultSunlight.ToRGBHexString(),
				WorldEnv.DefaultShadowlight.ToRGBHexString(),
				Weather.Sunny.ToString(),
				(1).ToString(),
				(game.World.Height / 2).ToString(),
			};
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new HexColourValidator(),
				new HexColourValidator(),
				new HexColourValidator(),
				new RealValidator( 0, 1000 ),
				new IntegerValidator( -10000, 10000 ),
				
				new HexColourValidator(),
				new HexColourValidator(),
				new EnumValidator( typeof(Weather) ),
				new RealValidator( -100, 100 ),
				new IntegerValidator( -2048, 2048 ),
			};
		}
		
		protected override void InputClosed() {
			if( widgets[defaultIndex] != null )
				widgets[defaultIndex].Dispose();
			widgets[defaultIndex] = null;
		}
		
		protected override void InputOpened() {
			widgets[defaultIndex] = ButtonWidget.Create(
				game, 0, 150, 201, 40, "Default value", Anchor.Centre, 
				Anchor.Centre, titleFont, DefaultButtonClick );
		}
		
		void DefaultButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			int index = Array.IndexOf<Widget>( widgets, targetWidget );
			string defValue = defaultValues[index];
			inputWidget.SetText( defValue );
		}
	}
}