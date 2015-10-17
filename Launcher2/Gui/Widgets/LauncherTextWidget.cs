using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherTextWidget : LauncherWidget {
		
		public string Text;
		
		public LauncherTextWidget( LauncherWindow window ) : base( window ) {			
		}

		public void DrawAt( IDrawer2D drawer, string text, Font font, 
		                   Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			Size size = drawer.MeasureSize( text, font, true );
			Width = size.Width; Height = size.Height;
			
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );				
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			DrawTextArgs args = new DrawTextArgs( text, true );
			drawer.DrawText( font, ref args, X, Y );
		}
	}
}
