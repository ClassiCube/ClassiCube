#ifndef CS_NETWORKENUMS_H
#define CS_NETWORKSENUM_H
#include "Typedefs.h"

/* Network packet opcodes. */
typedef UInt8 Opcode;
#define Opcode_Handshake 0
#define Opcode_Ping 1
#define Opcode_LevelInit 2
#define Opcode_LevelDataChunk 3
#define Opcode_LevelFinalise 4
#define Opcode_SetBlockClient 5
#define Opcode_SetBlock 6
#define Opcode_AddEntity 7
#define Opcode_EntityTeleport 8
#define Opcode_RelPosAndOrientationUpdate 9
#define Opcode_RelPosUpdate 10
#define Opcode_OrientationUpdate 11
#define Opcode_RemoveEntity 12
#define Opcode_Message 13
#define Opcode_Kick 14
#define Opcode_SetPermission 15

#define Opcode_CpeExtInfo 16
#define Opcode_CpeExtEntry 17
#define Opcode_CpeSetClickDistance 18
#define Opcode_CpeCustomBlockSupportLevel 19
#define Opcode_CpeHoldThis 20
#define Opcode_CpeSetTextHotkey 21
#define Opcode_CpeExtAddPlayerName 22
#define Opcode_CpeExtAddEntity 23
#define Opcode_CpeExtRemovePlayerName 24
#define Opcode_CpeEnvColours 25
#define Opcode_CpeMakeSelection 26
#define Opcode_CpeRemoveSelection 27
#define Opcode_CpeSetBlockPermission 28
#define Opcode_CpeChangeModel 29
#define Opcode_CpeEnvSetMapApperance 30
#define Opcode_CpeEnvWeatherType 31
#define Opcode_CpeHackControl 32
#define Opcode_CpeExtAddEntity2 33
#define Opcode_CpePlayerClick 34
#define Opcode_CpeDefineBlock 35
#define Opcode_CpeRemoveBlockDefinition 36
#define Opcode_CpeDefineBlockExt 37
#define Opcode_CpeBulkBlockUpdate 38
#define Opcode_CpeSetTextColor 39
#define Opcode_CpeSetMapEnvUrl 40
#define Opcode_CpeSetMapEnvProperty 41
#define Opcode_CpeSetEntityProperty 42


/* Chat message types.*/
typedef UInt8 MessageType;
/* CPE message types */

#define MessageType_Normal 0
#define MessageType_Status1 1
#define MessageType_Status2 2
#define MessageType_Status3 3
#define MessageType_BottomRight1 11
#define MessageType_BottomRight2 12
#define MessageType_BottomRight3 13
#define MessageType_Announcement 100

/* client defined message types */

#define MessageType_ClientStatus1 256
#define MessageType_ClientStatus2 257
/* Cuboid messages*/
#define MessageType_ClientStatus3 258
/* Clipboard invalid character */
#define MessageType_ClientStatus4 259
/* Tab list matching names*/
#define MessageType_ClientStatus5 260
/* No LongerMessages warning*/
#define MessageType_ClientStatus6 261 
#endif