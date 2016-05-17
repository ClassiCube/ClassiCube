// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public bool Shadow = true;
		public bool Active = false;
		const int border = 2;
		Size textSize;
		Font font;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {
		}
		
		public void SetDrawData( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                        Anchor verAnchor, int width, int height, int x, int y ) {
			Width = width; Height = height;
			CalculateOffset( x, y, horAnchor, verAnchor );
			this.font = font;

			Text = text;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			textSize = drawer.MeasureSize( ref args );
		}
		
		public override void Redraw( IDrawer2D drawer ) {
			if( Window.Minimised ) return;
			string text = Text;
			if( !Active ) text = "&7" + text;
			int xOffset = Width - textSize.Width, yOffset = Height - textSize.Height;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			
			DrawBorder( drawer );
			if( Window.ClassicBackground ) DrawClassic( drawer );
			else DrawNormal( drawer );
			
			drawer.DrawText( ref args, X + xOffset / 2, Y + yOffset / 2 );
		}
		
		void DrawBorder( IDrawer2D drawer ) {
			FastColour backCol = Window.ClassicBackground ? FastColour.Black : LauncherSkin.ButtonBorderCol;
			drawer.Clear( backCol, X + 1, Y, Width - 2, border );
			drawer.Clear( backCol, X + 1, Y + Height - border, Width - 2, border );
			drawer.Clear( backCol, X, Y + 1, border, Height - 2 );
			drawer.Clear( backCol, X + Width - border, Y + 1, border, Height - 2 );
		}
		
		void DrawNormal( IDrawer2D drawer ) {
			if( Active ) return;
			FastColour lineCol = LauncherSkin.ButtonHighlightCol;
			drawer.Clear( lineCol, X + border + 1, Y + border, Width - (border * 2 + 1), border );
		}
		
		void DrawClassic( IDrawer2D drawer ) {
			FastColour highlightCol = Active ? new FastColour( 189, 198, 255 ) : new FastColour( 168, 168, 168 );
			drawer.Clear( highlightCol, X + border + 1, Y + border, Width - (border * 2 + 1), border );
			drawer.Clear( highlightCol, X + border, Y + border + 1, border, Height - (border * 2 + 1) );
		}
		
		public void RedrawBackground() {
			if( Window.Minimised ) return;
			using( FastBitmap dst = new FastBitmap( Window.Framebuffer, true, false ) )
				RedrawBackground( dst );
		}
		
		public void RedrawBackground( FastBitmap dst ) {
			if( Window.Minimised ) return;
			Rectangle rect = new Rectangle( X + border, Y + border, Width - border * 2, Height - border * 2 );
			if( Window.ClassicBackground ) {
				FastColour foreCol = Active ? new FastColour( 126, 136, 191 ) : new FastColour( 111, 111, 111 );
				Drawer2DExt.DrawNoise( dst, rect, foreCol, 8 );
			} else {
				FastColour foreCol = Active ? LauncherSkin.ButtonForeActiveCol : LauncherSkin.ButtonForeCol;
				Drawer2DExt.FastClear( dst, rect, foreCol );
			}
		}
	}
}
