// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {
	public unsafe static class Gradient {
		
		public static void Noise( FastBitmap bmp, Rectangle rect,
		                         FastColour col, int variation ) {
			int x, y, width, height;
			if( !Drawer2DExt.ClampCoords( bmp, rect, out x, out y,
			                             out width, out height ) ) return;
			
			const int alpha = 255 << 24;
			for( int yy = 0; yy < height; yy++ ) {
				int* row = bmp.GetRowPtr( y + yy );
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
		
		public static void Vertical( FastBitmap bmp, Rectangle rect,
		                            FastColour a, FastColour b ) {
			int x, y, width, height;
			if( !Drawer2DExt.ClampCoords( bmp, rect, out x, out y,
			                             out width, out height ) ) return;
			FastColour c = a;
			
			for( int yy = 0; yy < height; yy++ ) {
				int* row = bmp.GetRowPtr( y + yy );
				float t = (float)yy / (height - 1); // so last row has b as its colour
				
				c.R = (byte)Utils.Lerp( a.R, b.R, t );
				c.G = (byte)Utils.Lerp( a.G, b.G, t );
				c.B = (byte)Utils.Lerp( a.B, b.B, t );
				int pixel = c.ToArgb();
				
				for( int xx = 0; xx < width; xx++ )
					row[x + xx] = pixel;
			}
		}
		
		public static void Blend( FastBitmap bmp, Rectangle rect,
		                         FastColour col, int blend ) {
			int x, y, width, height;
			if( !Drawer2DExt.ClampCoords( bmp, rect, out x, out y,
			                             out width, out height ) ) return;
			
			// Pre compute the alpha blended source colour
			col.R = (byte)(col.R * blend / 255);
			col.G = (byte)(col.G * blend / 255);
			col.B = (byte)(col.B * blend / 255);
			blend = 255 - blend; // inverse for existing pixels
			int t = 0;
			
			for( int yy = 0; yy < height; yy++ ) {
				int* row = bmp.GetRowPtr( y + yy );
				for( int xx = 0; xx < width; xx++ ) {
					int cur = row[x + xx], pixel = 0;
					
					// Blend B
					t = col.B + ((cur & 0xFF) * blend) / 255;
					t = t < 0 ? 0 : t; t = t > 255 ? 255 : t;
					pixel |= t;
					// Blend G
					t = col.G + (((cur >> 8) & 0xFF) * blend) / 255;
					t = t < 0 ? 0 : t; t = t > 255 ? 255 : t;
					pixel |= (t << 8);
					// Blend R
					t = col.R + (((cur >> 16) & 0xFF) * blend) / 255;
					t = t < 0 ? 0 : t; t = t > 255 ? 255 : t;
					pixel |= (t << 16);
					
					// Output pixel
					const int alphaMask = 255 << 24;
					pixel |= alphaMask;
					row[x + xx] = pixel;
				}
			}
		}
	}
}