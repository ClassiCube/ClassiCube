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
				Make( -140, -100, "Hacks enabled", OnWidgetClick,
				     g => g.LocalPlayer.HacksEnabled ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.HacksEnabled = v == "yes";
				     	Options.Set( OptionsKey.HacksEnabled, v == "yes" );
				     	g.LocalPlayer.CheckHacksConsistency();
				     } ),
				
				Make( -140, -50, "Speed multiplier", OnWidgetClick,
				     g => g.LocalPlayer.SpeedMultiplier.ToString(),
				     (g, v) => { g.LocalPlayer.SpeedMultiplier = Single.Parse( v );
				     	Options.Set( OptionsKey.Speed, v ); } ),
				
				Make( -140, 0, "Camera clipping", OnWidgetClick,
				     g => g.CameraClipping ? "yes" : "no",
				     (g, v) => { g.CameraClipping = v == "yes";
				     	Options.Set( OptionsKey.CameraClipping, v == "yes" ); } ),
				
				Make( -140, 50, "Jump height", OnWidgetClick,
				     g => g.LocalPlayer.JumpHeight.ToString(),
				     (g, v) => g.LocalPlayer.CalculateJumpVelocity( Single.Parse( v ) ) ),
				
				// Column 2
				Make( 140, -100, "Liquids breakable", OnWidgetClick,
				     g => g.LiquidsBreakable ? "yes" : "no",
				     (g, v) => { g.LiquidsBreakable = v == "yes";
				     	Options.Set( OptionsKey.LiquidsBreakable, v == "yes" ); } ),
				
				Make( 140, -50, "Pushback placing", OnWidgetClick,
				     g => g.LocalPlayer.PushbackPlacing ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.PushbackPlacing = v == "yes";
				     		Options.Set( OptionsKey.PushbackPlacing, v == "yes" ); }),
				
				Make( 140, 0, "Noclip slide", OnWidgetClick,
				     g => g.LocalPlayer.NoclipSlide ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.NoclipSlide = v == "yes";
				     	Options.Set( OptionsKey.NoclipSlide, v == "yes" ); } ),
				
				Make( 140, 50, "Field of view", OnWidgetClick,
				     g => g.FieldOfView.ToString(),
				     (g, v) => { g.FieldOfView = Int32.Parse( v );
				     	Options.Set( OptionsKey.FieldOfView, v );
				     	g.UpdateProjection();
				     } ),
				
				MakeBack( false, titleFont,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new RealValidator( 0.1f, 50 ),
				new BooleanValidator(),			
				new RealValidator( 0.1f, 1024f ),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 1, 150 ),				
			};
			okayIndex = buttons.Length - 1;
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			CheckHacksAllowed( null, null );
		}
		
		void CheckHacksAllowed( object sender, EventArgs e ) { 
			for( int i = 0; i < buttons.Length; i++ ) {
				if( buttons[i] == null ) continue;
				buttons[i].Disabled = false;
			}
			LocalPlayer p = game.LocalPlayer;
			bool noGlobalHacks = !p.CanAnyHacks || !p.HacksEnabled;
			buttons[3].Disabled = noGlobalHacks || !p.CanSpeed;
			buttons[5].Disabled = noGlobalHacks || !p.CanPushbackBlocks;
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre,
			                                          Anchor.Centre, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
	}
}