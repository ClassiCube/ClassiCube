#include "Logger.h"
#include "String.h"
#include "Platform.h"
#include "Window.h"
#include "Funcs.h"
#include "Stream.h"
#include "Errors.h"
#include "Utils.h"

#if defined CC_BUILD_WEB
/* Can't see native CPU state with javascript */
#undef CC_BUILD_POSIX
#elif defined CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <imagehlp.h>
#elif defined CC_BUILD_OPENBSD || defined CC_BUILD_HAIKU
#include <signal.h>
/* OpenBSD doesn't provide sys/ucontext.h */
#elif defined CC_BUILD_LINUX || defined CC_BUILD_ANDROID
/* Need to define this to get REG_ constants */
#define _GNU_SOURCE
#include <sys/ucontext.h>
#include <signal.h>
#elif defined CC_BUILD_POSIX
#include <signal.h>
#include <sys/ucontext.h>
#endif
#ifdef CC_BUILD_DARWIN
/* Need this to detect macOS < 10.4, and switch to NS* api instead if so */
#include <AvailabilityMacros.h>
#endif


/*########################################################################################################################*
*----------------------------------------------------------Warning--------------------------------------------------------*
*#########################################################################################################################*/
void Logger_DialogWarn(const cc_string* msg) {
	cc_string dst; char dstBuffer[512];
	String_InitArray_NT(dst, dstBuffer);

	String_Copy(&dst, msg);
	dst.buffer[dst.length] = '\0';
	Window_ShowDialog(Logger_DialogTitle, dst.buffer);
}
const char* Logger_DialogTitle = "Error";
Logger_DoWarn Logger_WarnFunc  = Logger_DialogWarn;

/* Returns a description for some ClassiCube specific error codes */
static const char* GetCCErrorDesc(cc_result res) {
	switch (res) {
	case ERR_END_OF_STREAM:    return "End of stream";
	case ERR_NOT_SUPPORTED:    return "Operation not supported";
	case ERR_INVALID_ARGUMENT: return "Invalid argument";
	case ERR_OUT_OF_MEMORY:    return "Out of memory";

	case OGG_ERR_INVALID_SIG:  return "Invalid OGG signature";
	case OGG_ERR_VERSION:      return "Invalid OGG format version";

	case WAV_ERR_STREAM_HDR:  return "Invalid WAV header";
	case WAV_ERR_STREAM_TYPE: return "Invalid WAV type";
	case WAV_ERR_DATA_TYPE:   return "Unsupported WAV audio format";
	case WAV_ERR_NO_DATA:     return "No audio in WAV";

	case ZIP_ERR_TOO_MANY_ENTRIES: return "Cannot load .zip files with over 1024 entries";

	case PNG_ERR_INVALID_SIG:      return "Only PNG images supported";
	case PNG_ERR_INVALID_HDR_SIZE: return "Invalid PNG header size";
	case PNG_ERR_TOO_WIDE:         return "PNG image too wide";
	case PNG_ERR_TOO_TALL:         return "PNG image too tall";
	case PNG_ERR_INTERLACED:       return "Interlaced PNGs unsupported";
	case PNG_ERR_REACHED_IEND:     return "Incomplete PNG image data";
	case PNG_ERR_NO_DATA:          return "No image in PNG";
	case PNG_ERR_INVALID_SCANLINE: return "Invalid PNG scanline type";

	case NBT_ERR_UNKNOWN:   return "Unknown NBT tag type";
	case CW_ERR_ROOT_TAG:   return "Invalid root NBT tag";
	case CW_ERR_STRING_LEN: return "NBT string too long";

	case ERR_DOWNLOAD_INVALID: return "Website denied download or doesn't exist";
	}
	return NULL;
}

/* Appends more detailed information about an error if possible */
static void AppendErrorDesc(cc_string* msg, cc_result res, Logger_DescribeError describeErr) {
	const char* cc_err;
	cc_string err; char errBuffer[128];
	String_InitArray(err, errBuffer);

	cc_err = GetCCErrorDesc(res);
	if (cc_err) {
		String_Format1(msg, "\n  Error meaning: %c", cc_err);
	} else if (describeErr(res, &err)) {
		String_Format1(msg, "\n  Error meaning: %s", &err);
	}
}

