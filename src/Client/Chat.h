#ifndef CC_CHAT_H
#define CC_CHAT_H
#include "Constants.h"
#include "Utils.h"
#include "Typedefs.h"
/* Manages sending and logging chat.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;

enum MSG_TYPE {
	MSG_TYPE_NORMAL = 0,
	MSG_TYPE_STATUS_1 = 1,
	MSG_TYPE_STATUS_2 = 2,
	MSG_TYPE_STATUS_3 = 3,
	MSG_TYPE_BOTTOMRIGHT_1 = 11,
	MSG_TYPE_BOTTOMRIGHT_2 = 12,
	MSG_TYPE_BOTTOMRIGHT_3 = 13,
	MSG_TYPE_ANNOUNCEMENT = 100,
	MSG_TYPE_CLIENTSTATUS_1 = 256, /* Cuboid messages*/
	MSG_TYPE_CLIENTSTATUS_2 = 257, /* Clipboard invalid character */
	MSG_TYPE_CLIENTSTATUS_3 = 258, /* Tab list matching names*/
};

struct ChatLine { UChar Buffer[String_BufferSize(STRING_SIZE)]; DateTime Received; };
struct ChatLine Chat_Status[3], Chat_BottomRight[3], Chat_ClientStatus[3], Chat_Announcement;
StringsBuffer Chat_Log, Chat_InputLog;
void Chat_GetLogTime(UInt32 index, Int64* timeMS);

void Chat_MakeComponent(struct IGameComponent* comp);
void Chat_SetLogName(STRING_PURE String* name);
void Chat_Send(STRING_PURE String* text, bool logUsage);
void Chat_Add(STRING_PURE String* text);
void Chat_AddOf(STRING_PURE String* text, Int32 messageType);
#define Chat_AddRaw(str, raw) String str = String_FromConst(raw); Chat_Add(&str);
#endif
