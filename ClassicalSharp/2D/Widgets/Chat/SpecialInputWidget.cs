// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class SpecialInputWidget : Widget {

		public SpecialInputWidget(Game game, Font font, InputWidget input) : base(game) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.BottomOrRight;
			this.font = font;
			this.input = input;
			Active = false;
		}
		
		public void UpdateColours() {
			elements[0].Contents = GetColourString();
			Redraw();
			SetActive(Active);
		}
		
		public Texture texture;
		readonly Font font;
		InputWidget input;
		Size elementSize;
		
		public void SetActive(bool active) {
			Active = active;
			Height = active ? (int)texture.Height : 0;
		}
		
		public override void Render(double delta) {
			texture.Render(gfx);
		}
		
		public override void Init() {
			X = 5; Y = 5;
			InitData();
			Redraw();
			SetActive(Active);
		}

		public void Redraw() {
			Make(elements[selectedIndex], font);
			Width = texture.Width;
			Height = texture.Height;
		}
		
		unsafe void Make(SpecialInputTab e, Font font) {
			Size* sizes = stackalloc Size[e.Contents.Length / e.CharsPerItem];
			MeasureContentSizes(e, font, sizes);
			Size bodySize = CalculateContentSize(e, sizes, out elementSize);
			int titleWidth = MeasureTitles(font), titleHeight = elements[0].TitleSize.Height;
			Size size = new Size(Math.Max(bodySize.Width, titleWidth), bodySize.Height + titleHeight);
			game.Graphics.DeleteTexture(ref texture);
			
			using (Bitmap bmp = IDrawer2D.CreatePow2Bitmap(size))
				using (IDrawer2D drawer = game.Drawer2D)
			{
				drawer.SetBitmap(bmp);
				DrawTitles(drawer, font);
				drawer.Clear(new FastColour(30, 30, 30, 200), 0, titleHeight,
				             size.Width, bodySize.Height);
				
				DrawContent(drawer, font, e, titleHeight);
				texture = drawer.Make2DTexture(bmp, size, X, Y);
			}
		}
		
		int selectedIndex = 0;
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			mouseX -= X; mouseY -= Y;
			if (IntersectsHeader(mouseX, mouseY)) {
				Redraw();
			} else {
				IntersectsBody(mouseX, mouseY);
			}
			return true;
		}
		
		bool IntersectsHeader(int widgetX, int widgetY) {
			Rectangle bounds = new Rectangle(0, 0, 0, 0);
			for (int i = 0; i < elements.Length; i++) {
				Size size = elements[i].TitleSize;
				bounds.Width = size.Width; bounds.Height = size.Height;
				if (bounds.Contains(widgetX, widgetY)) {
					selectedIndex = i;
					return true;
				}
				bounds.X += size.Width;
			}
			return false;
		}
		
		void IntersectsBody(int widgetX, int widgetY) {
			widgetY -= elements[0].TitleSize.Height;
			widgetX /= elementSize.Width; widgetY /= elementSize.Height;
			SpecialInputTab e = elements[selectedIndex];
			int index = widgetY * e.ItemsPerRow + widgetX;
			if (index * e.CharsPerItem < e.Contents.Length) {
				if (selectedIndex == 0) {
					// TODO: need to insert characters that don't affect caret index, adjust caret colour
					input.Append(e.Contents[index * e.CharsPerItem]);
					input.Append(e.Contents[index * e.CharsPerItem + 1]);
				} else {
					input.Append(e.Contents[index]);
				}
			}
		}

		public override void Dispose() {
			gfx.DeleteTexture(ref texture);
		}
		
		
		struct SpecialInputTab {
			public string Title;
			public Size TitleSize;
			public string Contents;
			public int ItemsPerRow;
			public int CharsPerItem;
			
			public SpecialInputTab(string title, int itemsPerRow, int charsPerItem, string contents) {
				Title = title;
				TitleSize = Size.Empty;
				Contents = contents;
				ItemsPerRow = itemsPerRow;
				CharsPerItem = charsPerItem;
			}
		}
		SpecialInputTab[] elements;
		
		void InitData() {
			elements = new SpecialInputTab[] {
				new SpecialInputTab("Colours", 10, 4, GetColourString()),
				new SpecialInputTab("Math", 16, 1, "ƒ½¼αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°√ⁿ²"),
				new SpecialInputTab("Line/Box", 17, 1, "░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀■"),
				new SpecialInputTab("Letters", 17, 1, "ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜáíóúñÑ"),
				new SpecialInputTab("Other", 16, 1, "☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼⌂¢£¥₧ªº¿⌐¬¡«»∙·"),
			};
		}
		
		string GetColourString() {
			int count = 0;
			for (int i = ' '; i <= '~'; i++) {
				if (i >= 'A' && i <= 'F') continue;
				if (IDrawer2D.Cols[i].A > 0) count++;
			}
			
			StringBuffer buffer = new StringBuffer(count * 4);
			int index = 0;
			for (int i = ' '; i <= '~'; i++) {
				if (i >= 'A' && i <= 'F') continue;
				if (IDrawer2D.Cols[i].A == 0) continue;
				
				buffer.Append(ref index, '&').Append(ref index, (char)i)
					.Append(ref index, '%').Append(ref index, (char)i);
			}
			return buffer.ToString();
		}
		
		
		unsafe void MeasureContentSizes(SpecialInputTab e, Font font, Size* sizes) {
			string s = new String('\0', e.CharsPerItem);
			DrawTextArgs args = new DrawTextArgs(s, font, false);
			// avoid allocating temporary strings here
			fixed(char* ptr = s) {
				for (int i = 0; i < e.Contents.Length; i += e.CharsPerItem) {
					for (int j = 0; j < e.CharsPerItem; j++)
						ptr[j] = e.Contents[i + j];
					sizes[i / e.CharsPerItem] = game.Drawer2D.MeasureSize(ref args);
				}
			}
		}
		
		unsafe Size CalculateContentSize(SpecialInputTab e, Size* sizes, out Size elemSize) {
			elemSize = Size.Empty;
			for (int i = 0; i < e.Contents.Length; i += e.CharsPerItem)
				elemSize.Width = Math.Max(elemSize.Width, sizes[i / e.CharsPerItem].Width);
			
			elemSize.Width += contentSpacing;
			elemSize.Height = sizes[0].Height + contentSpacing;
			int rows = Utils.CeilDiv(e.Contents.Length / e.CharsPerItem, e.ItemsPerRow);
			return new Size(elemSize.Width * e.ItemsPerRow, elemSize.Height * rows);
		}
		
		const int titleSpacing = 10, contentSpacing = 5;
		int MeasureTitles(Font font) {
			int totalWidth = 0;
			DrawTextArgs args = new DrawTextArgs(null, font, false);
			for (int i = 0; i < elements.Length; i++) {
				args.Text = elements[i].Title;
				elements[i].TitleSize = game.Drawer2D.MeasureSize(ref args);
				elements[i].TitleSize.Width += titleSpacing;
				totalWidth += elements[i].TitleSize.Width;
			}
			return totalWidth;
		}
		
		void DrawTitles(IDrawer2D drawer, Font font) {
			int x = 0;
			DrawTextArgs args = new DrawTextArgs(null, font, false);
			for (int i = 0; i < elements.Length; i++) {
				args.Text = elements[i].Title;
				FastColour col = i == selectedIndex ? new FastColour(30, 30, 30, 200) :
					new FastColour(0, 0, 0, 127);;
				Size size = elements[i].TitleSize;
				
				drawer.Clear(col, x, 0, size.Width, size.Height);
				drawer.DrawText(ref args, x + titleSpacing / 2, 0);
				x += size.Width;
			}
		}
		
		unsafe void DrawContent(IDrawer2D drawer, Font font, SpecialInputTab e, int yOffset) {
			string s = new String('\0', e.CharsPerItem);
			int wrap = e.ItemsPerRow;
			DrawTextArgs args = new DrawTextArgs(s, font, false);
			
			fixed(char* ptr = s) {
				for (int i = 0; i < e.Contents.Length; i += e.CharsPerItem) {
					for (int j = 0; j < e.CharsPerItem; j++)
						ptr[j] = e.Contents[i + j];
					int item = i / e.CharsPerItem;
					
					int x = (item % wrap) * elementSize.Width, y = (item / wrap) * elementSize.Height;
					y += yOffset;
					drawer.DrawText(ref args, x, y);
				}
			}
		}
	}
}