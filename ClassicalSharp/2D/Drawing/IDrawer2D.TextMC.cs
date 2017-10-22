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
		
		public Bitmap FontBitmap;
		
		/// <summary> Sets the bitmap that contains the bitmapped font characters as an atlas. </summary>
		public void SetFontBitmap(Bitmap bmp) {
			DisposeBitmappedText();
			
			FontBitmap = bmp;
			boxSize = FontBitmap.Width / 16;
			fontPixels = new FastBitmap(FontBitmap, true, true);
			CalculateTextWidths();
		}
		
		protected FastBitmap fontPixels;
		protected int boxSize;
		protected int[] widths = new int[256];
		
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
						if (a < 127) continue;
						
						// Check if this is the pixel furthest to the right
						int index = (tileY << 4) | tileX;
						widths[index] = Math.Max(widths[index], xx + 1);
						break;
					}
				}
			}
			widths[' '] = boxSize / 4;
		}
		
		protected void DrawBitmapTextImpl(FastBitmap dst, ref DrawTextArgs args, int x, int y) {
			bool underline = args.Font.Style == FontStyle.Underline;
			if (args.UseShadow) {
				int offset = ShadowOffset(args.Font.Size);
				DrawPart(dst, ref args, x + offset, y + offset, true);
#if !LAUNCHER
				if (underline) DrawUnderline(dst, x + offset, 0, ref args, true);
#endif
			}

			DrawPart(dst, ref args, x, y, false);
#if !LAUNCHER
			if (underline) DrawUnderline(dst, x, -2, ref args, false);
#endif
		}
		
		void DrawPart(FastBitmap dst, ref DrawTextArgs args, int x, int y, bool shadowCol) {
			FastColour col = Cols['f'];
			if (shadowCol)
				col = BlackTextShadows ? FastColour.Black : FastColour.Scale(col, 0.25f);
			FastColour lastCol = col;
			
			int runCount = 0, lastY = -1;
			string text = args.Text;
			int point = Utils.Floor(args.Font.Size);
			byte* coordsPtr = stackalloc byte[256];
			
			for (int i = 0; i < text.Length; i++) {
				char c = text[i];
				if (c == '&' && ValidColCode(text, i + 1)) {
					col = Cols[text[i + 1]];
					if (shadowCol)
						col = BlackTextShadows ? FastColour.Black : FastColour.Scale(col, 0.25f);
					i++; continue; // Skip over the colour code.
				}
				int coords = Utils.UnicodeToCP437(c);
				
				// First character in the string, begin run counting
				if (lastY == -1) {
					lastY = coords >> 4; lastCol = col;
					coordsPtr[0] = (byte)coords; runCount = 1;
					continue;
				}
				if (lastY == (coords >> 4) && col == lastCol) {
					coordsPtr[runCount] = (byte)coords; runCount++;
					continue;
				}
				
				DrawRun(dst, x, y, runCount, coordsPtr, point, lastCol);
				lastY = coords >> 4; lastCol = col;
				for (int j = 0; j < runCount; j++) {
					x += PaddedWidth(point, widths[coordsPtr[j]]);
				}
				coordsPtr[0] = (byte)coords; runCount = 1;
			}
			DrawRun(dst, x, y, runCount, coordsPtr, point, lastCol);
		}
		
		void DrawRun(FastBitmap dst, int x, int y, int runCount, byte* coords, int point, FastColour col) {
			if (runCount == 0) return;
			int srcY = (coords[0] >> 4) * boxSize;
			int textHeight = AdjTextSize(point), cellHeight = CellSize(textHeight);
			// inlined xPadding so we don't need to call PaddedWidth
			int xPadding = Utils.CeilDiv(point, 8), yPadding = (cellHeight - textHeight) / 2;
			int startX = x;
			
			ushort* dstWidths = stackalloc ushort[runCount];
			for (int i = 0; i < runCount; i++)
				dstWidths[i] = (ushort)Width(point, widths[coords[i]]);
			
			for (int yy = 0; yy < textHeight; yy++) {
				int fontY = srcY + yy * boxSize / textHeight;
				int* fontRow = fontPixels.GetRowPtr(fontY);
				int dstY = y + (yy + yPadding);
				if (dstY >= dst.Height) return;
				
				int* dstRow = dst.GetRowPtr(dstY);
				for (int i = 0; i < runCount; i++) {
					int srcX = (coords[i] & 0x0F) * boxSize;
					int srcWidth = widths[coords[i]], dstWidth = dstWidths[i];
					
					for (int xx = 0; xx < dstWidth; xx++) {
						int fontX = srcX + xx * srcWidth / dstWidth;
						int src = fontRow[fontX];
						if ((byte)(src >> 24) == 0) continue;
						int dstX = x + xx;
						if (dstX >= dst.Width) break;
						
						int pixel = src & ~0xFFFFFF;
						pixel |= ((src & 0xFF) * col.B / 255);
						pixel |= (((src >> 8) & 0xFF) * col.G / 255) << 8;
						pixel |= (((src >> 16) & 0xFF) * col.R / 255) << 16;
						dstRow[dstX] = pixel;
					}
					x += dstWidth + xPadding;
				}
				x = startX;
			}
		}

