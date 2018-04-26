#ifndef CC_SERVERCONNECTION_H
#define CC_SERVERCONNECTION_H
#include "Input.h"
#include "GameStructs.h"
#include "Vectors.h"
/* Represents a connection to either a singleplayer or multiplayer server.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

enum OPCODE_ {
	OPCODE_HANDSHAKE,
	OPCODE_PING,
	OPCODE_LEVEL_INIT,
	OPCODE_LEVEL_DATA_CHUNK,
	OPCODE_LEVEL_FINALISE,
	OPCODE_SET_BLOCK_CLIENT,
	OPCODE_SET_BLOCK,
	OPCODE_ADD_ENTITY,
	OPCODE_ENTITY_TELEPORT,
	OPCODE_RELPOS_AND_ORIENTATION_UPDATE,
	OPCODE_RELPOS_UPDATE,
	OPCODE_ORIENTATION_UPDATE,
	OPCODE_REMOVE_ENTITY,
	OPCODE_MESSAGE ,
	OPCODE_KICK,
	OPCODE_SET_PERMISSION,

	OPCODE_CPE_EXT_INFO,
	OPCODE_CPE_EXT_ENTRY,
	OPCODE_CPE_SET_CLICK_DISTANCE,
	OPCODE_CPE_CUSTOM_BLOCK_LEVEL,
	OPCODE_CPE_HOLD_THIS,
	OPCODE_CPE_SET_TEXT_HOTKEY,
	OPCODE_CPE_EXT_ADD_PLAYER_NAME,
	OPCODE_CPE_EXT_ADD_ENTITY,
	OPCODE_CPE_EXT_REMOVE_PLAYER_NAME,
	OPCODE_CPE_ENV_SET_COLOR,
	OPCODE_CPE_MAKE_SELECTION,
	OPCODE_CPE_REMOVE_SELECTION,
	OPCODE_CPE_SET_BLOCK_PERMISSION,
	OPCODE_CPE_SET_MODEL,
	OPCODE_CPE_ENV_SET_MAP_APPEARANCE,
	OPCODE_CPE_ENV_SET_WEATHER,
	OPCODE_CPE_HACK_CONTROL,
	OPCODE_CPE_EXT_ADD_ENTITY2,
	OPCODE_CPE_PLAYER_CLICK,
	OPCODE_CPE_DEFINE_BLOCK,
	OPCODE_CPE_UNDEFINE_BLOCK,
	OPCODE_CPE_DEFINE_BLOCK_EXT,
	OPCODE_CPE_BULK_BLOCK_UPDATE,
	OPCODE_CPE_SET_TEXT_COLOR,
	OPCODE_CPE_ENV_SET_MAP_URL,
	OPCODE_CPE_ENV_SET_MAP_PROPERTY,
	OPCODE_CPE_SET_ENTITY_PROPERTY,
	OPCODE_CPE_TWO_WAY_PING,
	OPCODE_CPE_SET_INVENTORY_ORDER,
};

typedef struct PickedPos_ PickedPos;
typedef struct Stream_ Stream;

UInt16 PingList_NextPingData(void);
void PingList_Update(UInt16 data);
Int32 PingList_AveragePingMs(void);

bool ServerConnection_IsSinglePlayer;
extern String ServerConnection_ServerName;
extern String ServerConnection_ServerMOTD;
extern String ServerConnection_AppName;

void (*ServerConnection_Connect)(STRING_PURE String* ip, Int32 port);
void (*ServerConnection_SendChat)(STRING_PURE String* text);
void (*ServerConnection_SendPosition)(Vector3 pos, Real32 rotY, Real32 headX);
void (*ServerConnection_SendPlayerClick)(MouseButton button, bool isDown, EntityID targetId, PickedPos* pos);
void (*ServerConnection_Tick)(ScheduledTask* task);
Stream* (*ServerConnection_ReadStream)(void);
Stream* (*ServerConnection_WriteStream)(void);

/* Whether the network processor is currently disconnected from the server. */
bool ServerConnection_Disconnected;
/* Whether the client supports extended player list management, with group names and ranks. */
bool ServerConnection_SupportsExtPlayerList;
/* Whether the server supports handling PlayerClick packets from the client. */
bool ServerConnection_SupportsPlayerClick;
/* Whether the server can handle partial message packets or not. */
bool ServerConnection_SupportsPartialMessages;
/* Whether the server supports receiving all code page 437 characters from this client. */
bool ServerConnection_SupportsFullCP437;

void ServerConnection_RetrieveTexturePack(STRING_PURE String* url);
void ServerConnection_DownloadTexturePack(STRING_PURE String* url);
void ServerConnection_BeginGeneration(Int32 width, Int32 height, Int32 length, Int32 seed, bool vanilla);
void ServerConnection_EndGeneration(void);
void ServerConnection_InitSingleplayer(void);
void ServerConnection_InitMultiplayer(void);
IGameComponent ServerConnection_MakeComponent(void);

typedef void (*Net_Handler)(Stream* stream);
#define OPCODE_COUNT 45
UInt16 Net_PacketSizes[OPCODE_COUNT];
Net_Handler Net_Handlers[OPCODE_COUNT];
void Net_Set(UInt8 opcode, Net_Handler handler, UInt16 size);
void Net_SendPacket(void);
#endif