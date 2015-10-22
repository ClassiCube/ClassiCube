using System;
using System.Drawing;
using ClassicalSharp;
using ClassicalSharp.Network;

namespace Launcher2 {
	
	public sealed class ResourcesScreen : LauncherScreen {
		
		Font infoFont, statusFont;
		public ResourcesScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			textFont = new Font( "Arial", 16, FontStyle.Bold );
			infoFont = new Font( "Arial", 14, FontStyle.Regular );
			statusFont = new Font( "Arial", 13, FontStyle.Italic );
			widgets = new LauncherWidget[4];
		}

		public override void Init() { Resize(); }
		
		public override void Tick() {
			if( fetcher == null ) return;
			
			fetcher.Check( SetStatus );
			if( fetcher.Done ) {
				ResourcePatcher patcher = new ResourcePatcher( fetcher );
				patcher.Run();
				game.SetScreen( new MainScreen( game ) );
				fetcher = null;
			}
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Active = false;
				button.Redraw( drawer, button.Text, textFont );
				Dirty = true;
			}
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			LauncherButtonWidget button = widget as LauncherButtonWidget;
			if( button != null ) {
				button.Active = true;
				button.Redraw( drawer, button.Text, textFont );
				Dirty = true;
			}
		}
		
		ResourceFetcher fetcher;
		void DownloadResources( int mouseX, int mouseY ) {
			if( game.Downloader == null )
				game.Downloader = new AsyncDownloader( "null" );
			if( fetcher != null ) return;
			
			fetcher = new ResourceFetcher( game.Downloader );
			fetcher.DownloadItems( SetStatus );
			selectedWidget = null;
			
			Resize();
		}
		
		Font textFont;
		static FastColour backCol = new FastColour( 120, 85, 151 );
		static readonly string mainText = "Some required resources weren't found" +
			Environment.NewLine + "Okay to download them?";
		static readonly string format = "Estimated size: {0} megabytes";
		static FastColour clearCol = new FastColour( 12, 12, 12 );
		
		void Draw() {
			widgetIndex = 0;
			drawer.Clear( clearCol );
			drawer.DrawRect( backCol, game.Width / 2 - 175,
			                game.Height / 2 - 70, 175 * 2, 70 * 2 );
			
			string text = widgets[0] == null ?
				String.Format( format, ResourceFetcher.EstimateDownloadSize().ToString( "F2" ) )
				: (widgets[0] as LauncherTextWidget).Text;
			MakeTextAt( statusFont, text, 0, 5 );

			if( fetcher == null ) {
				MakeTextAt( infoFont, mainText, 0, -30 );
				MakeButtonAt( "Yes", 60, 30, -50, 40, DownloadResources );
				
				MakeButtonAt( "No", 60, 30, 50, 40,
				             (x, y) => game.SetScreen( new MainScreen( game ) ) );
			} else {
				MakeButtonAt( "Dismiss", 120, 30, 0, 40,
				             (x, y) => game.SetScreen( new MainScreen( game ) ) );
				widgets[2] = null;
				widgets[3] = null;
			}
		}
		
		void SetStatus( string text ) {
			LauncherTextWidget widget = widgets[0] as LauncherTextWidget;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( backCol, widget.X, widget.Y, widget.Width, widget.Height );
				widget.Redraw( drawer, text, statusFont );
				Dirty = true;
			}
		}
		
		void MakeButtonAt( string text, int width,
		                  int height, int x, int y, Action<int, int> onClick ) {
			LauncherButtonWidget widget = new LauncherButtonWidget( game );
			widget.Text = text;
			widget.OnClick = onClick;
			
			widget.Active = false;
			widget.DrawAt( drawer, text, textFont, Anchor.Centre, Anchor.Centre, width, height, x, y );
			widgets[widgetIndex++] = widget;
		}
		
		void MakeTextAt( Font font, string text, int x, int y ) {
			LauncherTextWidget widget = new LauncherTextWidget( game, text );
			widget.DrawAt( drawer, text, font, Anchor.Centre, Anchor.Centre, x, y );
			widgets[widgetIndex++] = widget;
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
