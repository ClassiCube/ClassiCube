using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class MenuScreen : ClickableScreen {
		
		public MenuScreen( Game game ) : base( game ) {
		}
		protected ButtonWidget[] buttons;
		protected Font titleFont, regularFont;
		
		protected void RenderMenuBounds() {
			graphicsApi.Draw2DQuad( 0, 0, game.Width, game.Height, new FastColour( 60, 60, 60, 160 ) );
		}
		
		protected void RenderMenuButtons( double delta ) {
			for( int i = 0; i < buttons.Length; i++ ) {
				if( buttons[i] == null ) continue;
				buttons[i].Render( delta );
			}
		}
		
		public override void Init() {
			int size = game.Drawer2D.UseBitmappedChat ? 13 : 16;
			titleFont = new Font( "Arial", size, FontStyle.Bold );
		} 
		
		public override void Dispose() {
			for( int i = 0; i < buttons.Length; i++ ) {
				if( buttons[i] == null ) continue;
				buttons[i].Dispose();
			}
			titleFont.Dispose();
			if( regularFont != null )
				regularFont.Dispose();
		}

		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < buttons.Length; i++ ) {
				if( buttons[i] == null ) continue;
				buttons[i].OnResize( oldWidth, oldHeight, width, height );
			}
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( buttons, mouseX, mouseY, button );
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( buttons, mouseX, mouseY );
		}
		
		public override bool HandlesKeyPress( char key ) {
			return true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			return true;
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return true;
		}
	}
}