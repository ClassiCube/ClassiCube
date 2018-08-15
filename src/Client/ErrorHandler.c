#include "ErrorHandler.h"
#include "Platform.h"

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: write to log file */
}

void ErrorHandler_Log1(const UChar* format, const void* a1) {
	ErrorHandler_Log4(format, a1, NULL, NULL, NULL);
}
void ErrorHandler_Log2(const UChar* format, const void* a1, const void* a2) {
	ErrorHandler_Log4(format, a1, a2, NULL, NULL);
}
void ErrorHandler_Log3(const UChar* format, const void* a1, const void* a2, const void* a3) {
	ErrorHandler_Log4(format, a1, a2, a3, NULL);
}

void ErrorHandler_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	UChar msgBuffer[String_BufferSize(3071)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Chat_Add(&msg);

	String_AppendConst(&msg, Platform_NewLine);
	ErrorHandler_Backtrace(&msg);
	ErrorHandler_Log(&msg);
}

void ErrorHandler_LogError(ReturnCode result, const UChar* place) {
	ErrorHandler_Log4("&cError %y when %c", &result, place, NULL, NULL);
}

void ErrorHandler_FailWithCode(ReturnCode result, const UChar* raw_msg) {
	UChar logMsgBuffer[String_BufferSize(3071)];
	String logMsg = String_InitAndClearArray(logMsgBuffer);

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

void ErrorHandler_Fail(const UChar* raw_msg) { ErrorHandler_FailWithCode(0, raw_msg); }

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
	UChar msgBuffer[String_BufferSize(128)];
	String msg = String_InitAndClearArray(msgBuffer);

	UInt32 code = (UInt32)pInfo->ExceptionRecord->ExceptionCode;
	UInt64 addr = (UInt64)pInfo->ExceptionRecord->ExceptionAddress;
	String_Format2(&msg, "Unhandled exception 0x%y at 0x%x", &code, &addr);

	ErrorHandler_Fail(msgBuffer);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void ErrorHandler_Init(const UChar* logFile) {
	SetUnhandledExceptionFilter(ErrorHandler_UnhandledFilter);
	/* TODO: Open log file */
}

void ErrorHandler_ShowDialog(const UChar* title, const UChar* msg) {
	HWND win = GetActiveWindow(); /* TODO: It's probably wrong to use GetActiveWindow() here */
	MessageBoxA(win, msg, title, 0);
}

struct SymbolAndName { SYMBOL_INFO Symbol; UChar Name[256]; };
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
void ErrorHandler_Init(const UChar* logFile) {
	/* TODO: Implement this */
}

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: Implement this */
}

void ErrorHandler_Fail(const UChar* raw_msg) {
	/* TODO: Implement this */
	Platform_Exit(1);
}

void ErrorHandler_FailWithCode(ReturnCode code, const UChar* raw_msg) {
	/* TODO: Implement this */
	Platform_Exit(code);
}

void ErrorHandler_ShowDialog(const UChar* title, const UChar* msg) {
	/* TODO: Implement this */
}
#endif
