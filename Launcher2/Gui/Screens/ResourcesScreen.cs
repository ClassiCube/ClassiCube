// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using ClassicalSharp.Network;

namespace Launcher {
	
	public sealed class ResourcesScreen : LauncherScreen {
		
		Font infoFont, statusFont;
		public ResourcesScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			textFont = new Font( game.FontName, 16, FontStyle.Bold );
			infoFont = new Font( game.FontName, 14, FontStyle.Regular );
			statusFont = new Font( game.FontName, 13, FontStyle.Italic );
			buttonFont = textFont;
			widgets = new LauncherWidget[4];
		}

		public override void Init() { Resize(); }
		
		bool failed;
		public override void Tick() {
			if( fetcher == null || failed ) return;
			
			if( !fetcher.Check( SetStatus ) )
				failed = true;
			
			if( fetcher.Done ) {
				if( !fetcher.defaultZipExists ) {
					ResourcePatcher patcher = new ResourcePatcher( fetcher );
					patcher.Run();
				}
				fetcher = null;
				GC.Collect();
				game.TryLoadTexturePack();
				game.SetScreen( new MainScreen( game ) );
			}
		}
		
		public override void Resize() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( clearCol );
				drawer.Clear( backCol, game.Width / 2 - 175, game.Height / 2 - 70, 175 * 2, 70 * 2 );
				RedrawAll();
			}
			Dirty = true;
		}
		
		ResourceFetcher fetcher;
		void DownloadResources( int mouseX, int mouseY ) {
			if( game.Downloader == null )
				game.Downloader = new AsyncDownloader( "null" );
			if( fetcher != null ) return;
			
			fetcher = game.fetcher;
			fetcher.DownloadItems( game.Downloader, SetStatus );
			selectedWidget = null;
			Resize();
		}
		
		Font textFont;
		static FastColour backCol = new FastColour( 120, 85, 151 );
		static readonly string mainText = "Some required resources weren't found" +
			Environment.NewLine + "Okay to download them?";
		static readonly string format = "Download size: {0} megabytes";
		static FastColour clearCol = new FastColour( 12, 12, 12 );
		
		void MakeWidgets() {
			widgetIndex = 0;
			
			float dataSize = game.fetcher.DownloadSize;
			string text = widgets[0] != null ? widgets[0].Text
				: String.Format( format, dataSize.ToString( "F2" ) );
			MakeLabelAt( text, statusFont, Anchor.Centre, Anchor.Centre, 0, 5 );

			// Clear the entire previous widgets state.
			for( int i = 1; i < widgets.Length; i++ ) {
				widgets[i] = null;
				selectedWidget = null;
				lastClicked = null;
			}
			
			if( fetcher == null ) {
				MakeLabelAt( mainText, infoFont, Anchor.Centre, Anchor.Centre, 0, -30 );
				MakeButtonAt( "Yes", 60, 30, textFont, Anchor.Centre,
				             -50, 40, DownloadResources );
				
				MakeButtonAt( "No", 60, 30, textFont, Anchor.Centre,
				             50, 40, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			} else {
				MakeButtonAt( "Cancel", 120, 30, textFont, Anchor.Centre,
				             0, 40, (x, y) => game.SetScreen( new MainScreen( game ) ) );
			}
		}
		
		void SetStatus( string text ) {
			LauncherLabelWidget widget = (LauncherLabelWidget)widgets[0];
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( backCol, widget.X, widget.Y, widget.Width, widget.Height );
				widget.SetDrawData( drawer, text, statusFont, Anchor.Centre, Anchor.Centre, 0, 5 );
				widget.Redraw( drawer );
				Dirty = true;
			}
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			textFont.Dispose();
			infoFont.Dispose();
			statusFont.Dispose();
		}
	}
}
