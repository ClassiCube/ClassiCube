using System;
using System.Drawing;
using ClassicalSharp;
using OpenTK;

namespace Launcher2 {

	public sealed class FastButtonWidget {
		
		public int ButtonWidth, ButtonHeight;
		public int X, Y, Width, Height;
		public NativeWindow Window;
		public Action OnClick;
		public string Text;
		
		static FastColour boxCol = new FastColour( 169, 143, 192 ), shadowCol = new FastColour( 97, 81, 110 );
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                     Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width + 2; Height = height + 2;	// adjust for border size of 2	
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );				
		}
		
		void CalculateOffset( int x, int y, Anchor horAnchor, Anchor verAnchor ) {
			if( horAnchor == Anchor.LeftOrTop ) X = x;
			else if( horAnchor == Anchor.Centre ) X = x + Window.Width / 2 - Width / 2;
			else if( horAnchor == Anchor.BottomOrRight ) X = x + Window.Width - Width;
			
			if( verAnchor == Anchor.LeftOrTop ) Y = y;
			else if( verAnchor == Anchor.Centre ) Y = y + Window.Height / 2 - Height / 2;
			else if( verAnchor == Anchor.BottomOrRight ) Y = y + Window.Height - Height;
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
