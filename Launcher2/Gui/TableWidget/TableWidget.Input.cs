// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	public partial class TableWidget : Widget {

		DefaultComparer defComp = new DefaultComparer();
		NameComparer nameComp = new NameComparer();
		PlayersComparer playerComp = new PlayersComparer();
		UptimeComparer uptimeComp = new UptimeComparer();
		SoftwareComparer softwareComp = new SoftwareComparer();
		internal int DraggingColumn = -1;
		internal bool DraggingScrollbar = false;
		internal int mouseOffset;
		
		public void SortDefault() {
			SortEntries(defComp, true);
		}
		
		void SelectHeader(int mouseX, int mouseY) {
			int x = X;		
			for (int i = 0; i < ColumnWidths.Length; i++) {
				x += ColumnWidths[i] + 10;
				if (mouseX >= x - 8 && mouseX < x + 8) {
					DraggingColumn = i;
					lastIndex = -10; return;
				}
			}		
			TrySortColumns(mouseX);
		}
		
		void TrySortColumns(int mouseX) {
			int x = X;
			if (mouseX >= x && mouseX < x + ColumnWidths[0]) {
				SortEntries(nameComp, false); return;
			}
			
			x += ColumnWidths[0] + 10;
			if (mouseX >= x && mouseX < x + ColumnWidths[1]) {
				SortEntries(playerComp, false); return;
			}
			
			x += ColumnWidths[1] + 10;
			if (mouseX >= x && mouseX < x + ColumnWidths[2]) {
				SortEntries(uptimeComp, false); return;
			}
			
			x += ColumnWidths[2] + 10;
			if (mouseX >= x) {
				SortEntries(softwareComp, false); return;
			}
		}
		
		void SortEntries(TableEntryComparer comparer, bool noRedraw) {
			Array.Sort(usedEntries, 0, Count, comparer);
			Array.Sort(entries, 0, entries.Length, comparer);
			lastIndex = -10;
			if (noRedraw) return;
			
			comparer.Invert = !comparer.Invert;
			SetSelected(SelectedHash);
			NeedRedraw();
		}
		
		void GetSelectedServer(int mouseX, int mouseY) {
			for (int i = 0; i < Count; i++) {
				TableEntry entry = usedEntries[i];
				if (mouseY < entry.Y || mouseY >= entry.Y + entry.Height + 2) continue;
				
				if (lastIndex == i && (DateTime.UtcNow - lastPress).TotalSeconds < 1) {
					Window.ConnectToServer(servers, entry.Hash);
					lastPress = DateTime.MinValue;
					return;
				}
				
				SetSelected(i);
				NeedRedraw();
				break;
			}
		}
		
		void HandleOnClick(int mouseX, int mouseY) {			
			if (mouseX >= Window.Width - 10) {
				ScrollbarClick(mouseY);
				DraggingScrollbar = true;
				lastIndex = -10; return;
			}
			
			if (mouseY >= view.headerStartY && mouseY < view.headerEndY) {
				SelectHeader(mouseX, mouseY);
			} else {
				GetSelectedServer(mouseX, mouseY);
			}
			lastPress = DateTime.UtcNow;
		}
		
		int lastIndex = -10;
		DateTime lastPress;
		public void MouseMove(int x, int y, int deltaX, int deltaY) {
			if (DraggingScrollbar) {
				y -= Y;
				float scale = Height / (float)Count;
				CurrentIndex = (int)((y - mouseOffset) / scale);
				ClampIndex();
				NeedRedraw();
			} else if (DraggingColumn >= 0) {
				if (x >= Window.Width - 20) return;
				int col = DraggingColumn;
				ColumnWidths[col] += deltaX;
				Utils.Clamp(ref ColumnWidths[col], 20, Window.Width - 20);
				DesiredColumnWidths[col] = ColumnWidths[col];
				NeedRedraw();
			}
			
		}
		
		void ScrollbarClick(int mouseY) {
			mouseY -= Y;
			int y, height;
			GetScrollbarCoords(out y, out height);
			int delta = (view.maxIndex - CurrentIndex);
			
			if (mouseY < y) {
				CurrentIndex -= delta;
			} else if (mouseY >= y + height) {
				CurrentIndex += delta;
			} else {
				DraggingScrollbar = true;
				mouseOffset = mouseY - y;
			}
			ClampIndex();
			NeedRedraw();
		}
	}
}
