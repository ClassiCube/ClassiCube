using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class FilesScreen : ClickableScreen {
		
		public FilesScreen( Game game ) : base( game ) {
		}
		
		protected Font textFont, arrowFont, titleFont;
		protected string[] files;
		int currentIndex;
		protected ButtonWidget[] buttons;
		
		TextWidget title;
		protected string titleText;
		
		public override void Init() {
			textFont = new Font( game.FontName, 14, FontStyle.Bold );
			arrowFont = new Font( game.FontName, 18, FontStyle.Bold );
			int size = game.Drawer2D.UseBitmappedChat ? 13 : 16;
			titleFont = new Font( game.FontName, size, FontStyle.Bold );
			title = TextWidget.Create( game, 0, -130, titleText, Anchor.Centre, Anchor.Centre, titleFont );
			title.Init();
			
			buttons = new ButtonWidget[] {
				MakeText( 0, -80, Get( 0 ) ),
				MakeText( 0, -40, Get( 1 ) ),
				MakeText( 0, 0, Get( 2 ) ),
				MakeText( 0, 40, Get( 3 ) ),
				MakeText( 0, 80, Get( 4 ) ),
				
				Make( -160, 0, "<", (g, w) => PageClick( false ) ),
				Make( 160, 0, ">", (g, w) => PageClick( true ) ),
				null,
			};
		}
		
		string Get( int index ) {
			return index < files.Length ? files[index] : "-----";
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
			return ButtonWidget.Create( game, x, y, 240, 30, text,
			                           Anchor.Centre, Anchor.Centre, textFont, TextButtonClick );
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 40, 40, text,
			                           Anchor.Centre, Anchor.Centre, arrowFont, LeftOnly( onClick ) );
		}
		
		protected abstract void TextButtonClick( Game game, Widget widget, MouseButton mouseBtn );
		
		protected void PageClick( bool forward ) {
			currentIndex += forward ? 5 : -5;
			if( currentIndex >= files.Length )
				currentIndex -= 5;
			if( currentIndex < 0 )
				currentIndex = 0;
			
			for( int i = 0; i < 5; i++ ) {
				buttons[i].SetText( Get( currentIndex + i ) );
			}
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
			graphicsApi.Draw2DQuad( 0, 0, game.Width, game.Height, new FastColour( 60, 60, 60, 160 ) );
			graphicsApi.Texturing = true;
			title.Render( delta );
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Render( delta );
			graphicsApi.Texturing = false;
		}
	}
}