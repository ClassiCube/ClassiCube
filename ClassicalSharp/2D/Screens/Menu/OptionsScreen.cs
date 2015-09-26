using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class OptionsScreen : MenuInputScreen {
		
		public OptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			hintFont = new Font( "Arial", 14, FontStyle.Italic );
			INetworkProcessor network = game.Network;
			
			buttons = new ButtonWidget[] {
				Make( -140, -50, "Use animations", Docking.Centre, OnWidgetClick,
				     g => g.Animations.Enabled ? "yes" : "no",
				     (g, v) => g.Animations.Enabled = v == "yes" ),
				
				Make( -140, 0, "View distance", Docking.Centre, OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ) ) ),
				
				Make( -140, 50, "VSync active", Docking.Centre, OnWidgetClick,
				     g => g.VSync ? "yes" : "no",
				     (g, v) => g.Graphics.SetVSync( g, v == "yes" ) ),
				
				Make( 140, -50, "Mouse sensitivity", Docking.Centre, OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => g.MouseSensitivity = Int32.Parse( v ) ),
				
				Make( 140, 0, "Chat font size", Docking.Centre, OnWidgetClick,
				     g => g.ChatFontSize.ToString(),
				     (g, v) => g.ChatFontSize = Int32.Parse( v ) ),
				
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