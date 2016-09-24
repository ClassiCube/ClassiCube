// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Views {
	
	public sealed class ResourcesView : IView {
		
		Font statusFont;
		public ResourcesView( LauncherWindow game ) : base( game ) {
			widgets = new LauncherWidget[5];
		}

		public override void Init() {
			statusFont = new Font( game.FontName, 13, FontStyle.Italic );
			// TODO: figure out how to fix this.
		}
		
		const int boxWidth = 190 * 2, boxHeight = 70 * 2;
		public override void DrawAll() {
			using( FastBitmap bmp = game.LockBits() ) {
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

		internal int lastProgress = int.MinValue, sliderIndex;
		const string format = "&eDownload size: {0} megabytes";
		internal bool useStatus;
		
		internal void SetStatus() {
			widgetIndex = 0;
			if( useStatus ) {
				Makers.Label( this, widgets[0].Text, statusFont )
					.SetLocation( Anchor.Centre, Anchor.Centre, 0, -10 );
			} else {
				float dataSize = game.fetcher.DownloadSize;
				string text = String.Format( format, dataSize.ToString( "F2" ) );
				Makers.Label( this, text, statusFont )
					.SetLocation( Anchor.Centre, Anchor.Centre, 0, 10 );
			}
		}
		
		internal void RedrawStatus( string text ) {
			LauncherLabelWidget widget = (LauncherLabelWidget)widgets[0];
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( backCol, widget.X, widget.Y, widget.Width, widget.Height );
				widget.SetDrawData( drawer, text );
				widget.SetLocation( Anchor.Centre, Anchor.Centre, 0, -10 );
				widget.Redraw( drawer );
			}
		}
		
		internal void DrawProgressBox( int progress ) {
			LauncherSliderWidget slider = (LauncherSliderWidget)widgets[sliderIndex];
			slider.Visible = true;
			slider.Progress = progress;
			slider.Redraw( drawer );
		}
		
		public override void Dispose() {
			base.Dispose();
			statusFont.Dispose();
		}
		
		static FastColour backCol = new FastColour( 120, 85, 151 );
		static FastColour clearCol = new FastColour( 12, 12, 12 );
		
		protected override void MakeWidgets() { }
	}
}
