using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherTextInputWidget : LauncherWidget {
		
		public int ButtonWidth, ButtonHeight;
		public string Text;
		public bool Active;
		public bool Password;
		
		public LauncherTextInputWidget( LauncherWindow window ) : base( window ) {
		}

		public void DrawAt( IDrawer2D drawer, string text, Font font,
		                   Anchor horAnchor, Anchor verAnchor, int width, int height, int x, int y ) {
			ButtonWidth = width; ButtonHeight = height;
			Width = width; Height = height;
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			Text = text;
			if( Password )
				text = new String( '*', text.Length );
			Size size = drawer.MeasureSize( text, font, true );
			Width = Math.Max( ButtonWidth, size.Width + 7 );
			
			FastColour col = Active ? FastColour.White : new FastColour( 160, 160, 160 );
			drawer.DrawRectBounds( col, 2, X, Y, Width, Height );
			drawer.DrawRect( FastColour.Black, X + 2, Y + 2, Width - 4, Height - 4 );
			
			DrawTextArgs args = new DrawTextArgs( text, true );
			args.SkipPartsCheck = true;
			drawer.DrawText( font, ref args, X + 7, Y + 2 );		
		}
		
		public void AddChar( char c, Font font ) {
			if( c >= ' ' && c <= '~' ) {
				Text += c;
				Redraw( Window.Drawer, Text, font );
			}
		}
		
		public void RemoveChar( Font font ) {
			if( Text.Length == 0 ) return;
			
			Text = Text.Substring( 0, Text.Length - 1 );
			Redraw( Window.Drawer, Text, font );
		}
	}
}
