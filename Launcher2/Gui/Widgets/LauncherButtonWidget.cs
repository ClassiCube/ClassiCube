using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public bool Shadow = true;
		public bool Active = false;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {
		}
		
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                   Anchor verAnchor, int width, int height, int x, int y ) {
			Width = width; Height = height;
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );
		}
		
		const int border = 2;
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			if( !Active )
				text = "&7" + Text;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			int xOffset = Width - size.Width, yOffset = Height - size.Height;
			
			FastColour backCol = LauncherSkin.ButtonBackCol;
			drawer.Clear( backCol, X + 1, Y, Width - 2, border );
			drawer.Clear( backCol, X + 1, Y + Height - border, Width - 2, border );
			drawer.Clear( backCol, X, Y + 1, border, Height - 2 );
			drawer.Clear( backCol, X + Width - border, Y + 1, border, Height - 2 );
			
			FastColour foreCol = Active ? LauncherSkin.ButtonForeActiveCol : LauncherSkin.ButtonForeCol;
			drawer.Clear( foreCol, X + border, Y + border, Width - border * 2, Height - border * 2 );
			args.SkipPartsCheck = true;
			drawer.DrawText( ref args, X + xOffset / 2, Y + yOffset / 2 );
			if( !Active ) {
				FastColour lineCol = LauncherSkin.ButtonHighlightCol;
				drawer.Clear( lineCol, X + border + 1, Y + border, Width - (border * 2 + 1), border );
			}
		}
	}
}
