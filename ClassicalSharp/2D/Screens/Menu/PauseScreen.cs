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
			RenderMenuButtons( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			buttons = new ButtonWidget[] {
				// Column 1
				Make( -140, -100, "Options", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new OptionsScreen( g ) ) ),
				Make( -140, -50, "Environment settings", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new EnvSettingsScreen( g ) ) ),
				
				Make( -140, 0, "Hotkeys", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new HotkeyScreen( g ) ) ),
				Make( -140, 50, "Key bindings", Anchor.Centre,
				     (g, w) => g.SetNewScreen( new KeyBindingsScreen( g ) ) ),	
				// Column 2
				Make( 140, -100, "Save level", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					Make( 140, -50, "Load level", Anchor.Centre, 
					     (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),
				// TODO: singleplayer Generate level screen
				Make( 140, 50, "Select texture pack", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new TexturePackScreen( g ) ) ),
				// Other
				Make( 0, 55, "Back to game", Anchor.BottomOrRight, 
				     (g, w) => g.SetNewScreen( null ) ),
				Make( 0, 5, "Quit game", Anchor.BottomOrRight, (g, w) => g.Exit() ),
			};
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, vDocking, titleFont, onClick );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape )
				game.SetNewScreen( null );
			return true;
		}
	}
}