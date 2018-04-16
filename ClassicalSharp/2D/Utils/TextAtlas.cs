// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	public sealed class TextAtlas : IDisposable {
		
		public Texture tex;
		Game game;
		int[] widths;
		internal int offset, curX, totalWidth, fontSize;
		
		public TextAtlas(Game game, int fontSize) {
			this.game = game;
			this.fontSize = fontSize;
		}
		
		public void Pack(string chars, Font font, string prefix) {
			DrawTextArgs args = new DrawTextArgs("", font, true);
			widths = new int[chars.Length];
			
			using (IDrawer2D drawer = game.Drawer2D) {
				args.Text = prefix;
				Size size = game.Drawer2D.MeasureSize(ref args);
				offset = size.Width;
				size.Width += 16 * chars.Length;
				
				using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(size)) {
					drawer.SetBitmap(bmp);
					drawer.DrawText(ref args, 0, 0);
					
					for (int i = 0; i < chars.Length; i++) {
						args.Text = new String(chars[i], 1);
						widths[i] = game.Drawer2D.MeasureSize(ref args).Width;
						drawer.DrawText(ref args, offset + fontSize * i, 0);
					}
					
					tex = drawer.Make2DTexture(bmp, size, 0, 0);
					drawer.ReducePadding(ref tex, Utils.Floor(font.Size), 4);
					
					tex.U2 = (float)offset / bmp.Width;
					tex.Width = (ushort)offset;
					totalWidth = bmp.Width;
				}
			}
		}
		
		public void Dispose() { game.Graphics.DeleteTexture(ref tex); }
		
		
		public void Add(int charIndex, VertexP3fT2fC4b[] vertices, ref int index) {
			int width = widths[charIndex];
			Texture part = tex;
			part.X1 = curX; part.Width = (ushort)width;
			part.U1 = (offset + charIndex * fontSize) / (float)totalWidth;
			part.U2 = part.U1 + width / (float)totalWidth;
			
			curX += width;
			IGraphicsApi.Make2DQuad(ref part, FastColour.WhitePacked, 
			                        vertices, ref index);
		}
		
		public unsafe void AddInt(int value, VertexP3fT2fC4b[] vertices, ref int index) {
			if (value < 0) {
				Add(10, vertices, ref index); value = -value; // - sign
			}

			char[] digits = StringBuffer.numBuffer;
			int count = StringBuffer.MakeNum(value);
			for (int i = count - 1; i >= 0; i--) {
				Add(digits[i] - '0', vertices, ref index);
			}
		}
	}
}
