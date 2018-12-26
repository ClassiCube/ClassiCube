#ifndef CC_CHAT_H
#define CC_CHAT_H
#include "Constants.h"
#include "Utils.h"
#include "Core.h"
/* Manages sending, adding, and logging chat.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Chat_Component;

typedef enum MsgType_ {
	MSG_TYPE_NORMAL = 0,
	MSG_TYPE_STATUS_1 = 1,
	MSG_TYPE_STATUS_2 = 2,
	MSG_TYPE_STATUS_3 = 3,
	MSG_TYPE_BOTTOMRIGHT_1 = 11,
	MSG_TYPE_BOTTOMRIGHT_2 = 12,
	MSG_TYPE_BOTTOMRIGHT_3 = 13,
	MSG_TYPE_ANNOUNCEMENT = 100,
	MSG_TYPE_CLIENTSTATUS_1 = 256, /* Cuboid messages */
	MSG_TYPE_CLIENTSTATUS_2 = 257, /* Clipboard invalid character */
	MSG_TYPE_CLIENTSTATUS_3 = 258  /* Tab list matching names*/
} MsgType;

extern String Chat_Status[3], Chat_BottomRight[3], Chat_ClientStatus[3], Chat_Announcement;
extern StringsBuffer Chat_Log, Chat_InputLog;
/* Whether chat messages are logged to disc. */
extern bool Chat_Logging;

/* Time at which last announcement message was received. */
extern TimeMS Chat_AnnouncementReceived;
/* Gets the time the ith chat message was received at. */
TimeMS Chat_GetLogTime(int i);

struct ChatCommand;
/* Represents a client-side command/action. */
struct ChatCommand {
	const char* Name;      /* Full name of this command */
	/* Function pointer for the actual action the command performs */
	void (*Execute)(const String* args, int argsCount);
	bool SingleplayerOnly; /* Whether this command is only usable in singleplayer */
	const char* Help[5];   /* Messages to show when a player uses /help on this command */
	struct ChatCommand* Next; /* Next command in linked-list of client commands */
};
/* Registers a client-side command, allowing it to be used with /client [cmd name] */
CC_API void Commands_Register(struct ChatCommand* cmd);

/* Sets the name of log file (no .txt, so e.g. just "singleplayer") */
/* NOTE: This can only be set once. */
void Chat_SetLogName(const String* name);
/* Sends a chat message, raising ChatEvents.ChatSending event. */
/* NOTE: /client is always interpreted as client-side commands. */
/* In multiplayer this is sent to the server, in singleplayer just Chat_Add. */
CC_API void Chat_Send(const String* text, bool logUsage);
/* Shorthand for Chat_AddOf(str, MSG_TYPE_NORMAL) */
CC_API void Chat_Add(const String* text);
/* Adds a chat message, raising ChatEvents.ChatReceived event. */
/* MSG_TYPE_NORMAL is usually used for player chat and command messages. */
/* Other message types are usually used for info/status messages. */
CC_API void Chat_AddOf(const String* text, MsgType type);
/* Shorthand for Chat_AddOf(String_FromReadonly(raw), MSG_TYPE_NORMAL) */
void Chat_AddRaw(const char* raw);

void Chat_Add1(const char* format, const void* a1);
void Chat_Add2(const char* format, const void* a1, const void* a2);
void Chat_Add3(const char* format, const void* a1, const void* a2, const void* a3);
void Chat_Add4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);
#endif
