#include "ErrorHandler.h"
#include "Platform.h"
#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

/* TODO: These might be better off as a function. */
#define ErrorHandler_WriteLogBody(raw_msg)\
UInt8 logMsgBuffer[String_BufferSize(2047)];\
String logMsg = String_FromRawBuffer(logMsgBuffer, 2047);\
String_AppendConstant(&logMsg, "ClassicalSharp crashed.\r\n");\
String_AppendConstant(&logMsg, "Message: ");\
String_AppendConstant(&logMsg, raw_msg);\
String_AppendConstant(&logMsg, "\r\n");

#define ErrorHandler_WriteLogEnd()\
String_AppendConstant(&logMsg, "\r\nPlease report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.");


void ErrorHandler_Init(void) {
	/* TODO: Open log file */
}

void ErrorHandler_Log(String msg) {
	/* TODO: write to log file */
}

void ErrorHandler_Fail(const UInt8* raw_msg) {
	/* TODO: write to log file */
	ErrorHandler_WriteLogBody(raw_msg);
	ErrorHandler_WriteLogEnd();

	HWND win = GetActiveWindow();
	MessageBoxA(win, logMsg.buffer, "We're sorry", 0);
	ExitProcess(1);
}

void ErrorHandler_FailWithCode(ReturnCode code, const UInt8* raw_msg) {
	/* TODO: write to log file */
	ErrorHandler_WriteLogBody(raw_msg);
	String_AppendConstant(&logMsg, "Return code: ");
	String_AppendInt32(&logMsg, (Int32)code);
	String_AppendConstant(&logMsg, "\r\n");
	ErrorHandler_WriteLogEnd();

	HWND win = GetActiveWindow();
	MessageBoxA(win, logMsg.buffer, "We're sorry", 0);
	ExitProcess(code);
}