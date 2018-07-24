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
		const int prefixLen = 7; // "http://".Length
		
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
		
		unsafe string GetUrl(int index, int mX) {
			Texture tex = Textures[index]; mX -= tex.X1;
			DrawTextArgs args = default(DrawTextArgs);
			string text = lines[index];
			if (game.ClassicMode) return null;
			
			char* chars = stackalloc char[lines.Length * 96];
			Portion* portions = stackalloc Portion[(96 / prefixLen) * 2];
			int portionsCount = Reduce(chars, index, portions);
			
			for (int i = 0, x = 0; i < portionsCount; i++) {
				Portion bit = portions[i];
				args.Text = text.Substring(bit.LineBeg, bit.LineLen);
				args.Font = (bit.Len & 0x8000) == 0 ? font : underlineFont;
				
				int width = game.Drawer2D.MeasureSize(ref args).Width;
				if (args.Font != font && mX >= x && mX < x + width) {
					string url = new string(chars, bit.Beg, bit.Len & 0x7FFF);
					// replace multiline bits
					return Utils.StripColours(url).Replace("> ", "");
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
			Portion* portions = stackalloc Portion[(96 / prefixLen) * 2];
			int portionsCount = Reduce(chars, index, portions);
			
			Size total = Size.Empty;
			Size* partSizes = stackalloc Size[portionsCount];
			
			for (int i = 0; i < portionsCount; i++) {
				Portion bit = portions[i];
				args.Text = text.Substring(bit.LineBeg, bit.LineLen);
				args.Font = (bit.Len & 0x8000) == 0 ? font : underlineFont;
				
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
					args.Text = text.Substring(bit.LineBeg, bit.LineLen);
					args.Font = (bit.Len & 0x8000) == 0 ? font : underlineFont;
					
					drawer.DrawText(ref args, x, 0);
					x += partSizes[i].Width;
				}
				return drawer.Make2DTexture(bmp, total, 0, 0);
			}
		}
		
		unsafe static int NextUrl(char* chars, int i, int len) {
			for (; i < len; i++) {
				if (!(chars[i] == 'h' || chars[i] == '&')) continue;
				int left = len - i;
				if (left < prefixLen) return -1; // "http://".Length
				
				// colour codes at start of URL
				int start = i;
				while (left >= 2 && chars[i] == '&') { left -= 2; i += 2; }
				if (left < prefixLen) continue;
				
				// Starts with "http" ?
				if (chars[i] != 'h' || chars[i + 1] != 't' || chars[i + 2] != 't' || chars[i + 3] != 'p') continue;
				left -= 4; i += 4;
				
				// And then with "s://" or "://" ?
				if (chars[i] == 's') { left--; i++; }
				if (left >= 3 && chars[i] == ':' && chars[i + 1] == '/' && chars[i + 2] == '/') return start;
			}
			return -1;
		}
		
		unsafe static void Output(Portion bit, int lineBeg, int lineEnd, ref Portion* portions) {
			if (bit.Beg >= lineEnd || bit.Len == 0) return;
			bit.LineBeg = bit.Beg;
			bit.LineLen = bit.Len & 0x7FFF;
			
			// Adjust this portion to be within this line
			if (bit.Beg >= lineBeg) {
			} else if (bit.Beg + bit.LineLen > lineBeg) {
				// Adjust start of portion to be within this line
				int underBy = lineBeg - bit.Beg;
				bit.LineBeg += underBy; bit.LineLen -= underBy;
			} else { return; }
			
			// Limit length of portion to be within this line
			int overBy = (bit.LineBeg + bit.LineLen) - lineEnd;
			if (overBy > 0) bit.LineLen -= overBy;
			
			bit.LineBeg -= lineBeg;
			if (bit.LineLen == 0) return;
			
			*portions = bit; portions++;
		}
		
		struct Portion { public int Beg, Len, LineBeg, LineLen; }
		unsafe int Reduce(char* chars, int target, Portion* portions) {
			Portion* start = portions;
			int total = 0;
			int[] begs = new int[lines.Length];
			int[] ends = new int[lines.Length];

			for (int i = 0; i < lines.Length; i++) {
				string line = lines[i];
				begs[i] = -1; ends[i] = -1;
				if (line == null) continue;	
				
				begs[i] = total;
				for (int j = 0; j < line.Length; j++) { chars[total + j] = line[j]; }
				total += line.Length; ends[i] = total;
			}
			
			// Now find http:// and https:// urls
			int urlEnd = 0;
			for (;;) {
				int nextUrlStart = NextUrl(chars, urlEnd, total);
				if (nextUrlStart == -1) nextUrlStart = total;
				
				// add normal portion between urls
				Portion bit = default(Portion); bit.Beg = urlEnd;
				bit.Len = nextUrlStart - urlEnd;
				Output(bit, begs[target], ends[target], ref portions);
				if (nextUrlStart == total) break;
				
				// work out how long this url is
				urlEnd = nextUrlStart;
				for (; urlEnd < total && chars[urlEnd] != ' '; urlEnd++) {
					if (chars[urlEnd] != '>') { urlEnd++; continue; }
					int left = total - urlEnd;
					
					// Skip "> "
					while (left >= 2 && chars[urlEnd + 1] == '&') { left -= 2; urlEnd += 2; }
					if (left > 0 && chars[urlEnd + 1] == ' ') urlEnd++;
				}
				
				// add this url portion
				bit = default(Portion);
				bit.Beg = nextUrlStart;
				bit.Len = (urlEnd - nextUrlStart) | 0x8000;
				Output(bit, begs[target], ends[target], ref portions);
			}
			return (int)(portions - start);
		}
	}
}