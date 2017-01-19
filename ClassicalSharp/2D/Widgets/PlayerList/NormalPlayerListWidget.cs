// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;

namespace ClassicalSharp.Gui.Widgets {
	public class NormalPlayerListWidget : PlayerListWidget {
		
		public NormalPlayerListWidget(Game game, Font font) : base(game, font) {
			textures = new Texture[256];
		}
		
		PlayerInfo[] info = new PlayerInfo[256];
		protected class PlayerInfo {
			
			public string Name, ColouredName;
			public byte Id;
			
			public PlayerInfo(TabListEntry p) {
				ColouredName = p.PlayerName;
				Name = Utils.StripColours(p.PlayerName);
				Id = p.NameId;
			}
		}
		
		public override string GetNameUnder(int mouseX, int mouseY) {
			for (int i = 0; i < namesCount; i++) {
				Texture texture = textures[i];
				if (texture.IsValid && texture.Bounds.Contains(mouseX, mouseY))
					return Utils.StripColours(info[i].Name);
			}
			return null;
		}
		
		public override void Init() {
			base.Init();
			game.EntityEvents.TabListEntryAdded += TabEntryAdded;
			game.EntityEvents.TabListEntryRemoved += TabEntryRemoved;
			game.EntityEvents.TabListEntryChanged += TabEntryChanged;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.EntityEvents.TabListEntryAdded -= TabEntryAdded;
			game.EntityEvents.TabListEntryChanged -= TabEntryChanged;
			game.EntityEvents.TabListEntryRemoved -= TabEntryRemoved;
		}

		protected override void CreateInitialPlayerInfo() {
			TabListEntry[] entries = game.TabList.Entries;
			for (int i = 0; i < entries.Length; i++) {
				TabListEntry e = entries[i];
				if (e != null)
					AddPlayerInfo(new PlayerInfo(e), -1);
			}
		}
		
		void AddPlayerInfo(PlayerInfo pInfo, int index) {
			Texture tex = DrawName(pInfo);			
			if (index < 0) {
				info[namesCount] = pInfo;
				textures[namesCount] = tex;
				namesCount++;
			} else {
				info[index] = pInfo;
				textures[index] = tex;
			}
		}
		
		protected virtual Texture DrawName(PlayerInfo pInfo) {
			DrawTextArgs args = new DrawTextArgs(pInfo.ColouredName, font, false);
			Texture tex = game.Drawer2D.MakeChatTextTexture(ref args, 0, 0);
			game.Drawer2D.ReducePadding(ref tex, Utils.Floor(font.Size), 3);
			return tex;
		}
		
		void TabEntryAdded(object sender, IdEventArgs e) {
			AddPlayerInfo(new PlayerInfo(game.TabList.Entries[e.Id]), -1);
			SortPlayerInfo();
		}
		
		void TabEntryChanged(object sender, IdEventArgs e) {
			for (int i = 0; i < namesCount; i++) {
				PlayerInfo pInfo = info[i];
				if (pInfo.Id != e.Id) continue;
				
				Texture tex = textures[i];
				gfx.DeleteTexture(ref tex);
				AddPlayerInfo(new PlayerInfo(game.TabList.Entries[e.Id]), i);
				SortPlayerInfo();
				return;
			}
		}
		
		void TabEntryRemoved(object sender, IdEventArgs e) {
			for (int i = 0; i < namesCount; i++) {
				PlayerInfo pInfo = info[i];
				if (pInfo.Id == e.Id) {
					RemoveInfoAt(info, i);
					return;
				}
			}
		}
		
		PlayerInfoComparer comparer = new PlayerInfoComparer();
		class PlayerInfoComparer : IComparer<PlayerInfo> {
			
			public int Compare(PlayerInfo x, PlayerInfo y) {
				return x.Name.CompareTo(y.Name);
			}
		}
		
		protected override void SortInfoList() {
			Array.Sort(info, textures, 0, namesCount, comparer);
		}
	}
}