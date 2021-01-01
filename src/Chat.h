#ifndef CC_CHAT_H
#define CC_CHAT_H
#include "Core.h"
/* Manages sending, adding, logging and handling chat.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct StringsBuffer;
extern struct IGameComponent Chat_Component;

enum MsgType {
	MSG_TYPE_NORMAL   = 0,
	MSG_TYPE_STATUS_1 = 1,
	MSG_TYPE_STATUS_2 = 2,
	MSG_TYPE_STATUS_3 = 3,
	MSG_TYPE_BOTTOMRIGHT_1 = 11,
	MSG_TYPE_BOTTOMRIGHT_2 = 12,
	MSG_TYPE_BOTTOMRIGHT_3 = 13,
	MSG_TYPE_ANNOUNCEMENT  = 100,
	MSG_TYPE_CLIENTSTATUS_1 = 256, /* Cuboid messages */
	MSG_TYPE_CLIENTSTATUS_2 = 257  /* Tab list matching names */
};

extern cc_string Chat_Status[4], Chat_BottomRight[3], Chat_ClientStatus[2], Chat_Announcement;
/* All chat messages received. */
extern struct StringsBuffer Chat_Log;
/* Time each chat message was received at. */
extern double* Chat_LogTime;
/* All chat entered by the user. */
extern struct StringsBuffer Chat_InputLog;
/* Whether chat messages are logged to disc. */
extern cc_bool Chat_Logging;

/* Time at which last announcement message was received. */
extern double Chat_AnnouncementReceived;

struct ChatCommand;
/* Represents a client-side command/action. */
struct ChatCommand {
	const char* name;         /* Full name of this command */
	/* Function pointer for the actual action the command performs */
	void (*Execute)(const cc_string* args, int argsCount);
	cc_bool singleplayerOnly; /* Whether this command is only usable in singleplayer */
	const char* help[5];      /* Messages to show when a player uses /help on this command */
	struct ChatCommand* next; /* Next command in linked-list of client commands */
};
/* Registers a client-side command, allowing it to be used with /client [cmd name] */
CC_API void Commands_Register(struct ChatCommand* cmd);

/* Sets the name of log file (no .txt, so e.g. just "singleplayer") */
/* NOTE: This can only be set once. */
void Chat_SetLogName(const cc_string* name);
/* Disables chat logging and closes currently open chat log file. */
void Chat_DisableLogging(void);
/* Sends a chat message, raising ChatEvents.ChatSending event. */
/* NOTE: If logUsage is true, can press 'up' in chat input menu later to retype this. */
/* NOTE: /client is always interpreted as client-side commands. */
/* In multiplayer this is sent to the server, in singleplayer just Chat_Add. */
CC_API void Chat_Send(const cc_string* text, cc_bool logUsage);
/* Shorthand for Chat_AddOf(str, MSG_TYPE_NORMAL) */
CC_API void Chat_Add(const cc_string* text);
/* Adds a chat message, raising ChatEvents.ChatReceived event. */
/* MSG_TYPE_NORMAL is usually used for player chat and command messages. */
/* Other message types are usually used for info/status messages. */
CC_API void Chat_AddOf(const cc_string* text, int msgType);
/* Shorthand for Chat_AddOf(String_FromReadonly(raw), MSG_TYPE_NORMAL) */
void Chat_AddRaw(const char* raw);

void Chat_Add1(const char* format, const void* a1);
void Chat_Add2(const char* format, const void* a1, const void* a2);
void Chat_Add3(const char* format, const void* a1, const void* a2, const void* a3);
void Chat_Add4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);
#endif
