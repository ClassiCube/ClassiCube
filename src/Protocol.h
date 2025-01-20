#ifndef CC_PROTOCOL_H
#define CC_PROTOCOL_H
#include "Vectors.h"
CC_BEGIN_HEADER

/* 
Implements network protocols for original classic, CPE, and WoM textures
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct RayTracer;

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
	OPCODE_SET_SPAWNPOINT,      OPCODE_VELOCITY_CONTROL,
	OPCODE_DEFINE_EFFECT,       OPCODE_SPAWN_EFFECT,
	OPCODE_DEFINE_MODEL, OPCODE_DEFINE_MODEL_PART, OPCODE_UNDEFINE_MODEL,
	OPCODE_PLUGIN_MESSAGE, OPCODE_ENTITY_TELEPORT_EXT,
	OPCODE_LIGHTING_MODE, OPCODE_CINEMATIC_GUI, OPCODE_NOTIFY_ACTION,
	OPCODE_NOTIFY_POSITION_ACTION,

	OPCODE_COUNT
};

enum PROTOCOL_VERSION_ {
	PROTOCOL_0017 = 4, PROTOCOL_0019 = 5,
	PROTOCOL_0020 = 6, PROTOCOL_0030 = 7,
};

enum NOTIFY_ACTION_TYPE {
	NOTIFY_ACTION_BLOCK_LIST_SELECTED = 0, NOTIFY_ACTION_BLOCK_LIST_TOGGLED = 1, NOTIFY_ACTION_LEVEL_SAVED = 2,
	NOTIFY_ACTION_RESPAWNED = 3, NOTIFY_ACTION_SPAWN_UPDATED = 4, NOTIFY_ACTION_TEXTURE_PACK_CHANGED = 5,
	NOTIFY_ACTION_TEXTURE_PROMPT_RESPONDED = 6, NOTIFY_ACTION_THIRD_PERSON_CHANGED = 7
};

typedef void (*Net_Handler)(cc_uint8* data);
#define Net_Set(opcode, handler, size) Protocol.Handlers[opcode] = handler; Protocol.Sizes[opcode] = size;

CC_VAR extern struct _ProtocolData {
	/* Size of each packet including opcode */
	cc_uint16 Sizes[256];
	/* Handlers for processing received packets */
	Net_Handler Handlers[256];
} Protocol;

struct RayTracer;	
struct IGameComponent;
extern struct IGameComponent Protocol_Component;

void Protocol_Tick(void);

extern cc_bool cpe_needD3Fix;
void Classic_SendChat(const cc_string* text, cc_bool partial);
void Classic_SendSetBlock(int x, int y, int z, cc_bool place, BlockID block);
void Classic_SendLogin(void);
void CPE_SendPlayerClick(int button, cc_bool pressed, cc_uint8 targetId, struct RayTracer* t);
void CPE_SendNotifyAction(int action, cc_uint16 value);
void CPE_SendNotifyPositionAction(int action, int x, int y, int z);

/* Send a PluginMessage to the server; data must contain 64 bytes. */
CC_API void CPE_SendPluginMessage(cc_uint8 channel, cc_uint8* data);

CC_END_HEADER
#endif
