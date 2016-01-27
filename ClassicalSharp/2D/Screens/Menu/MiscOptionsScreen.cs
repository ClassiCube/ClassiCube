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
			
			widgets = new ButtonWidget[] {
				// Column 1
				!network.IsSinglePlayer ? null :
					Make( -140, -200, "Click distance", OnWidgetClick,
					     g => g.LocalPlayer.ReachDistance.ToString(),
					     (g, v) => g.LocalPlayer.ReachDistance = Single.Parse( v ) ),
				
				Make( -140, -150, "Music", OnWidgetClick,
				     g => g.UseMusic ? "yes" : "no",
				     (g, v) => { g.UseMusic = v == "yes";
				     	g.AudioPlayer.SetMusic( g.UseMusic );
				     	Options.Set( OptionsKey.UseMusic, v == "yes" ); }),
				
				Make( -140, -100, "Names mode", OnWidgetClick,
				     g => g.Players.NamesMode.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(NameMode), v );
				     	g.Players.NamesMode = (NameMode)raw;
				     	Options.Set( OptionsKey.NamesMode, v ); } ),
				
				Make( -140, -50, "FPS limit", OnWidgetClick,
				     g => g.FpsLimit.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(FpsLimitMethod), v );
				     	g.SetFpsLimitMethod( (FpsLimitMethod)raw );
				     	Options.Set( OptionsKey.FpsLimit, v ); } ),

				Make( -140, 0, "View distance", OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ), true ) ),
				
				// Column 2
				!network.IsSinglePlayer ? null :
					Make( 140, -200, "Block physics", OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => {
					     	((SinglePlayerServer)network).physics.Enabled = v == "yes";
					     	Options.Set( OptionsKey.SingleplayerPhysics, v == "yes" );
					     }),
				
				Make( 140, -150, "Sound", OnWidgetClick,
				     g => g.UseSound ? "yes" : "no",
				     (g, v) => { g.UseSound = v == "yes";
				     	g.AudioPlayer.SetSound( g.UseSound );
				     	Options.Set( OptionsKey.UseSound, v == "yes" ); }),
				
				Make( 140, -100, "View bobbing", OnWidgetClick,
				     g => g.ViewBobbing ? "yes" : "no",
				     (g, v) => { g.ViewBobbing = v == "yes";
				     	Options.Set( OptionsKey.ViewBobbing, v == "yes" ); }),
				
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
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			widgets[2].Metadata = typeof(NameMode);
			widgets[3].Metadata = typeof(FpsLimitMethod);
			MakeValidators();
			MakeDescriptions();
			okayIndex = widgets.Length - 1;
		}
		
		void MakeValidators() {
			INetworkProcessor network = game.Network;
			validators = new MenuInputValidator[] {
				network.IsSinglePlayer ? new RealValidator( 1, 1024 ) : null,
				new BooleanValidator(),
				new EnumValidator(),
				new EnumValidator(),
				new IntegerValidator( 16, 4096 ),
				
				network.IsSinglePlayer ? new BooleanValidator() : null,
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 1, 100 ),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[0] = new[] {
				"&aControls how far away you can place/delete blocks",
				"&eThe default click distance is 5 blocks.",
			};
			descriptions[2] = new[] {
				"&aDetermines how the names of other players are drawn",
				"&eNoNames: &fNo player names are drawn.",
				"&eHoveredOnly: &fName of the targeted player is drawn see-through.",
				"&eAllNames: &fAll player names are drawn normally.",
				"&eAllNamesAndHovered: &fName of the targeted player is drawn see-through.",
				"&f               All other player names are drawn normally.",
			};
			descriptions[3] = new[] {
				"&aDetermines the method used to limit the number of FPS",
				"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.",
				"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.",
				"&eNoLimit: &Renders as many frames as the GPU can handle each second.",
				"&cUsing NoLimit mode is discouraged for general usage.",
			};
		}
	}
}