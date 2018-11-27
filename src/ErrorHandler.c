#include "ErrorHandler.h"
#include "Platform.h"
#include "Chat.h"
#include "Window.h"
#include "Funcs.h"
#include "Stream.h"

static void ErrorHandler_FailCommon(ReturnCode result, const char* raw_msg, void* ctx);
static void ErrorHandler_DumpCommon(String* str, void* ctx);
static void ErrorHandler_DumpRegisters(void* ctx);

#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <imagehlp.h>

struct StackPointers { uintptr_t Instruction, Frame, Stack; };
struct SymbolAndName { IMAGEHLP_SYMBOL Symbol; char Name[256]; };


/*########################################################################################################################*
*-------------------------------------------------------Info dumping------------------------------------------------------*
*#########################################################################################################################*/
static int ErrorHandler_GetFrames(CONTEXT* ctx, struct StackPointers* pointers, int max) {
	STACKFRAME frame = { 0 };
	frame.AddrPC.Mode     = AddrModeFlat;
	frame.AddrFrame.Mode  = AddrModeFlat;
	frame.AddrStack.Mode  = AddrModeFlat;
	DWORD type;

#if defined _M_IX86
	type = IMAGE_FILE_MACHINE_I386;
	frame.AddrPC.Offset    = ctx->Eip;
	frame.AddrFrame.Offset = ctx->Ebp;
	frame.AddrStack.Offset = ctx->Esp;
#elif defined _M_X64
	type = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrPC.Offset    = ctx->Rip;
	frame.AddrFrame.Offset = ctx->Rsp;
	frame.AddrStack.Offset = ctx->Rsp;
#elif defined _M_IA64
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
	int count;
	CONTEXT copy = *ctx;

	for (count = 0; count < max; count++) {
		if (!StackWalk(type, process, thread, &frame, &copy, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) break;
		if (!frame.AddrFrame.Offset) break;

		pointers[count].Instruction = frame.AddrPC.Offset;
		pointers[count].Frame       = frame.AddrFrame.Offset;
		pointers[count].Stack       = frame.AddrStack.Offset;
	}
	return count;
}

static BOOL CALLBACK ErrorHandler_DumpModule(const char* name, ULONG_PTR base, ULONG size, void* ctx) {
	String str; char strBuffer[STRING_SIZE * 4];
	uintptr_t beg, end;

	beg = base; end = base + (size - 1);
	String_InitArray(str, strBuffer);

	String_Format3(&str, "%c = %x-%x\r\n", name, &beg, &end);
	ErrorHandler_Log(&str);
	return true;
}

static void ErrorHandler_Backtrace(String* backtrace, void* ctx) {
	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLength = 255;
	sym.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);

	HANDLE process = GetCurrentProcess();
	struct StackPointers pointers[40];
	int i, frames = ErrorHandler_GetFrames((CONTEXT*)ctx, pointers, 40);

	for (i = 0; i < frames; i++) {
		int number = i + 1;
		uintptr_t addr = pointers[i].Instruction;

		char strBuffer[STRING_SIZE * 10];
		String str = String_FromArray(strBuffer);

		/* instruction pointer */
		if (SymGetSymFromAddr(process, addr, NULL, &sym.Symbol)) {
			String_Format3(&str, "%i) 0x%x - %c\r\n", &number, &addr, sym.Symbol.Name);
		} else {
			String_Format2(&str, "%i) 0x%x\r\n", &number, &addr);
		}

		/* frame and stack address */
		String_AppendString(backtrace, &str);
		String_Format2(&str, "  fp: %x, sp: %x\r\n", &pointers[i].Frame, &pointers[i].Stack);

		/* line number */
		IMAGEHLP_LINE line = { 0 }; DWORD lineOffset;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
		if (SymGetLineFromAddr(process, addr, &lineOffset, &line)) {
			String_Format2(&str, "  line %i in %c\r\n", &line.LineNumber, line.FileName);
		}

		/* module address is in */
		IMAGEHLP_MODULE module = { 0 };
		module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
		if (SymGetModuleInfo(process, addr, &module)) {
			String_Format2(&str, "  in module %c (%c)\r\n", module.ModuleName, module.ImageName);
		}
		ErrorHandler_Log(&str);
	}
	String_AppendConst(backtrace, "\r\n");
}

