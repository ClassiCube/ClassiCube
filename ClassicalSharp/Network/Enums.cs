// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Network {
	
	public static class Opcode {
		public const byte Handshake = 0;
		public const byte Ping = 1;
		public const byte LevelInit = 2;
		public const byte LevelDataChunk = 3;
		public const byte LevelFinalise = 4;
		public const byte SetBlockClient = 5;
		public const byte SetBlock = 6;
		public const byte AddEntity = 7;
		public const byte EntityTeleport = 8;
		public const byte RelPosAndOrientationUpdate = 9;
		public const byte RelPosUpdate = 10;
		public const byte OrientationUpdate = 11;
		public const byte RemoveEntity = 12;
		public const byte Message = 13;
		public const byte Kick = 14;
		public const byte SetPermission = 15;

		public const byte CpeExtInfo = 16;
		public const byte CpeExtEntry = 17;
		public const byte CpeSetClickDistance = 18;
		public const byte CpeCustomBlockSupportLevel = 19;
		public const byte CpeHoldThis = 20;
		public const byte CpeSetTextHotkey = 21;
		public const byte CpeExtAddPlayerName = 22;
		public const byte CpeExtAddEntity = 23;
		public const byte CpeExtRemovePlayerName = 24;
		public const byte CpeEnvColours = 25;
		public const byte CpeMakeSelection = 26;
		public const byte CpeRemoveSelection = 27;
		public const byte CpeSetBlockPermission = 28;
		public const byte CpeChangeModel = 29;
		public const byte CpeEnvSetMapApperance = 30;
		public const byte CpeEnvWeatherType = 31;
		public const byte CpeHackControl = 32;
		public const byte CpeExtAddEntity2 = 33;
		public const byte CpePlayerClick = 34;
		public const byte CpeDefineBlock = 35;
		public const byte CpeUndefineBlock = 36;
		public const byte CpeDefineBlockExt = 37;
		public const byte CpeBulkBlockUpdate = 38;
		public const byte CpeSetTextColor = 39;
		public const byte CpeSetMapEnvUrl = 40;
		public const byte CpeSetMapEnvProperty = 41;
		public const byte CpeSetEntityProperty = 42;
		public const byte CpeTwoWayPing = 43;
		public const byte CpeSetInventoryOrder = 44;
	}
}

namespace ClassicalSharp {
	
	public enum MessageType {
		Normal = 0,
		Status1 = 1,
		Status2 = 2,
		Status3 = 3,
		BottomRight1 = 11,
		BottomRight2 = 12,
		BottomRight3 = 13,
		Announcement = 100,
		
		// client defined message ids
		ClientStatus1 = 256, // cuboid messages
		ClientStatus2 = 257, // clipboard invalid characters
		ClientStatus3 = 258, // tab list matching names
	}
	
	public enum BlockFace { XMax, XMin, YMax, YMin, ZMax, ZMin, }
}
