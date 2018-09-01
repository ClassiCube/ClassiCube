#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"
#include "Window.h"

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: write to log file */
}

void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	char logMsgBuffer[3070 + 1] = { 0 };
	String logMsg = { logMsgBuffer, 0, 3070 };

	String_Format3(&logMsg, "ClassiCube crashed.%cMessge: %c%c",
		Platform_NewLine, raw_msg, Platform_NewLine);
	if (result) {
		String_Format2(&logMsg, "%y%c", &result, Platform_NewLine);
	} else { result = 1; }

	ErrorHandler_Backtrace(&logMsg);
	/* TODO: write to log file */

	String_AppendConst(&logMsg,
		"Please report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.");
	ErrorHandler_ShowDialog("We're sorry", logMsg.buffer);
	Platform_Exit(result);
}

void ErrorHandler_Fail(const char* raw_msg) { ErrorHandler_FailWithCode(0, raw_msg); }

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <dbghelp.h>

static LONG WINAPI ErrorHandler_UnhandledFilter(struct _EXCEPTION_POINTERS* pInfo) {
	/* TODO: Write processor state to file*/
	/* TODO: Get address that caused the issue */
	/* TODO: Don't Backtrace here, because it's not the actual useful stack */
	char msgBuffer[128 + 1] = { 0 };
	String msg = { msgBuffer, 0, 128 };

	UInt32 code = (UInt32)pInfo->ExceptionRecord->ExceptionCode;
	UInt64 addr = (UInt64)pInfo->ExceptionRecord->ExceptionAddress;
	String_Format2(&msg, "Unhandled exception 0x%y at 0x%x", &code, &addr);

	ErrorHandler_Fail(msg.buffer);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void ErrorHandler_Init(const char* logFile) {
	SetUnhandledExceptionFilter(ErrorHandler_UnhandledFilter);
	/* TODO: Open log file */
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
	MessageBoxA(Window_GetWindowHandle(), msg, title, 0);
}

struct SymbolAndName { SYMBOL_INFO Symbol; char Name[256]; };
void ErrorHandler_Backtrace(STRING_TRANSIENT String* str) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	void* stack[56];
	UInt16 frames = CaptureStackBackTrace(0, 56, stack, NULL);

	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLen = 255;
	sym.Symbol.SizeOfStruct = sizeof(SYMBOL_INFO);

	String_AppendConst(str, "\r\nBacktrace: \r\n");
	UInt32 i;

	for (i = 0; i < frames; i++) {
		Int32 number = frames - i - 1;
		UInt64 addr = (UInt64)stack[i];

		/* TODO: SymGetLineFromAddr64 as well? */
		if (SymFromAddr(process, addr, NULL, &sym.Symbol)) {
			String_Format3(str, "%i) 0x%x - %c\r\n", &number, &addr, sym.Symbol.Name);
		} else {
			String_Format2(str, "%i) 0x%x\r\n", &number, &addr);
		}
	}
	String_AppendConst(str, "\r\n");
}
#elif CC_BUILD_NIX
void ErrorHandler_Init(const char* logFile) {
	/* TODO: Implement this */
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
	/* TODO: Implement this */
}

void ErrorHandler_Backtrace(STRING_TRANSIENT String* str) {
	/* TODO: Implement this */
}
#endif