static void ErrorHandler_DumpCommon(String* str, void* ctx) {
	static String backtrace = String_FromConst("-- backtrace --\r\n");
	static String modules   = String_FromConst("-- modules --\r\n");
	HANDLE process = GetCurrentProcess();

	SymInitialize(process, NULL, TRUE);
	ErrorHandler_Log(&backtrace);
	ErrorHandler_Backtrace(str, ctx);

	ErrorHandler_Log(&modules);
	EnumerateLoadedModules(process, ErrorHandler_DumpModule, NULL);
}

static void ErrorHandler_DumpRegisters(void* ctx) {
	String str; char strBuffer[STRING_SIZE * 8];
	CONTEXT* r;
	if (!ctx) return;

	r = (CONTEXT*)ctx;
	String_InitArray(str, strBuffer);
	String_AppendConst(&str, "-- registers --\r\n");

#if defined _M_IX86
	String_Format3(&str, "eax=%x ebx=%x ecx=%x\r\n", &r->Eax, &r->Ebx, &r->Ecx);
	String_Format3(&str, "edx=%x esi=%x edi=%x\r\n", &r->Edx, &r->Esi, &r->Edi);
	String_Format3(&str, "eip=%x ebp=%x esp=%x\r\n", &r->Eip, &r->Ebp, &r->Esp);
#elif defined _M_X64
	String_Format3(&str, "rax=%x rbx=%x rcx=%x\r\n", &r->Rax, &r->Rbx, &r->Rcx);
	String_Format3(&str, "rdx=%x rsi=%x rdi=%x\r\n", &r->Rdx, &r->Rsi, &r->Rdi);
	String_Format3(&str, "rip=%x rbp=%x rsp=%x\r\n", &r->Rip, &r->Rbp, &r->Rsp);
	String_Format3(&str, "r8 =%x r9 =%x r10=%x\r\n", &r->R8,  &r->R9,  &r->R10);
	String_Format3(&str, "r11=%x r12=%x r13=%x\r\n", &r->R11, &r->R12, &r->R13);
	String_Format2(&str, "r14=%x r15=%x\r\n"       , &r->R14, &r->R15);
#elif defined _M_IA64
	String_Format3(&str, "r1 =%x r2 =%x r3 =%x\r\n", &r->IntGp,  &r->IntT0,  &r->IntT1);
	String_Format3(&str, "r4 =%x r5 =%x r6 =%x\r\n", &r->IntS0,  &r->IntS1,  &r->IntS2);
	String_Format3(&str, "r7 =%x r8 =%x r9 =%x\r\n", &r->IntS3,  &r->IntV0,  &r->IntT2);
	String_Format3(&str, "r10=%x r11=%x r12=%x\r\n", &r->IntT3,  &r->IntT4,  &r->IntSp);
	String_Format3(&str, "r13=%x r14=%x r15=%x\r\n", &r->IntTeb, &r->IntT5,  &r->IntT6);
	String_Format3(&str, "r16=%x r17=%x r18=%x\r\n", &r->IntT7,  &r->IntT8,  &r->IntT9);
	String_Format3(&str, "r19=%x r20=%x r21=%x\r\n", &r->IntT10, &r->IntT11, &r->IntT12);
	String_Format3(&str, "r22=%x r23=%x r24=%x\r\n", &r->IntT13, &r->IntT14, &r->IntT15);
	String_Format3(&str, "r25=%x r26=%x r27=%x\r\n", &r->IntT16, &r->IntT17, &r->IntT18);
	String_Format3(&str, "r28=%x r29=%x r30=%x\r\n", &r->IntT19, &r->IntT20, &r->IntT21);
	String_Format3(&str, "r31=%x nat=%x pre=%x\r\n", &r->IntT22, &r->IntNats,&r->Preds);
#else
#error "Unknown machine type"
#endif
	ErrorHandler_Log(&str);
}


