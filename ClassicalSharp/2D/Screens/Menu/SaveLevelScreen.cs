// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Map;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class SaveLevelScreen : MenuScreen {
		
		public SaveLevelScreen( Game game ) : base( game ) {
		}
		
		MenuInputWidget inputWidget;
		TextWidget descWidget;
		const int overwriteIndex = 2;
		static FastColour grey = new FastColour( 150, 150, 150 );
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			gfx.Texturing = true;
			RenderMenuWidgets( delta );
			inputWidget.Render( delta );
			if( descWidget != null ) descWidget.Render( delta );
			gfx.Texturing = false;
			
			float cX = game.Width / 2, cY = game.Height / 2;
			gfx.Draw2DQuad( cX - 250, cY + 90, 500, 2, grey );
			if( textPath == null ) return;
			SaveMap( textPath );
			textPath = null;
		}
		
		public override bool HandlesKeyPress( char key ) {
			RemoveOverwrites();
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			RemoveOverwrites();
			if( key == Key.Escape ) {
				game.Gui.SetNewScreen( null );
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
				game, 0, -30, 500, 30, "", Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, new PathValidator() );
			
			widgets = new Widget[] {
				ButtonWidget.Create( game, 0, 20, 301, 40, "Save", Anchor.Centre,
				                    Anchor.Centre, titleFont, SaveClassic ),
				ButtonWidget.Create( game, -150, 120, 201, 40, "Save schematic", Anchor.Centre,
				                    Anchor.Centre, titleFont, SaveSchematic ),
				ChatTextWidget.Create( game, 110, 120, "&eCan be imported into MCEdit", Anchor.Centre,
				                    Anchor.Centre, regularFont ),
				null,
				MakeBack( false, titleFont,
				         (g, w) => g.Gui.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		
		public override void OnResize( int width, int height ) {
			inputWidget.OnResize( width, height );
			base.OnResize( width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			inputWidget.Dispose();
			DisposeDescWidget();
			base.Dispose();
		}
		
		void SaveClassic( Game game, Widget widget, MouseButton mouseBtn ) {
			DoSave( widget, mouseBtn, ".cw" );
		}
		
		void SaveSchematic( Game game, Widget widget, MouseButton mouseBtn ) {
			DoSave( widget, mouseBtn, ".schematic" );
		}
		
		void DoSave( Widget widget, MouseButton mouseBtn, string ext ) {
			if( mouseBtn != MouseButton.Left ) return;
			
			string text = inputWidget.GetText();
			if( text.Length == 0 ) {
				MakeDescWidget( "&ePlease enter a filename" ); return;
			}
			string file = Path.ChangeExtension( text, ext );
			text = Path.Combine( Program.AppDirectory, "maps" );
			text = Path.Combine( text, file );
			
			if( File.Exists( text ) && widget.Metadata == null ) {
				((ButtonWidget)widget).SetText( "&cOverwrite existing?" );
				((ButtonWidget)widget).Metadata = true;
			} else {
				// NOTE: We don't immediately save here, because otherwise the 'saving...'
				// will not be rendered in time because saving is done on the main thread.
				MakeDescWidget( "Saving.." );
				textPath = text;
				RemoveOverwrites();
			}
		}
		
		void RemoveOverwrites() {
			RemoveOverwrite( widgets[0] ); RemoveOverwrite( widgets[1] );
		}
		
		void RemoveOverwrite( Widget widget ) {
			ButtonWidget button = (ButtonWidget)widget;
			if( button.Metadata == null ) return;
			button.Metadata = null;
			button.SetText( "Save" );
		}
		
		string textPath;
		void SaveMap( string path ) {
			bool classic = path.EndsWith( ".cw" );
			try {
				if( File.Exists( path ) )
					File.Delete( path );
				using( FileStream fs = new FileStream( path, FileMode.CreateNew, FileAccess.Write ) ) {
					IMapFormatExporter exporter = null;
					if( classic ) exporter = new MapCwExporter();
					else exporter = new MapSchematicExporter();
					exporter.Save( fs, game );
				}
			} catch( Exception ex ) {
				ErrorHandler.LogError( "saving map", ex );
				MakeDescWidget( "&cError while trying to save map" );
				return;
			}
			game.Chat.Add( "&eSaved map to: " + Path.GetFileName( path ) );
			game.Gui.SetNewScreen( new PauseScreen( game ) );
		}
		
		void MakeDescWidget( string text ) {
			DisposeDescWidget();
			descWidget = ChatTextWidget.Create( game, 0, 65, text,
			                                   Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		void DisposeDescWidget() {
			if( descWidget != null ) {
				descWidget.Dispose();
				descWidget = null;
			}
		}
	}
}