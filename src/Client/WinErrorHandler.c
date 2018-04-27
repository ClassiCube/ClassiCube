#include "ErrorHandler.h"
#include "Platform.h"
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <Windows.h>
#include <Dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

/* TODO: These might be better off as a function. */
#define ErrorHandler_WriteLogBody(raw_msg)\
UInt8 logMsgBuffer[String_BufferSize(3071)];\
String logMsg = String_InitAndClearArray(logMsgBuffer);\
String_AppendConst(&logMsg, "ClassiCube crashed.\r\n");\
String_AppendConst(&logMsg, "Message: ");\
String_AppendConst(&logMsg, raw_msg);\
String_AppendConst(&logMsg, "\r\n");

#define ErrorHandler_WriteLogEnd()\
String_AppendConst(&logMsg, "\r\nPlease report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.");

void ErrorHandler_Init(const UInt8* logFile) {
	/* TODO: Open log file */
}

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: write to log file */
}

void ErrorHandler_Fail(const UInt8* raw_msg) {
	/* TODO: write to log file */
	ErrorHandler_WriteLogBody(raw_msg);
	ErrorHandler_Backtrace(&logMsg);
	ErrorHandler_WriteLogEnd();

	ErrorHandler_ShowDialog("We're sorry", logMsg.buffer);
	Platform_Exit(1);
}

void ErrorHandler_FailWithCode(ReturnCode code, const UInt8* raw_msg) {
	/* TODO: write to log file */
	ErrorHandler_WriteLogBody(raw_msg);
	String_AppendConst(&logMsg, "Return code: ");
	String_AppendInt32(&logMsg, (Int32)code);
	String_AppendConst(&logMsg, "\r\n");
	ErrorHandler_Backtrace(&logMsg);
	ErrorHandler_WriteLogEnd();

	ErrorHandler_ShowDialog("We're sorry", logMsg.buffer);
	Platform_Exit(code);
}

void ErrorHandler_ShowDialog(const UInt8* title, const UInt8* msg) {
	HWND win = GetActiveWindow(); /* TODO: It's probably wrong to use GetActiveWindow() here */
	MessageBoxA(win, msg, title, 0);
}

struct SymbolAndName { SYMBOL_INFO Symbol; UInt8 Name[256]; };
void ErrorHandler_Backtrace(STRING_TRANSIENT String* str) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	void* stack[50];
	UInt16 frames = CaptureStackBackTrace(0, 50, stack, NULL);

	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLen = 255;
	sym.Symbol.SizeOfStruct = sizeof(SYMBOL_INFO);

	UInt32 i;
	String_AppendConst(str, "Backtrace: \r\n");
	for (i = 0; i < frames; i++) {
		Int32 number = frames - i - 1;
		Int32 addr = (Int32)stack[i];

		if (SymFromAddr(process, (DWORD64)stfack[i], NULL, &sym.Symbol)) {
			String_Format3(str, "%i) %c - 0x%i\r\n", &number, sym.Symbol.Name, &addr);
		} else {
			String_Format2(str, "%i) 0x%i\r\n", &number, &addr);
		}
	}
	String_AppendConst(str, "\r\n");
}