// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Drawing {
	public unsafe static class BitmapDrawer {
		
		public static void DrawIndexed(byte[] indices, FastColour[] palette,
		                               int size, int x, int y, FastBitmap dst) {
			int* argb = stackalloc int[palette.Length];
			for (int i = 0; i < palette.Length; i++)
				argb[i] = palette[i].ToArgb();

			for (int i = 0, yy = 0; yy < size; yy++) {
				if ((y + yy) < 0) { i += size; continue; }
				if ((y + yy) >= dst.Height) break;
				int* row = dst.GetRowPtr(y + yy);
				
				for (int xx = 0; xx < size; xx++) {
					int col = argb[indices[i]]; i++;
					if (col == 0) continue; // transparent pixel
					if ((x + xx) < 0 || (x + xx) >= dst.Width) continue;
					row[x + xx] = col;
				}
			}
		}
		
		public static void DrawScaled(FastBitmap src, FastBitmap dst, Size scale,
		                              Rectangle srcRect, Rectangle dstRect, byte scaleA, byte scaleB) {
			int srcWidth = srcRect.Width, dstWidth = dstRect.Width;
			int srcHeight = srcRect.Height, dstHeight = dstRect.Height;
			int srcX = srcRect.X, dstX = dstRect.X;
			int srcY = srcRect.Y, dstY = dstRect.Y;
			int scaleWidth = scale.Width, scaleHeight = scale.Height;
			
			for (int yy = 0; yy < dstHeight; yy++) {
				int scaledY = (yy + dstY) * srcHeight / scaleHeight;
				int* srcRow = src.GetRowPtr(srcY + (scaledY % srcHeight));
				int* dstRow = dst.GetRowPtr(dstY + yy);
				byte rgbScale = (byte)Utils.Lerp(scaleA, scaleB, (float)yy / dstHeight);
				
				for (int xx = 0; xx < dstWidth; xx++) {
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
		
		public static void DrawTiled(FastBitmap src, FastBitmap dst,
		                             Rectangle srcRect, Rectangle dstRect) {
			int srcX = srcRect.X, srcWidth = srcRect.Width, srcHeight = srcRect.Height;
			int x, y, width, height;
			if (!Drawer2DExt.ClampCoords(dst, dstRect, out x, out y, out width, out height))
				return;
			
			for (int yy = 0; yy < height; yy++) {
				// srcY is always 0 so we don't need to add
				int* srcRow = src.GetRowPtr(((yy + y) % srcHeight));
				int* dstRow = dst.GetRowPtr(y + yy);
				
				for (int xx = 0; xx < width; xx++)
					dstRow[x + xx] = srcRow[srcX + ((xx + x) % srcWidth)];
			}
		}
		
		public static void Draw(FastBitmap src, FastBitmap dst, Rectangle dstRect) {
			int x, y, width, height;
			if (!Drawer2DExt.ClampCoords(dst, dstRect, out x, out y, out width, out height)) return;
			
			for (int yy = 0; yy < height; yy++) {
				int* srcRow = src.GetRowPtr(yy);
				int* dstRow = dst.GetRowPtr(y + yy);
				
				for (int xx = 0; xx < width; xx++)
					dstRow[x + xx] = srcRow[xx];
			}
		}
	}
}