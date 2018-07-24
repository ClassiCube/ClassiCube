// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	public sealed class TextAtlas : IDisposable {
		
		public Texture tex;
		internal int offset, curX, fontSize;		
		float uScale;
		int[] widths;		
		Game game;
		
		public TextAtlas(Game game, int fontSize) {
			this.game = game;
			this.fontSize = fontSize;
		}
		
		public void Pack(string chars, Font font, string prefix) {
			DrawTextArgs args = new DrawTextArgs("", font, true);
			widths = new int[chars.Length];
			
			using (IDrawer2D drawer = game.Drawer2D) {
				args.Text = prefix;
				Size size = game.Drawer2D.MeasureText(ref args);
				offset = size.Width;
				size.Width += fontSize * chars.Length;
				
				using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(size)) {
					drawer.SetBitmap(bmp);
					drawer.DrawText(ref args, 0, 0);
					
					for (int i = 0; i < chars.Length; i++) {
						args.Text = new String(chars[i], 1);
						widths[i] = game.Drawer2D.MeasureText(ref args).Width;
						drawer.DrawText(ref args, offset + fontSize * i, 0);
					}
					
					tex = drawer.Make2DTexture(bmp, size, 0, 0);
					drawer.ReducePadding(ref tex, Utils.Floor(font.Size), 4);
					
					uScale = 1.0f / bmp.Width;
					tex.U2 = offset * uScale;
					tex.Width = (ushort)offset;
				}
			}
		}
		
		public void Dispose() { game.Graphics.DeleteTexture(ref tex); }
		
		
		public void Add(int charIndex, VertexP3fT2fC4b[] vertices, ref int index) {
			int width = widths[charIndex];
			Texture part = tex;
			part.X1 = curX; part.Width = (ushort)width;
			part.U1 = (offset + charIndex * fontSize) * uScale;
			part.U2 = part.U1 + width * uScale;
			
			curX += width;
			IGraphicsApi.Make2DQuad(ref part, PackedCol.White, 
			                        vertices, ref index);
		}
		
		public void AddInt(int value, VertexP3fT2fC4b[] vertices, ref int index) {
			if (value < 0) {
				Add(10, vertices, ref index); value = -value; // - sign
			}

			char[] digits = StringBuffer.numBuffer;
			int count = StringBuffer.MakeNum((uint)value);
			for (int i = count - 1; i >= 0; i--) {
				Add(digits[i] - '0', vertices, ref index);
			}
		}
	}
}
