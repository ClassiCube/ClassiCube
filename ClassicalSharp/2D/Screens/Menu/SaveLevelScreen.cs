using System;
using System.Drawing;
using OpenTK.Input;
using System.IO;

namespace ClassicalSharp {
	
	public sealed class SaveLevelScreen : MenuScreen {
		
		public SaveLevelScreen( Game game ) : base( game ) {
		}
		
		MenuInputWidget inputWidget;
		Font hintFont;
		TextWidget descWidget;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			inputWidget.Render( delta );
			if( descWidget != null )
				descWidget.Render( delta );
			graphicsApi.Texturing = false;
			
			if( textPath != null ) {
				SaveMap( textPath );
				textPath = null;
			}
		}
		
		public override bool HandlesKeyPress( char key ) {
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( new NormalScreen( game ) );
				return true;
			}
			return inputWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return inputWidget.HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			hintFont = new Font( "Arial", 14, FontStyle.Italic );
			
			inputWidget = MenuInputWidget.Create(
				game, -30, 50, 500, 25, "", Docking.Centre, Docking.Centre,
				regularFont, titleFont, hintFont, new PathValidator() );
			
			buttons = new [] {
				ButtonWidget.Create( game, 260, 50, 60, 30, "Save", Docking.Centre,
				                    Docking.Centre, titleFont, OkButtonClick ),
				ButtonWidget.Create( game, 0, 5, 240, 35, "Back to menu", Docking.Centre, Docking.BottomOrRight,
				                    titleFont, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			inputWidget.OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			inputWidget.Dispose();
			hintFont.Dispose();
			DisposeDescWidget();
			base.Dispose();
		}
		
		void OkButtonClick( Game game, ButtonWidget widget ) {
			string text = inputWidget.GetText();
			if( text.Length == 0 ) {
				MakeDescWidget( "Please enter a filename" );
				return;
			}
			text = Path.ChangeExtension( text, ".cw" );
			
			if( File.Exists( text ) ) {
				MakeDescWidget( "&eFilename already exists" );
			} else {
				// NOTE: We don't immediately save here, because otherwise the 'saving...'
				// will not be rendered in time because saving is done on the main thread.
				MakeDescWidget( "Saving.." );
				textPath = text;		
			}			
		}
		
		string textPath;
		void SaveMap( string path ) {
			try {
				using( FileStream fs = new FileStream( path, FileMode.CreateNew, FileAccess.Write ) ) {
					IMapFile map = new MapCw();
					map.Save( fs, game );
				}
			} catch( Exception ex ) {
				Utils.LogError( "Error while trying to save map: {0}{1}", Environment.NewLine, ex );
				MakeDescWidget( "&cFailed to save map" );
				return;
			}
			game.SetNewScreen( new PauseScreen( game ) );
		}
		
		void MakeDescWidget( string text) {
			DisposeDescWidget();
			descWidget = TextWidget.Create( game, 0, 90, text, Docking.Centre, Docking.Centre, regularFont );
		}
		
		void DisposeDescWidget() {
			if( descWidget != null ) {
				descWidget.Dispose();
				descWidget = null;
			}
		}
	}
}