using System;
using System.Drawing;
using ClassicalSharp.Renderers;

namespace ClassicalSharp {
	
	public class EnvSettingsScreen : MenuInputScreen {
		
		public EnvSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			hintFont = new Font( "Arial", 14, FontStyle.Italic );
			
			buttons = new ButtonWidget[] {
				Make( -140, -100, "Clouds colour", Docking.Centre, OnWidgetClick,
				     g => g.Map.CloudsCol.ToRGBHexString(),
				     (g, v) => g.Map.SetCloudsColour( FastColour.Parse( v ) ) ),
				
				Make( -140, -50, "Sky colour", Docking.Centre, OnWidgetClick,
				     g => g.Map.SkyCol.ToRGBHexString(),
				     (g, v) => g.Map.SetSkyColour( FastColour.Parse( v ) ) ),
				
				Make( -140, 0, "Fog colour", Docking.Centre, OnWidgetClick,
				     g => g.Map.FogCol.ToRGBHexString(),
				     (g, v) => g.Map.SetFogColour( FastColour.Parse( v ) ) ),
				
				Make( -140, 50, "Clouds speed", Docking.Centre, OnWidgetClick,
				     g => {
				     	StandardEnvRenderer env = game.EnvRenderer as StandardEnvRenderer;
				     	return env == null ? "(not active)" : env.CloudsSpeed.ToString();
				     },
				     (g, v) => {
				     	StandardEnvRenderer env = game.EnvRenderer as StandardEnvRenderer;
				     	if( env != null ) 
				     		env.CloudsSpeed = Single.Parse( v );
				     } ),
				
				Make( 140, -100, "Sunlight colour", Docking.Centre, OnWidgetClick,
				     g => g.Map.Sunlight.ToRGBHexString(),
				     (g, v) => g.Map.SetSunlight( FastColour.Parse( v ) ) ),
				
				Make( 140, -50, "Shadow colour", Docking.Centre, OnWidgetClick,
				     g => g.Map.Shadowlight.ToRGBHexString(),
				     (g, v) => g.Map.SetShadowlight( FastColour.Parse( v ) ) ),
				
				Make( 140, 0, "Weather", Docking.Centre, OnWidgetClick,
				     g => ((int)g.Map.Weather).ToString(),
				     (g, v) => g.Map.SetWeather( (Weather)Int32.Parse( v ) ) ),
				
				Make( 140, 50, "Water level", Docking.Centre,OnWidgetClick,
				     g => g.Map.WaterHeight.ToString(),
				     (g, v) => g.Map.SetWaterLevel( Int32.Parse( v ) ) ),
				
				Make( 0, 5, "Back to menu", Docking.BottomOrRight,
				     g => g.SetNewScreen( new PauseScreen( g ) ), null, null ),
				null,
			};
			
			validators = new MenuInputValidator[] {
				new HexColourValidator(),
				new HexColourValidator(),
				new HexColourValidator(),
				new RealValidator( 0, 1000 ),
				new HexColourValidator(),
				new HexColourValidator(),
				new IntegerValidator( 0, 2 ),
				new IntegerValidator( -2048, 2048 ),
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game> onClick, Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}