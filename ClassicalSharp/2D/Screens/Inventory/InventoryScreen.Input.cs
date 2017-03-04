// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public partial class InventoryScreen : Screen {
		
		public override bool HandlesAllInput { get { return true; } }
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			if (draggingMouse) {
				mouseY -= TableY;
				scrollY = (int)((mouseY - mouseOffset) / ScrollbarScale);
				ClampScrollY();
				return true;
			}
			
			selIndex = -1;
			if (Contains(startX, startY + 3, blocksPerRow * blockSize,
			             maxRows * blockSize - 3 * 2, mouseX, mouseY)) {
				for (int i = 0; i < blocksTable.Length; i++) {
					int x, y;
					GetCoords(i, out x, out y);
					
					if (Contains(x, y, blockSize, blockSize, mouseX, mouseY)) {
						selIndex = i;
						break;
					}
				}
			}
			RecreateBlockInfoTexture();
			return true;
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			if (draggingMouse || game.Gui.hudScreen.hotbar.HandlesMouseClick(mouseX, mouseY, button))
				return true;
			
			int scrollX = TableX + TableWidth;
			if (button == MouseButton.Left && mouseX >= scrollX && mouseX < scrollX + scrollWidth) {
				ScrollbarClick(mouseY);
			} else if (button == MouseButton.Left) {
				if (selIndex != -1) {
					game.Inventory.Selected = blocksTable[selIndex];
				} else if (Contains(TableX, TableY, TableWidth, TableHeight, mouseX, mouseY)) {
					return true;
				}
				
				bool hotbar = game.IsKeyDown(Key.AltLeft) || game.IsKeyDown(Key.AltRight);
				if (!hotbar)
					game.Gui.SetNewScreen(null);
			}
			return true;
		}
		
		// We want the user to be able to press B to exit the inventory menu
		// however since we use KeyRepeat = true, we must wait until the first time they released B
		// before marking the next press as closing the menu
		bool releasedInv;
		public override bool HandlesKeyDown(Key key) {
			if (key == game.Mapping(KeyBind.PauseOrExit)) {
				game.Gui.SetNewScreen(null);
			} else if (key == game.Mapping(KeyBind.Inventory) && releasedInv) {
				game.Gui.SetNewScreen(null);
			} else if (key == Key.Enter && selIndex != -1) {
				game.Inventory.Selected = blocksTable[selIndex];
				game.Gui.SetNewScreen(null);
			} else if ((key == Key.Left || key == Key.Keypad4) && selIndex != -1) {
				ArrowKeyMove(-1);
			} else if ((key == Key.Right || key == Key.Keypad6) && selIndex != -1) {
				ArrowKeyMove(1);
			} else if ((key == Key.Up || key == Key.Keypad8) && selIndex != -1) {
				ArrowKeyMove(-blocksPerRow);
			} else if ((key == Key.Down || key == Key.Keypad2) && selIndex != -1) {
				ArrowKeyMove(blocksPerRow);
			} else if (game.Gui.hudScreen.hotbar.HandlesKeyDown(key)) {
			}
			return true;
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (key == game.Mapping(KeyBind.Inventory)) {
				releasedInv = true; return true;
			}
			return game.Gui.hudScreen.hotbar.HandlesKeyUp(key);
		}
		
		void ArrowKeyMove(int delta) {
			int startIndex = selIndex;
			selIndex += delta;
			if (selIndex < 0)
				selIndex -= delta;
			if (selIndex >= blocksTable.Length)
				selIndex -= delta;
			
			int scrollDelta = (selIndex / blocksPerRow) - (startIndex / blocksPerRow);
			scrollY += scrollDelta;
			ClampScrollY();
			RecreateBlockInfoTexture();
			MoveCursorToSelected();
		}
	}
}
