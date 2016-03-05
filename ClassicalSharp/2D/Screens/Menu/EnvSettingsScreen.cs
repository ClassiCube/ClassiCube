using System;
using System.Drawing;
using ClassicalSharp.Renderers;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class EnvSettingsScreen : MenuOptionsScreen {
		
		string[] defaultValues;
		int defaultIndex;
		public EnvSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				Make( -140, -150, "Clouds colour", OnWidgetClick,
				     g => g.Map.CloudsCol.ToRGBHexString(),
				     (g, v) => g.Map.SetCloudsColour( FastColour.Parse( v ) ) ),
				
				Make( -140, -100, "Sky colour", OnWidgetClick,
				     g => g.Map.SkyCol.ToRGBHexString(),
				     (g, v) => g.Map.SetSkyColour( FastColour.Parse( v ) ) ),
				
				Make( -140, -50, "Fog colour", OnWidgetClick,
				     g => g.Map.FogCol.ToRGBHexString(),
				     (g, v) => g.Map.SetFogColour( FastColour.Parse( v ) ) ),
				
				Make( -140, 0, "Clouds speed", OnWidgetClick,
				     g => g.Map.CloudsSpeed.ToString(),
				     (g, v) => g.Map.SetCloudsSpeed( Single.Parse( v ) ) ),
				
				Make( -140, 50, "Clouds height", OnWidgetClick,
				     g => g.Map.CloudHeight.ToString(),
				     (g, v) => g.Map.SetCloudsLevel( Int32.Parse( v ) ) ),
				
				Make( 140, -100, "Sunlight colour", OnWidgetClick,
				     g => g.Map.Sunlight.ToRGBHexString(),
				     (g, v) => g.Map.SetSunlight( FastColour.Parse( v ) ) ),
				
				Make( 140, -50, "Shadow colour", OnWidgetClick,
				     g => g.Map.Shadowlight.ToRGBHexString(),
				     (g, v) => g.Map.SetShadowlight( FastColour.Parse( v ) ) ),
				
				Make( 140, 0, "Weather", OnWidgetClick,
				     g => g.Map.Weather.ToString(),
				     (g, v) => g.Map.SetWeather( (Weather)Enum.Parse( typeof(Weather), v ) ) ),
				
				Make( 140, 50, "Water level", OnWidgetClick,
				     g => g.Map.EdgeHeight.ToString(),
				     (g, v) => g.Map.SetEdgeLevel( Int32.Parse( v ) ) ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				null, null, null,
			};
			widgets[7].Metadata = typeof(Weather);
			MakeDefaultValues();
			MakeValidators();
		}
		
		void MakeDefaultValues() {
			defaultIndex = widgets.Length - 3;			
			defaultValues = new [] {
				Map.DefaultCloudsColour.ToRGBHexString(),
				Map.DefaultSkyColour.ToRGBHexString(),
				Map.DefaultFogColour.ToRGBHexString(),
				(1).ToString(),
				(game.Map.Height + 2).ToString(),
				
				Map.DefaultSunlight.ToRGBHexString(),
				Map.DefaultShadowlight.ToRGBHexString(),
				Weather.Sunny.ToString(),
				(game.Map.Height / 2).ToString(),
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
				new EnumValidator(),
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
				game, 0, 200, 180, 35, "Default value", Anchor.Centre, 
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