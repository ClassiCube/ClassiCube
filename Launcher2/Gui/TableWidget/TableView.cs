// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	
	public sealed class TableView {
		
		internal static FastColour backGridCol = new FastColour(20, 20, 10),
		foreGridCol = new FastColour(40, 40, 40);
		
		int entryHeight, headerHeight;		
		internal int headerStartY, headerEndY;
		internal int numEntries, maxIndex;
		
		LauncherWindow window;
		TableWidget table;
		Font font, titleFont;
		
		public void Init(LauncherWindow window, TableWidget table) {
			this.window = window;
			this.table = table;
		}
		
		public void SetDrawData(IDrawer2D drawer, Font font, Font titleFont) {
			this.font = font;
			this.titleFont = titleFont;
			
			DrawTextArgs args = new DrawTextArgs("IMP", titleFont, true);
			headerHeight = drawer.MeasureSize(ref args).Height;
			args = new DrawTextArgs("IMP", font, true);
			entryHeight = drawer.MeasureSize(ref args).Height;
		}
		
		public void RecalculateDrawData() {
			for (int i = 0; i < table.ColumnWidths.Length; i++) {
				table.ColumnWidths[i] = table.DesiredColumnWidths[i];
				Utils.Clamp(ref table.ColumnWidths[i], 20, window.Width - 20);
			}
			table.Width = window.Width - table.X;
			ResetEntries();
			
			int y = table.Y + 3;
			y += headerHeight + 2;
			maxIndex = table.Count;
			y += 5;

			for (int i = table.CurrentIndex; i < table.Count; i++) {
				if (y + entryHeight > table.Y + table.Height) {
					maxIndex = i; return;
				}
				
				table.usedEntries[i].Y = y;
				table.usedEntries[i].Height = entryHeight;
				y += entryHeight + 2;
			}
		}
		
		public void RedrawData(IDrawer2D drawer) {
			int x = table.X + 5;
			DrawGrid(drawer);
			x += DrawColumn(drawer, false, font, titleFont,
			                "Name", table.ColumnWidths[0], x, e => e.Name) + 5;
			x += DrawColumn(drawer, true, font, titleFont,
			                "Players", table.ColumnWidths[1], x, e => e.Players) + 5;
			x += DrawColumn(drawer, true, font, titleFont,
			                "Uptime", table.ColumnWidths[2], x, e => e.Uptime) + 5;
			x += DrawColumn(drawer, true, font, titleFont,
			                "Software", table.ColumnWidths[3], x, e => e.Software) + 5;
			DrawScrollbar(drawer);
		}
		
		public void Redraw(IDrawer2D drawer) {
			RecalculateDrawData();
			RedrawData(drawer);
		}
		
		delegate string ColumnFilter(TableEntry entry);
		
		int DrawColumn(IDrawer2D drawer, bool separator, Font font, Font titleFont,
		               string header, int maxWidth, int x, ColumnFilter filter) {
			int y = table.Y + 3;
			DrawTextArgs args = new DrawTextArgs(header, titleFont, true);
			TableEntry headerEntry = default(TableEntry);
			DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref headerEntry);
			maxIndex = table.Count;
			y += 5;
			
			for (int i = table.CurrentIndex; i < table.Count; i++) {
				args = new DrawTextArgs(filter(table.usedEntries[i]), font, true);
				if (i == table.SelectedIndex && !separator) {
					int startY = y - 3;
					int height = Math.Min(startY + (entryHeight + 4), table.Y + table.Height) - startY;
					drawer.Clear(foreGridCol, table.X, startY, table.Width, height);
				}
				
				if (!DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref table.usedEntries[i])) {
					maxIndex = i; break;
				}
			}
			
			if (separator && !window.ClassicBackground)
				drawer.Clear(LauncherSkin.BackgroundCol, x - 7, table.Y, 2, table.Height);
			return maxWidth + 5;
		}
		
		bool DrawColumnEntry(IDrawer2D drawer, ref DrawTextArgs args,
		                     int maxWidth, int x, ref int y, ref TableEntry entry) {
			Size size = drawer.MeasureSize(ref args);
			bool empty = args.Text == "";
			if (empty)
				size.Height = entryHeight;
			if (y + size.Height > table.Y + table.Height) {
				y = table.Y + table.Height + 2; return false;
			}
			
			entry.Y = y; entry.Height = size.Height;
			if (!empty) {
				size.Width = Math.Min(maxWidth, size.Width);
				args.SkipPartsCheck = false;
				//Drawer2DExt.DrawClippedText(ref args, drawer, x, y, maxWidth);
				drawer.DrawClippedText(ref args, x, y, maxWidth, 30);
			}
			y += size.Height + 2;
			return true;
		}
		
		void ResetEntries() {
			for (int i = 0; i < table.Count; i++) {
				table.entries[i].Height = 0; table.usedEntries[i].Height = 0;
				table.entries[i].Y = -10; table.usedEntries[i].Y = -10;
			}
		}
				
		void DrawGrid(IDrawer2D drawer) {
			if (!window.ClassicBackground)
				drawer.Clear(LauncherSkin.BackgroundCol, 
				             table.X, table.Y + headerHeight + 5, table.Width, 2);
			headerStartY = table.Y;
			
			headerEndY = table.Y + headerHeight + 5;
			int startY = headerEndY + 3;
			numEntries = (table.Y + table.Height - startY) / (entryHeight + 3);
		}
		
		void DrawScrollbar(IDrawer2D drawer) {
			FastColour col = window.ClassicBackground ? new FastColour(80, 80, 80) : LauncherSkin.ButtonBorderCol;
			drawer.Clear(col, window.Width - 10, table.Y, 10, table.Height);
			col = window.ClassicBackground ? new FastColour(160, 160, 160) : LauncherSkin.ButtonForeActiveCol;
			int yOffset, height;
			table.GetScrollbarCoords(out yOffset, out height);
			drawer.Clear(col, window.Width - 10, table.Y + yOffset, 10, height);
		}
	}
}
