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
				// Column 1
				Make( -140, -100, "Show hover names", Anchor.Centre, OnWidgetClick,
				     g => g.Players.ShowHoveredNames ? "yes" : "no",
				     (g, v) => {
				     	g.Players.ShowHoveredNames = v == "yes";
				     	Options.Set( OptionsKey.ShowHoveredNames, v == "yes" );
				     }),
				
				Make( -140, -50, "Speed multiplier", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.SpeedMultiplier.ToString(),
				     (g, v) => { g.LocalPlayer.SpeedMultiplier = Single.Parse( v );
				     	Options.Set( OptionsKey.Speed, v ); } ),
				
				Make( -140, 0, "VSync active", Anchor.Centre, OnWidgetClick,
				     g => g.VSync ? "yes" : "no",
				     (g, v) => { g.Graphics.SetVSync( g, v == "yes" );
				     	Options.Set( OptionsKey.VSync, v == "yes" ); } ),

				Make( -140, 50, "View distance", Anchor.Centre, OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ) ) ),
				
				// Column 2
				!network.IsSinglePlayer ? null :
					Make( 140, -100, "Singleplayer physics", Anchor.Centre, OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => {
					     	((SinglePlayerServer)network).physics.Enabled = v == "yes";
					     	Options.Set( OptionsKey.SingleplayerPhysics, v == "yes" );
					     }),
				
				Make( 140, -50, "Auto close launcher", Anchor.Centre, OnWidgetClick,
				     g => Options.GetBool( OptionsKey.AutoCloseLauncher, false ) ? "yes" : "no",
				     (g, v) => Options.Set( OptionsKey.AutoCloseLauncher, v == "yes" ) ),
				
				Make( 140, 0, "Pushback block placing", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.PushbackBlockPlacing
				     && g.LocalPlayer.CanPushbackBlocks ? "yes" : "no",
				     (g, v) => {
				     	if( g.LocalPlayer.CanPushbackBlocks)
				     		g.LocalPlayer.PushbackBlockPlacing = v == "yes";
				     }),
				
				Make( 140, 50, "Mouse sensitivity", Anchor.Centre, OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => { g.MouseSensitivity = Int32.Parse( v );
				     	Options.Set( OptionsKey.Sensitivity, v ); } ),
				
				Make( 0, 5, "Back to menu", Anchor.BottomOrRight,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ), null, null ),
				null,
			};
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new RealValidator( 0.1f, 50 ),		
				new BooleanValidator(),
				new IntegerValidator( 16, 4096 ),
				
				network.IsSinglePlayer ? new BooleanValidator() : null,
				new BooleanValidator(),
				new IntegerValidator( 1, 100 ),
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, Widget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, vDocking, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}