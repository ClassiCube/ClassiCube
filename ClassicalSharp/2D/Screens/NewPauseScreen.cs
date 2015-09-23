using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class NewPauseScreen : Screen {
		
		public NewPauseScreen( Game game ) : base( game ) {
		}		
		ButtonWidget[] buttons;
		
		public override void Render( double delta ) {
			//graphicsApi.Draw2DQuad( 0, 0, game.Width, game.Height, new FastColour( 255, 255, 255, 100 ) );
			graphicsApi.Texturing = true;
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Render( delta );
			graphicsApi.Texturing = false;
		}
		
		Font titleFont;
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			buttons = new ButtonWidget[] {
				Make( 0, -50, "Options", Docking.Centre, g => { } ),
				Make( 0, 0, "Environment settings",Docking.Centre, g => { } ),
				Make( 0, 50, "Key mappings", Docking.Centre, g => { } ),
				Make( 0, 55, "Back to game", Docking.BottomOrRight, g => g.SetNewScreen( new NormalScreen( g ) ) ),
				Make( 0, 5, "Exit", Docking.BottomOrRight, g => g.Exit() ),
			};
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, titleFont, onClick );
		}
		
		public override void Dispose() {
			titleFont.Dispose();
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Dispose();
		}

		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left ) return false;
			for( int i = 0; i < buttons.Length; i++ ) {
				ButtonWidget widget = buttons[i];
				if( widget.ContainsPoint( mouseX, mouseY ) ) {
					widget.OnClick( game );
					return true;
				}
			}
			return false;
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Active = false;
			for( int i = 0; i < buttons.Length; i++ ) {
				ButtonWidget widget = buttons[i];
				if( widget.ContainsPoint( mouseX, mouseY ) ) {
					widget.Active = true;
					return true;
				}
			}
			return false;
		}
	}
}