void Logger_FormatWarn(cc_string* msg, cc_result res, const char* action, Logger_DescribeError describeErr) {
	String_Format2(msg, res < 20000 ? "Error %i when %c" : "Error %h when %c",
					&res, action);
	AppendErrorDesc(msg, res, describeErr);
}

void Logger_FormatWarn2(cc_string* msg, cc_result res, const char* action, const cc_string* path, Logger_DescribeError describeErr) {
	String_Format3(msg, res < 20000 ? "Error %i when %c '%s'" : "Error %h when %c '%s'",
					&res, action, path);
	AppendErrorDesc(msg, res, describeErr);
}

static cc_bool DescribeSimple(cc_result res, cc_string* dst) { return false; }
void Logger_SimpleWarn(cc_result res, const char* action) {
	Logger_Warn(res, action, DescribeSimple);
}
void Logger_SimpleWarn2(cc_result res, const char* action, const cc_string* path) {
	Logger_Warn2(res, action, path, DescribeSimple);
}

void Logger_Warn(cc_result res, const char* action, Logger_DescribeError describeErr) {
	cc_string msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);

	Logger_FormatWarn(&msg, res, action, describeErr);
	Logger_WarnFunc(&msg);
}

void Logger_Warn2(cc_result res, const char* action, const cc_string* path, Logger_DescribeError describeErr) {
	cc_string msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);

	Logger_FormatWarn2(&msg, res, action, path, describeErr);
	Logger_WarnFunc(&msg);
}

void Logger_DynamicLibWarn(const char* action, const cc_string* path) {
	cc_string err; char errBuffer[128];
	cc_string msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);
	String_InitArray(err, errBuffer);

	String_Format2(&msg, "Error %c '%s'", action, path);
	if (DynamicLib_DescribeError(&err)) {
		String_Format1(&msg, ":\n    %s", &err);
	}
	Logger_WarnFunc(&msg);
}

void Logger_SysWarn(cc_result res, const char* action) {
	Logger_Warn(res, action,  Platform_DescribeError);
}
void Logger_SysWarn2(cc_result res, const char* action, const cc_string* path) {
	Logger_Warn2(res, action, path, Platform_DescribeError);
}


/*########################################################################################################################*
*-------------------------------------------------------Backtracing-------------------------------------------------------*
*#########################################################################################################################*/
static void PrintFrame(cc_string* str, cc_uintptr addr, cc_uintptr symAddr, const char* symName, const char* modName) {
	cc_string module;
	int offset;
	if (!modName) modName = "???";

	module = String_FromReadonly(modName);
	Utils_UNSAFE_GetFilename(&module);
	String_Format2(str, "0x%x - %s", &addr, &module);
	
	if (symName && symName[0]) {
		offset = (int)(addr - symAddr);
		String_Format2(str, "(%c+%i)" _NL, symName, &offset);
	} else {
		String_AppendConst(str, _NL);
	}
}

#if defined CC_BUILD_WEB
void Logger_Backtrace(cc_string* trace, void* ctx) { }
#elif defined CC_BUILD_WIN
struct SymbolAndName { IMAGEHLP_SYMBOL Symbol; char Name[256]; };

static int GetFrames(CONTEXT* ctx, cc_uintptr* addrs, int max) {
	STACKFRAME frame = { 0 };
	HANDLE process, thread;
	int count, type;
	CONTEXT copy;

	frame.AddrPC.Mode    = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;

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
#else
	/* Always available after XP, so use that */
	return RtlCaptureStackBackTrace(0, max, (void**)addrs, NULL);
#endif
	process = GetCurrentProcess();
	thread  = GetCurrentThread();
	copy    = *ctx;

	for (count = 0; count < max; count++) {
		if (!StackWalk(type, process, thread, &frame, &copy, NULL, SymFunctionTableAccess, SymGetModuleBase, NULL)) break;
		if (!frame.AddrFrame.Offset) break;
		addrs[count] = frame.AddrPC.Offset;
	}
	return count;
}

