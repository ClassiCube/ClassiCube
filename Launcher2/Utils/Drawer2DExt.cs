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
	}
}
