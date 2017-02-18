// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public partial class InventoryScreen : Screen {
		
		const int scrollWidth = 22, scrollBorder = 2, nubsWidth = 3;
		static FastColour scrollCol = new FastColour(10, 10, 10, 220);
		static FastColour scrollUsedCol = new FastColour(100, 100, 100, 220);
		float ScrollbarScale { get { return (TableHeight - scrollBorder * 2) / (float)rows; } }
		
		void DrawScrollbar() {
			int x = TableX + TableWidth, width = scrollWidth;
			gfx.Draw2DQuad(x, TableY, width, TableHeight, scrollCol);
			
			int y, height;
			GetScrollbarCoords(out y, out height);			
			x += scrollBorder; width -= scrollBorder * 2;
			gfx.Draw2DQuad(x, TableY + y, width, height, scrollUsedCol);
			
			if (height < 20) return;
			x += nubsWidth; width -= nubsWidth * 2;
			
			y = TableY + y + (height / 2);
			gfx.Draw2DQuad(x, y - 1 - 4, width, scrollBorder, scrollCol);
			gfx.Draw2DQuad(x, y - 1,     width, scrollBorder, scrollCol);
			gfx.Draw2DQuad(x, y - 1 + 4, width, scrollBorder, scrollCol);
		}
		
		void GetScrollbarCoords(out int y, out int height) {
			float scale = ScrollbarScale;
			y = (int)Math.Ceiling(scrollY * scale) + scrollBorder;
			height = (int)Math.Ceiling(maxRows * scale);
			height = Math.Min(y + height, TableHeight - scrollBorder) - y;
		}
		
		public override bool HandlesMouseScroll(int delta) {
			bool bounds = Contains(TableX - scrollWidth, TableY, TableWidth + scrollWidth, 
			                       TableHeight, game.Mouse.X, game.Mouse.Y);
			bool hotbar = game.Input.AltDown || game.Input.ControlDown || game.Input.ShiftDown;
			if (!bounds || hotbar) return false;
			
			int rowY = (selIndex / blocksPerRow) - scrollY;
			scrollY -= delta;
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