void Logger_Backtrace(cc_string* trace, void* ctx) {
	char strBuffer[512]; cc_string str;
	cc_uintptr addrs[40];
	int i, frames;
	HANDLE process;
	cc_uintptr addr;

	process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	frames  = GetFrames((CONTEXT*)ctx, addrs, 40);

	for (i = 0; i < frames; i++) {
		addr = addrs[i];
		String_InitArray(str, strBuffer);

		struct SymbolAndName s = { 0 };
		s.Symbol.MaxNameLength = 255;
		s.Symbol.SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
		SymGetSymFromAddr(process, addr, NULL, &s.Symbol);

		IMAGEHLP_MODULE m = { 0 };
		m.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
		SymGetModuleInfo(process, addr, &m);

		PrintFrame(&str, addr, s.Symbol.Address, s.Symbol.Name, m.ModuleName);
		String_AppendString(trace, &str);

/* This function only works for .pdb debug info */
/* This function is also missing on Windows98 + KernelEX */
#if _MSC_VER
		IMAGEHLP_LINE line = { 0 }; DWORD lineOffset;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
		if (SymGetLineFromAddr(process, addr, &lineOffset, &line)) {
			String_Format2(&str, "  line %i in %c\r\n", &line.LineNumber, line.FileName);
		}
#endif
		Logger_Log(&str);
	}
	String_AppendConst(trace, _NL);
}
#elif defined CC_BUILD_POSIX
#ifndef __USE_GNU
/* need to define __USE_GNU for dladdr */
#define __USE_GNU
#endif

#if defined MAC_OS_X_VERSION_MIN_REQUIRED && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
static void DumpFrame(cc_string* trace, void* addr) {
	cc_string str; char strBuffer[384];
	String_InitArray(str, strBuffer);
	/* alas NSModuleForSymbol doesn't work with raw addresses */

	PrintFrame(&str, (cc_uintptr)addr, 0, NULL, NULL);
	String_AppendString(trace, &str);
	Logger_Log(&str);
}
#else
#include <dlfcn.h>
#undef __USE_GNU

static void DumpFrame(cc_string* trace, void* addr) {
	cc_string str; char strBuffer[384];
	Dl_info s;

	String_InitArray(str, strBuffer);
	s.dli_sname = NULL;
	s.dli_fname = NULL;
	dladdr(addr, &s);

	PrintFrame(&str, (cc_uintptr)addr, (cc_uintptr)s.dli_saddr, s.dli_sname, s.dli_fname);
	String_AppendString(trace, &str);
	Logger_Log(&str);
}
#endif

#if defined CC_BUILD_ANDROID
/* android's bionic libc doesn't provide backtrace (execinfo.h) */
#include <unwind.h>

static _Unwind_Reason_Code UnwindFrame(struct _Unwind_Context* ctx, void* arg) {
	cc_uintptr addr = _Unwind_GetIP(ctx);
	if (!addr) return _URC_END_OF_STACK;

	DumpFrame((cc_string*)arg, (void*)addr);
	return _URC_NO_REASON;
}

void Logger_Backtrace(cc_string* trace, void* ctx) {
	_Unwind_Backtrace(UnwindFrame, trace);
	String_AppendConst(trace, _NL);
}
#elif defined CC_BUILD_DARWIN
/* backtrace is only available on macOS since 10.5 */
void Logger_Backtrace(cc_string* trace, void* ctx) {
	void* addrs[40];
	unsigned i, frames;
	/* See lldb/tools/debugserver/source/MacOSX/stack_logging.h */
	/* backtrace uses this internally too, and exists since macOS 10.1 */
	extern void thread_stack_pcs(void** buffer, unsigned max, unsigned* nb);

	thread_stack_pcs(addrs, 40, &frames);
	for (i = 1; i < frames; i++) { /* 1 to skip thread_stack_pcs frame */
		DumpFrame(trace, addrs[i]);
	}
	String_AppendConst(trace, _NL);
}
#else
#include <execinfo.h>
void Logger_Backtrace(cc_string* trace, void* ctx) {
	void* addrs[40];
	int i, frames = backtrace(addrs, 40);

	for (i = 0; i < frames; i++) {
		DumpFrame(trace, addrs[i]);
	}
	String_AppendConst(trace, _NL);
}
#endif
#endif /* CC_BUILD_POSIX */
static void DumpBacktrace(cc_string* str, void* ctx) {
	static const cc_string backtrace = String_FromConst("-- backtrace --" _NL);
	Logger_Log(&backtrace);
	Logger_Backtrace(str, ctx);
}


