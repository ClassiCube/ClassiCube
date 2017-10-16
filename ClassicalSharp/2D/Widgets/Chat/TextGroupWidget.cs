// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp.Gui.Widgets {
	public sealed class TextGroupWidget : Widget {
		
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
		LinkData[] linkData;
		int ElementsCount, defaultHeight;
		readonly Font font, underlineFont;
		
		public override void Init() {
			Textures = new Texture[ElementsCount];
			PlaceholderHeight = new bool[ElementsCount];
			lines = new string[ElementsCount];
			linkData = new LinkData[ElementsCount];

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
			if (Textures[index].ID > 0) return;
			
			int newHeight = placeHolder ? defaultHeight : 0;
			Textures[index].Y1 = CalcY(index, newHeight);
			Textures[index].Height = (ushort)newHeight;
		}
		
		public void PushUpAndReplaceLast(string text) {
			int y = Y;
			gfx.DeleteTexture(ref Textures[0]);
			for (int i = 0; i < Textures.Length - 1; i++) {
				Textures[i] = Textures[i + 1];
				lines[i] = lines[i + 1];
				Textures[i].Y1 = y;
				y += Textures[i].Height;
				linkData[i] = linkData[i + 1];
			}
			
			linkData[Textures.Length - 1] = default(LinkData);
			Textures[Textures.Length - 1].ID = 0; // Delete() is called by SetText otherwise.
			SetText(Textures.Length - 1, text);
		}
		
		int CalcY(int index, int newHeight) {
			int y = 0;
			int deltaY = newHeight - Textures[index].Height;
			
			if (VerticalAnchor == Anchor.LeftOrTop) {
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
				if (texture.IsValid)
					texture.Render(gfx);
			}
		}
		
		public override void Dispose() {
			for (int i = 0; i < Textures.Length; i++)
				gfx.DeleteTexture(ref Textures[i]);
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
				if (tex.IsValid && tex.Bounds.Contains(mouseX, mouseY))
					return GetUrl(i, mouseX) ?? lines[i];
			}
			return null;
		}
		
		string GetUrl(int index, int mouseX) {
			Rectangle[] partBounds = linkData[index].bounds;
			if (partBounds == null) return null;
			Texture tex = Textures[index];
			mouseX -= tex.X1;
			
			for (int i = 1; i < partBounds.Length; i += 2) {
				if (mouseX >= partBounds[i].Left && mouseX < partBounds[i].Right)
					return linkData[index].urls[i];
			}
			return null;
		}
		
		
		public void SetText(int index, string text) {
			gfx.DeleteTexture(ref Textures[index]);
			DrawTextArgs args = new DrawTextArgs(text, font, true);
			linkData[index] = default(LinkData);
			LinkFlags prevFlags = index > 0 ? linkData[index - 1].flags : 0;
			
			if (!IDrawer2D.EmptyText(text)) {
				Texture tex = NextToken(text, 0, ref prevFlags) == -1 ? DrawSimple(ref args) :
					DrawAdvanced(ref args, index, text);
				game.Drawer2D.ReducePadding(ref tex, Utils.Floor(args.Font.Size), 3);
				
				tex.X1 = CalcPos(HorizontalAnchor, XOffset, tex.Width, game.Width);
				tex.Y1 = CalcY(index, tex.Height);
				Textures[index] = tex;
				lines[index] = text;
			} else {
				int height = PlaceholderHeight[index] ? defaultHeight : 0;
				int y = CalcY(index, height);
				Textures[index] = new Texture(-1, 0, y, 0, height, 0, 0);
				lines[index] = null;
			}
			UpdateDimensions();
		}
		
		Texture DrawSimple(ref DrawTextArgs args) {
			return game.Drawer2D.MakeTextTexture(ref args, 0, 0);
		}
		
		unsafe Texture DrawAdvanced(ref DrawTextArgs args, int index, string text) {
			LinkData data = Split(index, text);
			Size total = Size.Empty;
			Size* partSizes = stackalloc Size[data.parts.Length];
			linkData[index] = data;
			
			for (int i = 0; i < data.parts.Length; i++) {
				args.Text = data.parts[i];
				args.Font = (i & 1) == 0 ? font : underlineFont;
				partSizes[i] = game.Drawer2D.MeasureSize(ref args);
				total.Height = Math.Max(partSizes[i].Height, total.Height);
				total.Width += partSizes[i].Width;
			}
			
			using (IDrawer2D drawer = game.Drawer2D)
				using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(total))
			{
				drawer.SetBitmap(bmp);
				int x = 0;
				
				for (int i = 0; i < data.parts.Length; i++) {
					args.Text = data.parts[i];
					args.Font = (i & 1) == 0 ? font : underlineFont;
					Size size = partSizes[i];
					
					drawer.DrawText(ref args, x, 0);
					data.bounds[i].X = x;
					data.bounds[i].Width = size.Width;
					x += size.Width;
				}
				return drawer.Make2DTexture(bmp, total, 0, 0);
			}
		}
		
		LinkData Split(int index, string line) {
			int start = 0, lastEnd = 0, count = 0;
			LinkData data = default(LinkData);
			data.parts = new string[GetTokensCount(index, line)];
			data.urls = new string[data.parts.Length];
			data.bounds = new Rectangle[data.parts.Length];
			LinkFlags prevFlags = index > 0 ? linkData[index - 1].flags : 0;
			
			while ((start = NextToken(line, start, ref prevFlags)) >= 0) {
				int nextEnd = line.IndexOf(' ', start);
				if (nextEnd == -1) {
					nextEnd = line.Length;
					data.flags |= LinkFlags.Continue;
				}
				
				data.AddPart(count, GetPart(line, lastEnd, start));     // word bit
				data.AddPart(count + 1, GetPart(line, start, nextEnd)); // url bit
				count += 2;
				
				if ((prevFlags & LinkFlags.Append) != 0) {
					string url = linkData[index - 1].LastUrl + data.urls[count - 1];
					data.urls[count - 1] =  url;
					data.parts[count - 2] = "";
					UpdatePreviousUrls(index - 1, url);
				}
				
				if ((prevFlags & LinkFlags.NewLink) != 0)
					data.flags |= LinkFlags.NewLink;
				start = nextEnd;
				lastEnd = nextEnd;
			}
			
			if (lastEnd < line.Length)
				data.AddPart(count, GetPart(line, lastEnd, line.Length)); // word bit
			return data;
		}
		
		void UpdatePreviousUrls(int i, string url) {
			while (i >= 0 && linkData[i].urls != null && (linkData[i].flags & LinkFlags.Continue) != 0) {
				linkData[i].LastUrl = url;
				if (linkData[i].urls.Length > 2 || (linkData[i].flags & LinkFlags.NewLink) != 0)
					break;
				i--;
			}
		}
		
		string GetPart(string line, int start, int end) {
			string part = line.Substring(start, end - start);
			int lastCol = line.LastIndexOf('&', start, start);
			// We may split up a line into say %e<word><url>
			// url and word both need to have %e at the start.
			
			if (lastCol >= 0 && IDrawer2D.ValidColCode(line,lastCol + 1)) {
				part = "&" + line[lastCol + 1] + part;
			}
			return part;
		}
		
		int NextToken(string line, int start, ref LinkFlags prevFlags) {
			bool isWrapped = start == 0 && line.StartsWith("> ");
			if ((prevFlags & LinkFlags.Continue) != 0 && isWrapped) {
				prevFlags = 0;
				if (!Utils.IsUrlPrefix(Utils.StripColours(line), 2))
					prevFlags |= LinkFlags.Append;
				else
					prevFlags |= LinkFlags.NewLink;
				return 2;
			}
			
			prevFlags = LinkFlags.NewLink;
			int nextHttp = line.IndexOf("http://", start);
			int nextHttps = line.IndexOf("https://", start);
			return nextHttp == -1 ? nextHttps : nextHttp;
		}
		
		int GetTokensCount(int index, string line) {
			int start = 0, lastEnd = 0, count = 0;
			LinkFlags prevFlags = index > 0 ? linkData[index - 1].flags : 0;
			
			while ((start = NextToken(line, start, ref prevFlags)) >= 0) {
				int nextEnd = line.IndexOf(' ', start);
				if (nextEnd == -1)
					nextEnd = line.Length;
				
				start = nextEnd;
				lastEnd = nextEnd;
				count += 2;
			}
			
			if (lastEnd < line.Length) count++;
			return count;
		}
		
		struct LinkData {
			public Rectangle[] bounds;
			public string[] parts, urls;
			public LinkFlags flags;
			
			public void AddPart(int index, string part) {
				parts[index] = part;
				urls[index] = part;
			}
			
			public string LastUrl {
				get { return urls[parts.Length - 1]; }
				set { urls[parts.Length - 1] = value; }
			}
		}
		
		[Flags]
		enum LinkFlags : byte {
			Continue = 2, // "part1" "> part2" type urls
			Append = 4, // used for internally combining "part2" and "part2"
			NewLink = 8, // used to signify that part2 is a separate url from part1
		}
	}
}