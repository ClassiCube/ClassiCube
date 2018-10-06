#ifndef CC_CHAT_H
#define CC_CHAT_H
#include "Constants.h"
#include "Utils.h"
#include "Core.h"
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

struct ChatLine { char Buffer[STRING_SIZE]; TimeMS Received; };
struct ChatLine Chat_Status[3], Chat_BottomRight[3], Chat_ClientStatus[3], Chat_Announcement;
StringsBuffer Chat_Log, Chat_InputLog;
TimeMS Chat_GetLogTime(int i);

void Chat_MakeComponent(struct IGameComponent* comp);
void Chat_SetLogName(const String* name);
void Chat_Send(const String* text, bool logUsage);
void Chat_Add(const String* text);
void Chat_AddOf(const String* text, int messageType);
void Chat_AddRaw(const char* raw);

void Chat_LogError(ReturnCode result,  const char* place);
void Chat_LogError2(ReturnCode result, const char* place, const String* path);
void Chat_Add1(const char* format, const void* a1);
void Chat_Add2(const char* format, const void* a1, const void* a2);
void Chat_Add3(const char* format, const void* a1, const void* a2, const void* a3);
void Chat_Add4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);
#endif
