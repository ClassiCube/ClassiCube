using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public bool Shadow = true;
		public bool Active = false;
		const int border = 2;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {
		}
		
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                   Anchor verAnchor, int width, int height, int x, int y ) {
			Width = width; Height = height;
			CalculateOffset( x, y, horAnchor, verAnchor );
			RedrawBackground();
			Redraw( drawer, text, font );
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			Text = text;
			if( !Active )
				text = "&7" + text;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			int xOffset = Width - size.Width, yOffset = Height - size.Height;		
			
			DrawBorder( drawer );
			if( Window.ClassicMode )
				DrawClassic( drawer, ref args, xOffset, yOffset );
			else
				DrawNormal( drawer, ref args, xOffset, yOffset );
			
			args.SkipPartsCheck = true;
			drawer.DrawText( ref args, X + xOffset / 2, Y + yOffset / 2 );
		}
		
		void DrawBorder( IDrawer2D drawer ) {
			FastColour backCol = Window.ClassicMode ? FastColour.Black : LauncherSkin.ButtonBorderCol;
			drawer.Clear( backCol, X + 1, Y, Width - 2, border );
			drawer.Clear( backCol, X + 1, Y + Height - border, Width - 2, border );
			drawer.Clear( backCol, X, Y + 1, border, Height - 2 );
			drawer.Clear( backCol, X + Width - border, Y + 1, border, Height - 2 );
		}
		
		void DrawNormal( IDrawer2D drawer, ref DrawTextArgs args, int xOffset, int yOffset ) {
			if( Active ) return;
			FastColour lineCol = LauncherSkin.ButtonHighlightCol;
			drawer.Clear( lineCol, X + border + 1, Y + border, Width - (border * 2 + 1), border );
		}
		
		void DrawClassic( IDrawer2D drawer, ref DrawTextArgs args, int xOffset, int yOffset ) {
			FastColour highlightCol = Active ? new FastColour( 189, 198, 255 ) : new FastColour( 168, 168, 168 );
			drawer.Clear( highlightCol, X + border + 1, Y + border, Width - (border * 2 + 1), border );
			drawer.Clear( highlightCol, X + border, Y + border + 1, border, Height - (border * 2 + 1) );
		}
		
		public void RedrawBackground() {
			Rectangle rect = new Rectangle( X + border, Y + border, Width - border * 2, Height - border * 2 );
			if( Window.ClassicMode ) {
				FastColour foreCol = Active ? new FastColour( 126, 136, 191 ) : new FastColour( 111, 111, 111 );
				using( FastBitmap dst = new FastBitmap( Window.Framebuffer, true ) )
					Drawer2DExt.DrawNoise( dst, rect, foreCol, 8 );
			} else {
				FastColour foreCol = Active ? LauncherSkin.ButtonForeActiveCol : LauncherSkin.ButtonForeCol;
				using( FastBitmap dst = new FastBitmap( Window.Framebuffer, true ) )
					Drawer2DExt.FastClear( dst, rect, foreCol );
			}
		}
	}
}
