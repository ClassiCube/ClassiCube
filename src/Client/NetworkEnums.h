#ifndef CC_NETWORKENUMS_H
#define CC_NETWORKSENUM_H
#include "Typedefs.h"
/* Constants for network related data.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Network packet opcodes. */
typedef UInt8 Opcode;
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


/* Chat message types.*/
typedef UInt8 MessageType;
/* CPE message types */

#define MESSAGE_TYPE_NORMAL         0
#define MESSAGE_TYPE_STATUS_1       1
#define MESSAGE_TYPE_STATUS_2       2
#define MESSAGE_TYPE_STATUS_3       3
#define MESSAGE_TYPE_BOTTOMRIGHT_1  11
#define MESSAGE_TYPE_BOTTOMRIGHT_2  12
#define MESSAGE_TYPE_BOTTOMRIGHT_3  13
#define MESSAGE_TYPE_ANNOUNCEMENT   100

/* client defined message types */
/* Cuboid messages*/
#define MESSAGE_TYPE_CLIENTSTATUS_1 256
/* Clipboard invalid character */
#define MESSAGE_TYPE_CLIENTSTATUS_2 257
/* Tab list matching names*/
#define MESSAGE_TYPE_CLIENTSTATUS_3 258
#endif