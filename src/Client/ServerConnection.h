#ifndef CC_SERVERCONNECTION_H
#define CC_SERVERCONNECTION_H
#include "Input.h"
#include "GameStructs.h"
#include "Picking.h"
/* Represents a connection to either a singleplayer or multiplayer server.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define OPCODE_HANDSHAKE                      0
#define OPCODE_PING                           1
#define OPCODE_LEVEL_INIT                     2
#define OPCODE_LEVEL_DATA_CHUNK               3
#define OPCODE_LEVEL_FINALISE                 4
#define OPCODE_SET_BLOCK_CLIENT               5
#define OPCODE_SET_BLOCK                      6
#define OPCODE_ADD_ENTITY                     7
#define OPCODE_ENTITY_TELEPORT                8
#define OPCODE_RELPOSANDORIENTATION_UPDATE    9
#define OPCODE_RELPOS_UPDATE                  10
#define OPCODE_ORIENTATION_UPDATE             11
#define OPCODE_REMOVE_ENTITY                  12
#define OPCODE_MESSAGE                        13
#define OPCODE_KICK                           14
#define OPCODE_SET_PERMISSION                 15

#define OPCODE_CPE_EXT_INFO                   16
#define OPCODE_CPE_EXT_ENTRY                  17
#define OPCODE_CPE_SET_CLICK_DISTANCE         18
#define OPCODE_CPE_CUSTOM_BLOCK_SUPPORT_LEVEL 19
#define OPCODE_CPE_HOLD_THIS                  20
#define OPCODE_CPE_SET_TEXT_HOTKEY            21
#define OPCODE_CPE_EXT_ADD_PLAYER_NAME        22
#define OPCODE_CPE_EXT_ADD_ENTITY             23
#define OPCODE_CPE_EXT_REMOVE_PLAYER_NAME     24
#define OPCODE_CPE_ENV_SET_COLOR              25
#define OPCODE_CPE_MAKE_SELECTION             26
#define OPCODE_CPE_REMOVE_SELECTION           27
#define OPCODE_CPE_SET_BLOCK_PERMISSION       28
#define OPCODE_CPE_SET_MODEL                  29
#define OPCODE_CPE_ENV_SET_MAP_APPERANCE      30
#define OPCODE_CPE_ENV_SET_WEATHER            31
#define OPCODE_CPE_HACK_CONTROL               32
#define OPCODE_CPE_EXT_ADD_ENTITY_2           33
#define OPCODE_CPE_PLAYER_CLICK               34
#define OPCODE_CPE_DEFINE_BLOCK               35
#define OPCODE_CPE_UNDEFINE_BLOCK             36
#define OPCODE_CPE_DEFINE_BLOCK_EXT           37
#define OPCODE_CPE_BULK_BLOCK_UPDATE          38
#define OPCODE_CPE_SET_TEXT_COLOR             39
#define OPCODE_CPE_ENV_SET_MAP_URL            40
#define OPCODE_CPE_ENV_SET_MAP_PROPERTY       41
#define OPCODE_CPE_SET_ENTITY_PROPERTY        42
#define OPCODE_CPE_TWO_WAY_PING               43
#define Opcode_CPE_SET_INVENTORY_ORDER        44

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
void (*ServerConnection_Free)(void);

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
void ServerConnection_BeginGeneration(Int32 width, Int32 height, Int32 length, Int32 seed, bool vanilla);
void ServerConnection_EndGeneration(void);
void ServerConnection_InitSingleplayer(void);
void ServerConnection_InitMultiplayer(void);
#endif