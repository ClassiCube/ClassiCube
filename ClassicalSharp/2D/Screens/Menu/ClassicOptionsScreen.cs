using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class ClassicOptionsScreen : MenuOptionsScreen {
		
		public ClassicOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			widgets = new ButtonWidget[] {
				// Column 1
				Make( -140, -200, "Music", OnWidgetClick,
				     g => g.UseMusic ? "yes" : "no",
				     (g, v) => { g.UseMusic = v == "yes";
				     	g.AudioPlayer.SetMusic( g.UseMusic );
				     	Options.Set( OptionsKey.UseMusic, v == "yes" ); }),
				
				Make( -140, -150, "Invert mouse", OnWidgetClick,
				     g => g.InvertMouse ? "yes" : "no",
				     (g, v) => { g.InvertMouse = v == "yes";
				     	Options.Set( OptionsKey.InvertMouse, v == "yes" ); }),
				
				Make( -140, -100, "View distance", OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ), true ) ),
				
				!network.IsSinglePlayer ? null :
					Make( -140, -50, "Block physics", OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => {
					     	((SinglePlayerServer)network).physics.Enabled = v == "yes";
					     	Options.Set( OptionsKey.SingleplayerPhysics, v == "yes" );
					     }),
				
				Make( -140, 0, "Mouse sensitivity", OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => { g.MouseSensitivity = Int32.Parse( v );
				     	Options.Set( OptionsKey.Sensitivity, v ); } ),
				
				// Column 2
				Make( 140, -200, "Sound", OnWidgetClick,
				     g => g.UseSound ? "yes" : "no",
				     (g, v) => { g.UseSound = v == "yes";
				     	g.AudioPlayer.SetSound( g.UseSound );
				     	Options.Set( OptionsKey.UseSound, v == "yes" ); }),
				
				Make( 140, -150, "Show FPS", OnWidgetClick,
				     g => g.ShowFPS ? "yes" : "no",
				     (g, v) => g.ShowFPS = v == "yes" ),
				
				Make( 140, -100, "View bobbing", OnWidgetClick,
				     g => g.ViewBobbing ? "yes" : "no",
				     (g, v) => { g.ViewBobbing = v == "yes";
				     	Options.Set( OptionsKey.ViewBobbing, v == "yes" ); }),
				
				Make( 140, -50, "FPS limit", OnWidgetClick,
				     g => g.FpsLimit.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(FpsLimitMethod), v );
				     	g.SetFpsLimitMethod( (FpsLimitMethod)raw );
				     	Options.Set( OptionsKey.FpsLimit, v ); } ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			widgets[8].Metadata = typeof(FpsLimitMethod);
			MakeValidators();
			MakeDescriptions();
			okayIndex = widgets.Length - 1;
		}
		
		void MakeValidators() {
			INetworkProcessor network = game.Network;
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 16, 4096 ),
				network.IsSinglePlayer ? new BooleanValidator() : null,
				new IntegerValidator( 1, 100 ),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new EnumValidator(),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[8] = new[] {
				"&aDetermines the method used to limit the number of FPS",
				"&eVSync: &fNumber of frames rendered is at most the monitor's refresh rate.",
				"&e30/60/120 FPS: &f30/60/120 frames rendered at most each second.",
				"&eNoLimit: &Renders as many frames as the GPU can handle each second.",
				"&cUsing NoLimit mode is discouraged for general usage.",
			};
		}
	}
}