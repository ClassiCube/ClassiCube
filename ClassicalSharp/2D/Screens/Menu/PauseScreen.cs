using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class PauseScreen : MenuScreen {
		
		public PauseScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuWidgets( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			
			if( game.UseClassicOptions )
				MakeClassic();
			else
				MakeNormal();
			CheckHacksAllowed( null, null );
		}
		
		void MakeNormal() {
			widgets = new Widget[] {
				// Column 1
				Make( -140, -150, "Misc options",
				     (g, w) => g.SetNewScreen( new MiscOptionsScreen( g ) ) ),
				Make( -140, -100, "Gui options",
				     (g, w) => g.SetNewScreen( new GuiOptionsScreen( g ) ) ),
				Make( -140, -50, "Hacks settings",
				     (g, w) => g.SetNewScreen( new HacksSettingsScreen( g ) ) ),
				Make( -140, 0, "Env settings",
				     (g, w) => g.SetNewScreen( new EnvSettingsScreen( g ) ) ),
				Make( -140, 50, "Key bindings",
				     (g, w) => g.SetNewScreen( new NormalKeyBindingsScreen( g ) ) ),
				
				// Column 2
				Make( 140, -150, "Save level",
				     (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					Make( 140, -100, "Load level",
					     (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					Make( 140, -50, "Generate level",
					     (g, w) => g.SetNewScreen( new GenLevelScreen( g ) ) ),
				Make( 140, 0, "Select texture pack",
				     (g, w) => g.SetNewScreen( new TexturePackScreen( g ) ) ),
				Make( 140, 50, "Hotkeys",
				     (g, w) => g.SetNewScreen( new HotkeyScreen( g ) ) ),
				
				Make( 0, 100, "Nostalgia options",
				     (g, w) => g.SetNewScreen( new NostalgiaScreen( g ) ) ),
				
				// Other
				MakeOther( 10, 5, 120, "Quit game", Anchor.BottomOrRight,
				          (g, w) => g.Exit() ),
				MakeBack( true, titleFont,
				         (g, w) => g.SetNewScreen( null ) ),
			};
		}
		
		void MakeClassic() {
			widgets = new Widget[] {
				MakeClassic( 0, -100, "Options",
				     (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					MakeClassic( 0, -50, "Generate level",
					     (g, w) => g.SetNewScreen( new GenLevelScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					MakeClassic( 0, 0, "Load level",
					     (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),
				
				MakeClassic( 0, 50, "Save level",
				     (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
				
				MakeBack( true, titleFont,
				         (g, w) => g.SetNewScreen( null ) ),
				
				game.PureClassicMode ? null :
					MakeClassic( 0, 150, "Nostalgia options",
				     (g, w) => g.SetNewScreen( new NostalgiaScreen( g ) ) ),
			};
		}

		void CheckHacksAllowed( object sender, EventArgs e ) {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Disabled = false;
			}
			if( !game.LocalPlayer.CanAnyHacks && !game.UseClassicOptions ) {
				widgets[3].Disabled = true; // env settings
				widgets[8].Disabled = true; // select texture pack
			}
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeClassic( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 401, 41, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeOther( int x, int y, int width, string text, Anchor hAnchor, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, 35, text,
			                           hAnchor, Anchor.BottomOrRight, titleFont, LeftOnly( onClick ) );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape )
				game.SetNewScreen( null );
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
	}
}