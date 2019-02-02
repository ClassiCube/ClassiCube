// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Drawing {
	public unsafe static class Drawer2DExt {
		
		public static void Clear(FastBitmap bmp, Rectangle rect, PackedCol col) {
			int x, y, width, height;
			if (!ClampCoords(bmp, rect, out x, out y, out width, out height))
				return;
			int pixel = col.ToArgb();
			
			for (int yy = 0; yy < height; yy++) {
				int* row = bmp.GetRowPtr(y + yy);
				for (int xx = 0; xx < width; xx++)
					row[x + xx] = pixel;
			}
		}
		
		public static bool ClampCoords(FastBitmap bmp, Rectangle rect, out int x,
		                               out int y, out int width, out int height) {
			width = rect.Width; height = rect.Height;
			x = rect.X; y = rect.Y;
			if (x >= bmp.Width || y >= bmp.Height) return false;
			
			if (x < 0) { width += x; x = 0; }
			if (y < 0) { height += y; y = 0; }
			
			width  = Math.Min(x + width,  bmp.Width)  - x;
			height = Math.Min(y + height, bmp.Height) - y;
			return width > 0 && height > 0;
		}
		
		
		static bool TryDraw(ref DrawTextArgs args, IDrawer2D drawer,
		                    int x, int y, int maxWidth) {
			Size size = drawer.MeasureText(ref args);		
			if (size.Width > maxWidth) return false;
			
			args.SkipPartsCheck = true;
			drawer.DrawText(ref args, x, y);
			return true;
		}
		
		public static void DrawClippedText(ref DrawTextArgs args, IDrawer2D drawer,
		                                   int x, int y, int maxWidth) {
			Size size = drawer.MeasureText(ref args);
			// No clipping needed
			if (size.Width <= maxWidth) {
				args.SkipPartsCheck = true;
				drawer.DrawText(ref args, x, y); return;
			}
			
			string text = args.Text;
			char[] chars = new char[text.Length + 2];
			
			for (int i = 0; i < text.Length; i++) { chars[i] = text[i]; }
			chars[text.Length]     = '.';
			chars[text.Length + 1] = '.';
			
			DrawTextArgs part = args;
			for (int i = text.Length - 1; i > 0; i--) {
				chars[i] = '.';
				if (text[i - 1] == ' ') continue;
				
				part.Text = new string(chars, 0, i + 2);
				if (TryDraw(ref part, drawer, x, y, maxWidth)) return;
				
				// If down to <= 2 chars, try omit trailing ..
				if (i > 2) continue;
				part.Text = new string(chars, 0, i);
				if (TryDraw(ref part, drawer, x, y, maxWidth)) return;
			}
		}
	}
}