using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public int ButtonWidth, ButtonHeight;
		public string Text;
		public bool Shadow = true;
		public bool Active = false;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {
		}
		
		static FastColour boxCol = new FastColour( 119, 100, 135 ), shadowCol = new FastColour( 68, 57, 77 ),
		boxColActive = new FastColour( 169, 143, 192 ), shadowColActive = new FastColour( 97, 81, 110 );
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                   Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width + 2; Height = height + 2;	// adjust for border size of 2
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			if( !Active )
				text = "&7" + Text;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			int width = ButtonWidth, height = ButtonHeight;
			int xOffset = width - size.Width, yOffset = height - size.Height;
			
			if( Shadow )
				drawer.DrawRoundedRect( Active ? shadowColActive : shadowCol,
				                       3, X + IDrawer2D.Offset, Y + IDrawer2D.Offset, width, height );
			drawer.DrawRoundedRect( Active ? boxColActive : boxCol, 
			                       3, X, Y, width, height );
			
			args.SkipPartsCheck = true;
			drawer.DrawText( ref args, X + 1 + xOffset / 2, Y + 1 + yOffset / 2 );
		}
	}
}
