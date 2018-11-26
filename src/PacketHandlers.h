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

extern bool cpe_needD3Fix;
void Classic_WriteChat(const String* text, bool partial);
void Classic_WritePosition(Vector3 pos, float rotY, float headX);
void Classic_WriteSetBlock(int x, int y, int z, bool place, BlockID block);
void Classic_WriteLogin(const String* username, const String* verKey);
void CPE_WritePlayerClick(MouseButton button, bool buttonDown, uint8_t targetId, struct PickedPos* pos);
#endif
