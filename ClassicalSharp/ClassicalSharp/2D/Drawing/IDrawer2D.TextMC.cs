// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {

	/// <summary> Class responsible for performing drawing operations on bitmaps
	/// and for converting bitmaps into graphics api textures. </summary>
	/// <remarks> Uses GDI+ on Windows, uses Cairo on Mono. </remarks>
	unsafe partial class IDrawer2D {
		
		/// <summary> Sets the bitmap that contains the bitmapped font characters as an atlas. </summary>
		public void SetFontBitmap(Bitmap bmp) {
			DisposeFontBitmap();
			
			fontBitmap = bmp;
			boxSize = fontBitmap.Width / 16;
			fontPixels = new FastBitmap(fontBitmap, true, true);
			CalculateTextWidths();
		}
		
		protected void DisposeFontBitmap() {
			if (fontBitmap == null) return;
			fontPixels.Dispose();
			fontBitmap.Dispose();
		}
		
		Bitmap fontBitmap;
		FastBitmap fontPixels;
		int boxSize;
		int[] widths = new int[256];
		
		void CalculateTextWidths() {
			int width = fontPixels.Width, height = fontPixels.Height;
			for (int i = 0; i < widths.Length; i++)
				widths[i] = 0;
			
			// Iterate through each row in the bitmap
			for (int y = 0; y < height; y++) {
				int tileY = (y / boxSize);
				int* row = fontPixels.GetRowPtr(y);

				for (int x = 0; x < width; x += boxSize) {
					int tileX = (x / boxSize);
					// Iterate through each tile
					for (int xx = boxSize - 1; xx >= 0; xx--) {
						int pixel = row[x + xx];
						byte a = (byte)(pixel >> 24);
						if (a == 0) continue;
						
						// Check if this is the pixel furthest to the right
						int index = (tileY << 4) | tileX;
						widths[index] = Math.Max(widths[index], xx + 1);
						break;
					}
				}
			}
			widths[' '] = boxSize / 4;
		}
		
		void DrawCore(FastBitmap dst, ref DrawTextArgs args, int x, int y, bool shadow) {
			PackedCol col = Cols['f'];
			if (shadow)
				col = BlackTextShadows ? PackedCol.Black : PackedCol.Scale(col, 0.25f);
			
			string text = args.Text;
			int point = Utils.Floor(args.Font.Size), count = 0;
			
			byte* coords     = stackalloc byte[256];
			PackedCol* cols  = stackalloc PackedCol[256];
			ushort* dstWidths = stackalloc ushort[256];
			
			for (int i = 0; i < text.Length; i++) {
				char c = text[i];
				if (c == '&' && ValidColCode(text, i + 1)) {
					col = GetCol(text[i + 1]);
					if (shadow)
						col = BlackTextShadows ? PackedCol.Black : PackedCol.Scale(col, 0.25f);
					i++; continue; // Skip over the colour code.
				}
				
				byte cur = Utils.UnicodeToCP437(c);
				coords[count]    = cur;
				cols[count]      = col;
				dstWidths[count] = (ushort)Width(point, cur);
				count++;
			}
			
			int dstHeight = point, startX = x;
			// adjust coords to make drawn text match GDI fonts
			int xPadding = Utils.CeilDiv(point, 8);
			int yPadding = (AdjHeight(dstHeight) - dstHeight) / 2;
			
			for (int yy = 0; yy < dstHeight; yy++) {
				int dstY = y + (yy + yPadding);
				if (dstY >= dst.Height) return;
				
				int fontY = 0 + yy * boxSize / dstHeight;
				int* dstRow = dst.GetRowPtr(dstY);
				
				for (int i = 0; i < count; i++) {
					int srcX = (coords[i] & 0x0F) * boxSize;
					int srcY = (coords[i] >> 4)   * boxSize;
					int* fontRow = fontPixels.GetRowPtr(fontY + srcY);
					
					int srcWidth = widths[coords[i]], dstWidth = dstWidths[i];
					col = cols[i];
					
					for (int xx = 0; xx < dstWidth; xx++) {
						int fontX = srcX + xx * srcWidth / dstWidth;
						int src = fontRow[fontX];
						if ((byte)(src >> 24) == 0) continue;
						
						int dstX = x + xx;
						if (dstX >= dst.Width) break;
						
						int pixel = src & ~0xFFFFFF;
						pixel |= ((src & 0xFF)         * col.B / 255);
						pixel |= (((src >> 8) & 0xFF)  * col.G / 255) << 8;
						pixel |= (((src >> 16) & 0xFF) * col.R / 255) << 16;
						dstRow[dstX] = pixel;
					}
					x += dstWidth + xPadding;
				}
				x = startX;
			}
		}	

		#if !LAUNCHER
		void DrawUnderline(FastBitmap dst, ref DrawTextArgs args, int x, int y, bool shadow) {
			int point = Utils.Floor(args.Font.Size), dstHeight = point, startX = x;
			// adjust coords to make drawn text match GDI fonts
			int xPadding = Utils.CeilDiv(point, 8);
			int yPadding = (AdjHeight(dstHeight) - dstHeight) / 2;
			
			// scale up bottom row of a cell to drawn text font
			int startYY = (8 - 1) * dstHeight / 8;
			for (int yy = startYY; yy < dstHeight; yy++) {
				int dstY = y + (yy + yPadding);
				if (dstY >= dst.Height) return;
				int* dstRow = dst.GetRowPtr(dstY);
				
				PackedCol col = Cols['f'];
				string text = args.Text;
				
				for (int i = 0; i < text.Length; i++) {
					char c = text[i];
					if (c == '&' && ValidColCode(text, i + 1)) {
						col = GetCol(text[i + 1]);
						i++; continue; // Skip over the colour code.
					}
					
					byte cur = Utils.UnicodeToCP437(c);
					int dstWidth = Width(point, cur);
					col = shadow ? PackedCol.Black : col;
					int argb = col.ToArgb();
					
					for (int xx = 0; xx < dstWidth + xPadding; xx++) {
						if (x >= dst.Width) break;
						dstRow[x++] = argb;
					}
				}
				x = startX;
			}
		}
		#endif
		
		protected void DrawBitmapTextImpl(FastBitmap dst, ref DrawTextArgs args, int x, int y) {
			bool ul = args.Font.Style == FontStyle.Underline;
			int offset = Utils.Floor(args.Font.Size) / 8;
			
			if (args.UseShadow) {
				DrawCore(dst, ref args, x + offset, y + offset, true);
				#if !LAUNCHER
				if (ul) DrawUnderline(dst, ref args, x + offset, y + offset, true);
				#endif
			}

			DrawCore(dst, ref args, x, y, false);
			#if !LAUNCHER
			if (ul) DrawUnderline(dst, ref args, x, y, false);
			#endif
		}

		Size MeasureBitmapText(ref DrawTextArgs args) {
			int point = Utils.Floor(args.Font.Size);
			// adjust coords to make drawn text match GDI fonts
			int xPadding = Utils.CeilDiv(point, 8);
			Size total = new Size(0, AdjHeight(point));
			
			for (int i = 0; i < args.Text.Length; i++) {
				char c = args.Text[i];
				if (c == '&' && ValidColCode(args.Text, i + 1)) {
					i++; continue; // Skip over the colour code.
				}
				
				byte cur = Utils.UnicodeToCP437(c);
				total.Width += Width(point, cur) + xPadding;
			}
			
			// TODO: this should be uncommented
			// Remove padding at end
			//if (total.Width > 0) total.Width -= xPadding;
			
			if (args.UseShadow) {
				int offset = point / 8;
				total.Width += offset; total.Height += offset;
			}
			return total;
		}
		
		int Width(int point, byte c) { 
			return Utils.CeilDiv(widths[c] * point, boxSize); 
		}
		static int AdjHeight(int point) { return Utils.CeilDiv(point * 3, 2); }
	}
}
