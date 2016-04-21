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
				Make2( -1, -150, "Clouds col", OnWidgetClick,
				     g => g.World.CloudsCol.ToRGBHexString(),
				     (g, v) => g.World.SetCloudsColour( FastColour.Parse( v ) ) ),
				
				Make2( -1, -100, "Sky col", OnWidgetClick,
				     g => g.World.SkyCol.ToRGBHexString(),
				     (g, v) => g.World.SetSkyColour( FastColour.Parse( v ) ) ),
				
				Make2( -1, -50, "Fog col", OnWidgetClick,
				     g => g.World.FogCol.ToRGBHexString(),
				     (g, v) => g.World.SetFogColour( FastColour.Parse( v ) ) ),
				
				Make2( -1, 0, "Clouds speed", OnWidgetClick,
				     g => g.World.CloudsSpeed.ToString( "F2" ),
				     (g, v) => g.World.SetCloudsSpeed( Single.Parse( v ) ) ),
				
				Make2( -1, 50, "Clouds height", OnWidgetClick,
				     g => g.World.CloudHeight.ToString(),
				     (g, v) => g.World.SetCloudsLevel( Int32.Parse( v ) ) ),
				
				// Column 2
				Make2( 1, -100, "Sunlight col", OnWidgetClick,
				     g => g.World.Sunlight.ToRGBHexString(),
				     (g, v) => g.World.SetSunlight( FastColour.Parse( v ) ) ),
				
				Make2( 1, -50, "Shadow col", OnWidgetClick,
				     g => g.World.Shadowlight.ToRGBHexString(),
				     (g, v) => g.World.SetShadowlight( FastColour.Parse( v ) ) ),
				
				Make2( 1, 0, "Weather", OnWidgetClick,
				     g => g.World.Weather.ToString(),
				     (g, v) => g.World.SetWeather( (Weather)Enum.Parse( typeof(Weather), v ) ) ),
				
				Make2( 1, 50, "Water level", OnWidgetClick,
				     g => g.World.EdgeHeight.ToString(),
				     (g, v) => g.World.SetEdgeLevel( Int32.Parse( v ) ) ),
				
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
				World.DefaultCloudsColour.ToRGBHexString(),
				World.DefaultSkyColour.ToRGBHexString(),
				World.DefaultFogColour.ToRGBHexString(),
				(1).ToString(),
				(game.World.Height + 2).ToString(),
				
				World.DefaultSunlight.ToRGBHexString(),
				World.DefaultShadowlight.ToRGBHexString(),
				Weather.Sunny.ToString(),
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