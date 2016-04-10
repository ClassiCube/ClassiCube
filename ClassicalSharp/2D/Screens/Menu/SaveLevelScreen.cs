// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Map;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class SaveLevelScreen : MenuScreen {
		
		public SaveLevelScreen( Game game ) : base( game ) {
		}
		
		MenuInputWidget inputWidget;
		TextWidget descWidget;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			api.Texturing = true;
			RenderMenuWidgets( delta );
			inputWidget.Render( delta );
			if( descWidget != null )
				descWidget.Render( delta );
			api.Texturing = false;
			
			if( textPath != null ) {
				SaveMap( textPath );
				textPath = null;
			}
		}
		
		public override bool HandlesKeyPress( char key ) {
			RemoveOverwriteButton();
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			RemoveOverwriteButton();
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
				return true;
			}
			return inputWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return inputWidget.HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			base.Init();
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			
			inputWidget = MenuInputWidget.Create(
				game, -30, 50, 500, 30, "", Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, new PathValidator() );
			
			widgets = new [] {
				ButtonWidget.Create( game, 260, 50, 60, 30, "Save", Anchor.Centre,
				                    Anchor.Centre, titleFont, OkButtonClick ),
				null,
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			inputWidget.OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			inputWidget.Dispose();
			DisposeDescWidget();
			base.Dispose();
		}
		
		void OkButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string text = inputWidget.GetText();
			if( text.Length == 0 ) {
				MakeDescWidget( "Please enter a filename" );
				return;
			}
			string file = Path.ChangeExtension( text, ".cw" );
			text = Path.Combine( Program.AppDirectory, "maps" );
			text = Path.Combine( text, file );
			
			if( File.Exists( text ) ) {
				widgets[1] = ButtonWidget.Create( game, 0, 90, 260, 30, "Overwrite existing?",
				                                 Anchor.Centre, Anchor.Centre, titleFont, OverwriteButtonClick );
			} else {
				// NOTE: We don't immediately save here, because otherwise the 'saving...'
				// will not be rendered in time because saving is done on the main thread.
				MakeDescWidget( "Saving.." );
				textPath = text;
				RemoveOverwriteButton();
			}
		}
		
		void OverwriteButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			string text = inputWidget.GetText();
			string file = Path.ChangeExtension( text, ".cw" );
			text = Path.Combine( Program.AppDirectory, "maps" );
			text = Path.Combine( text, file );
			
			MakeDescWidget( "Saving.." );
			textPath = text;
			RemoveOverwriteButton();
		}
		
		void RemoveOverwriteButton() {
			if( widgets[1] == null ) return;
			widgets[1].Dispose();
			widgets[1] = null;
		}
		
		string textPath;
		void SaveMap( string path ) {
			try {
				if( File.Exists( path ) )
					File.Delete( path );
				using( FileStream fs = new FileStream( path, FileMode.CreateNew, FileAccess.Write ) ) {
					IMapFormatExporter exporter = new MapCw();
					exporter.Save( fs, game );
				}
			} catch( Exception ex ) {
				ErrorHandler.LogError( "saving map", ex );
				MakeDescWidget( "&cError while trying to save map" );
				return;
			}
			game.Chat.Add( "&eSaved map to: " + Path.GetFileName( path ) );
			game.SetNewScreen( new PauseScreen( game ) );
		}
		
		void MakeDescWidget( string text ) {
			DisposeDescWidget();
			descWidget = TextWidget.Create( game, 0, 90, text, Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		void DisposeDescWidget() {
			if( descWidget != null ) {
				descWidget.Dispose();
				descWidget = null;
			}
		}
	}
}