#if !LAUNCHER		
		void DrawUnderline(FastBitmap dst, int x, int yOffset, ref DrawTextArgs args, bool shadowCol) {
			int point = Utils.Floor(args.Font.Size);
			int padding = CellSize(point) - AdjTextSize(point);
			int height = AdjTextSize(point) + Utils.CeilDiv(padding, 2);
			int offset = ShadowOffset(args.Font.Size);
			
			int col = FastColour.White.ToArgb();
			string text = args.Text;
			if (args.UseShadow) height += offset;
			int startX = x;
			
			for (int yy = height - offset; yy < height; yy++) {
				int* dstRow = dst.GetRowPtr(yy + yOffset);
				
				for (int i = 0; i < text.Length; i++) {
					char c = text[i];
					if (c == '&' && ValidColCode(text, i + 1)) {
						col = Cols[text[i + 1]].ToArgb();
						i++; continue; // Skip over the colour code.
					}
					if (shadowCol) col = FastColour.Black.ToArgb();
					
					int coords = Utils.UnicodeToCP437(c);
					int width = PaddedWidth(point, widths[coords]);
					for (int xx = 0; xx < width; xx++)
						dstRow[x + xx] = col;
					x += width;
				}
				x = startX;
				col = FastColour.White.ToArgb();
			}
		}
#endif

		protected Size MeasureBitmappedSize(ref DrawTextArgs args) {
			if (EmptyText(args.Text)) return Size.Empty;
			int textHeight = AdjTextSize(Utils.Floor(args.Font.Size));
			Size total = new Size(0, CellSize(textHeight));
			int point = Utils.Floor(args.Font.Size);
			
			for (int i = 0; i < args.Text.Length; i++) {
				char c = args.Text[i];
				if (c == '&' && ValidColCode(args.Text, i + 1)) {
					i++; continue; // Skip over the colour code.
				}
				
				int coords = Utils.UnicodeToCP437(c);
				total.Width += PaddedWidth(point, widths[coords]);
			}
			
			if (args.UseShadow) {
				int offset = ShadowOffset(args.Font.Size);
				total.Width += offset; total.Height += offset;
			}
			return total;
		}
		
		protected int ShadowOffset(float fontSize) {
			return (int)(fontSize / 8);
		}

		protected int Width(int point, int value) {
			return Utils.CeilDiv(value * point, boxSize);
		}
		
		protected int PaddedWidth(int point, int value) {
			return Utils.CeilDiv(value * point, boxSize) + Utils.CeilDiv(point, 8);
		}
		
		/// <summary> Rounds the given font size up to the nearest whole
		/// multiple of the size of a character in default.png. </summary>
		protected int AdjTextSize(int point) {
			return point; //Utils.CeilDiv(point, boxSize) * boxSize;
		}
		
		/// <summary> Returns the height of the bounding box that
		/// contains both the given font size in addition to padding. </summary>
		protected static int CellSize(int point) {
			return Utils.CeilDiv(point * 3, 2);
		}
		
		protected void DisposeBitmappedText() {
			if (FontBitmap == null) return;
			fontPixels.Dispose();
			FontBitmap.Dispose();
		}
	}
}
