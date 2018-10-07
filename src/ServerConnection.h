#ifndef CC_SERVERCONNECTION_H
#define CC_SERVERCONNECTION_H
#include "Input.h"
#include "Vectors.h"
/* Represents a connection to either a singleplayer or multiplayer server.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

enum OPCODE_ {
	OPCODE_HANDSHAKE,             OPCODE_PING,
	OPCODE_LEVEL_BEGIN, OPCODE_LEVEL_DATA, OPCODE_LEVEL_END,
	OPCODE_SET_BLOCK_CLIENT,      OPCODE_SET_BLOCK,
	OPCODE_ADD_ENTITY,            OPCODE_ENTITY_TELEPORT,
	OPCODE_RELPOS_AND_ORI_UPDATE, OPCODE_RELPOS_UPDATE, 
	OPCODE_ORI_UPDATE,            OPCODE_REMOVE_ENTITY,
	OPCODE_MESSAGE,               OPCODE_KICK,
	OPCODE_SET_PERMISSION,

	/* CPE packets */
	OPCODE_EXT_INFO,            OPCODE_EXT_ENTRY,
	OPCODE_SET_REACH,           OPCODE_CUSTOM_BLOCK_LEVEL,
	OPCODE_HOLD_THIS,           OPCODE_SET_TEXT_HOTKEY,
	OPCODE_EXT_ADD_PLAYER_NAME,    OPCODE_EXT_ADD_ENTITY,
	OPCODE_EXT_REMOVE_PLAYER_NAME, OPCODE_ENV_SET_COLOR,
	OPCODE_MAKE_SELECTION,         OPCODE_REMOVE_SELECTION,
	OPCODE_SET_BLOCK_PERMISSION,   OPCODE_SET_MODEL,
	OPCODE_ENV_SET_MAP_APPEARANCE, OPCODE_ENV_SET_WEATHER,
	OPCODE_HACK_CONTROL,        OPCODE_EXT_ADD_ENTITY2,
	OPCODE_PLAYER_CLICK,        OPCODE_DEFINE_BLOCK,
	OPCODE_UNDEFINE_BLOCK,      OPCODE_DEFINE_BLOCK_EXT,
	OPCODE_BULK_BLOCK_UPDATE,   OPCODE_SET_TEXT_COLOR,
	OPCODE_ENV_SET_MAP_URL,     OPCODE_ENV_SET_MAP_PROPERTY,
	OPCODE_SET_ENTITY_PROPERTY, OPCODE_TWO_WAY_PING,
	OPCODE_SET_INVENTORY_ORDER,

	OPCODE_COUNT,
};

struct PickedPos;
struct Stream;
struct IGameComponent;
struct ScheduledTask;

uint16_t PingList_NextPingData(void);
void PingList_Update(uint16_t data);
int PingList_AveragePingMs(void);

bool ServerConnection_IsSinglePlayer;
bool ServerConnection_Disconnected;
extern String ServerConnection_ServerName;
extern String ServerConnection_ServerMOTD;
extern String ServerConnection_AppName;

void (*ServerConnection_BeginConnect)(void);
void (*ServerConnection_SendChat)(const String* text);
void (*ServerConnection_SendPosition)(Vector3 pos, float rotY, float headX);
void (*ServerConnection_SendPlayerClick)(MouseButton button, bool isDown, EntityID targetId, struct PickedPos* pos);
void (*ServerConnection_Tick)(struct ScheduledTask* task);
uint8_t* ServerConnection_WriteBuffer;

bool ServerConnection_SupportsExtPlayerList;
bool ServerConnection_SupportsPlayerClick;
bool ServerConnection_SupportsPartialMessages;
bool ServerConnection_SupportsFullCP437;

void ServerConnection_RetrieveTexturePack(const String* url);
void ServerConnection_DownloadTexturePack(const String* url);
void ServerConnection_InitSingleplayer(void);
void ServerConnection_InitMultiplayer(void);
void ServerConnection_MakeComponent(struct IGameComponent* comp);

typedef void (*Net_Handler)(uint8_t* data);
uint16_t Net_PacketSizes[OPCODE_COUNT];
Net_Handler Net_Handlers[OPCODE_COUNT];
void Net_Set(uint8_t opcode, Net_Handler handler, uint16_t size);
void Net_SendPacket(void);
#endif
