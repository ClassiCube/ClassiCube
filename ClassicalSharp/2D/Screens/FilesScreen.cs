using System;
using System.Drawing;
using OpenTK.Input;
using System.IO;
using ClassicalSharp.TexturePack;

namespace ClassicalSharp {
	
	public abstract class FilesScreen : Screen {
		
		public FilesScreen( Game game ) : base( game ) {
		}
		
		protected Font textFont, arrowFont, titleFont;
		protected string[] files;
		int currentIndex;
		protected ButtonWidget[] buttons;
		
		TextWidget title;
		protected string titleText;
		
		public override void Init() {
			textFont = new Font( "Arial", 14, FontStyle.Bold );
			arrowFont = new Font( "Arial", 18, FontStyle.Bold );
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
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
		
		ButtonWidget Make( int x, int y, string text, Action<Game, ButtonWidget> onClick ) {
			return ButtonWidget.Create( game, x, y, 40, 40, text,
			                           Anchor.Centre, Anchor.Centre, arrowFont, onClick );
		}
		
		protected abstract void TextButtonClick( Game game, ButtonWidget widget );
		
		void PageClick( bool forward ) {
			currentIndex += forward ? 5 : -5;
			if( currentIndex >= files.Length )
				currentIndex -= 5;
			if( currentIndex < 0 )
				currentIndex = 0;
			
			for( int i = 0; i < 5; i++ ) {
				buttons[i].SetText( Get( currentIndex + i ) );
			}
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			for( int i = 0; i < buttons.Length; i++ )
				buttons[i].Active = false;
			
			for( int i = 0; i < buttons.Length; i++ ) {
				ButtonWidget widget = buttons[i];
				if( widget.Bounds.Contains( mouseX, mouseY ) ) {
					widget.Active = true;
					return true;
				}
			}
			return false;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left ) return false;
			for( int i = 0; i < buttons.Length; i++ ) {
				ButtonWidget widget = buttons[i];
				if( widget.Bounds.Contains( mouseX, mouseY ) ) {
					widget.OnClick( game, widget );
					return true;
				}
			}
			return false;
		}
		
		public override bool HandlesAllInput {
			get { return true; }
		}
		
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
	
	public sealed class TexturePackScreen : FilesScreen {
		
		public TexturePackScreen( Game game ) : base( game ) {
			titleText = "Select a texture pack zip";
			string directory = Environment.CurrentDirectory;
			files = Directory.GetFiles( directory, "*.zip", SearchOption.AllDirectories );
			
			for( int i = 0; i < files.Length; i++ ) {
				string absolutePath = files[i];
				files[i] = absolutePath.Substring( directory.Length + 1 );
			}
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( new NormalScreen( game ) );
				return true;
			}
			return false;
		}
		
		public override void Init() {
			base.Init();
			buttons[buttons.Length - 1] = 
				Make( 0, 5, "Back to menu", (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, ButtonWidget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text,
			                           Anchor.Centre, Anchor.BottomOrRight, titleFont, onClick );
		}
		
		protected override void TextButtonClick( Game game, ButtonWidget widget ) {
			string path = widget.Text;
			if( File.Exists( path ) ) {
				TexturePackExtractor extractor = new TexturePackExtractor();
				extractor.Extract( path, game );
			}
		}
	}
}
