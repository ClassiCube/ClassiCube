using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class HacksSettingsScreen : MenuOptionsScreen {
		
		public HacksSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				Make( -140, -150, "Hacks enabled", OnWidgetClick,
				     g => g.LocalPlayer.HacksEnabled ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.HacksEnabled = v == "yes";
				     	Options.Set( OptionsKey.HacksEnabled, v == "yes" );
				     	g.LocalPlayer.CheckHacksConsistency();
				     } ),
				
				Make( -140, -100, "Speed multiplier", OnWidgetClick,
				     g => g.LocalPlayer.SpeedMultiplier.ToString(),
				     (g, v) => { g.LocalPlayer.SpeedMultiplier = Single.Parse( v );
				     	Options.Set( OptionsKey.Speed, v ); } ),
				
				Make( -140, -50, "Camera clipping", OnWidgetClick,
				     g => g.CameraClipping ? "yes" : "no",
				     (g, v) => { g.CameraClipping = v == "yes";
				     	Options.Set( OptionsKey.CameraClipping, v == "yes" ); } ),
				
				Make( -140, 0, "Jump height", OnWidgetClick,
				     g => g.LocalPlayer.JumpHeight.ToString(),
				     (g, v) => g.LocalPlayer.CalculateJumpVelocity( Single.Parse( v ) ) ),
				
				Make( -140, 50, "Double jump", OnWidgetClick,
				     g => g.LocalPlayer.DoubleJump ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.DoubleJump = v == "yes";
				     	Options.Set( OptionsKey.DoubleJump, v == "yes" ); } ),
				
				// Column 2
				Make( 140, -150, "Liquids breakable", OnWidgetClick,
				     g => g.LiquidsBreakable ? "yes" : "no",
				     (g, v) => { g.LiquidsBreakable = v == "yes";
				     	Options.Set( OptionsKey.LiquidsBreakable, v == "yes" ); } ),
				
				Make( 140, -100, "Pushback placing", OnWidgetClick,
				     g => g.LocalPlayer.PushbackPlacing ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.PushbackPlacing = v == "yes";
				     		Options.Set( OptionsKey.PushbackPlacing, v == "yes" ); }),
				
				Make( 140, -50, "Noclip slide", OnWidgetClick,
				     g => g.LocalPlayer.NoclipSlide ? "yes" : "no",
				     (g, v) => { g.LocalPlayer.NoclipSlide = v == "yes";
				     	Options.Set( OptionsKey.NoclipSlide, v == "yes" ); } ),
				
				Make( 140, 0, "Field of view", OnWidgetClick,
				     g => g.FieldOfView.ToString(),
				     (g, v) => { g.FieldOfView = Int32.Parse( v );
				     	Options.Set( OptionsKey.FieldOfView, v );
				     	g.UpdateProjection();
				     } ),
				
				MakeBack( false, titleFont,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null, null,
			};
			
			MakeValidators();
			MakeDescriptions();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			CheckHacksAllowed( null, null );
		}
		
		void CheckHacksAllowed( object sender, EventArgs e ) { 
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Disabled = false;
			}
			
			LocalPlayer p = game.LocalPlayer;
			bool noGlobalHacks = !p.CanAnyHacks || !p.HacksEnabled;
			widgets[3].Disabled = noGlobalHacks || !p.CanSpeed;
			widgets[4].Disabled = noGlobalHacks || !p.CanSpeed;
			widgets[6].Disabled = noGlobalHacks || !p.CanPushbackBlocks;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new RealValidator( 0.1f, 50 ),
				new BooleanValidator(),			
				new RealValidator( 0.1f, 1024f ),
				new BooleanValidator(),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 1, 150 ),				
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[5] = new [] {
				"&eIf breaking liquids is set to true, then water/lava",
				"&ecan be deleted the same way as any regular block.",
			};
			descriptions[6] = new [] {
				"&eWhen this is active, placing blocks that intersect your own position",
				"&ecause the block to be placed, and you to be moved out of the way.",
				"&fThis is mainly useful for quick pillaring/towering.",
			};
			descriptions[7] = new [] {
				"&eIf noclip sliding isn't used, you will immediately stop when",
				"&eyou are in noclip mode and no movement keys are held down.",
			};
		}
	}
}