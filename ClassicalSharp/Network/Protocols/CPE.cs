// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Hotkeys;
using ClassicalSharp.Map;
using ClassicalSharp.Textures;
using OpenTK.Input;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Network.Protocols {

	/// <summary> Implements the packets for classic protocol extension. </summary>
	public sealed class CPEProtocol : IProtocol {
		
		public CPEProtocol(Game game) : base(game) { }
		
		public override void Init() { Reset(); }
		
		public override void Reset() {
			if (!game.UseCPE) return;
			net.Set(Opcode.CpeExtInfo, HandleExtInfo, 67);
			net.Set(Opcode.CpeExtEntry, HandleExtEntry, 69);
			net.Set(Opcode.CpeSetClickDistance, HandleSetClickDistance, 3);
			net.Set(Opcode.CpeCustomBlockSupportLevel, HandleCustomBlockSupportLevel, 2);
			net.Set(Opcode.CpeHoldThis, HandleHoldThis, 3);
			net.Set(Opcode.CpeSetTextHotkey, HandleSetTextHotkey, 134);
			
			net.Set(Opcode.CpeExtAddPlayerName, HandleExtAddPlayerName, 196);
			net.Set(Opcode.CpeExtAddEntity, HandleExtAddEntity, 130);
			net.Set(Opcode.CpeExtRemovePlayerName, HandleExtRemovePlayerName, 3);
			
			net.Set(Opcode.CpeEnvColours, HandleEnvColours, 8);
			net.Set(Opcode.CpeMakeSelection, HandleMakeSelection, 86);
			net.Set(Opcode.CpeRemoveSelection, HandleRemoveSelection, 2);
			net.Set(Opcode.CpeSetBlockPermission, HandleSetBlockPermission, 4);
			net.Set(Opcode.CpeChangeModel, HandleChangeModel, 66);
			net.Set(Opcode.CpeEnvSetMapApperance, HandleEnvSetMapAppearance, 69);
			net.Set(Opcode.CpeEnvWeatherType, HandleEnvWeatherType, 2);
			net.Set(Opcode.CpeHackControl, HandleHackControl, 8);
			net.Set(Opcode.CpeExtAddEntity2, HandleExtAddEntity2, 138);
			
			net.Set(Opcode.CpeBulkBlockUpdate, HandleBulkBlockUpdate, 1282);
			net.Set(Opcode.CpeSetTextColor, HandleSetTextColor, 6);
			net.Set(Opcode.CpeSetMapEnvUrl, HandleSetMapEnvUrl, 65);
			net.Set(Opcode.CpeSetMapEnvProperty, HandleSetMapEnvProperty, 6);
			net.Set(Opcode.CpeSetEntityProperty, HandleSetEntityProperty, 7);
			net.Set(Opcode.CpeTwoWayPing, HandleTwoWayPing, 4);
			net.Set(Opcode.CpeSetInventoryOrder, HandleSetInventoryOrder, 3);
		}
		
		#region Read
		void HandleExtInfo() {
			string appName = reader.ReadString();
			game.Chat.Add("Server software: " + appName);
			if (Utils.CaselessStarts(appName, "D3 server"))
				net.cpeData.needD3Fix = true;
			
			// Workaround for MCGalaxy that send ExtEntry sync but ExtInfoAsync. This means
			// ExtEntry may sometimes arrive before ExtInfo, and thus we have to use += instead of =
			net.cpeData.ServerExtensionsCount += reader.ReadInt16();
			SendCpeExtInfoReply();
		}
		
		void HandleExtEntry() {
			string extName = reader.ReadString();
			int extVersion = reader.ReadInt32();
			Utils.LogDebug("cpe ext: {0}, {1}", extName, extVersion);
			
			net.cpeData.HandleEntry(extName, extVersion, net);
			SendCpeExtInfoReply();
		}
		
		void HandleSetClickDistance() {
			game.LocalPlayer.ReachDistance = reader.ReadUInt16() / 32f;
		}
		
		void HandleCustomBlockSupportLevel() {
			byte supportLevel = reader.ReadUInt8();
			WriteCustomBlockSupportLevel(1);
			net.SendPacket();
			game.UseCPEBlocks = true;

			if (supportLevel == 1) {
				game.Events.RaiseBlockPermissionsChanged();
			} else {
				Utils.LogDebug("Server's block support level is {0}, this client only supports level 1.", supportLevel);
				Utils.LogDebug("You won't be able to see or use blocks from levels above level 1");
			}
		}
		
		void HandleHoldThis() {
			BlockID block = reader.ReadUInt8();
			if (block == Block.Air) block = Block.Invalid;
			bool canChange = reader.ReadUInt8() == 0;
			
			game.Inventory.CanChangeHeldBlock = true;
			game.Inventory.Selected = block;
			game.Inventory.CanChangeHeldBlock = canChange;
		}
		
		void HandleSetTextHotkey() {
			string label = reader.ReadString();
			string action = reader.ReadString();
			int keyCode = reader.ReadInt32();
			byte keyMods = reader.ReadUInt8();
			
			#if !ANDROID
			if (keyCode < 0 || keyCode > 255) return;
			Key key = LwjglToKey.Map[keyCode];
			if (key == Key.Unknown) return;
			
			Utils.LogDebug("CPE Hotkey added: " + key + "," + keyMods + " : " + action);
			if (action == "") {
				game.Input.Hotkeys.RemoveHotkey(key, keyMods);
			} else if (action[action.Length - 1] == '◙') { // graphical form of \n
				action = action.Substring(0, action.Length - 1);
				game.Input.Hotkeys.AddHotkey(key, keyMods, action, false);
			} else { // more input needed by user
				game.Input.Hotkeys.AddHotkey(key, keyMods, action, true);
			}
			#endif
		}
		
		void HandleExtAddPlayerName() {
			int id = reader.ReadInt16() & 0xFF;
			string playerName = Utils.StripColours(reader.ReadString());
			playerName = Utils.RemoveEndPlus(playerName);
			string listName = reader.ReadString();
			listName = Utils.RemoveEndPlus(listName);
			string groupName = reader.ReadString();
			byte groupRank = reader.ReadUInt8();
			
			// Some server software will declare they support ExtPlayerList, but send AddEntity then AddPlayerName
			// we need to workaround this case by removing all the tab names we added for the AddEntity packets
			net.DisableAddEntityHack();
			net.AddTablistEntry((byte)id, playerName, listName, groupName, groupRank);
		}
		
		void HandleExtAddEntity() {
			byte id = reader.ReadUInt8();
			string displayName = reader.ReadString();
			string skinName = reader.ReadString();
			net.CheckName(id, ref displayName, ref skinName);
			net.AddEntity(id, displayName, skinName, false);
		}
		
		void HandleExtRemovePlayerName() {
			int id = reader.ReadInt16() & 0xFF;
			net.RemoveTablistEntry((byte)id);
		}
		
		void HandleMakeSelection() {
			byte selectionId = reader.ReadUInt8();
			string label = reader.ReadString();
			Vector3I start, end;
			
			start.X = reader.ReadInt16();
			start.Y = reader.ReadInt16();
			start.Z = reader.ReadInt16();
			
			end.X = reader.ReadInt16();
			end.Y = reader.ReadInt16();
			end.Z = reader.ReadInt16();
			
			byte r = (byte)reader.ReadInt16();
			byte g = (byte)reader.ReadInt16();
			byte b = (byte)reader.ReadInt16();
			byte a = (byte)reader.ReadInt16();
			
			Vector3I p1 = Vector3I.Min(start, end), p2 = Vector3I.Max(start, end);
			FastColour col = new FastColour(r, g, b, a);
			game.SelectionManager.AddSelection(selectionId, p1, p2, col);
		}
		
		void HandleRemoveSelection() {
			byte selectionId = reader.ReadUInt8();
			game.SelectionManager.RemoveSelection(selectionId);
		}
		
		void HandleEnvColours() {
			byte variable = reader.ReadUInt8();
			short red = reader.ReadInt16();
			short green = reader.ReadInt16();
			short blue = reader.ReadInt16();
			bool invalid = red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255;
			FastColour col = new FastColour(red, green, blue);

			if (variable == 0) {
				game.World.Env.SetSkyColour(invalid ? WorldEnv.DefaultSkyColour : col);
			} else if (variable == 1) {
				game.World.Env.SetCloudsColour(invalid ? WorldEnv.DefaultCloudsColour : col);
			} else if (variable == 2) {
				game.World.Env.SetFogColour(invalid ? WorldEnv.DefaultFogColour : col);
			} else if (variable == 3) {
				game.World.Env.SetShadowlight(invalid ? WorldEnv.DefaultShadowlight : col);
			} else if (variable == 4) {
				game.World.Env.SetSunlight(invalid ? WorldEnv.DefaultSunlight : col);
			}
		}
		
		void HandleSetBlockPermission() {
			byte blockId = reader.ReadUInt8();
			bool canPlace = reader.ReadUInt8() != 0;
			bool canDelete = reader.ReadUInt8() != 0;
			Inventory inv = game.Inventory;
			
			if (blockId == 0) {
				int count = game.UseCPEBlocks ? Block.CpeCount : Block.OriginalCount;
				for (int i = 1; i < count; i++) {
					BlockInfo.CanPlace[i] = canPlace;
					BlockInfo.CanDelete[i] = canDelete;
				}
			} else {
				BlockInfo.CanPlace[blockId] = canPlace;
				BlockInfo.CanDelete[blockId] = canDelete;
			}
			game.Events.RaiseBlockPermissionsChanged();
		}
		
		void HandleChangeModel() {
			byte id = reader.ReadUInt8();
			string modelName = Utils.ToLower(reader.ReadString());
			Entity entity = game.Entities.List[id];
			if (entity != null) entity.SetModel(modelName);
		}
		
		void HandleEnvSetMapAppearance() {
			HandleSetMapEnvUrl();
			game.World.Env.SetSidesBlock(reader.ReadUInt8());
			game.World.Env.SetEdgeBlock(reader.ReadUInt8());
			game.World.Env.SetEdgeLevel(reader.ReadInt16());
			if (net.cpeData.envMapVer == 1) return;
			
			// Version 2			
			game.World.Env.SetCloudsLevel(reader.ReadInt16());
			short maxViewDist = reader.ReadInt16();
			game.MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
			game.SetViewDistance(game.UserViewDistance, false);
		}
		
		void HandleEnvWeatherType() {
			game.World.Env.SetWeather((Weather)reader.ReadUInt8());
		}
		
		void HandleHackControl() {
			LocalPlayer p = game.LocalPlayer;
			p.Hacks.CanFly = reader.ReadUInt8() != 0;
			p.Hacks.CanNoclip = reader.ReadUInt8() != 0;
			p.Hacks.CanSpeed = reader.ReadUInt8() != 0;
			p.Hacks.CanRespawn = reader.ReadUInt8() != 0;
			p.Hacks.CanUseThirdPersonCamera = reader.ReadUInt8() != 0;
			p.CheckHacksConsistency();
			
			ushort jumpHeight = reader.ReadUInt16();
			if (jumpHeight == ushort.MaxValue) { // special value of -1 to reset default
				p.physics.jumpVel = p.Hacks.CanJumpHigher ? p.physics.userJumpVel : 0.42f;
			} else {
				p.physics.CalculateJumpVelocity(false, jumpHeight / 32f);
			}
			
			p.physics.serverJumpVel = p.physics.jumpVel;
			game.Events.RaiseHackPermissionsChanged();
		}
		
		void HandleExtAddEntity2() {
			byte id = reader.ReadUInt8();
			string displayName = reader.ReadString();
			string skinName = reader.ReadString();
			net.CheckName(id, ref displayName, ref skinName);
			net.AddEntity(id, displayName, skinName, true);
		}
		
		const int bulkCount = 256;
		unsafe void HandleBulkBlockUpdate() {
			int count = reader.ReadUInt8() + 1;
			if (game.World.blocks == null) {
				#if DEBUG_BLOCKS
				Utils.LogDebug("Server tried to update a block while still sending us the map!");
				#endif
				reader.Skip(bulkCount * (sizeof(int) + 1));
				return;
			}
			int* indices = stackalloc int[bulkCount];
			for (int i = 0; i < count; i++)
				indices[i] = reader.ReadInt32();
			reader.Skip((bulkCount - count) * sizeof(int));
			
			for (int i = 0; i < count; i++) {
				BlockID block = reader.ReadUInt8();
				Vector3I coords = game.World.GetCoords(indices[i]);
				
				if (coords.X < 0) {
					#if DEBUG_BLOCKS
					Utils.LogDebug("Server tried to update a block at an invalid position!");
					#endif
					continue;
				}
				game.UpdateBlock(coords.X, coords.Y, coords.Z, block);
			}
			reader.Skip(bulkCount - count);
		}
		
		void HandleSetTextColor() {
			FastColour col = new FastColour(reader.ReadUInt8(), reader.ReadUInt8(),
			                                reader.ReadUInt8(), reader.ReadUInt8());
			byte code = reader.ReadUInt8();
			
			if (code <= ' ' || code > '~') return; // Control chars, space, extended chars cannot be used
			if (code == '%' || code == '&') return; // colour code signifiers cannot be used
			
			game.Drawer2D.Colours[code] = col;
			game.Events.RaiseColourCodeChanged((char)code);
		}
		
		void HandleSetMapEnvUrl() {
			string url = reader.ReadString();
			if (!game.UseServerTextures) return;
			
			if (url == "") {
				TexturePack.ExtractDefault(game);
			} else if (Utils.IsUrlPrefix(url, 0)) {
				net.RetrieveTexturePack(url);
			}
			Utils.LogDebug("Image url: " + url);
		}
		
		void HandleSetMapEnvProperty() {
			byte type = reader.ReadUInt8();
			int value = reader.ReadInt32();
			WorldEnv env = game.World.Env;
			Utils.Clamp(ref value, short.MinValue, short.MaxValue);
			
			switch (type) {
				case 0:
					Utils.Clamp(ref value, byte.MinValue, byte.MaxValue);
					env.SetSidesBlock((byte)value); break;
				case 1:
					Utils.Clamp(ref value, byte.MinValue, byte.MaxValue);
					env.SetEdgeBlock((byte)value); break;
				case 2:
					env.SetEdgeLevel(value); break;
				case 3:
					env.SetCloudsLevel(value); break;
				case 4:
					game.MaxViewDistance = value <= 0 ? 32768 : value;
					game.SetViewDistance(game.UserViewDistance, false); break;
				case 5:
					env.SetCloudsSpeed(value / 256f); break;
				case 6:
					env.SetWeatherSpeed(value / 256f); break;
				case 7:
					Utils.Clamp(ref value, byte.MinValue, byte.MaxValue);
					env.SetWeatherFade(value / 128f); break;
				case 8:
					env.SetExpFog(value != 0); break;
				case 9:
					env.SetSidesOffset(value); break;
			}
		}
		
		void HandleSetEntityProperty() {
			byte id = reader.ReadUInt8();
			byte type = reader.ReadUInt8();
			int value = reader.ReadInt32();
			
			Entity entity = game.Entities.List[id];
			if (entity == null) return;
			LocationUpdate update = LocationUpdate.Empty();
			
			switch (type) {
				case 0:
					update.RotX = LocationUpdate.Clamp(value); break;
				case 1:
					update.RotY = LocationUpdate.Clamp(value); break;
				case 2:
					update.RotZ = LocationUpdate.Clamp(value); break;
					
				case 3:
				case 4:
				case 5:
					float scale = value / 1000.0f;
					Utils.Clamp(ref scale, 0.01f, entity.Model.MaxScale);
					if (type == 3) entity.ModelScale.X = scale;
					if (type == 4) entity.ModelScale.Y = scale;
					if (type == 5) entity.ModelScale.Z = scale;
					
					entity.UpdateModelBounds();
					return;
				default:
					return;
			}
			entity.SetLocation(update, true);
		}
		
		void HandleTwoWayPing() {
			bool serverToClient = reader.ReadUInt8() != 0;
			ushort data = reader.ReadUInt16();
			if (!serverToClient) { PingList.Update(data); return; }
			
			WriteTwoWayPing(true, data); // server to client reply
			net.SendPacket();
		}
		
		void HandleSetInventoryOrder() {
			byte block = reader.ReadUInt8();
			byte order = reader.ReadUInt8();
			
			game.Inventory.Remove(block);
			if (order != Block.Invalid) {
				game.Inventory.Insert(order, block);
			}
		}					
		
		#endregion
		
		#region Write
		
		internal void WritePlayerClick(MouseButton button, bool buttonDown,
		                              byte targetId, PickedPos pos) {
			Player p = game.LocalPlayer;
			writer.WriteUInt8((byte)Opcode.CpePlayerClick);
			writer.WriteUInt8((byte)button);
			writer.WriteUInt8(buttonDown ? (byte)0 : (byte)1);
			writer.WriteInt16((short)Utils.DegreesToPacked(p.HeadY, 65536));
			writer.WriteInt16((short)Utils.DegreesToPacked(p.HeadX, 65536));
			
			writer.WriteUInt8(targetId);
			writer.WriteInt16((short)pos.BlockPos.X);
			writer.WriteInt16((short)pos.BlockPos.Y);
			writer.WriteInt16((short)pos.BlockPos.Z);
			writer.WriteUInt8((byte)pos.Face);
		}
		
		internal void WriteExtInfo(string appName, int extensionsCount) {
			writer.WriteUInt8((byte)Opcode.CpeExtInfo);
			writer.WriteString(appName);
			writer.WriteInt16((short)extensionsCount);
		}
		
		internal void WriteExtEntry(string extensionName, int extensionVersion) {
			writer.WriteUInt8((byte)Opcode.CpeExtEntry);
			writer.WriteString(extensionName);
			writer.WriteInt32(extensionVersion);
		}
		
		internal void WriteCustomBlockSupportLevel(byte version) {
			writer.WriteUInt8((byte)Opcode.CpeCustomBlockSupportLevel);
			writer.WriteUInt8(version);
		}
		
		internal void WriteTwoWayPing(bool serverToClient, ushort data) {
			writer.WriteUInt8((byte)Opcode.CpeTwoWayPing);
			writer.WriteUInt8((byte)(serverToClient ? 1 : 0));
			writer.WriteInt16((short)data);			
		}
		
		void SendCpeExtInfoReply() {
			if (net.cpeData.ServerExtensionsCount != 0) return;
			string[] clientExts = CPESupport.ClientExtensions;
			int count = clientExts.Length;
			if (!game.UseCustomBlocks) count -= 2;
			
			WriteExtInfo(net.AppName, count);
			net.SendPacket();
			for (int i = 0; i < clientExts.Length; i++) {
				string name = clientExts[i];
				int ver = 1;
				if (name == "ExtPlayerList") ver = 2;
				if (name == "EnvMapAppearance") ver = net.cpeData.envMapVer;
				if (name == "BlockDefinitionsExt") ver = net.cpeData.blockDefsExtVer;
				
				if (!game.UseCustomBlocks && name.StartsWith("BlockDefinitions")) continue;
				
				WriteExtEntry(name, ver);
				net.SendPacket();
			}
		}
		#endregion
	}
}
