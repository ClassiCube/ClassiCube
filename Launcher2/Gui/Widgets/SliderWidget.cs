// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;
using Launcher.Drawing;

namespace Launcher.Gui.Widgets {
	/// <summary> Represents a slider bar that may or may not be modifiable by the user. </summary>
	public sealed class SliderWidget : Widget {
		
		public int Progress;
		public FastColour ProgressColour;
		
		public SliderWidget( LauncherWindow window, int width, int height ) : base( window ) {
			Width = width; Height = height;
		}
		
		public override void Redraw( IDrawer2D drawer ) {
			if( Window.Minimised || !Visible ) return;
			using( FastBitmap bmp = Window.LockBits() ) {
				Rectangle r = new Rectangle( X, Y, Width, Height );
				DrawBoxBounds( bmp, r );
				DrawBox( bmp, r );
				
				r.Width = (int)(Width * Progress / 100);
				Drawer2DExt.Clear( bmp, r, ProgressColour );
			}
		}
		
		void DrawBoxBounds( FastBitmap bmp, Rectangle r ) {
			const int border = 1;
			int y1 = r.Y - border, y2 = y1 + Height + border;
			
			r.X -= border;
			r.Height = border; r.Width += border * 2;
			r.Y = y1;
			Drawer2DExt.Clear( bmp, r, boundsTop );
			r.Y = y2;
			Drawer2DExt.Clear( bmp, r, boundsBottom );
			
			r.Y = y1;
			r.Width = border; r.Height = y2 - y1;
			Gradient.Vertical( bmp, r, boundsTop, boundsBottom );
			r.X += Width + border;
			Gradient.Vertical( bmp, r, boundsTop, boundsBottom );
		}
		
		void DrawBox( FastBitmap bmp, Rectangle r ) {
			r.Height /= 2;
			Gradient.Vertical( bmp, r, progTop, progBottom );
			r.Y += Height / 2;
			Gradient.Vertical( bmp, r, progBottom, progTop );
		}
		
		static FastColour progTop = new FastColour( 220, 204, 233 );
		static FastColour progBottom = new FastColour( 207, 181, 216 );
		
		static FastColour boundsTop = new FastColour( 119, 100, 132 );
		static FastColour boundsBottom = new FastColour( 150, 130, 165 );
	}
}
