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
void Classic_WriteChat(struct Stream* stream, STRING_PURE String* text, bool partial);
void Classic_WritePosition(struct Stream* stream, Vector3 pos, Real32 rotY, Real32 headX);
void Classic_WriteSetBlock(struct Stream* stream, Int32 x, Int32 y, Int32 z, bool place, BlockID block);
void Classic_WriteLogin(struct Stream* stream, STRING_PURE String* username, STRING_PURE String* verKey);
void CPE_WritePlayerClick(struct Stream* stream, MouseButton button, bool buttonDown, UInt8 targetId, struct PickedPos* pos);
#endif
