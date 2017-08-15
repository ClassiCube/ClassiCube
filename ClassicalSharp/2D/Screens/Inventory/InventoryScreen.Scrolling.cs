// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public partial class InventoryScreen : Screen {
		
		const int scrollWidth = 22, scrollBorder = 2, nubsWidth = 3;
		static FastColour scrollBackCol = new FastColour(10, 10, 10, 220);
		static FastColour scrollBarCol = new FastColour(100, 100, 100, 220);
		static FastColour scrollHoverCol = new FastColour(122, 122, 122, 220);
		float ScrollbarScale { get { return (TableHeight - scrollBorder * 2) / (float)rows; } }
		
		void DrawScrollbar() {
			int x = TableX + TableWidth, width = scrollWidth;
			gfx.Draw2DQuad(x, TableY, width, TableHeight, scrollBackCol);
			
			int y, height;
			GetScrollbarCoords(out y, out height);
			x += scrollBorder; width -= scrollBorder * 2; y += TableY;
			
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
			y = (int)Math.Ceiling(scrollY * scale) + scrollBorder;
			height = (int)Math.Ceiling(maxRows * scale);
			height = Math.Min(y + height, TableHeight - scrollBorder) - y;
		}
		
		float invAcc;
		public override bool HandlesMouseScroll(float delta) {
			bool bounds = Contains(TableX - scrollWidth, TableY, TableWidth + scrollWidth, 
			                       TableHeight, game.Mouse.X, game.Mouse.Y);
			bool hotbar = game.Input.AltDown || game.Input.ControlDown || game.Input.ShiftDown;
			if (!bounds || hotbar) return false;
			
			int rowY = (selIndex / blocksPerRow) - scrollY;
			int steps = Utils.AccumulateWheelDelta(ref invAcc, delta);
			scrollY -= steps;
			ClampScrollY();
			if (selIndex == - 1) return true;
			
			selIndex = scrollY * blocksPerRow + (selIndex % blocksPerRow);
			for (int row = 0; row < rowY; row++) {
				selIndex += blocksPerRow;
			}
			
			if (selIndex >= blocksTable.Length)
				selIndex = -1;			
			RecreateBlockInfoTexture();
			return true;
		}
		int scrollY;
		
		void ClampScrollY() {
			if (scrollY >= rows - maxRows) scrollY = rows - maxRows;
			if (scrollY < 0) scrollY = 0;
		}
		
		void ScrollbarClick(int mouseY) {
			mouseY -= TableY;
			int y, height;
			GetScrollbarCoords(out y, out height);
			
			if (mouseY < y) {
				scrollY -= maxRows;
			} else if (mouseY >= y + height) {
				scrollY += maxRows;
			} else {
				draggingMouse = true;
				mouseOffset = mouseY - y;
			}
			ClampScrollY();
		}
		
		bool draggingMouse = false;
		int mouseOffset = 0;
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) {
			draggingMouse = false;
			mouseOffset = 0;
			return true;
		}
	}
}
