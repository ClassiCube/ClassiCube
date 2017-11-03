// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class TableWidget : Widget {
		
		public TableWidget(Game game) : base(game) { }
		
		public const int MaxRowsDisplayed = 8;
		public BlockID[] Elements;
		public int ElementsPerRow;
		public int SelectedIndex;
		public Font font;
		internal ScrollbarWidget scroll;
		public bool PendingClose;
		
		Texture descTex;
		int totalRows, blockSize;
		float selBlockExpand;
		StringBuffer buffer = new StringBuffer(128);
		IsometricBlockDrawer drawer = new IsometricBlockDrawer();
		
		int TableX { get { return X - 5 - 10; } }
		int TableY { get { return Y - 5 - 30; } }
		int TableWidth { get { return ElementsPerRow * blockSize + 10 + 20; } }
		int TableHeight { get { return Math.Min(totalRows, MaxRowsDisplayed) * blockSize + 10 + 40; } }
		
		// These were sourced by taking a screenshot of vanilla
		// Then using paint to extract the colour components
		// Then using wolfram alpha to solve the glblendfunc equation
		static FastColour topBackCol = new FastColour(34, 34, 34, 168);
		static FastColour bottomBackCol = new FastColour(57, 57, 104, 202);
		static FastColour topSelCol = new FastColour(255, 255, 255, 142);
		static FastColour bottomSelCol = new FastColour(255, 255, 255, 192);
		
		static VertexP3fT2fC4b[] vertices = new VertexP3fT2fC4b[8 * 10 * (4 * 4)];
		int vb;
		public override void Render(double delta) {
			gfx.Draw2DQuad(TableX, TableY, TableWidth, TableHeight, topBackCol, bottomBackCol);
			if (totalRows > MaxRowsDisplayed) scroll.Render(delta);
			
			if (SelectedIndex != -1 && game.ClassicMode) {
				int x, y;
				GetCoords(SelectedIndex, out x, out y);
				float off = blockSize * 0.1f;
				int size = (int)(blockSize + off * 2);
				gfx.Draw2DQuad((int)(x - off), (int)(y - off),
				               size, size, topSelCol, bottomSelCol);
			}
			gfx.Texturing = true;
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			
			drawer.BeginBatch(game, vertices, vb);
			for (int i = 0; i < Elements.Length; i++) {
				int x, y;
				if (!GetCoords(i, out x, out y)) continue;
				
				// We want to always draw the selected block on top of others
				if (i == SelectedIndex) continue;
				drawer.DrawBatch(Elements[i], blockSize * 0.7f / 2f,
				                 x + blockSize / 2, y + blockSize / 2);
			}
			
			if (SelectedIndex != -1) {
				int x, y;
				GetCoords(SelectedIndex, out x, out y);
				drawer.DrawBatch(Elements[SelectedIndex], (blockSize + selBlockExpand) * 0.7f / 2,
				                 x + blockSize / 2, y + blockSize / 2);
			}
			drawer.EndBatch();
			
			if (descTex.IsValid)
				descTex.Render(gfx);
			gfx.Texturing = false;
		}
		
		bool GetCoords(int i, out int screenX, out int screenY) {
			int x = i % ElementsPerRow, y = i / ElementsPerRow;
			screenX = X + blockSize * x;
			screenY = Y + blockSize * y + 3;
			screenY -= scroll.ScrollY * blockSize;
			y -= scroll.ScrollY;
			return y >= 0 && y < MaxRowsDisplayed;
		}
		
		Point GetMouseCoords(int i) {
			int x, y;
			GetCoords(i, out x, out y);
			x += blockSize / 2; y += blockSize / 2;
			
			Point topLeft = game.PointToScreen(Point.Empty);
			x += topLeft.X; y += topLeft.Y;
			return new Point(x, y);
		}
		
		public override void Init() {
			scroll = new ScrollbarWidget(game);
			RecreateElements();
			Reposition();
			SetBlockTo(game.Inventory.Selected);
			Recreate();
		}
		
		public override void Dispose() {
			gfx.DeleteVb(ref vb);
			gfx.DeleteTexture(ref descTex);
			lastCreatedIndex = -1000;
		}
		
		public override void Recreate() {
			Dispose();
			vb = gfx.CreateDynamicVb(VertexFormat.P3fT2fC4b, vertices.Length);
			RecreateDescTex();
		}
		
		public override void Reposition() {
			blockSize = (int)(50 * Math.Sqrt(game.GuiInventoryScale));
			selBlockExpand = (float)(25 * Math.Sqrt(game.GuiInventoryScale));
			UpdatePos();
			UpdateScrollbarPos();
		}
		
		void UpdateDescTexPos() {
			descTex.X1 = X + Width / 2 - descTex.Width / 2;
			descTex.Y1 = Y - descTex.Height - 5;
		}
		
		void UpdatePos() {
			int rowsDisplayed = Math.Min(MaxRowsDisplayed, totalRows);
			Width  = blockSize * ElementsPerRow;
			Height = blockSize * rowsDisplayed;
			X = game.Width  / 2 - Width  / 2;
			Y = game.Height / 2 - Height / 2;
			UpdateDescTexPos();
		}
		
		void UpdateScrollbarPos() {
			scroll.X = TableX + TableWidth;
			scroll.Y = TableY;
			scroll.Height = TableHeight;
			scroll.TotalRows = totalRows;
		}

		public void SetBlockTo(BlockID block) {
			SelectedIndex = -1;
			for (int i = 0; i < Elements.Length; i++) {
				if (Elements[i] == block) SelectedIndex = i;
			}
			
			scroll.ScrollY = (SelectedIndex / ElementsPerRow) - (MaxRowsDisplayed - 1);
			scroll.ClampScrollY();
			MoveCursorToSelected();
			RecreateDescTex();
		}
		
		public void OnInventoryChanged() {
			RecreateElements();
			if (SelectedIndex >= Elements.Length)
				SelectedIndex = Elements.Length - 1;
			
			scroll.ScrollY = SelectedIndex / ElementsPerRow;
			scroll.ClampScrollY();
			RecreateDescTex();
		}
		
		void MoveCursorToSelected() {
			if (SelectedIndex == -1) return;
			game.DesktopCursorPos = GetMouseCoords(SelectedIndex);
		}
		
		void UpdateBlockInfoString(BlockID block) {
			int index = 0;
			buffer.Clear();
			if (game.PureClassic) { buffer.Append(ref index, "Select block"); return; }
			
			buffer.Append(ref index, BlockInfo.Name[block]);
			if (game.ClassicMode) return;
			
			buffer.Append(ref index, " (ID ");
			buffer.AppendNum(ref index, block);
			buffer.Append(ref index, "&f, place ");
			buffer.Append(ref index, BlockInfo.CanPlace[block] ? "&aYes" : "&cNo");
			buffer.Append(ref index, "&f, delete ");
			buffer.Append(ref index, BlockInfo.CanDelete[block] ? "&aYes" : "&cNo");
			buffer.Append(ref index, "&f)");
		}
		
		int lastCreatedIndex = -1000;
		void RecreateDescTex() {
			if (SelectedIndex == lastCreatedIndex || Elements == null) return;
			lastCreatedIndex = SelectedIndex;
			
			gfx.DeleteTexture(ref descTex);
			if (SelectedIndex == -1) return;
			
			BlockID block = Elements[SelectedIndex];
			UpdateBlockInfoString(block);
			string value = buffer.ToString();
			
			DrawTextArgs args = new DrawTextArgs(value, font, true);
			descTex = game.Drawer2D.MakeTextTexture(ref args, 0, 0);
			UpdateDescTexPos();
		}
		
		void RecreateElements() {
			int totalElements = 0;
			int count = game.UseCPE ? Block.Count : Block.OriginalCount;
			for (int i = 0; i < count; i++) {
				BlockID block = game.Inventory.Map[i];
				if (Show(block)) totalElements++;
			}
			
			totalRows = Utils.CeilDiv(totalElements, ElementsPerRow);
			UpdateScrollbarPos();
			UpdatePos();

			Elements = new BlockID[totalElements];
			int index = 0;
			for (int i = 0; i < count; i++) {
				BlockID block = game.Inventory.Map[i];
				if (Show(block)) Elements[index++] = block;
			}
		}
		
		bool Show(BlockID block) {
			if (block == Block.Invalid) return false;

			if (block < Block.CpeCount) {
				int count = game.UseCPEBlocks ? Block.CpeCount : Block.OriginalCount;
				return block < count;
			}
			return true;
		}
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			if (scroll.HandlesMouseMove(mouseX, mouseY)) return true;
			
			SelectedIndex = -1;
			if (Contains(X, Y + 3, Width, MaxRowsDisplayed * blockSize - 3 * 2, mouseX, mouseY)) {
				for (int i = 0; i < Elements.Length; i++) {
					int x, y;
					GetCoords(i, out x, out y);
					
					if (Contains(x, y, blockSize, blockSize, mouseX, mouseY)) {
						SelectedIndex = i;
						break;
					}
				}
			}
			RecreateDescTex();
			return true;
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			PendingClose = false;
			if (button != MouseButton.Left) return false;

			if (scroll.HandlesMouseClick(mouseX, mouseY, button)) {
				return true;
			} else if (SelectedIndex != -1) {
				game.Inventory.Selected = Elements[SelectedIndex];
				PendingClose = true;
				return true;
			} else if (Contains(TableX, TableY, TableWidth, TableHeight, mouseX, mouseY)) {
				return true;
			}
			return false;
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (SelectedIndex == -1) return false;
			
			if (key == Key.Left || key == Key.Keypad4) {
				ScrollRelative(-1);
			} else if (key == Key.Right || key == Key.Keypad6) {
				ScrollRelative(1);
			} else if (key == Key.Up || key == Key.Keypad8) {
				ScrollRelative(-ElementsPerRow);
			} else if (key == Key.Down || key == Key.Keypad2) {
				ScrollRelative(ElementsPerRow);
			} else {
				return false;
			}
			return true;
		}
		
		void ScrollRelative(int delta) {
			int startIndex = SelectedIndex;
			SelectedIndex += delta;
			if (SelectedIndex < 0) SelectedIndex -= delta;
			if (SelectedIndex >= Elements.Length) SelectedIndex -= delta;
			
			int scrollDelta = (SelectedIndex / ElementsPerRow) - (startIndex / ElementsPerRow);
			scroll.ScrollY += scrollDelta;
			scroll.ClampScrollY();
			RecreateDescTex();
			MoveCursorToSelected();
		}
		
		public override bool HandlesMouseScroll(float delta) {
			int startScrollY = scroll.ScrollY;
			bool bounds = Contains(TableX - scroll.Width, TableY, TableWidth + scroll.Width,
			                       TableHeight, game.Mouse.X, game.Mouse.Y);
			if (!bounds) return false;
			
			scroll.HandlesMouseScroll(delta);
			if (SelectedIndex == - 1) return true;
			
			SelectedIndex += (scroll.ScrollY - startScrollY) * ElementsPerRow;
			if (SelectedIndex >= Elements.Length) SelectedIndex = -1;
			
			RecreateDescTex();
			return true;
		}
		
		public override bool HandlesMouseUp(int mouseX, int mouseY, MouseButton button) {
			return scroll.HandlesMouseUp(mouseX, mouseY, button);
		}
	}
}
