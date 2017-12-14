﻿// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
			
			headerHeight = drawer.FontHeight(titleFont, true);
			entryHeight = drawer.FontHeight(font, true);
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
			x += DrawColumn(drawer, "Name",     0, x, filterName)     + 5;
			x += DrawColumn(drawer, "Players",  1, x, filterPlayers)  + 5;
			x += DrawColumn(drawer, "Uptime",   2, x, filterUptime)   + 5;
			x += DrawColumn(drawer, "Software", 3, x, filterSoftware) + 5;
			DrawScrollbar(drawer);
		}
		
		public void Redraw(IDrawer2D drawer) {
			RecalculateDrawData();
			RedrawData(drawer);
		}
		
		delegate string ColumnFilter(TableEntry entry);
		// cache to avoid allocations every redraw
		static string FilterName(TableEntry e)     { return e.Name; }     static ColumnFilter filterName     = FilterName; 
		static string FilterPlayers(TableEntry e)  { return e.Players; }  static ColumnFilter filterPlayers  = FilterPlayers;
		static string FilterUptime(TableEntry e)   { return e.Uptime; }   static ColumnFilter filterUptime   = FilterUptime;
		static string FilterSoftware(TableEntry e) { return e.Software; } static ColumnFilter filterSoftware = FilterSoftware;
		
		int DrawColumn(IDrawer2D drawer, string header, int columnI, int x, ColumnFilter filter) {
			int y = table.Y + 3;
			int maxWidth = table.ColumnWidths[columnI];
			bool separator = columnI > 0;
			
			DrawTextArgs args = new DrawTextArgs(header, titleFont, true);
			TableEntry headerEntry = default(TableEntry);
			DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref headerEntry);
			maxIndex = table.Count;

            y += 5;		
			for (int i = table.CurrentIndex; i < table.Count; i++) {
                foreGridCol = (table.entries[i].Featured ? FastColour.Red : new FastColour(40, 40, 40));
                args = new DrawTextArgs(filter(table.usedEntries[i]), font, true);
				if ((i == table.SelectedIndex || table.entries[i].Featured) && !separator) {
					int startY = y - 3;
					int height = Math.Min(startY + (entryHeight + 4), table.Y + table.Height) - startY;
                    drawer.Clear(table.entries[i].Featured ? new FastColour(87, 89, 0) : foreGridCol, table.X, startY, table.Width, height);
                }

                if (!DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref table.usedEntries[i])) {
					maxIndex = i; break;
				}
			}
			
			if (separator && !window.ClassicBackground) {
				drawer.Clear(LauncherSkin.BackgroundCol, x - 7, table.Y, 2, table.Height);
			}
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
