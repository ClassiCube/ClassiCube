// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;

namespace ClassicalSharp.Gui.Widgets {
	public class PlayerListWidget : Widget {
		
		readonly Font font;
		bool classic;
		public PlayerListWidget(Game game, Font font, bool classic) : base(game) {
			HorizontalAnchor = Anchor.Centre;
			VerticalAnchor = Anchor.Centre;
			this.font = font;
			this.classic = classic;
			elementOffset = classic ? 0 : 10;
		}

		const int columnPadding = 5;
		const int boundsSize = 10;
		const int namesPerColumn = 16;
		const ushort groupNameID = 65535;
		
		int elementOffset, namesCount = 0;
		Texture[] textures = new Texture[512];
		ushort[] IDs = new ushort[512];
		int xMin, xMax, yHeight;
		
		static FastColour topCol = new FastColour(0, 0, 0, 180);
		static FastColour bottomCol = new FastColour(50, 50, 50, 205);
		TextWidget overview;
		
		public override void Init() {
			TabListEntry[] entries = TabList.Entries;
			for (int id = 0; id < entries.Length; id++) {
				TabListEntry e = entries[id];
				if (e != null) AddName((byte)id, -1);
			}
			SortAndReposition();

			overview = TextWidget.Create(game, "Connected players:", font)
				.SetLocation(Anchor.Centre, Anchor.LeftOrTop, 0, 0);
			game.EntityEvents.TabListEntryAdded += TabEntryAdded;
			game.EntityEvents.TabListEntryRemoved += TabEntryRemoved;
			game.EntityEvents.TabListEntryChanged += TabEntryChanged;
		}
		
		public override void Render(double delta) {
			gfx.Texturing = false;
			int offset = overview.Height + 10;
			int height = Math.Max(300, Height + overview.Height);
			gfx.Draw2DQuad(X, Y - offset, Width, height, topCol, bottomCol);
			
			gfx.Texturing = true;
			overview.YOffset = Y - offset + 5;
			overview.Reposition();
			overview.Render(delta);
			
			int highlightedI = HighlightedName(game.Mouse.X, game.Mouse.Y);			
			for (int i = 0; i < namesCount; i++) {
				if (!textures[i].IsValid) continue;
				
				Texture tex = textures[i];
				if (i == highlightedI) tex.X += 4;
				tex.Render(gfx);
			}
		}
		
		public override void Dispose() {
			for (int i = 0; i < namesCount; i++) {
				Texture tex = textures[i];
				gfx.DeleteTexture(ref tex);
				textures[i] = tex;
			}
			
			overview.Dispose();			
			game.EntityEvents.TabListEntryAdded -= TabEntryAdded;
			game.EntityEvents.TabListEntryChanged -= TabEntryChanged;
			game.EntityEvents.TabListEntryRemoved -= TabEntryRemoved;
		}
		
		int HighlightedName(int mouseX, int mouseY) {
			for (int i = 0; i < namesCount; i++) {
				if (!textures[i].IsValid || IDs[i] == groupNameID) continue;
				
				Texture t = textures[i];
				if (GuiElement.Contains(t.X, t.Y, t.Width, t.Height, mouseX, mouseY)) return i;
			}
			return -1;
		}
		
		public string GetNameUnder(int mouseX, int mouseY) {
			int i = HighlightedName(mouseX, mouseY);
			if (i == -1) return null;
			return TabList.Entries[IDs[i]].PlayerName;
		}
		
		
		void RepositionColumns() {
			int width = 0, centreX = game.Width / 2;
			yHeight = 0;
			
			int columns = Utils.CeilDiv(namesCount, namesPerColumn);
			for (int col = 0; col < columns; col++) {
				width += GetColumnWidth(col);
				yHeight = Math.Max(GetColumnHeight(col), yHeight);
			}
			
			if (width < 480) width = 480;		
			xMin = centreX - width / 2;
			xMax = centreX + width / 2;
			
			int x = xMin, y = game.Height / 2 - yHeight / 2;
			for (int col = 0; col < columns; col++) {
				SetColumnPos(col, x, y);
				x += GetColumnWidth(col);
			}
		}
		
		void UpdateTableDimensions() {
			int width = xMax - xMin;
			X = xMin - boundsSize;
			Y = game.Height / 2 - yHeight / 2 - boundsSize;
			Width = width + boundsSize * 2;
			Height = yHeight + boundsSize * 2;
		}
		
		int GetColumnWidth(int column) {
			int i = column * namesPerColumn;
			int maxWidth = 0;
			int maxIndex = Math.Min(namesCount, i + namesPerColumn);
			
			for (; i < maxIndex; i++)
				maxWidth = Math.Max(maxWidth, textures[i].Width);
			return maxWidth + columnPadding + elementOffset;
		}
		
		int GetColumnHeight(int column) {
			int i = column * namesPerColumn;
			int total = 0;
			int maxIndex = Math.Min(namesCount, i + namesPerColumn);
			
			for (; i < maxIndex; i++)
				total += textures[i].Height + 1;
			return total;
		}
		
