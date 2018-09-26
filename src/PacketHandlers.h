#ifndef CC_PACKETHANDLERS_H
#define CC_PACKETHANDLERS_H
#include "Input.h"
#include "String.h"
#include "Vectors.h"
/* Implements network protocol handlers for original classic, CPE, and WoM textures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct PickedPos;
struct Stream;
void Handlers_RemoveEntity(EntityID id);
void Handlers_Reset(void);
void Handlers_Tick(void);

bool cpe_sendHeldBlock, cpe_useMessageTypes, cpe_needD3Fix, cpe_extEntityPos, cpe_blockPerms, cpe_fastMap;
void Classic_WriteChat(const String* text, bool partial);
void Classic_WritePosition(Vector3 pos, float rotY, float headX);
void Classic_WriteSetBlock(Int32 x, Int32 y, Int32 z, bool place, BlockID block);
void Classic_WriteLogin(const String* username, const String* verKey);
void CPE_WritePlayerClick(MouseButton button, bool buttonDown, UInt8 targetId, struct PickedPos* pos);
#endif
