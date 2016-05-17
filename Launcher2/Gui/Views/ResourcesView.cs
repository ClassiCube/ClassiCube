// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	
	public sealed class ResourcesView : IView {
		
		Font statusFont;
		public ResourcesView( LauncherWindow game ) : base( game ) {
			widgets = new LauncherWidget[4];
		}

		public override void Init() {
			statusFont = new Font( game.FontName, 13, FontStyle.Italic );
			// TODO: figure out how to fix this.
		}
		
		public override void DrawAll() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( clearCol );
				drawer.Clear( backCol, game.Width / 2 - 190, game.Height / 2 - 70, 190 * 2, 70 * 2 );
			}
			
			RedrawAllButtonBackgrounds();
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
		}

		internal int lastProgress = int.MinValue;
		const string format = "&eDownload size: {0} megabytes";
		internal bool useStatus;
		
		internal void UpdateStatus() {
			widgetIndex = 0;
			if( useStatus ) {
				MakeLabelAt( widgets[0].Text, statusFont, Anchor.Centre, Anchor.Centre, 0, -10 );
			} else {
				float dataSize = game.fetcher.DownloadSize;
				string text = String.Format( format, dataSize.ToString( "F2" ) );
				MakeLabelAt( text, statusFont, Anchor.Centre, Anchor.Centre, 0, 10 );
			}	
		}
		
		internal void RedrawStatus( string text ) {
			LauncherLabelWidget widget = (LauncherLabelWidget)widgets[0];
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( backCol, widget.X, widget.Y, widget.Width, widget.Height );
				widget.SetDrawData( drawer, text, statusFont, Anchor.Centre, Anchor.Centre, 0, -10 );
				widget.Redraw( drawer );
			}
		}
		
		internal void DrawProgressBox( int progress ) {
			progress = (200 * progress) / 100;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.DrawRect( progBack, game.Width / 2 - 100, game.Height / 2 + 10, 200, 4 );
				drawer.DrawRect( progFront, game.Width / 2 - 100, game.Height / 2 + 10, progress, 4 );
			}
		}
		
		public override void Dispose() {
			base.Dispose();
			statusFont.Dispose();
		}
		
		static FastColour backCol = new FastColour( 120, 85, 151 );
		static FastColour clearCol = new FastColour( 12, 12, 12 );		
		static FastColour progBack = new FastColour( 220, 220, 220 );
		static FastColour progFront = new FastColour( 0, 220, 0 );
	}
}
