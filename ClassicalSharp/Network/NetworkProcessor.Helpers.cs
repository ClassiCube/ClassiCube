// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor : IServerConnection {
		
		internal bool addEntityHack = true;
		
		public override void SendChat(string text, bool partial) {
			if (String.IsNullOrEmpty(text)) return;
			classic.WriteChat(text, partial);
			SendPacket();
		}
		
		public override void SendPosition(Vector3 pos, float rotY, float headX) {
			classic.WritePosition(pos, rotY, headX);
			SendPacket();
		}
		
		public override void SendPlayerClick(MouseButton button, bool buttonDown, byte targetId, PickedPos pos) {
			cpe.WritePlayerClick(button, buttonDown, targetId, pos);
			SendPacket();
		}

		
		internal void CheckName(byte id, ref string displayName, ref string skinName) {
			displayName = Utils.RemoveEndPlus(displayName);
			skinName = Utils.RemoveEndPlus(skinName);
			skinName = Utils.StripColours(skinName);
			
			// Server is only allowed to change our own name colours.
			if (id != EntityList.SelfID) return;
			if (Utils.StripColours(displayName) != game.Username)
				displayName = game.Username;
			if (skinName == "")
				skinName = game.Username;
		}
		
		internal void AddEntity(byte id, string displayName, string skinName, bool readPosition) {
			if (id != EntityList.SelfID) {
				Entity oldEntity = game.Entities.List[id];
				if (oldEntity != null) game.Entities.RemoveEntity(id);

				game.Entities.List[id] = new NetPlayer(displayName, skinName, game);
				game.EntityEvents.RaiseAdded(id);
			} else {
				game.LocalPlayer.Despawn();
				// Always reset the texture here, in case other network players are using the same skin as us.
				// In that case, we don't want the fetching of new skin for us to delete the texture used by them.
				game.LocalPlayer.ResetSkin();
				game.LocalPlayer.fetchedSkin = false;
				
				game.LocalPlayer.DisplayName = displayName;
				game.LocalPlayer.SkinName = skinName;				
				game.LocalPlayer.UpdateName();
			}
			
			if (!readPosition) return;
			classic.ReadAbsoluteLocation(id, false);
			if (id != EntityList.SelfID) return;
			
			LocalPlayer p = game.LocalPlayer;
			p.Spawn = p.Position;
			p.SpawnRotY = p.HeadY;
			p.SpawnHeadX = p.HeadX;
		}
		
		internal void RemoveEntity(byte id) {
			Entity entity = game.Entities.List[id];
			if (entity == null) return;			
			if (id != EntityList.SelfID) game.Entities.RemoveEntity(id);
			
			// See comment about some servers in HandleAddEntity
			int mask = id >> 3, bit = 1 << (id & 0x7);
			if (!addEntityHack || (needRemoveNames[mask] & bit) == 0) return;
			
			RemoveTablistEntry(id);
			needRemoveNames[mask] &= (byte)~bit;
		}
		
		internal void UpdateLocation(byte playerId, LocationUpdate update, bool interpolate) {
			Entity entity = game.Entities.List[playerId];
			if (entity != null) {
				entity.SetLocation(update, interpolate);
			}
		}
		
		
		internal void AddTablistEntry(byte id, string playerName, string listName,
		                              string groupName, byte groupRank) {
			TabListEntry oldInfo = game.TabList.Entries[id];
			TabListEntry info = new TabListEntry(playerName, listName, groupName, groupRank);
			game.TabList.Entries[id] = info;
			
			if (oldInfo != null) {
				// Only redraw the tab list if something changed.
				if (info.PlayerName != oldInfo.PlayerName || info.ListName != oldInfo.ListName ||
				    info.GroupName != oldInfo.GroupName || info.GroupRank != oldInfo.GroupRank) {
					game.EntityEvents.RaiseTabListEntryChanged(id);
				}
			} else {
				game.EntityEvents.RaiseTabEntryAdded(id);
			}
		}
		
		internal void RemoveTablistEntry(byte id) {
			game.EntityEvents.RaiseTabEntryRemoved(id);
			game.TabList.Entries[id] = null;
		}
		
		internal void DisableAddEntityHack() {
			if (!addEntityHack) return;
			addEntityHack = false;
			
			for (int id = 0; id < EntityList.MaxCount; id++) {
				int mask = id >> 3, bit = 1 << (id & 0x7);
				if ((needRemoveNames[mask] & bit) == 0) continue;
				
				RemoveTablistEntry((byte)id);
				needRemoveNames[mask] &= (byte)~bit;
			}
		}
	}
}