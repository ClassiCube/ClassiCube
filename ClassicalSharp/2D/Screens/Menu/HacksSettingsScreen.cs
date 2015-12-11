using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class HacksSettingsScreen : MenuInputScreen {
		
		public HacksSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			buttons = new ButtonWidget[] {
				// Column 1
				Make( -140, -100, "Hacks enabled", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.HacksEnabled ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.HacksEnabled = v == "yes";
				     	Options.Set( OptionsKey.HacksEnabled, v == "yes" );
				     	g.LocalPlayer.CheckHacksConsistency();
				     } ),
				
				Make( -140, -50, "Speed multiplier", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.SpeedMultiplier.ToString(),
				     (g, v) => { g.LocalPlayer.SpeedMultiplier = Single.Parse( v );
				     	Options.Set( OptionsKey.Speed, v ); } ),
				
				// Column 2
				Make( 140, -100, "Liquids breakable", Anchor.Centre, OnWidgetClick,
				     g => g.LiquidsBreakable ? "yes" : "no",
				     (g, v) => { g.LiquidsBreakable = v == "yes";
				     	Options.Set( OptionsKey.LiquidsBreakable, v == "yes" ); } ),
				
				Make( 140, -50, "Pushback placing", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.PushbackPlacing
				     && g.LocalPlayer.CanPushbackBlocks ? "yes" : "no",
				     (g, v) => {
				     	if( g.LocalPlayer.CanPushbackBlocks) {
				     		g.LocalPlayer.PushbackPlacing = v == "yes";
				     		Options.Set( OptionsKey.PushbackPlacing, v == "yes" );
				     	}
				     }),
				
				MakeBack( false, titleFont,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new RealValidator( 0.1f, 50 ),
				
				new BooleanValidator(),
				new BooleanValidator(),
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, Widget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, 
			                                          Anchor.Centre, vDocking, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}