/*########################################################################################################################*
*-----------------------------------------------------CPU registers-------------------------------------------------------*
*#########################################################################################################################*/
/* Unfortunately, operating systems vary wildly in how they name and access registers for dumping */
/* So this is the simplest way to avoid duplicating code on each platform */
#define Dump_X86() \
String_Format3(str, "eax=%x ebx=%x ecx=%x" _NL, REG_GET(ax,AX), REG_GET(bx,BX), REG_GET(cx,CX));\
String_Format3(str, "edx=%x esi=%x edi=%x" _NL, REG_GET(dx,DX), REG_GET(si,SI), REG_GET(di,DI));\
String_Format3(str, "eip=%x ebp=%x esp=%x" _NL, REG_GET(ip,IP), REG_GET(bp,BP), REG_GET(sp,SP));

#define Dump_X64() \
String_Format3(str, "rax=%x rbx=%x rcx=%x" _NL, REG_GET(ax,AX), REG_GET(bx,BX), REG_GET(cx,CX));\
String_Format3(str, "rdx=%x rsi=%x rdi=%x" _NL, REG_GET(dx,DX), REG_GET(si,SI), REG_GET(di,DI));\
String_Format3(str, "rip=%x rbp=%x rsp=%x" _NL, REG_GET(ip,IP), REG_GET(bp,BP), REG_GET(sp,SP));\
String_Format3(str, "r8 =%x r9 =%x r10=%x" _NL, REG_GET(8,8),   REG_GET(9,9),   REG_GET(10,10));\
String_Format3(str, "r11=%x r12=%x r13=%x" _NL, REG_GET(11,11), REG_GET(12,12), REG_GET(13,13));\
String_Format2(str, "r14=%x r15=%x" _NL,        REG_GET(14,14), REG_GET(15,15));

#define Dump_PPC() \
String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, REG_GNUM(28), REG_GNUM(29), REG_GNUM(30), REG_GNUM(31)); \
String_Format3(str, "pc =%x lr =%x ctr=%x" _NL,  REG_GET_PC(), REG_GET_LR(), REG_GET_CTR());

