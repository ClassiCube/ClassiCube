#ifndef CC_SERVERCONNECTION_H
#define CC_SERVERCONNECTION_H
#include "Input.h"
#include "Vectors.h"
/* Represents a connection to either a multiplayer or an internal singleplayer server.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
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
	OPCODE_SET_INVENTORY_ORDER, OPCODE_SET_HOTBAR,

	OPCODE_COUNT
};

struct PickedPos;
struct Stream;
struct IGameComponent;
struct ScheduledTask;
extern struct IGameComponent Server_Component;

/* Prepares a ping entry for sending to the server, then returns its ID. */
int Ping_NextPingId(void);
/* Updates received time for ping entry with matching ID. */
void Ping_Update(int id);
/* Calculates average ping time based on most recent ping entries. */
int Ping_AveragePingMS(void);

/* Data for currently active connection to a server. */
CC_VAR extern struct _ServerConnectionData {
	/* Begins connecting to the server. */
	/* NOTE: Usually asynchronous, but not always. */
	void (*BeginConnect)(void);
	/* Ticks state of the server. */
	void (*Tick)(struct ScheduledTask* task);
	/* Sends a block update to the server. */
	void (*SendBlock)(int x, int y, int z, BlockID old, BlockID now);
	/* Sends a chat message to the server. */
	void (*SendChat)(const String* text);
	/* Sends a position update to the server. */
	void (*SendPosition)(Vec3 pos, float rotY, float headX);
	/* Sends raw data to the server. */
	/* NOTE: Prefer SendBlock/Position/Chat instead, this does NOT work in singleplayer. */
	void (*SendData)(const cc_uint8* data, cc_uint32 len);

	/* The current name of the server. (Shows as first line when loading) */
	String Name;
	/* The current MOTD of the server. (Shows as second line when loading) */
	String MOTD;
	/* The software name the client identifies itself as being to the server. */
	/* By default this is GAME_APP_NAME. */
	String AppName;

	/* Buffer to data to send to the server. */
	cc_uint8* WriteBuffer;
	/* Whether the player is connected to singleplayer/internal server. */
	cc_bool IsSinglePlayer;
	/* Whether the player has been disconnected from the server. */
	cc_bool Disconnected;

	/* Whether the server supports separate tab list from entities in world. */
	cc_bool SupportsExtPlayerList;
	/* Whether the server supports packet with detailed info on mouse clicks. */
	cc_bool SupportsPlayerClick;
	/* Whether the server supports combining multiple chat packets into one. */
	cc_bool SupportsPartialMessages;
	/* Whether the server supports all of code page 437, not just ASCII. */
	cc_bool SupportsFullCP437;

	/* IP address of the server if multiplayer, empty string if singleplayer. */
	String IP;
	/* Port of the server if multiplayer, 0 if singleplayer. */
	int Port;
} Server;

/* If user hasn't previously accepted url, displays a dialog asking to confirm downloading it. */
/* Otherwise just calls World_ApplyTexturePack. */
void Server_RetrieveTexturePack(const String* url);

typedef void (*Net_Handler)(cc_uint8* data);
extern cc_uint16 Net_PacketSizes[OPCODE_COUNT];
extern Net_Handler Net_Handlers[OPCODE_COUNT];
#define Net_Set(opcode, handler, size) Net_Handlers[opcode] = handler; Net_PacketSizes[opcode] = size;

void Net_SendPacket(void);
#endif
