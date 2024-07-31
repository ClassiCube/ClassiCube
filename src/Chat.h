#ifndef CC_CHAT_H
#define CC_CHAT_H
#include "Core.h"
CC_BEGIN_HEADER

/* Manages sending, adding, logging and handling chat.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
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
	MSG_TYPE_BIGANNOUNCEMENT = 101,
	MSG_TYPE_SMALLANNOUNCEMENT = 102,
	MSG_TYPE_CLIENTSTATUS_1 = 256, /* Cuboid messages */
	MSG_TYPE_CLIENTSTATUS_2 = 257, /* Tab list matching names */
	MSG_TYPE_EXTRASTATUS_1  = 360,
	MSG_TYPE_EXTRASTATUS_2  = 361
};

/* NOTE: this must be: (next power of next larger than GUI_MAX_CHATLINES) - 1 */
#define CHATLOG_TIME_MASK 31
/* Time most recent chat message were received at */
extern double Chat_RecentLogTimes[CHATLOG_TIME_MASK + 1];
#define Chat_GetLogTime(i) Chat_RecentLogTimes[(i) & CHATLOG_TIME_MASK]

/* Times at which last announcement messages were received */
extern double Chat_AnnouncementReceived;
extern double Chat_BigAnnouncementReceived;
extern double Chat_SmallAnnouncementReceived;

extern cc_string Chat_Status[5], Chat_BottomRight[3], Chat_ClientStatus[2];
extern cc_string Chat_Announcement, Chat_BigAnnouncement, Chat_SmallAnnouncement;
/* All chat messages received */
extern struct StringsBuffer Chat_Log;
/* All chat input entered by the user */
extern struct StringsBuffer Chat_InputLog;
/* Whether chat messages are logged to disc */
extern cc_bool Chat_Logging;

/* Sets the name of log file (no .txt, so e.g. just "singleplayer") */
/* NOTE: This can only be set once. */
void Chat_SetLogName(const cc_string* name);
/* Disables chat logging and closes currently open chat log file. */
void Chat_DisableLogging(void);
/* Sends a chat message, raising ChatEvents.ChatSending event. */
/*  NOTE: If logUsage is true, can press 'up' in chat input menu later to retype this. */
/*  NOTE: /client is always interpreted as client-side commands. */
/* In multiplayer this is sent to the server, in singleplayer just Chat_Add. */
CC_API  void Chat_Send(      const cc_string* text, cc_bool logUsage);
typedef void (*FP_Chat_Send)(const cc_string* text, cc_bool logUsage);
/* Shorthand for Chat_AddOf(str, MSG_TYPE_NORMAL) */
CC_API  void Chat_Add(      const cc_string* text);
typedef void (*FP_Chat_Add)(const cc_string* text);
/* Adds a chat message, raising ChatEvents.ChatReceived event. */
/* MSG_TYPE_NORMAL is usually used for player chat and command messages. */
/* Other message types are usually used for info/status messages. */
CC_API  void Chat_AddOf(      const cc_string* text, int msgType);
typedef void (*FP_Chat_AddOf)(const cc_string* text, int msgType);
/* Shorthand for Chat_AddOf(String_FromReadonly(raw), MSG_TYPE_NORMAL) */
void Chat_AddRaw(const char* raw);

CC_API void Chat_Add1(const char* format, const void* a1);
CC_API void Chat_Add2(const char* format, const void* a1, const void* a2);
CC_API void Chat_Add3(const char* format, const void* a1, const void* a2, const void* a3);
CC_API void Chat_Add4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

CC_END_HEADER
#endif
