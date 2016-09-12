// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {
	public unsafe static class Gradient {
		
		public static void Noise( FastBitmap dst, Rectangle dstRect,
		                         FastColour col, int variation ) {
			int x, y, width, height;
			if( !Drawer2DExt.ClampCoords( dst, dstRect, out x, out y,
			                             out width, out height ) ) return;
			
			const int alpha = 255 << 24;
			for( int yy = 0; yy < height; yy++ ) {
				int* row = dst.GetRowPtr( y + yy );
				for( int xx = 0; xx < width; xx++ ) {
					int n = (x + xx) + (y + yy) * 57;
					n = (n << 13) ^ n;
					float noise = 1f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824f;
					
					int r = col.R + (int)(noise * variation);
					r = r < 0 ? 0 : (r > 255 ? 255 : r);
					int g = col.G + (int)(noise * variation);
					g = g < 0 ? 0 : (g > 255 ? 255 : g);
					int b = col.B + (int)(noise * variation);
					b = b < 0 ? 0 : (b > 255 ? 255 : b);
					row[x + xx] = alpha | (r << 16) | (g << 8) | b;
				}
			}
		}
		
		public static void Vertical( FastBitmap dst, Rectangle dstRect,
		                            FastColour a, FastColour b ) {
			int x, y, width, height;
			if( !Drawer2DExt.ClampCoords( dst, dstRect, out x, out y,
			                             out width, out height ) ) return;
			FastColour c = a;
			
			for( int yy = 0; yy < height; yy++ ) {
				int* row = dst.GetRowPtr( y + yy );
				float t = (float)yy / height;
				
				c.R = (byte)Utils.Lerp( a.R, b.R, t );
				c.G = (byte)Utils.Lerp( a.G, b.G, t );
				c.B = (byte)Utils.Lerp( a.B, b.B, t );
				int pixel = c.ToArgb();
				
				for( int xx = 0; xx < width; xx++ )
					row[x + xx] = pixel;
			}
		}
	}
}