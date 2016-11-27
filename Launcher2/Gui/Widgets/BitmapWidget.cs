// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	/// <summary> Represents an image that cannot be modified by the user. </summary>
	/// <remarks> Uses 4 bit (16 colour) palette. </remarks>
	public sealed class BitmapWidget : Widget {
		
		/// <summary> The raw indices within the palette that make up this image. </summary>
		public byte[] Indices;
		
		/// <summary> The ARGB palette for this image. </summary>
		public FastColour[] Palette;
		
		public BitmapWidget(LauncherWindow window, int size,
		                    byte[] indices, FastColour[] palette) : base(window) {
			Indices = indices;
			Palette = palette;
			Width = size; Height = size;
		}
		
		public unsafe override void Redraw(IDrawer2D drawer) {
			if (Window.Minimised || !Visible) return;
			int* palette = stackalloc int[Palette.Length];
			CalculatePalette(palette);
			
			using (FastBitmap bmp = Window.LockBits()) {
				int i = 0;
				for (int yy = 0; yy < Height; yy++) {
					if ((Y + yy) < 0) continue;
					if ((Y + yy) >= bmp.Height) break;
					int* row = bmp.GetRowPtr(Y + yy);
					
					for (int xx = 0; xx < Width; xx++) {
						int index = Indices[i >> 1]; // each byte has even and odd 4bits
						int selector = 4 * ((i + 1) & 1);
						index = (index >> selector) & 0xF;
						i++;
						
						int col = palette[index];
						if (col == 0) continue; // transparent pixel
						if ((X + xx) < 0 || (X + xx) >= bmp.Width) continue;
						row[X + xx] = col;
					}
				}
			}
		}
		
		unsafe void CalculatePalette(int* palette) {
			for (int i = 0; i < Palette.Length; i++) {
				FastColour col = Palette[i];
				if (!Active) col = FastColour.Scale(col, 0.7f);
				palette[i] = col.ToArgb();
			}
		}
	}
}
