// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
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
		protected const int italicSize = 8;
		protected int[] widths = new int[256];
		void CalculateTextWidths() {
			for (int i = 0; i < 256; i++)
				MakeTile(i, (i & 0x0F) * boxSize, (i >> 4) * boxSize);
			widths[(int)' '] = boxSize / 4;
		}
		
		void MakeTile(int i, int tileX, int tileY) {
			// find first column (from right) where there is a solid pixel
			for (int x = boxSize - 1; x >= 0; x--)
				for (int y = 0; y < boxSize; y++)
			{
				int pixel = fontPixels.GetRowPtr(tileY + y)[tileX + x];
				byte a = (byte)(pixel >> 24);
				if (a >= 127) { // found a solid pixel
					widths[i] = x + 1;
					return;
				}
			}
			widths[i] = 0;
		}
		
		protected void DrawBitmapTextImpl(FastBitmap dst, ref DrawTextArgs args, int x, int y) {
			bool underline = args.Font.Style == FontStyle.Underline;
			if (args.UseShadow) {
				int offset = ShadowOffset(args.Font.Size);
				DrawPart(dst, ref args, x + offset, y + offset, true);
				if (underline) DrawUnderline(dst, x + offset, 0, ref args, true);
			}

			DrawPart(dst, ref args, x, y, false);
			if (underline) DrawUnderline(dst, x, -2, ref args, false);
		}
		
		void DrawPart(FastBitmap dst, ref DrawTextArgs args, int x, int y, bool shadowCol) {
			FastColour col = Colours['f'];
			if (shadowCol)
				col = BlackTextShadows ? FastColour.Black : FastColour.Scale(col, 0.25f);
			FastColour lastCol = col;
			
			int xMul = args.Font.Style == FontStyle.Italic ? 1 : 0;
			int runCount = 0, lastY = -1;
			string text = args.Text;
			int point = Utils.Floor(args.Font.Size);
			byte* coordsPtr = stackalloc byte[256];
			
			for (int i = 0; i < text.Length; i++) {
				char c = text[i];
				bool code = c == '&' && i < text.Length - 1;
				if (code && ValidColour(text[i + 1])) {
					col = Colours[text[i + 1]];
					if (shadowCol)
						col = BlackTextShadows ? FastColour.Black : FastColour.Scale(col, 0.25f);
					i++; continue; // Skip over the colour code.
				}
				int coords = ConvertToCP437(c);
				
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
				
				DrawRun(dst, x, y, xMul, runCount, coordsPtr, point, lastCol);
				lastY = coords >> 4; lastCol = col;
				for (int j = 0; j < runCount; j++) {
					x += PaddedWidth(point, widths[coordsPtr[j]]);
				}
				coordsPtr[0] = (byte)coords; runCount = 1;
			}
			DrawRun(dst, x, y, xMul, runCount, coordsPtr, point, lastCol);
		}
		
		void DrawRun(FastBitmap dst, int x, int y, int xMul,
		             int runCount, byte* coords, int point, FastColour col) {
			if (runCount == 0) return;
			int srcY = (coords[0] >> 4) * boxSize;
			int textHeight = AdjTextSize(point), cellHeight = CellSize(textHeight);
			int padding = (cellHeight - textHeight) / 2;
			int startX = x;
			
			ushort* dstWidths = stackalloc ushort[runCount];
			for (int i = 0; i < runCount; i++)
				dstWidths[i] = (ushort)Width(point, widths[coords[i]]);
			
			for (int yy = 0; yy < textHeight; yy++) {
				int fontY = srcY + yy * boxSize / textHeight;
				int* fontRow = fontPixels.GetRowPtr(fontY);
				int dstY = y + (yy + padding);
				if (dstY >= dst.Height) return;
				
				int* dstRow = dst.GetRowPtr(dstY);
				int xOffset = xMul * ((textHeight - 1 - yy) / italicSize);
				for (int i = 0; i < runCount; i++) {
					int srcX = (coords[i] & 0x0F) * boxSize;
					int srcWidth = widths[coords[i]], dstWidth = dstWidths[i];
					
					for (int xx = 0; xx < dstWidth; xx++) {
						int fontX = srcX + xx * srcWidth / dstWidth;
						int src = fontRow[fontX];
						if ((byte)(src >> 24) == 0) continue;
						int dstX = x + xx + xOffset;
						if (dstX >= dst.Width) break;
						
						int pixel = src & ~0xFFFFFF;
						pixel |= ((src & 0xFF) * col.B / 255);
						pixel |= (((src >> 8) & 0xFF) * col.G / 255) << 8;
						pixel |= (((src >> 16) & 0xFF) * col.R / 255) << 16;
						dstRow[dstX] = pixel;
					}
					x += PaddedWidth(point, srcWidth);
				}
				x = startX;
			}
		}
		
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
					bool code = c == '&' && i < text.Length - 1;
					if (code && ValidColour(text[i + 1])) {
						col = Colours[text[i + 1]].ToArgb();
						i++; continue; // Skip over the colour code.
					}
					if (shadowCol) col = FastColour.Black.ToArgb();
					
					int coords = ConvertToCP437(c);
					int width = PaddedWidth(point, widths[coords]);
					for (int xx = 0; xx < width; xx++)
						dstRow[x + xx] = col;
					x += width;
				}
				x = startX;
				col = FastColour.White.ToArgb();
			}
		}
		
		protected Size MeasureBitmappedSizeImpl(ref DrawTextArgs args) {
			if (String.IsNullOrEmpty(args.Text)) return Size.Empty;
			int textHeight = AdjTextSize(Utils.Floor(args.Font.Size));
			Size total = new Size(0, CellSize(textHeight));
			int point = Utils.Floor(args.Font.Size);
			
			for (int i = 0; i < args.Text.Length; i++) {
				char c = args.Text[i];
				bool code = c == '&' && i < args.Text.Length - 1;
				if (code && ValidColour(args.Text[i + 1])) {
					i++; continue; // Skip over the colour code.
				}
				
				int coords = ConvertToCP437(c);
				total.Width += PaddedWidth(point, widths[coords]);
			}
			
			if (args.Font.Style == FontStyle.Italic)
				total.Width += Utils.CeilDiv(total.Height, italicSize);
			if (args.UseShadow) {
				int offset = ShadowOffset(args.Font.Size);
				total.Width += offset; total.Height += offset;
			}
			return total;
		}
		
		protected static int ConvertToCP437(char c) {
			if (c >= ' ' && c <= '~')
				return (int)c;
			
			int cIndex = Utils.ControlCharReplacements.IndexOf(c);
			if (cIndex >= 0) return cIndex;
			int eIndex = Utils.ExtendedCharReplacements.IndexOf(c);
			if (eIndex >= 0) return 127 + eIndex;
			return (int)'?';
		}
		
		protected int ShadowOffset(float fontSize) {
			if (fontSize < 13) return 1;
			if (fontSize < 32) return 2;
			return 3;
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