/*########################################################################################################################*
*------------------------------------------------------Error handling-----------------------------------------------------*
*#########################################################################################################################*/
static LONG WINAPI ErrorHandler_UnhandledFilter(struct _EXCEPTION_POINTERS* pInfo) {
	String msg; char msgBuffer[STRING_SIZE * 2 + 1];
	uint32_t code;
	uintptr_t addr;

	code = (uint32_t)pInfo->ExceptionRecord->ExceptionCode;
	addr = (uintptr_t)pInfo->ExceptionRecord->ExceptionAddress;

	String_InitArray_NT(msg, msgBuffer);
	String_Format2(&msg, "Unhandled exception 0x%h at 0x%x", &code, &addr);
	msg.buffer[msg.length] = '\0';

	ErrorHandler_DumpRegisters(pInfo->ContextRecord);
	ErrorHandler_FailCommon(0, msg.buffer, pInfo->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}

void ErrorHandler_Init(void) {
	SetUnhandledExceptionFilter(ErrorHandler_UnhandledFilter);
}

/* Don't want compiler doing anything fancy with registers */
#if _MSC_VER
#pragma optimize ("", off)
#endif
void ErrorHandler_Fail2(ReturnCode result, const char* raw_msg) {
	CONTEXT ctx;
#ifndef _M_IX86
	/* This method is guaranteed to exist on 64 bit windows */
	/* It is missing in 32 bit Windows 2000 however */
	RtlCaptureContext(&ctx);
#elif _MSC_VER
	/* Stack frame layout on x86: */
	/* [ebp] is previous frame's EBP */
	/* [ebp+4] is previous frame's EIP (return address) */
	/* address of [ebp+8] is previous frame's ESP */
	__asm {
		mov eax, [ebp]
		mov [ctx.Ebp], eax
		mov eax, [ebp+4]
		mov [ctx.Eip], eax
		lea eax, [ebp+8]
		mov [ctx.Esp], eax
		mov [ctx.ContextFlags], CONTEXT_CONTROL
	}
#else
	int32_t _ebp, _eip, _esp;
	/* TODO: I think this is right, not sure.. */
	__asm__(
		"mov 0(%%ebp), %%eax \n\t"
		"mov %%eax, %0       \n\t"
		"mov 4(%%ebp), %%eax \n\t"
		"mov %%eax, %1       \n\t"
		"lea 8(%%ebp), %%eax \n\t"
		"mov %%eax, %2"
		: "=m" (_ebp), "=m" (_eip), "=m" (_esp)
		:
		: "eax", "memory");

	ctx.Ebp = _ebp;
	ctx.Eip = _eip;
	ctx.Esp = _esp;
	ctx.ContextFlags = CONTEXT_CONTROL;
#endif

	ErrorHandler_FailCommon(result, raw_msg, &ctx);
}
#if _MSC_VER
#pragma optimize ("", on)
#endif
#endif
/* POSIX is mainly shared between Linux and OSX */
#ifdef CC_BUILD_POSIX
#include <ucontext.h>
#include <execinfo.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

/*########################################################################################################################*
*-------------------------------------------------------Info dumping------------------------------------------------------*
*#########################################################################################################################*/
static void ErrorHandler_Backtrace(String* backtrace_, void* ctx) {
	String str; char strBuffer[STRING_SIZE * 5];
	void* addrs[40];
	int i, frames, num;
	char** strings;
	uintptr_t addr;

	frames  = backtrace(addrs, 40);
	strings = backtrace_symbols(addrs, frames);

	for (i = 0; i < frames; i++) {
		num  = i + 1;
		addr = (uintptr_t)addrs[i];
		String_InitArray(str, strBuffer);

		/* instruction pointer */
		if (strings && strings[i]) {
			String_Format3(&str, "%i) 0x%x - %c\n", &num, &addr, strings[i]);
		} else {
			String_Format2(&str, "%i) 0x%x\n", &num, &addr);
		}

		String_AppendString(backtrace_, &str);
		ErrorHandler_Log(&str);
	}

	String_AppendConst(backtrace_, "\n");
	free(strings);
}


/*########################################################################################################################*
*------------------------------------------------------Error handling-----------------------------------------------------*
*#########################################################################################################################*/
static void ErrorHandler_SignalHandler(int sig, siginfo_t* info, void* ctx) {
	String msg; char msgBuffer[STRING_SIZE * 2 + 1];
	int type, code;
	uintptr_t addr;

	/* Uninstall handler to avoid chance of infinite loop */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS,  SIG_DFL);
	signal(SIGILL,  SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE,  SIG_DFL);

	type = info->si_signo;
	code = info->si_code;
	addr = (uintptr_t)info->si_addr;

	String_InitArray_NT(msg, msgBuffer);
	String_Format3(&msg, "Unhandled signal %i (code %i) at 0x%x", &type, &code, &addr);
	msg.buffer[msg.length] = '\0';

	ErrorHandler_DumpRegisters(ctx);
	ErrorHandler_FailCommon(0, msg.buffer, ctx);
}

