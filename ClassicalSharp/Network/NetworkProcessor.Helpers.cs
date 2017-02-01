// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor : IServerConnection {
		
		public override void SendChat(string text, bool partial) {
			if (String.IsNullOrEmpty(text)) return;
			classic.SendChat(text, partial);
		}
		
		public override void SendPosition(Vector3 pos, float rotY, float headX) {
			classic.SendPosition(pos, rotY, headX);
		}
		
		public override void SendPlayerClick(MouseButton button, bool buttonDown, byte targetId, PickedPos pos) {
			cpe.SendPlayerClick(button, buttonDown, targetId, pos);
		}

		
		internal void CheckName(byte id, ref string displayName, ref string skinName) {
			displayName = Utils.RemoveEndPlus(displayName);
			skinName = Utils.RemoveEndPlus(skinName);
			skinName = Utils.StripColours(skinName);
			
			// Server is only allowed to change our own name colours.
			if (id != 0xFF) return;
			if (Utils.StripColours(displayName) != game.Username)
				displayName = game.Username;
			if (skinName == "")
				skinName = game.Username;
		}
		
		internal void AddEntity(byte id, string displayName, string skinName, bool readPosition) {
			if (id != 0xFF) {
				Entity oldEntity = game.Entities[id];
				if (oldEntity != null) {
					game.EntityEvents.RaiseRemoved(id);
					oldEntity.Despawn();
				}
				game.Entities[id] = new NetPlayer(displayName, skinName, game, id);
				game.EntityEvents.RaiseAdded(id);
			} else {
				game.LocalPlayer.DisplayName = displayName;
				game.LocalPlayer.SkinName = skinName;
				game.LocalPlayer.fetchedSkin = false;
				game.LocalPlayer.UpdateName();
			}
			if (!readPosition) return;
			
			classic.ReadAbsoluteLocation(id, false);
			if (id != 0xFF) return;
			LocalPlayer p = game.LocalPlayer;
			p.Spawn = p.Position;
			p.SpawnRotY = p.HeadY;
			p.SpawnHeadX = p.HeadX;
		}
		
		internal void RemoveEntity(byte id) {
			Entity entity = game.Entities[id];
			if (entity == null) return;
			
			if (id != 0xFF) {
				game.EntityEvents.RaiseRemoved(id);
				entity.Despawn();
				game.Entities[id] = null;
			}
			
			// See comment about LegendCraft in HandleAddEntity
			int mask = id >> 3, bit = 1 << (id & 0x7);
			if ((needRemoveNames[mask] & bit) == 0) return;
			game.EntityEvents.RaiseTabEntryRemoved(id);
			game.TabList.Entries[id] = null;
			needRemoveNames[mask] &= (byte)~bit;
		}
		
		internal void AddTablistEntry(byte id, string playerName, string listName,
		                              string groupName, byte groupRank) {
			TabListEntry oldInfo = game.TabList.Entries[id];
			TabListEntry info = new TabListEntry((byte)id, playerName, listName, groupName, groupRank);
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
		
		internal void UpdateLocation(byte playerId, LocationUpdate update, bool interpolate) {
			Entity entity = game.Entities[playerId];
			if (entity != null) {
				entity.SetLocation(update, interpolate);
			}
		}
	}
}