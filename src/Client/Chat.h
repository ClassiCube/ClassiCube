#ifndef CC_CHAT_H
#define CC_CHAT_H
#include "Typedefs.h"
#include "Constants.h"
#include "DateTime.h"
#include "String.h"
#include "NetworkEnums.h"
#include "GameStructs.h"
/* Manages sending and logging chat.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct ChatLine_ { UInt8 Buffer[String_BufferSize(STRING_SIZE)]; DateTime Received; } ChatLine;
typedef struct InputLine_ { UInt8 Buffer[String_BufferSize(STRING_SIZE)]; } InputLine;

ChatLine Chat_Status[3];
ChatLine Chat_BottomRight[3];
ChatLine Chat_ClientStatus[3];
ChatLine Chat_Announcement;

void Chat_Get(UInt32 idx, STRING_TRANSIENT String* dst);
UInt32 Chat_Count;
void Chat_InputGet(UInt32 idx, STRING_TRANSIENT String* dst);
UInt32 Chat_InputCount;

IGameComponent Chat_MakeGameComponent(void);
void Chat_SetLogName(STRING_TRANSIENT String* name);
void Chat_Send(STRING_TRANSIENT String* text);
void Chat_Add(STRING_TRANSIENT String* text);
void Chat_AddOf(STRING_TRANSIENT String* text, MessageType type);
#endif