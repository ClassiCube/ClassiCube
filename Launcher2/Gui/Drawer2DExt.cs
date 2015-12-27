using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	public static class Drawer2DExt {
		
		public unsafe static void CopyScaledPixels( FastBitmap src, FastBitmap dst,
		                                           Rectangle srcRect, Rectangle dstRect ) {
			CopyScaledPixels( src, dst, new Size( dstRect.Width, dstRect.Height ), srcRect, dstRect, 255 );
		}
		public unsafe static void CopyScaledPixels( FastBitmap src, FastBitmap dst, Size scale,
		                                           Rectangle srcRect, Rectangle dstRect, byte rgbScale ) {
			int srcWidth = srcRect.Width, dstWidth = dstRect.Width;
			int srcHeight = srcRect.Height, dstHeight = dstRect.Height;
			int srcX = srcRect.X, dstX = dstRect.X;
			int srcY = srcRect.Y, dstY = dstRect.Y;
			int scaleWidth = scale.Width, scaleHeight = scale.Height;
			
			if( dstX >= dst.Width || dstY >= dst.Height ) return;
			dstWidth = Math.Min( dstX + dstWidth, dst.Width ) - dstX;
			dstHeight = Math.Min( dstY + dstHeight, dst.Height ) - dstY;
			
			for( int yy = 0; yy < dstHeight; yy++ ) {
				int scaledY = yy * srcHeight / scaleHeight;
				int* srcRow = src.GetRowPtr( srcY + scaledY );
				int* dstRow = dst.GetRowPtr( dstY + yy );
				
				for( int xx = 0; xx < dstWidth; xx++ ) {
					int scaledX = xx * srcWidth / scaleWidth;
					int pixel = srcRow[srcX + scaledX];
					
					int col = pixel & ~0xFFFFFF; // keep a, but clear rgb
					col |= ((pixel & 0xFF) * rgbScale / 255);
					col |= (((pixel >> 8) & 0xFF) * rgbScale / 255) << 8;
					col |= (((pixel >> 16) & 0xFF) * rgbScale / 255) << 16;
					dstRow[dstX + xx] = col;
				}
			}
		}
		
		public unsafe static void DrawNoise( FastBitmap dst, Rectangle dstRect, FastColour col ) {
			int dstWidth = dstRect.Width, dstHeight = dstRect.Height;
			int dstX = dstRect.X, dstY = dstRect.Y;
			
			if( dstX >= dst.Width || dstY >= dst.Height ) return;
			dstWidth = Math.Min( dstX + dstWidth, dst.Width ) - dstX;
			dstHeight = Math.Min( dstY + dstHeight, dst.Height ) - dstY;
			const int alpha = 255 << 24;
			
			for( int yy = 0; yy < dstHeight; yy++ ) {
				int* row = dst.GetRowPtr( dstY + yy );
				for( int xx = 0; xx < dstWidth; xx++ ) {
					float n = Noise( dstX + xx, dstY + yy );
					int r = col.R + (int)(n * 6); Utils.Clamp( ref r, 0, 255 );
					int g = col.G + (int)(n * 6); Utils.Clamp( ref g, 0, 255 );
					int b = col.B + (int)(n * 6); Utils.Clamp( ref b, 0, 255 );
					row[dstX + xx] = alpha | (r << 16) | (g << 8) | b;
				}
			}
		}
		
		static float Noise( int x, int y ) {
			int n = x + y * 57;
			n = (n << 13) ^ n;
			return 1f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824f;
		}
		
		static float SmoothNoise2D( int x, int y ) {
			float corners = (Noise( x - 1, y - 1 ) + Noise( x + 1, y -  1) +
			                 Noise( x - 1, y + 1 ) + Noise( x + 1, y + 1 )) / 16;
			float sides = (Noise( x - 1, y ) + Noise( x + 1, y ) + Noise( x, y - 1 ) + Noise( x, y + 1 )) / 8;
			return corners + sides + Noise(x, y) / 4;
		}
	}
}