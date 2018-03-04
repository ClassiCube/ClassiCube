// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK.Input;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Gui.Widgets {
	public sealed class ScrollbarWidget : Widget {
		
		public ScrollbarWidget(Game game) : base(game) {
			Width = scrollWidth;
		}
		
		public int TotalRows, ScrollY;
		
		public void ClampScrollY() {
			int maxRows = TotalRows - TableWidget.MaxRowsDisplayed;
			if (ScrollY >= maxRows) ScrollY = maxRows;
			if (ScrollY < 0) ScrollY = 0;
		}		
		
		public override void Init() { }
		public override void Dispose() { }

		const int scrollWidth = 22, scrollBorder = 2, nubsWidth = 3;
		static FastColour scrollBackCol = new FastColour(10, 10, 10, 220);
		static FastColour scrollBarCol = new FastColour(100, 100, 100, 220);
		static FastColour scrollHoverCol = new FastColour(122, 122, 122, 220);
		float ScrollbarScale { get { return (Height - scrollBorder * 2) / (float)TotalRows; } }
		
		public override void Render(double delta) {
			IGraphicsApi gfx = game.Graphics;
			int x = X, width = Width;
			gfx.Draw2DQuad(x, Y, width, Height, scrollBackCol);
			
			int y, height;
			GetScrollbarCoords(out y, out height);
			x += scrollBorder; width -= scrollBorder * 2; y += Y;
			
			bool hovered = game.Mouse.Y >= y && game.Mouse.Y < (y + height) &&
				game.Mouse.X >= x && game.Mouse.X < (x + width);
			FastColour barCol = hovered ? scrollHoverCol : scrollBarCol;
			gfx.Draw2DQuad(x, y, width, height, barCol);
			
			if (height < 20) return;
			x += nubsWidth; width -= nubsWidth * 2; y += (height / 2);
			
			gfx.Draw2DQuad(x, y - 1 - 4, width, scrollBorder, scrollBackCol);
			gfx.Draw2DQuad(x, y - 1,     width, scrollBorder, scrollBackCol);
			gfx.Draw2DQuad(x, y - 1 + 4, width, scrollBorder, scrollBackCol);
		}
		
		void GetScrollbarCoords(out int y, out int height) {
			float scale = ScrollbarScale;
			y = (int)Math.Ceiling(ScrollY * scale) + scrollBorder;
			height = (int)Math.Ceiling(TableWidget.MaxRowsDisplayed * scale);
			height = Math.Min(y + height, Height - scrollBorder) - y;
		}
		
		
		float invAcc;
		public override bool HandlesMouseScroll(float delta) {
			int steps = Utils.AccumulateWheelDelta(ref invAcc, delta);
			ScrollY -= steps;
			ClampScrollY();
			return true;
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			if (draggingMouse) {
				mouseY -= Y;
				ScrollY = (int)((mouseY - mouseOffset) / ScrollbarScale);
				ClampScrollY();
				return true;
			}
			return false;
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			if (draggingMouse) return true;
			if (button != MouseButton.Left) return false;
			
			if (mouseX >= X && mouseX < X + Width) {
				ScrollbarClick(mouseY);
				return true;
			}
			return false;
		}

		void ScrollbarClick(int mouseY) {
			mouseY -= Y;
			int y, height;
			GetScrollbarCoords(out y, out height);
			
			if (mouseY < y) {
				ScrollY -= TableWidget.MaxRowsDisplayed;
			} else if (mouseY >= y + height) {
				ScrollY += TableWidget.MaxRowsDisplayed;
			} else {
				draggingMouse = true;
				mouseOffset = mouseY - y;
			}
			ClampScrollY();
		}
		
		internal bool draggingMouse = false;
		int mouseOffset = 0;		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) {
			draggingMouse = false;
			mouseOffset = 0;
			return true;
		}
	}
}