		void SetColumnPos(int column, int x, int y) {
			int i = column * namesPerColumn;
			int maxIndex = Math.Min(namesCount, i + namesPerColumn);
			
			for (; i < maxIndex; i++) {
				Texture tex = textures[i];
				tex.X1 = x; tex.Y1 = y - 10;
				
				y += tex.Height + 1;
				// offset player names a bit, compared to group name
				if (!classic && IDs[i] != groupNameID) {
					tex.X1 += elementOffset;
				}
				textures[i] = tex;
			}
		}
		
		public override void Reposition() {
			YOffset = -Math.Max(0, game.Height / 4 - Height / 2);
			int oldX = X, oldY = Y;
			base.Reposition();
			
			for (int i = 0; i < namesCount; i++) {
				textures[i].X1 += X - oldX;
				textures[i].Y1 += Y - oldY;
			}
		}
		
		
		void AddName(byte id, int index) {
			string name = TabList.Entries[id].ListName;
			Texture tex = DrawName(name);
			
			// insert at end of list
			if (index == -1) { index = namesCount; namesCount++; }
			IDs[index] = id;
			textures[index] = tex;
		}
		
		void TabEntryAdded(object sender, IdEventArgs e) {
			AddName(e.Id, -1);
			SortAndReposition();
		}
		
		void TabEntryChanged(object sender, IdEventArgs e) {
			for (int i = 0; i < namesCount; i++) {
				if (IDs[i] != e.Id) continue;
				
				Texture tex = textures[i];
				gfx.DeleteTexture(ref tex);
				AddName(e.Id, i);
				SortAndReposition();
				return;
			}
		}
		
		void TabEntryRemoved(object sender, IdEventArgs e) {
			for (int i = 0; i < namesCount; i++) {
				if (IDs[i] != e.Id) continue;
				DeleteAt(i);
				SortAndReposition();
				return;
			}
		}
		
		void DeleteAt(int i) {
			Texture tex = textures[i];
			gfx.DeleteTexture(ref tex);
			
			for (; i < namesCount - 1; i++) {
				IDs[i] = IDs[i + 1];
				textures[i] = textures[i + 1];
			}
			
			IDs[namesCount] = 0;
			textures[namesCount] = default(Texture);
			namesCount--;
		}
		
		
		void SortAndReposition() {
			SortEntries();
			RepositionColumns();
			UpdateTableDimensions();
			Reposition();
		}
		
		Texture DrawName(string name) {
			if (game.PureClassic) name = Utils.StripColours(name);
			
			DrawTextArgs args = new DrawTextArgs(name, font, !classic);
			Texture tex = game.Drawer2D.MakeTextTexture(ref args, 0, 0);
			game.Drawer2D.ReducePadding(ref tex, Utils.Floor(font.Size), 3);
			return tex;
		}
		
		IComparer<ushort> comparer = new PlayerComparer();
		class PlayerComparer : IComparer<ushort> {
			public int Compare(ushort x, ushort y) {
				byte xRank = TabList.Entries[x].GroupRank;
				byte yRank = TabList.Entries[y].GroupRank;
				int rankOrder = xRank.CompareTo(yRank);
				if (rankOrder != 0) return rankOrder;
				
				string xName = TabList.Entries[x].ListNameColourless;
				string yName = TabList.Entries[y].ListNameColourless;
				return xName.CompareTo(yName);
			}
		}
		
		IComparer<ushort> grpComparer = new GroupComparer();
		class GroupComparer : IComparer<ushort> {
			public int Compare(ushort x, ushort y) {
				string xGroup = TabList.Entries[x].Group;
				string yGroup = TabList.Entries[y].Group;
				return xGroup.CompareTo(yGroup);
			}
		}
		
		void SortEntries() {
			if (namesCount == 0) return;
			if (classic) {
				Array.Sort(IDs, textures, 0, namesCount, comparer);
				return;
			}
			
			// Sort the list into groups
			for (int i = 0; i < namesCount; i++) {
				if (IDs[i] == groupNameID) DeleteGroup(ref i);
			}
			Array.Sort(IDs, textures, 0, namesCount, grpComparer);
			
			// Sort the entries in each group
			int index = 0;
			while (index < namesCount) {
				ushort id = IDs[index];
				AddGroup(id, ref index);
				int count = GetGroupCount(id, index);
				Array.Sort(IDs, textures, index, count, comparer);
				index += count;
			}
		}
		
		void DeleteGroup(ref int i) { DeleteAt(i); i--; }
		void AddGroup(ushort id, ref int index) {
			for (int i = IDs.Length - 1; i > index; i--) {
				IDs[i] = IDs[i - 1];
				textures[i] = textures[i - 1];
			}
			
			IDs[index] = groupNameID;
			string group = TabList.Entries[id].Group;
			textures[index] = DrawName(group);
			
			index++;
			namesCount++;
		}
		
		int GetGroupCount(ushort id, int idx) {
			string group = TabList.Entries[id].Group;
			int count = 0;
			while (idx < namesCount && TabList.Entries[IDs[idx]].Group == group) {
				idx++; count++;
			}
			return count;
		}
	}
}