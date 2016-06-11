// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {

	public static class Drawer2DExt {
		
		public unsafe static void DrawScaledPixels( FastBitmap src, FastBitmap dst, Size scale,
		                                           Rectangle srcRect, Rectangle dstRect, byte scaleA, byte scaleB ) {
			int srcWidth = srcRect.Width, dstWidth = dstRect.Width;
			int srcHeight = srcRect.Height, dstHeight = dstRect.Height;
			int srcX = srcRect.X, dstX = dstRect.X;
			int srcY = srcRect.Y, dstY = dstRect.Y;
			int scaleWidth = scale.Width, scaleHeight = scale.Height;
			
			for( int yy = 0; yy < dstHeight; yy++ ) {
				int scaledY = (yy + dstY) * srcHeight / scaleHeight;
				int* srcRow = src.GetRowPtr( srcY + (scaledY % srcHeight) );
				int* dstRow = dst.GetRowPtr( dstY + yy );
				byte rgbScale = (byte)Utils.Lerp( scaleA, scaleB, (float)yy / dstHeight );
				
				for( int xx = 0; xx < dstWidth; xx++ ) {
					int scaledX = (xx + dstX) * srcWidth / scaleWidth;
					int pixel = srcRow[srcX + (scaledX % srcWidth)];
					
					int col = pixel & ~0xFFFFFF; // keep a, but clear rgb
					col |= ((pixel & 0xFF) * rgbScale / 255);
					col |= (((pixel >> 8) & 0xFF) * rgbScale / 255) << 8;
					col |= (((pixel >> 16) & 0xFF) * rgbScale / 255) << 16;
					dstRow[dstX + xx] = col;
				}
			}
		}
		
		public unsafe static void DrawTiledPixels( FastBitmap src, FastBitmap dst,
		                                          Rectangle srcRect, Rectangle dstRect ) {
			int srcX = srcRect.X, srcWidth = srcRect.Width, srcHeight = srcRect.Height;
			int dstX, dstY, dstWidth, dstHeight;
			if( !CheckCoords( dst, dstRect, out dstX, out dstY, out dstWidth, out dstHeight ) )
				return;
			
			for( int yy = 0; yy < dstHeight; yy++ ) {
				// srcY is always 0 so we don't need to add
				int* srcRow = src.GetRowPtr( ((yy + dstY) % srcHeight) );
				int* dstRow = dst.GetRowPtr( dstY + yy );
				
				for( int xx = 0; xx < dstWidth; xx++ )
					dstRow[dstX + xx] = srcRow[srcX + ((xx + dstX) % srcWidth)];
			}
		}
		
		public unsafe static void DrawNoise( FastBitmap dst, Rectangle dstRect, FastColour col, int variation ) {
			int dstX, dstY, dstWidth, dstHeight;
			if( !CheckCoords( dst, dstRect, out dstX, out dstY, out dstWidth, out dstHeight ) )
				return;
			const int alpha = 255 << 24;
			for( int yy = 0; yy < dstHeight; yy++ ) {
				int* row = dst.GetRowPtr( dstY + yy );
				for( int xx = 0; xx < dstWidth; xx++ ) {
					int n = (dstX + xx) + (dstY + yy) * 57;
					n = (n << 13) ^ n;
					float noise = 1f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824f;
					
					int r = col.R + (int)(noise * variation);
					r = r < 0 ? 0 : (r > 255 ? 255 : r);
					int g = col.G + (int)(noise * variation);
					g = g < 0 ? 0 : (g > 255 ? 255 : g);
					int b = col.B + (int)(noise * variation);
					b = b < 0 ? 0 : (b > 255 ? 255 : b);
					row[dstX + xx] = alpha | (r << 16) | (g << 8) | b;
				}
			}
		}
		
		public unsafe static void FastClear( FastBitmap dst, Rectangle dstRect, FastColour col ) {
			int dstX, dstY, dstWidth, dstHeight;
			if( !CheckCoords( dst, dstRect, out dstX, out dstY, out dstWidth, out dstHeight ) )
				return;
			int pixel = col.ToArgb();
			
			for( int yy = 0; yy < dstHeight; yy++ ) {
				int* row = dst.GetRowPtr( dstY + yy );
				for( int xx = 0; xx < dstWidth; xx++ )
					row[dstX + xx] = pixel;
			}
		}
		
		static bool CheckCoords( FastBitmap dst, Rectangle dstRect, out int dstX,
		                        out int dstY, out int dstWidth, out int dstHeight ) {
			dstWidth = dstRect.Width; dstHeight = dstRect.Height;
			dstX = dstRect.X; dstY = dstRect.Y;
			if( dstX >= dst.Width || dstY >= dst.Height ) return false;
			
			if( dstX < 0 ) { dstWidth += dstX; dstX = 0; }
			if( dstY < 0 ) { dstHeight += dstY; dstY = 0; }
			
			dstWidth = Math.Min( dstX + dstWidth, dst.Width ) - dstX;
			dstHeight = Math.Min( dstY + dstHeight, dst.Height ) - dstY;
			if( dstWidth < 0 || dstHeight < 0 ) return false;
			return true;
		}
	}
}