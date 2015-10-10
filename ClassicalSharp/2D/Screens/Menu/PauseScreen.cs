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
			if( game.Network.IsSinglePlayer ) {
				buttons = new ButtonWidget[] {
					Make( -140, -50, "Options", Anchor.Centre, (g, w) => g.SetNewScreen( new OptionsScreen( g ) ) ),
					Make( -140, 0, "Environment settings", Anchor.Centre, (g, w) => g.SetNewScreen( new EnvSettingsScreen( g ) ) ),
					Make( -140, 50, "Select texture pack", Anchor.Centre, (g, w) => g.SetNewScreen( new TexturePackScreen( g ) ) ),
					Make( 140, -50, "Save level", Anchor.Centre, (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
					Make( 140, 0, "Load level", Anchor.Centre, (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),
					// TODO: singleplayer Make( 0, 50, "Load/Save/Gen level", Docking.Centre, (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
					Make( 0, 55, "Back to game", Anchor.BottomOrRight, (g, w) => g.SetNewScreen( new NormalScreen( g ) ) ),
					Make( 0, 5, "Quit game", Anchor.BottomOrRight, (g, w) => g.Exit() ),
				};
			} else {
				buttons = new ButtonWidget[] {
					Make( 0, -100, "Options", Anchor.Centre, (g, w) => g.SetNewScreen( new OptionsScreen( g ) ) ),
					Make( 0, -50, "Environment settings", Anchor.Centre, (g, w) => g.SetNewScreen( new EnvSettingsScreen( g ) ) ),
					Make( 0, 0, "Select texture pack", Anchor.Centre, (g, w) => g.SetNewScreen( new TexturePackScreen( g ) ) ),
					Make( 0, 50, "Save level", Anchor.Centre, (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
					Make( 0, 55, "Back to game", Anchor.BottomOrRight, (g, w) => g.SetNewScreen( new NormalScreen( g ) ) ),
					Make( 0, 5, "Quit game", Anchor.BottomOrRight, (g, w) => g.Exit() ),
				};
			}
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, ButtonWidget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, vDocking, titleFont, onClick );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape )
				game.SetNewScreen( new NormalScreen( game ) );
			return true;
		}
	}
}