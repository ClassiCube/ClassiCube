using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public int ButtonWidth, ButtonHeight;
		public bool Shadow = true;
		public bool Active = false;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {
		}
		
		static FastColour backCol = new FastColour( 97, 81, 110 ), lineCol = new FastColour( 182, 158, 201 );
		static FastColour colActive = new FastColour( 185, 162, 204 ), col = new FastColour( 164, 138, 186 );
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                   Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width + 2; Height = height + 2;	// adjust for border size of 2
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );
		}
		
		const int border = 2;
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			if( !Active )
				text = "&7" + Text;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			int width = ButtonWidth, height = ButtonHeight;
			int xOffset = width - size.Width, yOffset = height - size.Height;
			
			// draw box bounds first
			drawer.Clear( backCol, X + 1, Y, width - 2, border );
			drawer.Clear( backCol, X + 1, Y + height - border, width - 2, border );
			drawer.Clear( backCol, X, Y + 1, border, height - 2 );
			drawer.Clear( backCol, X + width - border, Y + 1, border, height - 2 );
			
			FastColour foreCol = Active ? colActive : col;
			drawer.Clear( foreCol, X + border, Y + border, width - border * 2, height - border * 2 );
			args.SkipPartsCheck = true;
			drawer.DrawText( ref args, X + xOffset / 2, Y + yOffset / 2 );
			if( !Active )
				drawer.Clear( lineCol, X + border + 1, Y + border, width - (border * 2 + 1), border );
		}
	}
}
