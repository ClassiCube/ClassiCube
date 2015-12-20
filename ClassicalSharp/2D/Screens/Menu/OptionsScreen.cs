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
				Make( -140, -150, "Simple arms anim", OnWidgetClick,
				     g => g.SimpleArmsAnim? "yes" : "no",
				     (g, v) => { g.SimpleArmsAnim = v == "yes";
				     	Options.Set( OptionsKey.SimpleArmsAnim, v == "yes" ); }),
				
				Make( -140, -100, "Use sound", OnWidgetClick,
				     g => g.UseSound ? "yes" : "no",
				     (g, v) => { g.UseSound = v == "yes";
				     	g.AudioPlayer.SetSound( g.UseSound );
				     	Options.Set( OptionsKey.UseSound, v == "yes" ); }),
				
				Make( -140, -50, "Names mode", OnWidgetClick,
				     g => g.Players.NamesMode.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(NameMode), v );
				     	g.Players.NamesMode = (NameMode)raw;
				     	Options.Set( OptionsKey.NamesMode, v ); } ),
				
				Make( -140, 0, "FPS limit", OnWidgetClick,
				     g => g.FpsLimit.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(FpsLimitMethod), v );
				     	g.SetFpsLimitMethod( (FpsLimitMethod)raw );
				     	Options.Set( OptionsKey.FpsLimit, v ); } ),

				Make( -140, 50, "View distance", OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ) ) ),
				
				// Column 2
				!network.IsSinglePlayer ? null :
					Make( 140, -150, "Block physics", OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => {
					     	((SinglePlayerServer)network).physics.Enabled = v == "yes";
					     	Options.Set( OptionsKey.SingleplayerPhysics, v == "yes" );
					     }),
				
				Make( 140, -100, "Use music", OnWidgetClick,
				     g => g.UseMusic ? "yes" : "no",
				     (g, v) => { g.UseMusic = v == "yes";
				     	g.AudioPlayer.SetMusic( g.UseMusic );
				     	Options.Set( OptionsKey.UseMusic, v == "yes" ); }),
				
				Make( 140, -50, "View bobbing", OnWidgetClick,
				     g => g.ViewBobbing ? "yes" : "no",
				     (g, v) => { g.ViewBobbing = v == "yes";
				     	Options.Set( OptionsKey.ViewBobbing, v == "yes" ); }),
				
				Make( 140, 0, "Auto close launcher", OnWidgetClick,
				     g => Options.GetBool( OptionsKey.AutoCloseLauncher, false ) ? "yes" : "no",
				     (g, v) => Options.Set( OptionsKey.AutoCloseLauncher, v == "yes" ) ),
				
				Make( 140, 50, "Mouse sensitivity", OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => { g.MouseSensitivity = Int32.Parse( v );
				     	Options.Set( OptionsKey.Sensitivity, v ); } ),
				
				MakeBack( false, titleFont,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			buttons[2].Metadata = typeof(NameMode);
			buttons[3].Metadata = typeof(FpsLimitMethod);
			
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new EnumValidator(),
				new EnumValidator(),
				new IntegerValidator( 16, 4096 ),
				
				network.IsSinglePlayer ? new BooleanValidator() : null,
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 1, 100 ),
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, 
			                                          Anchor.Centre, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}