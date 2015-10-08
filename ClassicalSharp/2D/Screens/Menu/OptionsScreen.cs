using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class OptionsScreen : MenuInputScreen {
		
		public OptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			buttons = new ButtonWidget[] {
				Make( -140, -50, "Show FPS", Docking.Centre, OnWidgetClick,
				     g => g.ShowFPS ? "yes" : "no",
				     (g, v) => g.ShowFPS = v == "yes" ),
				
				Make( -140, 0, "View distance", Docking.Centre, OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ) ) ),
				
				Make( -140, 50, "VSync active", Docking.Centre, OnWidgetClick,
				     g => g.VSync ? "yes" : "no",
				     (g, v) => g.Graphics.SetVSync( g, v == "yes" ) ),
				
				Make( 140, -50, "Mouse sensitivity", Docking.Centre, OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => { g.MouseSensitivity = Int32.Parse( v );
				     		Options.Set( OptionsKey.Sensitivity, v ); } ),
				
				Make( 140, 0, "Chat font size", Docking.Centre, OnWidgetClick,
				     g => g.Chat.FontSize.ToString(),
				     (g, v) => { g.Chat.FontSize = Int32.Parse( v ); 
				     	Options.Set( OptionsKey.FontSize, v ); } ),
				
				!network.IsSinglePlayer ? null :
					Make( 140, 50, "Singleplayer physics", Docking.Centre, OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => ((SinglePlayerServer)network).physics.Enabled = (v == "yes") ),
				
				Make( 0, 5, "Back to menu", Docking.BottomOrRight,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ), null, null ),
				null,
			};
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new IntegerValidator( 16, 8192 ),
				new BooleanValidator(),
				new IntegerValidator( 1, 100 ),
				new IntegerValidator( 6, 30 ),
				network.IsSinglePlayer ? new BooleanValidator() : null,
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game, ButtonWidget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}