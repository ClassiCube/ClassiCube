#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"
#include "Window.h"
static void ErrorHandler_FailCore(ReturnCode result, const char* raw_msg, void* ctx);
static void ErrorHandler_Backtrace(STRING_TRANSIENT String* str, void* ctx);
static void ErrorHandler_Registers(STRING_TRANSIENT String* str, void* ctx);

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <dbghelp.h>

struct StackPointers { UInt64 Instruction, Frame, Stack; };
struct SymbolAndName { IMAGEHLP_SYMBOL64 Symbol; char Name[256]; };

static LONG WINAPI ErrorHandler_UnhandledFilter(struct _EXCEPTION_POINTERS* pInfo) {
	/* TODO: Write processor state to file*/
	char msgBuffer[128 + 1] = { 0 };
	String msg = { msgBuffer, 0, 128 };

	UInt32 code = (UInt32)pInfo->ExceptionRecord->ExceptionCode;
	UInt64 addr = (UInt64)pInfo->ExceptionRecord->ExceptionAddress;
	String_Format2(&msg, "Unhandled exception 0x%y at 0x%x", &code, &addr);

	ErrorHandler_FailCore(0, msg.buffer, pInfo->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void ErrorHandler_Init(const char* logFile) {
	SetUnhandledExceptionFilter(ErrorHandler_UnhandledFilter);
	/* TODO: Open log file */
}

void ErrorHandler_ShowDialog(const char* title, const char* msg) {
	MessageBoxA(Window_GetWindowHandle(), msg, title, 0);
}

void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	CONTEXT ctx;
	RtlCaptureContext(&ctx);
	ErrorHandler_FailCore(result, raw_msg, &ctx);
}

static Int32 ErrorHandler_GetFrames(CONTEXT* ctx, struct StackPointers* pointers, Int32 max) {
	STACKFRAME64 frame = { 0 };
	frame.AddrPC.Mode     = AddrModeFlat;
	frame.AddrFrame.Mode  = AddrModeFlat;
	frame.AddrStack.Mode  = AddrModeFlat;	
	DWORD type;

#ifdef _M_IX86
	type = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset    = ctx->Eip;
	frame.AddrFrame.Offset = ctx->Ebp;
	frame.AddrStack.Offset = ctx->Esp;
#elif _M_X64
	type = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset    = ctx->Rip;
	frame.AddrFrame.Offset = ctx->Rsp;
	frame.AddrStack.Offset = ctx->Rsp;
#elif _M_IA64
	type = IMAGE_FILE_MACHINE_IA64;
	frame.AddrPC.Offset     = ctx->StIIP;
	frame.AddrFrame.Offset  = ctx->IntSp;
	frame.AddrBStore.Offset = ctx->RsBSP;
	frame.AddrStack.Offset  = ctx->IntSp;
	frame.AddrBStore.Mode   = AddrModeFlat;
#else
	#error "Unknown machine type"
#endif

	HANDLE process = GetCurrentProcess();
	HANDLE thread  = GetCurrentThread();
	Int32 count;
	CONTEXT copy = *ctx;

	for (count = 0; count < max; count++) {		
		if (!StackWalk64(type, process, thread, &frame, &copy, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
		if (!frame.AddrFrame.Offset) break;

		pointers[count].Instruction = frame.AddrPC.Offset;
		pointers[count].Frame       = frame.AddrFrame.Offset;
		pointers[count].Stack       = frame.AddrStack.Offset;
	}
	return count;
}

static void ErrorHandler_Backtrace(STRING_TRANSIENT String* str, void* ctx) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);

	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLength = 255;
	sym.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);

	struct StackPointers pointers[40];
	Int32 frames = ErrorHandler_GetFrames((CONTEXT*)ctx, pointers, 40);

	String_AppendConst(str, "\r\nBacktrace: \r\n");
	UInt32 i;

	for (i = 0; i < frames; i++) {
		Int32 number = i + 1;
		UInt64 addr = (UInt64)pointers[i].Instruction;

		/* TODO: Log ESP and EBP here too */
		/* TODO: SymGetLineFromAddr64 as well? */
		/* TODO: Log module here too */
		if (SymGetSymFromAddr64(process, addr, NULL, &sym.Symbol)) {
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

void ErrorHandler_Backtrace(STRING_TRANSIENT String* str, void* ctx) {
	/* TODO: Implement this */
}

void ErrorHandler_FailWithCode(ReturnCode result, const char* raw_msg) {
	/* TODO: Implement this */
	ErrorHandler_FailCore(result, raw_msg, NULL);
}
#endif

static void ErrorHandler_FailCore(ReturnCode result, const char* raw_msg, void* ctx) {
	char logMsgBuffer[3070 + 1] = { 0 };
	String logMsg = { logMsgBuffer, 0, 3070 };

	String_Format1(&logMsg, "ClassiCube crashed.\nMessge: %c\n", raw_msg);
	if (result) { String_Format1(&logMsg, "%y\n", &result); } else { result = 1; }

	//ErrorHandler_Registers(ctx, &logMsg);
	ErrorHandler_Backtrace(&logMsg, ctx);
	/* TODO: write to log file */

	String_AppendConst(&logMsg,
		"Please report the crash to github.com/UnknownShadow200/ClassicalSharp/issues so we can fix it.");
	ErrorHandler_ShowDialog("We're sorry", logMsg.buffer);
	Platform_Exit(result);
}

void ErrorHandler_Log(STRING_PURE String* msg) {
	/* TODO: write to log file */
}

void ErrorHandler_Fail(const char* raw_msg) { ErrorHandler_FailWithCode(0, raw_msg); }
