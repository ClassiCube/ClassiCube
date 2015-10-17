using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public int ButtonWidth, ButtonHeight;
		public string Text;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {			
		}
		
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                     Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width + 2; Height = height + 2;	// adjust for border size of 2	
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );				
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			Size size = drawer.MeasureSize( text, font, true );
			int width = ButtonWidth, height = ButtonHeight;
			int xOffset = width - size.Width, yOffset = height - size.Height;
			
			drawer.DrawRoundedRect( shadowCol, 3, X + IDrawer2D.Offset, Y + IDrawer2D.Offset,
			                       width, height );
			drawer.DrawRoundedRect( boxCol, 3, X, Y, width, height );
			
			DrawTextArgs args = new DrawTextArgs( text, true );
			args.SkipPartsCheck = true;
			drawer.DrawText( font, ref args,
			                X + 1 + xOffset / 2, Y + 1 + yOffset / 2 );
		}
	}
}