void ErrorHandler_Init(void) {
	struct sigaction sa, old;
	sa.sa_sigaction = ErrorHandler_SignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, &old);
	sigaction(SIGBUS,  &sa, &old);
	sigaction(SIGILL,  &sa, &old);
	sigaction(SIGABRT, &sa, &old);
	sigaction(SIGFPE,  &sa, &old);
}

void ErrorHandler_Fail2(ReturnCode result, const char* raw_msg) {
	ucontext_t ctx;
	getcontext(&ctx);
	ErrorHandler_FailCommon(result, raw_msg, &ctx);
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Info dumping------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_NIX || defined CC_BUILD_SOLARIS
static void ErrorHandler_DumpRegisters(void* ctx) {
	String str; char strBuffer[STRING_SIZE * 8];
	mcontext_t r;
	if (!ctx) return;

	r = ((ucontext_t*)ctx)->uc_mcontext;
	String_InitArray(str, strBuffer);
	String_AppendConst(&str, "-- registers --\n");

	/* TODO: There must be a better way of getting these.. */
#ifdef __i386__
	String_Format3(&str, "eax=%x ebx=%x ecx=%x\n", &r.gregs[11], &r.gregs[8], &r.gregs[10]);
	String_Format3(&str, "edx=%x esi=%x edi=%x\n", &r.gregs[9],  &r.gregs[5], &r.gregs[4]);
	String_Format3(&str, "eip=%x ebp=%x esp=%x\n", &r.gregs[14], &r.gregs[6], &r.gregs[7]);
#elif __x86_64__
	String_Format3(&str, "rax=%x rbx=%x rcx=%x\n", &r.gregs[13], &r.gregs[11], &r.gregs[14]);
	String_Format3(&str, "rdx=%x rsi=%x rdi=%x\n", &r.gregs[12], &r.gregs[9],  &r.gregs[8]);
	String_Format3(&str, "rip=%x rbp=%x rsp=%x\n", &r.gregs[16], &r.gregs[10], &r.gregs[15]);
	String_Format3(&str, "r8 =%x r9 =%x r10=%x\n", &r.gregs[0],  &r.gregs[1],  &r.gregs[2]);
	String_Format3(&str, "r11=%x r12=%x r13=%x\n", &r.gregs[3],  &r.gregs[4],  &r.gregs[5]);
	String_Format2(&str, "r14=%x r15=%x\n",        &r.gregs[6],  &r.gregs[7]);
#else
#error "Unknown machine type"
#endif
	ErrorHandler_Log(&str);
}

static void ErrorHandler_DumpMemoryMap(void) {
	String str; char strBuffer[STRING_SIZE * 5];
	int n, fd;
	
	fd = open("/proc/self/maps", O_RDONLY);
	if (fd < 0) return;
	String_InitArray(str, strBuffer);

	while ((n = read(fd, str.buffer, str.capacity)) > 0) {
		str.length = n;
		ErrorHandler_Log(&str);
	}

	close(fd);
}

static void ErrorHandler_DumpCommon(String* str, void* ctx) {
	static String backtrace = String_FromConst("-- backtrace --\n");
	static String memMap    = String_FromConst("-- memory map --\n");

	ErrorHandler_Log(&backtrace);
	ErrorHandler_Backtrace(str, ctx);
	ErrorHandler_Log(&memMap);
	ErrorHandler_DumpMemoryMap();
}
#elif defined CC_BUILD_OSX
static void ErrorHandler_DumpRegisters(void* ctx) {
	String str; char strBuffer[STRING_SIZE * 8];
	mcontext_t r;
	if (!ctx) return;

	r = ((ucontext_t*)ctx)->uc_mcontext;
	String_InitArray(str, strBuffer);
	String_AppendConst(&str, "-- registers --\n");

	/* You can find these definitions at /usr/include/mach/i386/_structs.h */
#if defined __i386__
	String_Format3(&str, "eax=%x ebx=%x ecx=%x\n", &r->__ss.__eax, &r->__ss.__ebx, &r->__ss.__ecx);
	String_Format3(&str, "edx=%x esi=%x edi=%x\n", &r->__ss.__edx, &r->__ss.__esi, &r->__ss.__edi);
	String_Format3(&str, "eip=%x ebp=%x esp=%x\n", &r->__ss.__eip, &r->__ss.__ebp, &r->__ss.__esp);
#elif defined __x86_64__
	String_Format3(&str, "rax=%x rbx=%x rcx=%x\n", &r->__ss.__rax, &r->__ss.__rbx, &r->__ss.__rcx);
	String_Format3(&str, "rdx=%x rsi=%x rdi=%x\n", &r->__ss.__rdx, &r->__ss.__rsi, &r->__ss.__rdi);
	String_Format3(&str, "rip=%x rbp=%x rsp=%x\n", &r->__ss.__rip, &r->__ss.__rbp, &r->__ss.__rsp);
	String_Format3(&str, "r8 =%x r9 =%x r10=%x\n", &r->__ss.__r8,  &r->__ss.__r9,  &r->__ss.__r10);
	String_Format3(&str, "r11=%x r12=%x r13=%x\n", &r->__ss.__r11, &r->__ss.__r12, &r->__ss.__r13);
	String_Format2(&str, "r14=%x r15=%x\n",        &r->__ss.__r14, &r->__ss.__r15);
#else
#error "Unknown machine type"
#endif
	ErrorHandler_Log(&str);
}

static void ErrorHandler_DumpCommon(String* str, void* ctx) {
	static String backtrace = String_FromConst("-- backtrace --\n");
	ErrorHandler_Log(&backtrace);
	ErrorHandler_Backtrace(str, ctx);
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Common---------------------------------------------------------*
*#########################################################################################################################*/
static FileHandle logFile;
static struct Stream logStream;
static bool logOpen;

void ErrorHandler_Log(const String* msg) {
	static String path = String_FromConst("client.log");
	ReturnCode res;

	if (!logOpen) {
		logOpen = true;
		res     = File_Append(&logFile, &path);
		if (!res) Stream_FromFile(&logStream, logFile);
	}

	if (!logStream.Meta.File) return;
	Stream_Write(&logStream, msg->buffer, msg->length);
}

static void ErrorHandler_FailCommon(ReturnCode result, const char* raw_msg, void* ctx) {	
	String msg; char msgBuffer[3070 + 1];
	String_InitArray_NT(msg, msgBuffer);

	String_Format3(&msg, "ClassiCube crashed.%cMessage: %c%c", Platform_NewLine, raw_msg, Platform_NewLine);
	#ifdef CC_COMMIT_SHA
	String_Format2(&msg, "Commit SHA: %c%c", CC_COMMIT_SHA, Platform_NewLine);
	#endif

	if (result) {
		String_Format2(&msg, "%h%c", &result, Platform_NewLine);
	} else { result = 1; }

	ErrorHandler_Log(&msg);
	ErrorHandler_DumpCommon(&msg, ctx);
	if (logStream.Meta.File) File_Close(logFile);

	String_AppendConst(&msg, "Full details of the crash have been logged to 'client.log'.\n");
	String_AppendConst(&msg, "Please report the crash on the ClassiCube forums so we can fix it.");

	msg.buffer[msg.length] = '\0';
	Window_ShowDialog("We're sorry", msg.buffer);
	Platform_Exit(result);
}

void ErrorHandler_Fail(const char* raw_msg) { ErrorHandler_Fail2(0, raw_msg); }
