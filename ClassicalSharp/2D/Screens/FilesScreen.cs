// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public abstract class FilesScreen : ClickableScreen {
		
		public FilesScreen( Game game ) : base( game ) {
		}
		
		protected Font textFont, arrowFont, titleFont;
		protected string[] entries;
		protected int currentIndex;
		protected ButtonWidget[] buttons;
		protected const int items = 5;
		
		TextWidget title;
		protected string titleText;
		
		public override void Init() {
			textFont = new Font( game.FontName, 16, FontStyle.Bold );
			arrowFont = new Font( game.FontName, 18, FontStyle.Bold );
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			title = ChatTextWidget.Create( game, 0, -155, titleText, 
			                          Anchor.Centre, Anchor.Centre, titleFont );
			
			buttons = new ButtonWidget[] {
				MakeText( 0, -100, Get( 0 ) ),
				MakeText( 0, -50, Get( 1 ) ),
				MakeText( 0, 0, Get( 2 ) ),
				MakeText( 0, 50, Get( 3 ) ),
				MakeText( 0, 100, Get( 4 ) ),
				
				Make( -220, 0, "<", (g, w) => PageClick( false ) ),
				Make( 220, 0, ">", (g, w) => PageClick( true ) ),
				MakeBack( false, titleFont, 
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
			UpdateArrows();
		}
		
		string Get( int index ) {
			return index < entries.Length ? entries[index] : "-----";
		}
		
		public override void Dispose() {
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Dispose();
			textFont.Dispose();
			arrowFont.Dispose();
			title.Dispose();
			titleFont.Dispose();
		}
		
		ButtonWidget MakeText( int x, int y, string text ) {
			return ButtonWidget.Create( game, x, y, 301, 40, text,
			                           Anchor.Centre, Anchor.Centre, textFont, TextButtonClick );
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 41, 40, text,
			                           Anchor.Centre, Anchor.Centre, arrowFont, LeftOnly( onClick ) );
		}
		
		protected abstract void TextButtonClick( Game game, Widget widget, MouseButton mouseBtn );
		
		protected void PageClick( bool forward ) {
			SetCurrentIndex( currentIndex + (forward ? items : -items) );
		}
		
		protected void SetCurrentIndex( int index ) {			
			if( index >= entries.Length ) index -= items;
			if( index < 0 ) index = 0;
			currentIndex = index;
			
			for( int i = 0; i < items; i++ )
				buttons[i].SetText( Get( currentIndex + i ) );
			UpdateArrows();
		}
		
		protected void UpdateArrows() {
			buttons[5].Disabled = false;
			buttons[6].Disabled = false;
			if( currentIndex < items )
				buttons[5].Disabled = true;
			if( currentIndex >= entries.Length - items )
				buttons[6].Disabled = true;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
			} else if( key == Key.Left ) {
				PageClick( false );
			} else if( key == Key.Right ) {
				PageClick( true );
			} else {
				return false;
			}
			return true;
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( buttons, mouseX, mouseY );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( buttons, mouseX, mouseY, button );
		}
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].OnResize( oldWidth, oldHeight, width, height );
			title.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Render( double delta ) {
			api.Draw2DQuad( 0, 0, game.Width, game.Height, new FastColour( 60, 60, 60, 160 ) );
			api.Texturing = true;
			title.Render( delta );
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Render( delta );
			api.Texturing = false;
		}
	}
}