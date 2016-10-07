// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Drawing {
	public unsafe static class Drawer2DExt {
		
		public static void Clear( FastBitmap bmp, Rectangle rect, FastColour col ) {
			int x, y, width, height;
			if( !ClampCoords( bmp, rect, out x, out y, out width, out height ) )
				return;
			int pixel = col.ToArgb();
			
			for( int yy = 0; yy < height; yy++ ) {
				int* row = bmp.GetRowPtr( y + yy );
				for( int xx = 0; xx < width; xx++ )
					row[x + xx] = pixel;
			}
		}
				
		public static bool ClampCoords( FastBitmap bmp, Rectangle rect, out int x,
		                               out int y, out int width, out int height ) {
			width = rect.Width; height = rect.Height;
			x = rect.X; y = rect.Y;
			if( x >= bmp.Width || y >= bmp.Height ) return false;
			
			if( x < 0 ) { width += x; x = 0; }
			if( y < 0 ) { height += y; y = 0; }
			
			width = Math.Min( x + width, bmp.Width ) - x;
			height = Math.Min( y + height, bmp.Height ) - y;
			return width > 0 && height > 0;
		}		
		
		
		public static void DrawClippedText( ref DrawTextArgs args, IDrawer2D drawer,
		                                   int x, int y, int maxWidth ) {
			Size size = drawer.MeasureSize( ref args );
			// No clipping necessary
			if( size.Width <= maxWidth ) { drawer.DrawText( ref args, x, y ); return; }
			DrawTextArgs copy = args;
			copy.SkipPartsCheck = true;
			
			char[] chars = new char[args.Text.Length + 2];
			for( int i = 0; i < args.Text.Length; i++ )
				chars[i] = args.Text[i];
			chars[args.Text.Length] = '.';
			chars[args.Text.Length + 1] = '.';
			
			for( int len = args.Text.Length; len > 0; len-- ) {
				chars[len] = '.';
				if( chars[len - 1] == ' ' ) continue;
				
				copy.Text = new string( chars, 0, len + 2 );
				size = drawer.MeasureSize( ref copy );
				if( size.Width > maxWidth ) continue;
				
				drawer.DrawText( ref copy, x, y ); return;
			}
		}
	}
}