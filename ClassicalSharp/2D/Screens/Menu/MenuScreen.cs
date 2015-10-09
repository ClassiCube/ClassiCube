using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class MenuScreen : Screen {
		
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
			if( button != MouseButton.Left ) return false;
			for( int i = 0; i < buttons.Length; i++ ) {
				ButtonWidget widget = buttons[i];
				if( widget != null && widget.Bounds.Contains( mouseX, mouseY ) ) {
					if( widget.OnClick != null )
						widget.OnClick( game, widget );
					return true;
				}
			}
			return false;
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			for( int i = 0; i < buttons.Length; i++ ) {
				if( buttons[i] == null ) continue;
				buttons[i].Active = false;
			}
				
			for( int i = 0; i < buttons.Length; i++ ) {
				ButtonWidget widget = buttons[i];
				if( widget != null && widget.Bounds.Contains( mouseX, mouseY ) ) {
					widget.Active = true;
					WidgetSelected( widget );
					return true;
				}
			}
			WidgetSelected( null );
			return false;
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
		
		protected virtual void WidgetSelected( ButtonWidget widget ) {			
		}
	}
}