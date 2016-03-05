using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class MiscOptionsScreen : MenuOptionsScreen {
		
		public MiscOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			widgets = new Widget[] {
				// Column 1
				!network.IsSinglePlayer ? null :
					Make( -140, -100, "Click distance", OnWidgetClick,
					     g => g.LocalPlayer.ReachDistance.ToString(),
					     (g, v) => g.LocalPlayer.ReachDistance = Single.Parse( v ) ),
				
				Make( -140, -50, "Music", OnWidgetClick,
				     g => g.UseMusic ? "yes" : "no",
				     (g, v) => { g.UseMusic = v == "yes";
				     	g.AudioPlayer.SetMusic( g.UseMusic );
				     	Options.Set( OptionsKey.UseMusic, v == "yes" ); }),
				
				Make( -140, 0, "Sound", OnWidgetClick,
				     g => g.UseSound ? "yes" : "no",
				     (g, v) => { g.UseSound = v == "yes";
				     	g.AudioPlayer.SetSound( g.UseSound );
				     	Options.Set( OptionsKey.UseSound, v == "yes" ); }),
				
				Make( -140, 50, "View bobbing", OnWidgetClick,
				     g => g.ViewBobbing ? "yes" : "no",
				     (g, v) => { g.ViewBobbing = v == "yes";
				     	Options.Set( OptionsKey.ViewBobbing, v == "yes" ); }),
				
				// Column 2
				!network.IsSinglePlayer ? null :
					Make( 140, -100, "Block physics", OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => {
					     	((SinglePlayerServer)network).physics.Enabled = v == "yes";
					     	Options.Set( OptionsKey.SingleplayerPhysics, v == "yes" );
					     }),
				
				Make( 140, -50, "Auto close launcher", OnWidgetClick,
				     g => Options.GetBool( OptionsKey.AutoCloseLauncher, false ) ? "yes" : "no",
				     (g, v) => Options.Set( OptionsKey.AutoCloseLauncher, v == "yes" ) ),
				
				Make( 140, 0, "Invert mouse", OnWidgetClick,
				     g => g.InvertMouse ? "yes" : "no",
				     (g, v) => { g.InvertMouse = v == "yes";
				     	Options.Set( OptionsKey.InvertMouse, v == "yes" ); }),
				
				Make( 140, 50, "Mouse sensitivity", OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => { g.MouseSensitivity = Int32.Parse( v );
				     	Options.Set( OptionsKey.Sensitivity, v ); } ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				null, null,
			};
			MakeValidators();
			MakeDescriptions();
		}
		
		void MakeValidators() {
			INetworkProcessor network = game.Network;
			validators = new MenuInputValidator[] {
				network.IsSinglePlayer ? new RealValidator( 1, 1024 ) : null,
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				
				network.IsSinglePlayer ? new BooleanValidator() : null,			
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 1, 100 ),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[0] = new[] {
				"&eSets how far away you can place/delete blocks",
				"&fThe default click distance is 5 blocks.",
			};
		}
	}
}