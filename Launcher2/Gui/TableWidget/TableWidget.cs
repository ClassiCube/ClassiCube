// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp;
using Launcher.Web;

namespace Launcher.Gui.Widgets {

	internal struct TableEntry {
		public string Hash, Name, Players, Uptime, Software, RawUptime, Flag;
		public int Y, Height;
		public bool Featured;
	}
	
	public delegate void TableNeedsRedrawHandler();
	
	public partial class TableWidget : Widget {
		
		TableView view;
		public TableWidget(LauncherWindow window) : base(window) {
			OnClick = HandleOnClick;
			view = new TableView();
			view.Init(window, this);
		}
		
		public TableNeedsRedrawHandler NeedRedraw;
		public Action<string> SelectedChanged;
		public int SelectedIndex = -1;
		public string SelectedHash = "";
		public int CurrentIndex, Count;
		
		internal TableEntry[] entries, usedEntries;
		internal List<ServerListEntry> servers;
		
		public void SetEntries(List<ServerListEntry> servers) {
			entries = new TableEntry[servers.Count];
			usedEntries = new TableEntry[servers.Count];
			this.servers = servers;
			int index = 0;
			
			for (int i = 0; i < servers.Count; i++) {
				ServerListEntry e = servers[i];
				TableEntry tableEntry = default(TableEntry);
				tableEntry.Hash = e.Hash;
				tableEntry.Name = e.Name;
				tableEntry.Players = e.Players + "/" + e.MaxPlayers;
				tableEntry.Software = e.Software;
				tableEntry.Uptime = MakeUptime(e.Uptime);
				tableEntry.RawUptime = e.Uptime;
				tableEntry.Featured = e.Featured;
				tableEntry.Flag = e.Flag;
				
				entries[index] = tableEntry;
				usedEntries[index] = tableEntry;
				index++;
			}
			Count = entries.Length;
		}
		
		public void FilterEntries(string filter) {
			Count = 0;
			int index = 0;
			
			for (int i = 0; i < entries.Length; i++) {
				TableEntry entry = entries[i];
				if (entry.Name.IndexOf(filter, StringComparison.OrdinalIgnoreCase) >= 0) {
					Count++;
					usedEntries[index++] = entry;
				}
			}
			SetSelected(SelectedHash);
		}		
		
		internal void GetScrollbarCoords(out int y, out int height) {
			if (Count == 0) { y = 0; height = 0; return; }
			
			float scale = Height / (float)Count;
			y = (int)Math.Ceiling(CurrentIndex * scale);
			height = (int)Math.Ceiling((view.maxIndex - CurrentIndex) * scale);
			height = Math.Min(y + height, Height) - y;
		}
		
		public void SetSelected(int index) {
			if (index >= view.maxIndex) CurrentIndex = index + 1 - view.numEntries;
			if (index < CurrentIndex) CurrentIndex = index;
			if (index >= Count) index = Count - 1;
			if (index < 0) index = 0;
			
			SelectedHash = "";
			SelectedIndex = index;
			lastIndex = index;
			ClampIndex();
			
			if (Count > 0) {
				SelectedChanged(usedEntries[index].Hash);
				SelectedHash = usedEntries[index].Hash;
			}
		}
		
		public void SetSelected(string hash) {
			SelectedIndex = -1;
			for (int i = 0; i < Count; i++) {
				if (usedEntries[i].Hash == hash) {
					SetSelected(i);
					return;
				}
			}
		}
		
		public void ClampIndex() {
			if (CurrentIndex > Count - view.numEntries)
				CurrentIndex = Count - view.numEntries;
			if (CurrentIndex < 0)
				CurrentIndex = 0;
		}
		
		string MakeUptime(string rawSeconds) {
			TimeSpan t = TimeSpan.FromSeconds(Double.Parse(rawSeconds));
			if (t.TotalHours >= 24 * 7)
				return (int)t.TotalDays + "d";
			if (t.TotalHours >= 1)
				return (int)t.TotalHours + "h";
			if (t.TotalMinutes >= 1)
				return (int)t.TotalMinutes + "m";
			return (int)t.TotalSeconds + "s";
		}
		
		
		public int[] ColumnWidths = new int[] { 320, 65, 65, 140 };
		public int[] DesiredColumnWidths = new int[] { 320, 65, 65, 140 };

		public void SetDrawData(IDrawer2D drawer, Font font, Font titleFont,
		                        Anchor horAnchor, Anchor verAnchor, int x, int y) {
			SetLocation(horAnchor, verAnchor, x, y);
			view.SetDrawData(drawer, font, titleFont);
		}
		
		public void RecalculateDrawData() { view.RecalculateDrawData(); }
		public void RedrawData(IDrawer2D drawer) { view.RedrawData(drawer); }
		public void RedrawFlags() { view.DrawFlags(); }
		
		public override void Redraw(IDrawer2D drawer) {
			RecalculateDrawData();
			RedrawData(drawer);
		}		
		

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
			int x = X + 15;	
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
			int x = X + TableView.flagPadding;
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
