#include "Chat.h"
#include "Stream.h"
#include "Platform.h"
#include "Events.h"
#include "Game.h"
#include "ErrorHandler.h"

void ChatLine_Make(ChatLine* line, STRING_TRANSIENT String* text) {
	String dst = String_FromRawBuffer(line->Buffer, STRING_SIZE);
	String_AppendString(&dst, text);
	line->Received = Platform_CurrentUTCTime();
}

void InputLine_Make(InputLine* line, STRING_TRANSIENT String* text) {
	String dst = String_FromRawBuffer(line->Buffer, STRING_SIZE);
	String_AppendString(&dst, text);
}

ChatLine Log[256];
InputLine InputLog[256];

UInt8 Chat_LogNameBuffer[String_BufferSize(STRING_SIZE)];
String Chat_LogName;
Stream Chat_LogStream;
DateTime ChatLog_LastLogDate;

void Chat_Send(STRING_PURE String* text) {
	if (text->length == 0) return;

	if (game.CommandList.IsCommandPrefix(text)) {
		game.CommandList.Execute(text);
	} else {
		game.Server.SendChat(text);
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

	Chat_LogName = String_FromEmptyBuffer(Chat_LogNameBuffer, STRING_SIZE);
	String_Clear(&Chat_LogName);
	Int32 i;
	for (i = 0; i < name->length; i++) {
		if (Chat_AllowedLogChar(name->buffer[i])) {
			String_Append(&Chat_LogName, name->buffer[i]);
		}
	}
}

void Chat_OpenLog(DateTime* now) {
	String logsDir = String_FromConstant("logs");
	if (!Platform_DirectoryExists(&logsDir)) {
		Platform_DirectoryCreate(&logsDir);
	}

	/* Ensure multiple instances do not end up overwriting each other's log entries. */
	UInt32 i;
	for (i = 0; i < 20; i++) {
		UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
		String path = String_FromRawBuffer(pathBuffer, FILENAME_SIZE);
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
	ErrorHandler_LogError("creating chat log", "Failed to open chat log file after 20 tries, giving up");
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
	String str = String_FromRawBuffer(logBuffer, STRING_SIZE * 2);

	/* [HH:mm:ss] text */
	String_Append(&str, '['); String_AppendPaddedInt32(&str, now.Hour,   2);
	String_Append(&str, ':'); String_AppendPaddedInt32(&str, now.Minute, 2);
	String_Append(&str, ':'); String_AppendPaddedInt32(&str, now.Second, 2);
	String_Append(&str, ']'); String_Append(&str, ' ');

	Int32 i;
	for (i = 0; i < text->length; i++) {
		UInt8 c = text->buffer[i];
		if (c == '&') { i++; continue; } /* Skip over the following colour code */
		String_Append(&str, c);
	}
	Stream_WriteLine(&Chat_LogStream, &str);
}

void Chat_Add(STRING_PURE String* text) { Chat_AddOf(text, MessageType_Normal); }
void Chat_AddOf(STRING_PURE String* text, MessageType type) {
	if (type == MessageType_Normal) {
		/* todo use a single byte array for all strings, each 'string' is 22 bits offsrt, 10 bits length into this array. */
		/* means resizing is expensive tho*/
		Log.Add(text);
		Chat_AppendLog(text);
	} else if (type >= MessageType_Status1 && type <= MessageType_Status3) {
		ChatLine_Make(&Chat_Status[type - MessageType_Status1], text);
	} else if (type >= MessageType_BottomRight1 && type <= MessageType_BottomRight3) {
		ChatLine_Make(&Chat_BottomRight[type - MessageType_BottomRight1], text);
	} else if (type == MessageType_Announcement) {
		ChatLine_Make(&Chat_Announcement, text);
	} else if (type >= MessageType_ClientStatus1 && type <= MessageType_ClientStatus3) {
		ChatLine_Make(&Chat_ClientStatus[type - MessageType_ClientStatus1], text);
	}
	Events_RaiseChatReceived(text, type);
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