// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
			
			widgets = new Widget[] {
				// Column 1
				MakeClassic( -165, -200, "Music", OnWidgetClick,
				     g => g.UseMusic ? "yes" : "no",
				     (g, v) => { g.UseMusic = v == "yes";
				     	g.AudioPlayer.SetMusic( g.UseMusic );
				     	Options.Set( OptionsKey.UseMusic, v == "yes" ); }),
				
				MakeClassic( -165, -150, "Invert mouse", OnWidgetClick,
				     g => g.InvertMouse ? "yes" : "no",
				     (g, v) => { g.InvertMouse = v == "yes";
				     	Options.Set( OptionsKey.InvertMouse, v == "yes" ); }),
				
				MakeClassic( -165, -100, "View distance", OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ), true ) ),
				
				!network.IsSinglePlayer ? null :
					MakeClassic( -165, -50, "Block physics", OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => {
					     	((SinglePlayerServer)network).physics.Enabled = v == "yes";
					     	Options.Set( OptionsKey.SingleplayerPhysics, v == "yes" );
					     }),
				
				// Column 2
				MakeClassic( 165, -200, "Sound", OnWidgetClick,
				     g => g.UseSound ? "yes" : "no",
				     (g, v) => { g.UseSound = v == "yes";
				     	g.AudioPlayer.SetSound( g.UseSound );
				     	Options.Set( OptionsKey.UseSound, v == "yes" ); }),
				
				MakeClassic( 165, -150, "Show FPS", OnWidgetClick,
				     g => g.ShowFPS ? "yes" : "no",
				    (g, v) => { g.ShowFPS = v == "yes";
				     	Options.Set( OptionsKey.ShowFPS, v == "yes" ); }),
				
				MakeClassic( 165, -100, "View bobbing", OnWidgetClick,
				     g => g.ViewBobbing ? "yes" : "no",
				     (g, v) => { g.ViewBobbing = v == "yes";
				     	Options.Set( OptionsKey.ViewBobbing, v == "yes" ); }),
				
				MakeClassic( 165, -50, "FPS limit mode", OnWidgetClick,
				     g => g.FpsLimit.ToString(),
				     (g, v) => { object raw = Enum.Parse( typeof(FpsLimitMethod), v );
				     	g.SetFpsLimitMethod( (FpsLimitMethod)raw );
				     	Options.Set( OptionsKey.FpsLimit, v ); } ),
				
				MakeClassic( 0, 50, "Controls", LeftOnly(
					(g, w) => g.SetNewScreen( new ClassicKeyBindingsScreen( g ) ) ), null, null ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null, null,
			};
			widgets[7].Metadata = typeof(FpsLimitMethod);
			MakeValidators();
		}
		
		void MakeValidators() {
			INetworkProcessor network = game.Network;
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 16, 4096 ),
				network.IsSinglePlayer ? new BooleanValidator() : null,
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new EnumValidator(),
			};
		}
	}
}