// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;
using Launcher.Drawing;
using Launcher.Web;

namespace Launcher.Gui.Widgets {
	
	public sealed class TableView {
		
		internal static FastColour backGridCol = new FastColour(20, 20, 10),
		foreGridCol = new FastColour(40, 40, 40);
		
		int entryHeight, headerHeight;
		internal int headerStartY, headerEndY;
		internal int numEntries, maxIndex;
		
		LauncherWindow game;
		TableWidget table;
		Font font, titleFont;
		
		public void Init(LauncherWindow game, TableWidget table) {
			this.game = game;
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
				Utils.Clamp(ref table.ColumnWidths[i], 20, game.Width - 20);
			}
			table.Width = game.Width - table.X;
			ResetEntries();
			
			int y = table.Y + 3;
			y += headerHeight + 2;
			maxIndex = table.Count;
			y += 5;

			for (int i = table.CurrentIndex; i < table.Count; i++) {
				if (y + entryHeight > table.Y + table.Height) {
					maxIndex = i; return;
				}
				
				table.entries[table.order[i]].Y = y;
				table.entries[table.order[i]].Height = entryHeight;
				y += entryHeight + 2;
			}
		}
		
		public const int flagPadding = 15;
		public void RedrawData(IDrawer2D drawer) {
			DrawGrid(drawer);
			int x = table.X + flagPadding + 5;
			x += DrawColumn(drawer, "Name",     0, x, filterName)     + 5;
			x += DrawColumn(drawer, "Players",  1, x, filterPlayers)  + 5;
			x += DrawColumn(drawer, "Uptime",   2, x, filterUptime)   + 5;
			x += DrawColumn(drawer, "Software", 3, x, filterSoftware) + 5;
			
			DrawScrollbar(drawer);
			DrawFlags();
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
		
		static FastBitmap GetFlag(string flag) {
			List<string> flags = FetchFlagsTask.Flags;
			List<FastBitmap> bitmaps = FetchFlagsTask.Bitmaps;

			for (int i = 0; i < flags.Count; i++) {
				if (flags[i] != flag) continue;
				return i < bitmaps.Count ? bitmaps[i] : null;
			}
			return null;
		}
		
		public void DrawFlags() {
			using (FastBitmap dst = game.LockBits()) {
				for (int i = table.CurrentIndex; i < maxIndex; i++) {
					TableEntry entry = table.Get(i);					
					FastBitmap flag = GetFlag(entry.Flag);
					if (flag == null) continue;
					
					int x = table.X, y = entry.Y;
					Rectangle rect = new Rectangle(x + 2, y + 3, 16, 11);
					BitmapDrawer.Draw(flag, dst, rect);
				}
			}
		}
		
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
				TableEntry entry = table.Get(i);
				args = new DrawTextArgs(filter(entry), font, true);
				if ((i == table.SelectedIndex || entry.Featured) && !separator) {
					int startY = y - 3;
					int height = Math.Min(startY + (entryHeight + 4), table.Y + table.Height) - startY;
					drawer.Clear(GetGridCol(entry.Featured, i == table.SelectedIndex), table.X, startY, table.Width, height);
				}				
				if (!DrawColumnEntry(drawer, ref args, maxWidth, x, ref y, ref entry)) {
					maxIndex = i; break;
				}
			}			
			if (separator && !game.ClassicBackground) {
				drawer.Clear(LauncherSkin.BackgroundCol, x - 7, table.Y, 2, table.Height);
			}
			return maxWidth + 5;
		}

		FastColour GetGridCol(bool featured, bool selected) {
			if (featured) {
				if (selected) return new FastColour(50, 53, 0);
				return new FastColour(101, 107, 0);
			}
			return foreGridCol;
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
				table.entries[i].Height = 0;
				table.entries[i].Y = -10;
			}
		}
		
		void DrawGrid(IDrawer2D drawer) {
			if (!game.ClassicBackground)
				drawer.Clear(LauncherSkin.BackgroundCol,
				             table.X, table.Y + headerHeight + 5, table.Width, 2);
			headerStartY = table.Y;
			
			headerEndY = table.Y + headerHeight + 5;
			int startY = headerEndY + 3;
			numEntries = (table.Y + table.Height - startY) / (entryHeight + 3);
		}
		
		void DrawScrollbar(IDrawer2D drawer) {
			FastColour col = game.ClassicBackground ? new FastColour(80, 80, 80) : LauncherSkin.ButtonBorderCol;
			drawer.Clear(col, game.Width - 10, table.Y, 10, table.Height);
			col = game.ClassicBackground ? new FastColour(160, 160, 160) : LauncherSkin.ButtonForeActiveCol;
			int yOffset, height;
			table.GetScrollbarCoords(out yOffset, out height);
			drawer.Clear(col, game.Width - 10, table.Y + yOffset, 10, height);
		}
	}
}