#define Dump_ARM32() \
String_Format3(str, "r0 =%x r1 =%x r2 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2));\
String_Format3(str, "r3 =%x r4 =%x r5 =%x" _NL, REG_GNUM(3),  REG_GNUM(4),  REG_GNUM(5));\
String_Format3(str, "r6 =%x r7 =%x r8 =%x" _NL, REG_GNUM(6),  REG_GNUM(7),  REG_GNUM(8));\
String_Format3(str, "r9 =%x r10=%x fp =%x" _NL, REG_GNUM(9),  REG_GNUM(10), REG_GET_FP());\
String_Format3(str, "ip =%x sp =%x lr =%x" _NL, REG_GET_IP(),REG_GET(sp,Sp),REG_GET(lr,Lr));\
String_Format1(str, "pc =%x"               _NL, REG_GET(pc,Pc));

#define Dump_ARM64() \
String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format3(str, "r28=%x r29=%x r30=%x" _NL,        REG_GNUM(28), REG_GNUM(29), REG_GNUM(30)); \
String_Format2(str, "sp =%x pc =%x" _NL,               REG_GET_SP(), REG_GET_PC());

#define Dump_SPARC() \
String_Format4(str, "o0=%x o1=%x o2=%x o3=%x" _NL, REG_GET(o0,O0), REG_GET(o1,O1), REG_GET(o2,O2), REG_GET(o3,O3)); \
String_Format4(str, "o4=%x o5=%x o6=%x o7=%x" _NL, REG_GET(o4,O4), REG_GET(o5,O5), REG_GET(o6,O6), REG_GET(o7,O7)); \
String_Format4(str, "g1=%x g2=%x g3=%x g4=%x" _NL, REG_GET(g1,G1), REG_GET(g2,G2), REG_GET(g3,G3), REG_GET(g4,G4)); \
String_Format4(str, "g5=%x g6=%x g7=%x y =%x" _NL, REG_GET(g5,G5), REG_GET(g6,G6), REG_GET(g7,G7), REG_GET( y, Y)); \
String_Format2(str, "pc=%x nc=%x" _NL,             REG_GET(pc,PC), REG_GET(npc,nPC));

#define Dump_RISCV() \
String_Format4(str, "pc =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GET_PC(), REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format4(str, "r28=%x r29=%x r30=%x r31=%x" _NL, REG_GNUM(28), REG_GNUM(29), REG_GNUM(30), REG_GNUM(31));

#if defined CC_BUILD_WEB
static void PrintRegisters(cc_string* str, void* ctx) { }
#elif defined CC_BUILD_WIN
/* See CONTEXT in WinNT.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	CONTEXT* r = (CONTEXT*)ctx;
#if defined _M_IX86
	#define REG_GET(reg, ign) &r->E ## reg
	Dump_X86()
#elif defined _M_X64
	#define REG_GET(reg, ign) &r->R ## reg
	Dump_X64()
#elif defined _M_ARM
	#define REG_GNUM(num)     &r->R ## num
	#define REG_GET(ign, reg) &r-> ## reg
	#define REG_GET_FP()      &r->R11
	#define REG_GET_IP()      &r->R12
	Dump_ARM32()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_DARWIN && __DARWIN_UNIX03
/* See /usr/include/mach/i386/_structs.h (macOS 10.5+) */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r->__ss.__e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r->__ss.__r##reg
	Dump_X64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->__ss.__r[num]
	#define REG_GET(reg, ign) &r->__ss.__ ## reg
	#define REG_GET_FP()      &r->__ss.__r[11]
	#define REG_GET_IP()      &r->__ss.__pc
	Dump_ARM32()
#elif defined __ppc__
	#define REG_GNUM(num)     &r->__ss.__r##num
	#define REG_GET_PC()      &r->__ss.__srr0
	#define REG_GET_LR()      &r->__ss.__lr
	#define REG_GET_CTR()     &r->__ss.__ctr
	Dump_PPC()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_DARWIN
/* See /usr/include/mach/i386/thread_status.h (macOS 10.4) */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r->ss.e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r->ss.r##reg
	Dump_X64()
#elif defined __ppc__
	#define REG_GNUM(num)     &r->ss.r##num
	#define REG_GET_PC()      &r->ss.srr0
	#define REG_GET_LR()      &r->ss.lr
	#define REG_GET_CTR()     &r->ss.ctr
	Dump_PPC()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_LINUX || defined CC_BUILD_ANDROID
/* See /usr/include/sys/ucontext.h */
static void PrintRegisters(cc_string* str, void* ctx) {
#if __PPC__ && __WORDSIZE == 32
	/* See sysdeps/unix/sysv/linux/powerpc/sys/ucontext.h in glibc */
	mcontext_t r = *((ucontext_t*)ctx)->uc_mcontext.uc_regs;
#else
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;
#endif

#if defined __i386__
	#define REG_GET(ign, reg) &r.gregs[REG_E##reg]
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(ign, reg) &r.gregs[REG_R##reg]
	Dump_X64()
#elif defined __aarch64__
	#define REG_GNUM(num)     &r.regs[num]
	#define REG_GET_SP()      &r.sp
	#define REG_GET_PC()      &r.pc
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r.arm_r##num
	#define REG_GET(reg, ign) &r.arm_##reg
	#define REG_GET_FP()      &r.arm_fp
	#define REG_GET_IP()      &r.arm_ip
	Dump_ARM32()
#elif defined __sparc__
	#define REG_GET(ign, reg) &r.gregs[REG_##reg]
	Dump_SPARC()
#elif defined __PPC__
	#define REG_GNUM(num)     &r.gregs[num]
	#define REG_GET_PC()      &r.gregs[32]
	#define REG_GET_LR()      &r.gregs[35]
	#define REG_GET_CTR()     &r.gregs[34]
	Dump_PPC()
#elif defined __riscv
	#define REG_GNUM(num)     &r.__gregs[num]
	#define REG_GET_PC()      &r.__gregs[REG_PC]
	Dump_RISCV()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_SOLARIS
/* See /usr/include/sys/regset.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	#define REG_GET(ign, reg) &r.gregs[E##reg]
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(ign, reg) &r.gregs[REG_R##reg]
	Dump_X64()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_NETBSD
/* See /usr/include/i386/mcontext.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(ign, reg) &r.__gregs[_REG_E##reg]
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(ign, reg) &r.__gregs[_REG_R##reg]
	Dump_X64()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_FREEBSD
/* See /usr/include/machine/ucontext.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r.mc_e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r.mc_r##reg
	Dump_X64()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_OPENBSD
/* See /usr/include/machine/signal.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	struct sigcontext r = *((ucontext_t*)ctx);
#if defined __i386__
	#define REG_GET(reg, ign) &r.sc_e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r.sc_r##reg
	Dump_X64()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_HAIKU
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t r = ((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r.me##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r.r##reg
	Dump_X64()
#else
	#error "Unknown CPU architecture"
#endif
}
#endif
static void DumpRegisters(void* ctx) {
	cc_string str; char strBuffer[768];
	String_InitArray(str, strBuffer);

	String_AppendConst(&str, "-- registers --" _NL);
	PrintRegisters(&str, ctx);
	Logger_Log(&str);
}


/*########################################################################################################################*
*------------------------------------------------Module/Memory map handling-----------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
static BOOL CALLBACK DumpModule(const char* name, ULONG_PTR base, ULONG size, void* ctx) {
	cc_string str; char strBuffer[256];
	cc_uintptr beg, end;

	beg = base; end = base + (size - 1);
	String_InitArray(str, strBuffer);

	String_Format3(&str, "%c = %x-%x\r\n", name, &beg, &end);
	Logger_Log(&str);
	return true;
}

static void DumpMisc(void* ctx) {
	static const cc_string modules = String_FromConst("-- modules --\r\n");
	HANDLE process = GetCurrentProcess();

	Logger_Log(&modules);
	EnumerateLoadedModules(process, DumpModule, NULL);
}

#elif defined CC_BUILD_LINUX || defined CC_BUILD_SOLARIS || defined CC_BUILD_ANDROID
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef CC_BUILD_ANDROID
static int SkipRange(const cc_string* str) {
	/* Android has a lot of ranges in /maps, which produces 100-120 kb of logs for one single crash! */
	/* As such, to cut down the crash logs to more relevant information, ignore shared memory and fonts */
	/* (e.g. removes a ton of '/dev/ashmem/dalvik-thread local mark stack (deleted)' entries */
	return String_ContainsConst(str, "/system/fonts/") || String_ContainsConst(str, "/dev/ashmem/");
}
#else
static int SkipRange(const cc_string* str) { return false; }
#endif

static void DumpMisc(void* ctx) {
	static const cc_string memMap = String_FromConst("-- memory map --\n");
	cc_string str; char strBuffer[320];
	int n, fd;

	Logger_Log(&memMap);
	/* dumps all known ranges of memory */
	fd = open("/proc/self/maps", O_RDONLY);
	if (fd < 0) return;
	String_InitArray(str, strBuffer);

	while ((n = read(fd, str.buffer, str.capacity)) > 0) {
		str.length = n;
		if (!SkipRange(&str)) Logger_Log(&str);
	}

	close(fd);
}
#elif defined CC_BUILD_DARWIN
#include <mach-o/dyld.h>

static void DumpMisc(void* ctx) {
	static const cc_string modules = String_FromConst("-- modules --\n");
	static const cc_string newLine = String_FromConst(_NL);
	cc_uint32 i, count;
	const char* path;
	cc_string str;
	
	/* TODO: Add Logger_LogRaw / Logger_LogConst too */
	Logger_Log(&modules);
	count = _dyld_image_count();
	/* dumps absolute path of each module */

	for (i = 0; i < count; i++) {
		path = _dyld_get_image_name(i);
		if (!path) continue;

		str = String_FromReadonly(path);
		Logger_Log(&str);
		Logger_Log(&newLine);
	}
}
#else
static void DumpMisc(void* ctx) { }
#endif


/*########################################################################################################################*
*------------------------------------------------------Error handling-----------------------------------------------------*
*#########################################################################################################################*/
static void AbortCommon(cc_result result, const char* raw_msg, void* ctx);

#if defined CC_BUILD_WEB
void Logger_Hook(void) { }
void Logger_Abort2(cc_result result, const char* raw_msg) {
	AbortCommon(result, raw_msg, NULL);
}
#elif defined CC_BUILD_WIN
static LONG WINAPI UnhandledFilter(struct _EXCEPTION_POINTERS* info) {
	cc_string msg; char msgBuffer[128 + 1];
	cc_uint32 code;
	cc_uintptr addr;
	DWORD i, numArgs;

	code = (cc_uint32)info->ExceptionRecord->ExceptionCode;
	addr = (cc_uintptr)info->ExceptionRecord->ExceptionAddress;

	String_InitArray_NT(msg, msgBuffer);
	String_Format2(&msg, "Unhandled exception 0x%h at 0x%x", &code, &addr);

	numArgs = info->ExceptionRecord->NumberParameters;
	if (numArgs) {
		numArgs = min(numArgs, EXCEPTION_MAXIMUM_PARAMETERS);
		String_AppendConst(&msg, " [");

		for (i = 0; i < numArgs; i++) {
			String_Format1(&msg, "0x%x,", &info->ExceptionRecord->ExceptionInformation[i]);
		}
		String_Append(&msg, ']');
	}

	msg.buffer[msg.length] = '\0';
	AbortCommon(0, msg.buffer, info->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}
void Logger_Hook(void) { SetUnhandledExceptionFilter(UnhandledFilter); }

#if __GNUC__
/* Don't want compiler doing anything fancy with registers */
void __attribute__((optimize("O0"))) Logger_Abort2(cc_result result, const char* raw_msg) {
#else
void Logger_Abort2(cc_result result, const char* raw_msg) {
#endif
	CONTEXT ctx;
#ifndef _M_IX86
	/* This method is guaranteed to exist on 64 bit windows.  */
	/* NOTE: This is missing in 32 bit Windows 2000 however,  */
	/*  so an alternative is provided for MinGW below so that */
	/*  the game can cross-compiled for Windows 98 / 2000 */
	RtlCaptureContext(&ctx);
#elif __GNUC__
	/* Stack frame layout on x86: */
	/*  [ebp] is previous frame's EBP */
	/*  [ebp+4] is previous frame's EIP (return address) */
	/*  address of [ebp+8] is previous frame's ESP */
	__asm__(
		"mov 0(%%ebp), %%eax \n\t" /* mov eax, [ebp]     */
		"mov %%eax, %0       \n\t" /* mov [ctx.Ebp], eax */
		"mov 4(%%ebp), %%eax \n\t" /* mov eax, [ebp+4]   */
		"mov %%eax, %1       \n\t" /* mov [ctx.Eip], eax */
		"lea 8(%%ebp), %%eax \n\t" /* lea eax, [ebp+8]   */
		"mov %%eax, %2"            /* mov [ctx.Esp], eax */
		: "=m" (ctx.Ebp), "=m" (ctx.Eip), "=m" (ctx.Esp)
		:
		: "eax", "memory"
	);
	ctx.ContextFlags = CONTEXT_CONTROL;
#endif
	AbortCommon(result, raw_msg, &ctx);
}
#elif defined CC_BUILD_POSIX
static void SignalHandler(int sig, siginfo_t* info, void* ctx) {
	cc_string msg; char msgBuffer[128 + 1];
	int type, code;
	cc_uintptr addr;

	/* Uninstall handler to avoid chance of infinite loop */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS,  SIG_DFL);
	signal(SIGILL,  SIG_DFL);
	signal(SIGABRT, SIG_DFL);
	signal(SIGFPE,  SIG_DFL);

	type = info->si_signo;
	code = info->si_code;
	addr = (cc_uintptr)info->si_addr;

	String_InitArray_NT(msg, msgBuffer);
	String_Format3(&msg, "Unhandled signal %i (code %i) at 0x%x", &type, &code, &addr);
	msg.buffer[msg.length] = '\0';

	AbortCommon(0, msg.buffer, ctx);
}

void Logger_Hook(void) {
	struct sigaction sa, old;
	sa.sa_sigaction = SignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, &old);
	sigaction(SIGBUS,  &sa, &old);
	sigaction(SIGILL,  &sa, &old);
	sigaction(SIGABRT, &sa, &old);
	sigaction(SIGFPE,  &sa, &old);
}

void Logger_Abort2(cc_result result, const char* raw_msg) {
	AbortCommon(result, raw_msg, NULL);
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Common---------------------------------------------------------*
*#########################################################################################################################*/
static struct Stream logStream;
static cc_bool logOpen;

#ifdef CC_BUILD_MINFILES
void Logger_Log(const cc_string* msg) { }
static void LogCrashHeader(void)   { }
#else
void Logger_Log(const cc_string* msg) {
	static const cc_string path = String_FromConst("client.log");
	if (!logOpen) {
		logOpen = true;
		Stream_AppendFile(&logStream, &path);
	}

	if (!logStream.Meta.File) return;
	Stream_Write(&logStream, (const cc_uint8*)msg->buffer, msg->length);
}

static void LogCrashHeader(void) {
	cc_string msg; char msgBuffer[96];
	struct DateTime now;

	String_InitArray(msg, msgBuffer);
	String_AppendConst(&msg, _NL "----------------------------------------" _NL);
	Logger_Log(&msg);
	msg.length = 0;

	DateTime_CurrentLocal(&now);
	String_Format3(&msg, "Crash time: %p2/%p2/%p4 ", &now.day,  &now.month,  &now.year);
	String_Format3(&msg, "%p2:%p2:%p2" _NL,          &now.hour, &now.minute, &now.second);
	Logger_Log(&msg);
}
#endif

static void AbortCommon(cc_result result, const char* raw_msg, void* ctx) {	
	cc_string msg; char msgBuffer[3070 + 1];
	String_InitArray_NT(msg, msgBuffer);

	String_Format1(&msg, "ClassiCube crashed." _NL "Reason: %c" _NL, raw_msg);
	#ifdef CC_COMMIT_SHA
	String_AppendConst(&msg, "Commit SHA: " CC_COMMIT_SHA _NL);
	#endif

	if (result) {
		String_Format1(&msg, "%h" _NL, &result);
	} else { result = 1; }

	LogCrashHeader();
	Logger_Log(&msg);

	String_AppendConst(&msg, "Full details of the crash have been logged to 'client.log'.\n");
	String_AppendConst(&msg, "Please report this on the ClassiCube forums or to UnknownShadow200.\n\n");

	if (ctx) DumpRegisters(ctx);
	DumpBacktrace(&msg, ctx);
	DumpMisc(ctx);
	if (logStream.Meta.File) logStream.Close(&logStream);

	msg.buffer[msg.length] = '\0';
	Window_ShowDialog("We're sorry", msg.buffer);
	Process_Exit(result);
}

void Logger_Abort(const char* raw_msg) { Logger_Abort2(0, raw_msg); }
