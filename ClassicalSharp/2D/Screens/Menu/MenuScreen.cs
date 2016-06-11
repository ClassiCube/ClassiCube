// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public abstract class MenuScreen : ClickableScreen {
		
		public MenuScreen( Game game ) : base( game ) {
		}
		protected Widget[] widgets;
		protected Font titleFont, regularFont;
		protected FastColour backCol = new FastColour( 60, 60, 60, 160 );
		
		protected void RenderMenuBounds() {
			api.Draw2DQuad( 0, 0, game.Width, game.Height, backCol );
		}
		
		protected void RenderMenuWidgets( double delta ) {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Render( delta );
			}
		}
		
		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
		} 
		
		public override void Dispose() {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].Dispose();
			}
			titleFont.Dispose();
			if( regularFont != null )
				regularFont.Dispose();
		}

		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < widgets.Length; i++ ) {
				if( widgets[i] == null ) continue;
				widgets[i].OnResize( oldWidth, oldHeight, width, height );
			}
		}
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( widgets, mouseX, mouseY, button );
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( widgets, mouseX, mouseY );
		}
		
		public override bool HandlesMouseScroll( int delta ) { return true; }
		
		public override bool HandlesKeyPress( char key ) { return true; }
		
		public override bool HandlesKeyDown( Key key ) { return true; }
		
		public override bool HandlesKeyUp( Key key ) { return true; }
	}
}