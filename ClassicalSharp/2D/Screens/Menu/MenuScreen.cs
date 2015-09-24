using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class MenuScreen : Screen {
		
		public MenuScreen( Game game ) : base( game ) {
		}		
		protected ButtonWidget[] buttons;
		protected Font titleFont;
		
		public override void Render( double delta ) {
			graphicsApi.Draw2DQuad( 0, 0, game.Width, game.Height, new FastColour( 255, 255, 255, 100 ) );
			graphicsApi.Texturing = true;
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Dispose();
			titleFont.Dispose();
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