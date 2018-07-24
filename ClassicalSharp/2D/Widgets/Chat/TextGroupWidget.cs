// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp.Gui.Widgets {
	public class TextGroupWidget : Widget {
		
		public TextGroupWidget(Game game, int elementsCount, Font font, Font underlineFont) : base(game) {
			ElementsCount = elementsCount;
			this.font = font;
			this.underlineFont = underlineFont;
		}
		
		public TextGroupWidget SetLocation(Anchor horAnchor, Anchor verAnchor, int xOffset, int yOffset) {
			HorizontalAnchor = horAnchor; VerticalAnchor = verAnchor;
			XOffset = xOffset; YOffset = yOffset;
			Reposition();
			return this;
		}
		
		public Texture[] Textures;
		public bool[] PlaceholderHeight;
		internal string[] lines;
		int ElementsCount, defaultHeight;
		readonly Font font, underlineFont;
		
		public override void Init() {
			Textures = new Texture[ElementsCount];
			PlaceholderHeight = new bool[ElementsCount];
			lines = new string[ElementsCount];

			int height = game.Drawer2D.FontHeight(font, true);
			game.Drawer2D.ReducePadding(ref height, Utils.Floor(font.Size), 3);
			defaultHeight = height;
			
			for (int i = 0; i < Textures.Length; i++) {
				Textures[i].Height = (ushort)defaultHeight;
				PlaceholderHeight[i] = true;
			}
			UpdateDimensions();
		}
		
		public void SetUsePlaceHolder(int index, bool placeHolder) {
			PlaceholderHeight[index] = placeHolder;
			if (Textures[index].ID != 0) return;
			
			int newHeight = placeHolder ? defaultHeight : 0;
			Textures[index].Y1 = CalcY(index, newHeight);
			Textures[index].Height = (ushort)newHeight;
		}
		
		public void PushUpAndReplaceLast(string text) {
			int y = Y;
			game.Graphics.DeleteTexture(ref Textures[0]);
			for (int i = 0; i < Textures.Length - 1; i++) {
				Textures[i] = Textures[i + 1];
				lines[i] = lines[i + 1];
				Textures[i].Y1 = y;
				y += Textures[i].Height;
			}
			
			Textures[Textures.Length - 1].ID = 0; // Delete() is called by SetText otherwise.
			SetText(Textures.Length - 1, text);
		}
		
		int CalcY(int index, int newHeight) {
			int y = 0;
			int deltaY = newHeight - Textures[index].Height;
			
			if (VerticalAnchor == Anchor.Min) {
				y = Y;
				for (int i = 0; i < index; i++)
					y += Textures[i].Height;
				for (int i = index + 1; i < Textures.Length; i++)
					Textures[i].Y1 += deltaY;
			} else {
				y = game.Height - YOffset;
				for (int i = index + 1; i < Textures.Length; i++)
					y -= Textures[i].Height;
				
				y -= newHeight;
				for (int i = 0; i < index; i++)
					Textures[i].Y1 -= deltaY;
			}
			return y;
		}
		
		public int GetUsedHeight() {
			int height = 0, i = 0;
			
			for (i = 0; i < Textures.Length; i++) {
				if (Textures[i].IsValid) break;
			}
			for (; i < Textures.Length; i++) {
				height += Textures[i].Height;
			}
			return height;
		}
		
		void UpdateDimensions() {
			Width = 0;
			Height = 0;
			for (int i = 0; i < Textures.Length; i++) {
				Width = Math.Max(Width, Textures[i].Width);
				Height += Textures[i].Height;
			}
			Reposition();
		}
		
		public override void Render(double delta) {
			for (int i = 0; i < Textures.Length; i++) {
				Texture texture = Textures[i];
				if (texture.IsValid) {
					texture.Render(game.Graphics);
				}
			}
		}
		
		public override void Dispose() {
			for (int i = 0; i < Textures.Length; i++) {
				game.Graphics.DeleteTexture(ref Textures[i]);
			}
		}
		
		public override void Reposition() {
			int oldY = Y;
			base.Reposition();
			if (Textures == null) return;
			
			for (int i = 0; i < Textures.Length; i++) {
				Textures[i].X1 = CalcPos(HorizontalAnchor, XOffset, Textures[i].Width, game.Width);
				Textures[i].Y1 += Y - oldY;
			}
		}
		
		public string GetSelected(int mouseX, int mouseY) {
			for (int i = 0; i < Textures.Length; i++) {
				Texture tex = Textures[i];
				if (tex.IsValid && GuiElement.Contains(tex.X, tex.Y, tex.Width, tex.Height, mouseX, mouseY))
					return GetUrl(i, mouseX) ?? lines[i];
			}
			return null;
		}
		
		unsafe string GetUrl(int index, int mouseX) {
			Texture tex = Textures[index]; mouseX -= tex.X1;
			DrawTextArgs args = default(DrawTextArgs);
			string text = lines[index];
			
			char* chars = stackalloc char[lines.Length * 96];
			Portion* portions = stackalloc Portion[(96 / 7) * 2];
			int portionsCount = Reduce(chars, index, portions);
			
			for (int i = 0, x = 0; i < portionsCount; i++) {
				Portion bit = portions[i];
				args.Text = text.Substring(bit.Beg, bit.Len);
				args.Font = (bit.ReducedLen & 0x8000) == 0 ? font : underlineFont;
				
				int width = game.Drawer2D.MeasureSize(ref args).Width;
				if (args.Font != font && mouseX >= x && mouseX < x + width) {
					return new string(chars, bit.ReducedBeg, bit.ReducedLen);
				}
				x += width;
			}
			return null;
		}
		
		public void SetText(int index, string text) {
			game.Graphics.DeleteTexture(ref Textures[index]);
			Texture tex;
			
			if (IDrawer2D.EmptyText(text)) {
				lines[index] = null;
				tex = default(Texture); 
				tex.Height = (ushort)(PlaceholderHeight[index] ? defaultHeight : 0);
			} else {
				lines[index] = text;
				DrawTextArgs args = new DrawTextArgs(text, font, true);

				if (game.ClassicMode || !MightHaveUrls()) {
					tex = game.Drawer2D.MakeTextTexture(ref args, 0, 0);
				} else {
					tex = DrawAdvanced(ref args, index, text);
				}
				game.Drawer2D.ReducePadding(ref tex, Utils.Floor(args.Font.Size), 3);
			}
			
			tex.X1 = CalcPos(HorizontalAnchor, XOffset, tex.Width, game.Width);
			tex.Y1 = CalcY(index, tex.Height);
			Textures[index] = tex;
			UpdateDimensions();
		}
		
		bool MightHaveUrls() {
			for (int i = 0; i < lines.Length; i++) {
				if (lines[i] == null) continue;
				if (lines[i].IndexOf('/') >= 0) return true;
			}
			return false;
		}
		
		unsafe Texture DrawAdvanced(ref DrawTextArgs args, int index, string text) {
			char* chars = stackalloc char[lines.Length * 96];
			Portion* portions = stackalloc Portion[(96 / 7) * 2];
			int portionsCount = Reduce(chars, index, portions);
			
			Size total = Size.Empty;
			Size* partSizes = stackalloc Size[portionsCount];
			
			for (int i = 0; i < portionsCount; i++) {
				Portion bit = portions[i];
				args.Text = text.Substring(bit.Beg, bit.Len);
				args.Font = (bit.ReducedLen & 0x8000) == 0 ? font : underlineFont;
				
				partSizes[i] = game.Drawer2D.MeasureSize(ref args);
				total.Height = Math.Max(partSizes[i].Height, total.Height);
				total.Width += partSizes[i].Width;
			}
			
			using (IDrawer2D drawer = game.Drawer2D)
				using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(total))
			{
				drawer.SetBitmap(bmp);
				int x = 0;
				
				for (int i = 0; i < portionsCount; i++) {
					Portion bit = portions[i];
					args.Text = text.Substring(bit.Beg, bit.Len);
					args.Font = (bit.ReducedLen & 0x8000) == 0 ? font : underlineFont;
					
					drawer.DrawText(ref args, x, 0);
					x += partSizes[i].Width;
				}
				return drawer.Make2DTexture(bmp, total, 0, 0);
			}
		}
		
		unsafe static int NextUrl(char* chars, int i, int len) {
			for (; i < len; i++) {
				if (chars[i] != 'h') continue;
				int left = len - i;
				if (left < 7) return -1; // "http://".Length
				
				// Starts with "http" ?
				if (chars[i + 1] != 't' || chars[i + 2] != 't' || chars[i + 3] != 'p') continue;
				left -= 4; i += 4;
				
				// And then with "s://" or "://" ?
				if (chars[i] == 's') { left--; i++; }
				if (left >= 3 && chars[i] == ':' && chars[i + 1] == '/' && chars[i + 2] == '/') return i;
			}
			return -1;
		}
		
		unsafe static int ReduceLine(char* chars, ushort* mappings,
		                             int count, int offset, string line) {
			bool lineStart = true;
			for (int i = 0, last = line.Length - 1; i < line.Length;) {
				char cur = line[i];
				
				// Trim colour codes and "> " line continues
				if (cur == '&' && i < last && IDrawer2D.ValidColCode(line[i + 1])) {
					i += 2; continue;
				}
				if (cur == '>' && i < last && lineStart && line[i + 1] == ' ') {
					lineStart = false; i += 2; continue;
				}
				
				lineStart = false;
				chars[count] = cur;
				mappings[count] = (ushort)(offset + i);
				i++; count++;
			}
			return count;
		}
		
		unsafe static void Output(Portion bit, ushort* mappings, int count,
		                          int target, string[] lines, ref Portion* portions) {
			int lineBeg = 0, lineLen = 0, lineEnd = 0, total = 0;
			for (int i = 0; i < lines.Length; i++) {
				string line = lines[i];
				if (line == null) continue;
				
				if (i == target) {
					lineBeg = total; lineLen = line.Length;
					lineEnd = lineBeg + lineEnd;
				}
				total += line.Length;
			}
			
			bit.Beg = mappings[bit.ReducedBeg];
			if (bit.Beg >= lineEnd) return;
			
			// Map back this reduced portion to original lines
			int end = bit.ReducedBeg + (bit.ReducedLen & 0x7FFF);
			end     = end < count ? mappings[end] : total;
			bit.Len = end - bit.Beg;
			
			// Adjust this reduced portion to lie inside line we care about
			if (bit.Beg >= lineBeg) {
			} else if (bit.Beg + bit.Len > lineBeg) {
				// Clamp start of portion to lie in this line
				int underBy = (bit.Beg + bit.Len) - lineBeg;
				bit.Beg += underBy; bit.Len -= underBy;
			} else {
				return;
			}
			
			// Clamp length of portion to lie in this line
			int overBy = (bit.Beg + bit.Len) - lineEnd;
			if (overBy > 0) bit.Len -= overBy;
			
			bit.Beg -= lineBeg;
			if (bit.Len == 0) return;
			*portions = bit; portions++;
		}
		
		struct Portion { public int ReducedBeg, ReducedLen, Beg, Len; }
		unsafe int Reduce(char* chars, int target, Portion* portions) {
			ushort* mappings = stackalloc ushort[lines.Length * 96];
			Portion* portionsStart = portions;
			int count = 0;

			for (int i = 0, offset = 0; i < lines.Length; i++) {
				string line = lines[i];
				if (line == null) continue;
				
				count = ReduceLine(chars, mappings, count, offset, line);
				offset += line.Length;
			}
			
			// Now find http:// and https:// urls
			int urlEnd = 0;
			for (;;) {
				int nextUrlStart = NextUrl(chars, urlEnd, count);
				if (nextUrlStart == -1) nextUrlStart = count;
				
				// add normal portion between urls
				Portion bit = default(Portion); bit.ReducedBeg = urlEnd;
				bit.ReducedLen = nextUrlStart - urlEnd;
				Output(bit, mappings, count, target, lines, ref portions);
				
				if (nextUrlStart == count) break;
				// work out how long this url is
				urlEnd = nextUrlStart;
				for (; urlEnd < count && chars[urlEnd] != ' '; urlEnd++) { }
				
				// add this url portion
				bit = default(Portion); bit.ReducedBeg = nextUrlStart;
				bit.ReducedLen = (urlEnd - nextUrlStart) | 0x8000;
				Output(bit, mappings, count, target, lines, ref portions);
			}
			return (int)(portions - portionsStart);
		}
	}
}