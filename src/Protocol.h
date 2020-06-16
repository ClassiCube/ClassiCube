#ifndef CC_PROTOCOL_H
#define CC_PROTOCOL_H
#include "String.h"
#include "Vectors.h"
/* Implements network protocols for original classic, CPE, and WoM textures.
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
	OPCODE_SET_SPAWNPOINT,      OPCODE_VELOCITY_CONTROL,
	OPCODE_DEFINE_EFFECT,       OPCODE_SPAWN_EFFECT,
	OPCODE_DEFINE_MODEL, OPCODE_DEFINE_MODEL_PART, OPCODE_UNDEFINE_MODEL,

	OPCODE_COUNT
};


typedef void (*Net_Handler)(cc_uint8* data);
/* Size of each packet including opcode */
extern cc_uint16 Net_PacketSizes[OPCODE_COUNT];
/* Functions that handle processing received packets. */
extern Net_Handler Net_Handlers[OPCODE_COUNT];
#define Net_Set(opcode, handler, size) Net_Handlers[opcode] = handler; Net_PacketSizes[opcode] = size;

struct RayTracer;	
struct IGameComponent;
extern struct IGameComponent Protocol_Component;

void Protocol_RemoveEntity(EntityID id);
void Protocol_Tick(void);

extern cc_bool cpe_needD3Fix;
void Classic_SendChat(const String* text, cc_bool partial);
void Classic_WritePosition(Vec3 pos, float yaw, float pitch);
void Classic_WriteSetBlock(int x, int y, int z, cc_bool place, BlockID block);
void Classic_SendLogin(void);
void CPE_SendPlayerClick(int button, cc_bool pressed, cc_uint8 targetId, struct RayTracer* t);
#endif
