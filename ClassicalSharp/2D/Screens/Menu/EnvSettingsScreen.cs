using System;
using System.Drawing;
using ClassicalSharp.Renderers;

namespace ClassicalSharp {
	
	public class EnvSettingsScreen : MenuInputScreen {
		
		string[] defaultValues;
		int defaultIndex;
		public EnvSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			buttons = new ButtonWidget[] {
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
				
				Make( 140, -150, "Sunlight colour", OnWidgetClick,
				     g => g.Map.Sunlight.ToRGBHexString(),
				     (g, v) => g.Map.SetSunlight( FastColour.Parse( v ) ) ),
				
				Make( 140, -100, "Shadow colour", OnWidgetClick,
				     g => g.Map.Shadowlight.ToRGBHexString(),
				     (g, v) => g.Map.SetShadowlight( FastColour.Parse( v ) ) ),
				
				Make( 140, -50, "Weather", OnWidgetClick,
				     g => g.Map.Weather.ToString(),
				     (g, v) => g.Map.SetWeather( (Weather)Enum.Parse( typeof(Weather), v ) ) ),
				
				Make( 140, 0, "Water level", OnWidgetClick,
				     g => g.Map.EdgeHeight.ToString(),
				     (g, v) => g.Map.SetEdgeLevel( Int32.Parse( v ) ) ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
				null,
			};
			buttons[7].Metadata = typeof(Weather);
			defaultIndex = buttons.Length - 2;
			okayIndex = buttons.Length - 1;
			
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
			if( buttons[defaultIndex] != null )
				buttons[defaultIndex].Dispose();
			buttons[defaultIndex] = null;
		}
		
		protected override void InputOpened() {
			buttons[defaultIndex] = ButtonWidget.Create(
				game, 0, 200, 180, 35, "Default value", Anchor.Centre, 
				Anchor.Centre, titleFont, DefaultButtonClick );
		}
		
		void DefaultButtonClick( Game game, Widget widget ) {
			int index = Array.IndexOf<ButtonWidget>( buttons, targetWidget );
			string defValue = defaultValues[index];
			inputWidget.SetText( defValue );
		}
	}
}