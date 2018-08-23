// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using BlockID = System.UInt16;

namespace ClassicalSharp.Network {

	public sealed class CPESupport {

		internal int ServerExtensionsCount;
		internal bool sendHeldBlock, useMessageTypes;
		internal int envMapVer = 2, blockDefsExtVer = 2;
		internal bool needD3Fix, extEntityPos, twoWayPing, blockPerms, fastMap, extTexs;
		public Game game;
		
		public void Reset() {
			ServerExtensionsCount = 0;
			sendHeldBlock = false; useMessageTypes = false;
			envMapVer = 2; blockDefsExtVer = 2;
			needD3Fix = false; extEntityPos = false; twoWayPing = false; fastMap = false;
			extTexs = false;
			game.SupportsCPEBlocks = false;
		}
		
		public void HandleEntry(string ext, int version, NetworkProcessor net) {
			ServerExtensionsCount--;
			
			if (Utils.CaselessEq(ext, "HeldBlock")) {
				sendHeldBlock = true;
			} else if (Utils.CaselessEq(ext, "MessageTypes")) {
				useMessageTypes = true;
			} else if (Utils.CaselessEq(ext, "ExtPlayerList")) {
				net.UsingExtPlayerList = true;
			} else if (Utils.CaselessEq(ext, "BlockPermissions")) {
				blockPerms = true;
			} else if (Utils.CaselessEq(ext, "PlayerClick")) {
				net.UsingPlayerClick = true;
			} else if (Utils.CaselessEq(ext, "EnvMapAppearance")) {
				envMapVer = version;
				if (version == 1) return;
				net.packetSizes[Opcode.CpeEnvSetMapApperance] += 4;
			} else if (Utils.CaselessEq(ext, "LongerMessages")) {
				net.SupportsPartialMessages = true;
			} else if (Utils.CaselessEq(ext, "FullCP437")) {
				net.SupportsFullCP437 = true;
			} else if (Utils.CaselessEq(ext, "BlockDefinitionsExt")) {
				blockDefsExtVer = version;
				if (version == 1) return;
				net.packetSizes[Opcode.CpeDefineBlockExt] += 3;
			} else if (Utils.CaselessEq(ext, "ExtEntityPositions")) {
				extEntityPos = true;
				net.packetSizes[Opcode.EntityTeleport] += 6;
				net.packetSizes[Opcode.AddEntity] += 6;
				net.packetSizes[Opcode.CpeExtAddEntity2] += 6;
				
				net.reader.ExtendedPositions = true;
				net.writer.ExtendedPositions = true;
			} else if (Utils.CaselessEq(ext, "TwoWayPing")) {
				twoWayPing = true;
			} else if (Utils.CaselessEq(ext, "FastMap")) {
				net.packetSizes[Opcode.LevelInit] += 4;
				fastMap = true;
			} else if (Utils.CaselessEq(ext, "ExtendedTextures")) {
				net.packetSizes[Opcode.CpeDefineBlock] += 3;
				net.packetSizes[Opcode.CpeDefineBlockExt] += 6;
				extTexs = true;
			}
			#if !ONLY_8BIT
			else if (Utils.CaselessEq(ext, "ExtendedBlocks")) {
				if (!game.AllowCustomBlocks) return;
				net.packetSizes[Opcode.SetBlock] += 1;
				net.packetSizes[Opcode.CpeHoldThis] += 1;
				net.packetSizes[Opcode.CpeDefineBlock] += 1;
				net.packetSizes[Opcode.CpeSetBlockPermission] += 1;
				net.packetSizes[Opcode.CpeUndefineBlock] += 1;
				net.packetSizes[Opcode.CpeDefineBlockExt] += 1;
				net.packetSizes[Opcode.CpeSetInventoryOrder] += 2;
				net.packetSizes[Opcode.CpeBulkBlockUpdate] += 256 / 4;
				
				net.reader.ExtendedBlocks = true;
				net.writer.ExtendedBlocks = true;
				if (BlockInfo.Count < 768) {
					BlockInfo.Allocate(768);
					BlockInfo.Reset();
					game.Inventory.Map = new BlockID[768];
					game.Inventory.SetDefaultMapping();
				}
			}
			#endif
		}
		
		public static string[] ClientExtensions = new string[] {
			"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
			"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
			"EnvWeatherType", "MessageTypes", "HackControl", "PlayerClick", "FullCP437", "LongerMessages", 
			"BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors", "EnvMapAspect", 
			"EntityProperty", "ExtEntityPositions", "TwoWayPing", "InventoryOrder", "InstantMOTD", "FastMap",
			"ExtendedTextures",
			#if !ONLY_8BIT
			"ExtendedBlocks",
			#endif
		};
	}
}
