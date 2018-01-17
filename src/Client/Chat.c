#include "Chat.h"
#include "Stream.h"
#include "Platform.h"
#include "Event.h"
#include "Game.h"
#include "ErrorHandler.h"
#include "ServerConnection.h"

void ChatLine_Make(ChatLine* line, STRING_TRANSIENT String* text) {
	String dst = String_InitAndClearArray(line->Buffer);
	String_AppendString(&dst, text);
	line->Received = Platform_CurrentUTCTime();
}

UInt8 Chat_LogNameBuffer[String_BufferSize(STRING_SIZE)];
String Chat_LogName = String_FromEmptyArray(Chat_LogNameBuffer);
Stream Chat_LogStream;
DateTime ChatLog_LastLogDate;

void Chat_Send(STRING_PURE String* text) {
	if (text->length == 0) return;
	StringsBuffer_Add(&InputLog, text);

	if (game.CommandList.IsCommandPrefix(text)) {
		game.CommandList.Execute(text);
	} else {
		ServerConnection_SendChat(text);
	}
}

void Chat_CloseLog(void) {
	if (Chat_LogStream.Data == NULL) return;
	ReturnCode code = Chat_LogStream.Close(&Chat_LogStream);
	ErrorHandler_CheckOrFail(code, "Chat - closing log file");
}

bool Chat_AllowedLogChar(UInt8 c) {
	return
		c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' ||
		(c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void Chat_SetLogName(STRING_PURE String* name) {
	if (Chat_LogName.length > 0) return;
	String_Clear(&Chat_LogName);

	UInt8 noColsBuffer[String_BufferSize(STRING_SIZE)];
	String noColsName = String_InitAndClearArray(noColsBuffer);
	String_AppendColorless(&noColsName, name);

	UInt32 i;
	for (i = 0; i < noColsName.length; i++) {
		if (Chat_AllowedLogChar(noColsName.buffer[i])) {
			String_Append(&Chat_LogName, noColsName.buffer[i]);
		}
	}
}

void Chat_OpenLog(DateTime* now) {
	String logsDir = String_FromConst("logs");
	if (!Platform_DirectoryExists(&logsDir)) {
		Platform_DirectoryCreate(&logsDir);
	}

	/* Ensure multiple instances do not end up overwriting each other's log entries. */
	UInt32 i;
	for (i = 0; i < 20; i++) {
		UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
		String path = String_InitAndClearArray(pathBuffer);
		String_AppendConst(&path, "logs");
		String_Append(&path, Platform_DirectorySeparator);

		String_AppendPaddedInt32(&path, 4, now->Year);
		String_Append(&path, '-');
		String_AppendPaddedInt32(&path, 2, now->Month);
		String_Append(&path, '-');
		String_AppendPaddedInt32(&path, 2, now->Day);

		String_AppendString(&path, &Chat_LogName);
		if (i > 0) {
			String_AppendConst(&path, " _");
			String_AppendInt32(&path, i);
		}
		String_AppendConst(&path, ".log");

		void* file;
		ReturnCode code = Platform_FileOpen(&file, &path, false);
		if (code != 0 && code != ReturnCode_FileShareViolation) {
			ErrorHandler_FailWithCode(code, "Chat - opening log file");
		}

		Stream_FromFile(&Chat_LogStream, file, &path);
		return;
	}

	Chat_LogStream.Data = NULL;
	String failedMsg = String_FromConst("Failed to open chat log file after 20 tries, giving up");
	ErrorHandler_Log(&failedMsg);
}

void Chat_AppendLog(STRING_PURE String* text) {
	if (Chat_LogName.length == 0 || !Game_ChatLogging) return;
	DateTime now = Platform_CurrentLocalTime();

	if (now.Day != ChatLog_LastLogDate.Day || now.Month != ChatLog_LastLogDate.Month || now.Year != ChatLog_LastLogDate.Year) {
		Chat_CloseLog();
		Chat_OpenLog(&now);
	}

	ChatLog_LastLogDate = now;
	if (Chat_LogStream.Data == NULL) return;
	UInt8 logBuffer[String_BufferSize(STRING_SIZE * 2)];
	String str = String_InitAndClearArray(logBuffer);

	/* [HH:mm:ss] text */
	String_Append(&str, '['); String_AppendPaddedInt32(&str, now.Hour,   2);
	String_Append(&str, ':'); String_AppendPaddedInt32(&str, now.Minute, 2);
	String_Append(&str, ':'); String_AppendPaddedInt32(&str, now.Second, 2);
	String_Append(&str, ']'); String_Append(&str, ' ');
	String_AppendColorless(&str, text);

	Stream_WriteLine(&Chat_LogStream, &str);
}

void Chat_Add(STRING_PURE String* text) { Chat_AddOf(text, MESSAGE_TYPE_NORMAL); }
void Chat_AddOf(STRING_PURE String* text, MessageType type) {
	if (type == MESSAGE_TYPE_NORMAL) {
		StringsBuffer_Add(&ChatLog, text);
		Chat_AppendLog(text);
	} else if (type >= MESSAGE_TYPE_STATUS_1 && type <= MESSAGE_TYPE_STATUS_3) {
		ChatLine_Make(&Chat_Status[type - MESSAGE_TYPE_STATUS_1], text);
	} else if (type >= MESSAGE_TYPE_BOTTOMRIGHT_1 && type <= MESSAGE_TYPE_BOTTOMRIGHT_3) {
		ChatLine_Make(&Chat_BottomRight[type - MESSAGE_TYPE_BOTTOMRIGHT_1], text);
	} else if (type == MESSAGE_TYPE_ANNOUNCEMENT) {
		ChatLine_Make(&Chat_Announcement, text);
	} else if (type >= MESSAGE_TYPE_CLIENTSTATUS_1 && type <= MESSAGE_TYPE_CLIENTSTATUS_3) {
		ChatLine_Make(&Chat_ClientStatus[type - MESSAGE_TYPE_CLIENTSTATUS_1], text);
	}
	Event_RaiseChat(&ChatEvents_ChatReceived, text, type);
}

void Chat_Reset(void) {
	Chat_CloseLog();
	String_Clear(&Chat_LogName);
}

IGameComponent Chat_MakeGameComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Reset = Chat_Reset;
	return comp;
}