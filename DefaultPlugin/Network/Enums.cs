using System;

namespace DefaultPlugin.Network {
	
	public enum PacketId {
		ServerIdentification = 0,
		Ping = 1,
		LevelInitialise = 2,
		LevelDataChunk = 3,
		LevelFinalise = 4,
		SetBlockClient = 5,
		SetBlock = 6,
		AddEntity = 7,
		EntityTeleport = 8,
		RelPosAndOrientationUpdate = 9,
		RelPosUpdate = 10,
		OrientationUpdate = 11,
		RemoveEntity = 12,
		Message = 13,
		Kick = 14,
		SetPermission = 15,
		
		CpeExtInfo = 16,
		CpeExtEntry = 17,
		CpeSetClickDistance = 18,
		CpeCustomBlockSupportLevel = 19,
		CpeHoldThis = 20,
		CpeSetTextHotkey = 21,
		CpeExtAddPlayerName = 22,
		CpeExtAddEntity = 23,
		CpeExtRemovePlayerName = 24,
		CpeEnvColours = 25,
		CpeMakeSelection = 26,
		CpeRemoveSelection = 27,
		CpeSetBlockPermission = 28,
		CpeChangeModel = 29,
		CpeEnvSetMapApperance = 30,
		CpeEnvWeatherType = 31,
		CpeHackControl = 32,
		CpeExtAddEntity2 = 33,
	}
}
