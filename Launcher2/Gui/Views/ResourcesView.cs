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
		
		const int boxWidth = 190 * 2, boxHeight = 70 * 2;
		public override void DrawAll() {
			using( FastBitmap bmp = game.LockBits() ) {
				drawer.SetBitmap( game.Framebuffer );
				Rectangle r = new Rectangle( 0, 0, bmp.Width, bmp.Height );
				Drawer2DExt.Clear( bmp, r, clearCol );
				
				r = new Rectangle( game.Width / 2 - boxWidth / 2, 
				                  game.Height / 2 - boxHeight / 2,
				                  boxWidth, boxHeight );
				Gradient.Noise( bmp, r, backCol, 4 );
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
		
		const int progWidth = 200, progHalf = 5;
		internal void DrawProgressBox( int progress ) {
			progress = (progWidth * progress) / 100;
			using( FastBitmap bmp = game.LockBits() ) {
				Rectangle r = new Rectangle( game.Width / 2 - progWidth / 2,
				                            game.Height / 2 + 10, progWidth, progHalf );
				DrawBoxBounds( bmp, r );
				DrawBox( bmp, r );
				DrawBar( bmp, progress, r );
			}
		}
		
		static void DrawBoxBounds( FastBitmap bmp, Rectangle r ) {
			const int border = 1;
			int y1 = r.Y - border, y2 = y1 + progHalf * 2 + border;	
			
			r.X -= border;			
			r.Height = border; r.Width += border * 2;
			r.Y = y1;
			Drawer2DExt.Clear( bmp, r, boundsTop );
			r.Y = y2;
			Drawer2DExt.Clear( bmp, r, boundsBottom );
			
			r.Y = y1;
			r.Width = border; r.Height = y2 - y1;		
			Gradient.Vertical( bmp, r, boundsTop, boundsBottom );
			r.X += progWidth + border;
			Gradient.Vertical( bmp, r, boundsTop, boundsBottom );
		}
		
		static void DrawBox( FastBitmap bmp, Rectangle r ) {
			Gradient.Vertical( bmp, r, progTop, progBottom );
			r.Y += progHalf;
			Gradient.Vertical( bmp, r, progBottom, progTop );
		}
		
		static void DrawBar( FastBitmap bmp, int progress, Rectangle r ) {
			r.Height = progHalf * 2;
			r.Width = progress;
			Drawer2DExt.Clear( bmp, r, progFront );
		}
		
		public override void Dispose() {
			base.Dispose();
			statusFont.Dispose();
		}
		
		static FastColour backCol = new FastColour( 120, 85, 151 );
		static FastColour clearCol = new FastColour( 12, 12, 12 );
		
		static FastColour progTop = new FastColour( 220, 204, 233 );
		static FastColour progBottom = new FastColour( 207, 181, 216 );
		static FastColour progFront = new FastColour( 0, 220, 0 );
		
		static FastColour boundsTop = new FastColour( 119, 100, 132 );
		static FastColour boundsBottom = new FastColour( 150, 130, 165 );
	}
}
