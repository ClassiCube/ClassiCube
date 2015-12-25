using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public sealed class LauncherButtonWidget : LauncherWidget {
		
		public int ButtonWidth;
		public bool Shadow = true;
		public bool Active = false;
		public const int ButtonHeight = 42;
		
		public LauncherButtonWidget( LauncherWindow window ) : base( window ) {
		}
		
		static FastColour boxCol = new FastColour( 132, 111, 149 ), shadowCol = new FastColour( 68, 57, 77 ),
		boxColActive = new FastColour( 169, 143, 192 ), shadowColActive = new FastColour( 97, 81, 110 );
		public void DrawAt( IDrawer2D drawer, string text, Font font, Anchor horAnchor,
		                   Anchor verAnchor, int width, int x, int y ) {
			ButtonWidth = width;
			Width = width;
			Height = ButtonHeight;
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			if( !Active )
				text = "&7" + Text;
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			int xOffset = ButtonWidth - size.Width, yOffset = ButtonHeight - size.Height;
			
			using( FastBitmap dst = new FastBitmap( Window.Framebuffer, true ) )
				DrawRaw( Window.fastButtonsBmp, dst );
			args.SkipPartsCheck = true;
			drawer.DrawText( ref args, X + xOffset / 2, Y + yOffset / 2 );
		}
		
		void DrawRaw( FastBitmap src, FastBitmap dst ) {
			Drawer2DExt.CopyScaledPixels( src, dst,
			                             new Rectangle( 0, 0, 16, 42 ),
			                             new Rectangle( X, Y, 16, Height ) );
			Drawer2DExt.CopyScaledPixels( src, dst,
			                             new Rectangle( 16, 0, 32, 42 ),
			                             new Rectangle( X + 16, Y, Width - 32, Height ) );
			Drawer2DExt.CopyScaledPixels( src, dst,
			                             new Rectangle( 48, 0, 16, 42 ),
			                             new Rectangle( X + Width - 16, Y, 16, Height ) );
		}
	}
}
