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
#elif defined CC_BUILD_WIN
	#define WIN32_LEAN_AND_MEAN
	#define NOSERVICE
	#define NOMCX
	#define NOIME
	#define CUR_PROCESS_HANDLE ((HANDLE)-1) /* GetCurrentProcess() always returns -1 */
	
	#include <windows.h>
	#include <imagehlp.h>
	#ifndef CC_BUILD_UWP
	/* Compatibility version so compiling works on older Windows SDKs */
	#include "../misc/windows/min-imagehlp.h"
	#define CC_KERN32_FUNC extern /* main use is Platform_Windows.c */
	#include "../misc/windows/min-kernel32.h"
	#endif

	static HANDLE curProcess = CUR_PROCESS_HANDLE;
	static cc_uintptr spRegister;
#elif defined CC_BUILD_OPENBSD || defined CC_BUILD_HAIKU || defined CC_BUILD_SERENITY
	#include <signal.h>
	/* These operating systems don't provide sys/ucontext.h */
	/*  But register constants be found from includes in <signal.h> */
#elif defined CC_BUILD_OS2
	#include <signal.h>
	#include <386/ucontext.h>
#elif defined CC_BUILD_LINUX || defined CC_BUILD_ANDROID
	/* Need to define this to get REG_ constants */
	#define _GNU_SOURCE
	#include <sys/ucontext.h>
	#include <signal.h>
#elif defined CC_BUILD_SOLARIS
	#include <signal.h>
	#include <sys/ucontext.h>
	#include <sys/regset.h>
#elif defined CC_BUILD_POSIX
	#include <signal.h>
	#include <sys/ucontext.h>
#endif

#ifdef CC_BUILD_DARWIN
/* Need this to detect macOS < 10.4, and switch to NS* api instead if so */
#include <AvailabilityMacros.h>
#endif
/* Only show up to 50 frames in backtrace */
#define MAX_BACKTRACE_FRAMES 50


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

	case OGG_ERR_INVALID_SIG:  return "Only OGG music files supported";
	case OGG_ERR_VERSION:      return "Invalid OGG format version";
	case AUDIO_ERR_MP3_SIG:    return "MP3 audio files are not supported";

	case WAV_ERR_STREAM_HDR:  return "Only WAV sound files supported";
	case WAV_ERR_STREAM_TYPE: return "Invalid WAV type";
	case WAV_ERR_DATA_TYPE:   return "Unsupported WAV audio format";

	case ZIP_ERR_TOO_MANY_ENTRIES: return "Cannot load .zip files with over 1024 entries";

	case PNG_ERR_INVALID_SIG:      return "Only PNG images supported";
	case PNG_ERR_INVALID_HDR_SIZE: return "Invalid PNG header size";
	case PNG_ERR_TOO_WIDE:         return "PNG image too wide";
	case PNG_ERR_TOO_TALL:         return "PNG image too tall";
	case PNG_ERR_INTERLACED:       return "Interlaced PNGs unsupported";
	case PNG_ERR_REACHED_IEND:     return "Incomplete PNG image data";
	case PNG_ERR_NO_DATA:          return "No image in PNG";
	case PNG_ERR_INVALID_SCANLINE: return "Invalid PNG scanline type";
	case PNG_ERR_16BITSAMPLES:     return "16 bpp PNGs unsupported";

	case NBT_ERR_UNKNOWN:   return "Unknown NBT tag type";
	case CW_ERR_ROOT_TAG:   return "Invalid root NBT tag";
	case CW_ERR_STRING_LEN: return "NBT string too long";

	case ERR_DOWNLOAD_INVALID: return "Website denied download or doesn't exist";
	case ERR_NO_AUDIO_OUTPUT:  return "No audio output devices plugged in";
	case ERR_INVALID_DATA_URL: return "Cannot download from invalid URL";
	case ERR_INVALID_OPEN_URL: return "Cannot navigate to invalid URL";

	case NBT_ERR_EXPECTED_I8:  return "Expected Int8 NBT tag";
	case NBT_ERR_EXPECTED_I16: return "Expected Int16 NBT tag";
	case NBT_ERR_EXPECTED_I32: return "Expected Int32 NBT tag";
	case NBT_ERR_EXPECTED_F32: return "Expected Float32 NBT tag";
	case NBT_ERR_EXPECTED_STR: return "Expected String NBT tag";
	case NBT_ERR_EXPECTED_ARR: return "Expected ByteArray NBT tag";
	case NBT_ERR_ARR_TOO_SMALL:return "ByteArray NBT tag too small";

	case HTTP_ERR_NO_SSL: return "HTTPS URLs are not currently supported";
	case SOCK_ERR_UNKNOWN_HOST: return "Host could not be resolved to an IP address";
	case ERR_NO_NETWORKING: return "No working network access";
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
	String_Format2(msg, "Error %e when %c", &res, action);
	AppendErrorDesc(msg, res, describeErr);
}

void Logger_FormatWarn2(cc_string* msg, cc_result res, const char* action, const cc_string* path, Logger_DescribeError describeErr) {
	String_Format3(msg, "Error %e when %c '%s'", &res, action, path);
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
*------------------------------------------------------Frame dumping------------------------------------------------------*
*#########################################################################################################################*/
static void PrintFrame(cc_string* str, cc_uintptr addr, 
						cc_uintptr symAddr, const char* symName, 
						cc_uintptr modBase, const char* modName) {
	cc_string module;
	cc_uintptr modAddr = addr - modBase;
	int offset;
	if (!modName) modName = "???";

	module = String_FromReadonly(modName);
	Utils_UNSAFE_GetFilename(&module);
	String_Format2(str, "%x - %s", &modAddr, &module);
	
	if (symName && symName[0]) {
		offset = (int)(addr - symAddr);
		String_Format2(str, "(%c+%i)" _NL, symName, &offset);
	} else {
		String_AppendConst(str, _NL);
	}
}

#if defined CC_BUILD_UWP
static void DumpFrame(cc_string* trace, void* addr) {
	cc_uintptr addr_ = (cc_uintptr)addr;
	String_Format1(trace, "%x", &addr_);
}
#elif defined CC_BUILD_WIN
struct SymbolAndName { IMAGEHLP_SYMBOL symbol; char name[256]; };

static void DumpFrame(HANDLE process, cc_string* trace, cc_uintptr addr) {
	char strBuffer[512]; cc_string str;
	struct SymbolAndName s = { 0 };
	IMAGEHLP_MODULE m = { 0 };

	if (_SymGetSymFromAddr) {
		s.symbol.MaxNameLength = 255;
		s.symbol.SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
		_SymGetSymFromAddr(process, addr, NULL, &s.symbol);
	}
	
	if (_SymGetModuleInfo) {
		m.SizeOfStruct    = sizeof(IMAGEHLP_MODULE);
		_SymGetModuleInfo(process, addr, &m);
	}

	String_InitArray(str, strBuffer);
	PrintFrame(&str, addr, 
				s.symbol.Address, s.symbol.Name, 
				m.BaseOfImage, m.ModuleName);
	String_AppendString(trace, &str);

	/* This function only works for .pdb debug info anyways */
	/* This function is also missing on Windows 9X */
#if _MSC_VER
	{
		IMAGEHLP_LINE line = { 0 }; DWORD lineOffset;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
		if (_SymGetLineFromAddr(process, addr, &lineOffset, &line)) {
			String_Format2(&str, "  line %i in %c\r\n", &line.LineNumber, line.FileName);
		}
	}
#endif
	Logger_Log(&str);
}
#elif defined MAC_OS_X_VERSION_MIN_REQUIRED && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
/* dladdr does not exist prior to macOS tiger */
static void DumpFrame(cc_string* trace, void* addr) {
	cc_string str; char strBuffer[384];
	String_InitArray(str, strBuffer);
	/* alas NSModuleForSymbol doesn't work with raw addresses */

	PrintFrame(&str, (cc_uintptr)addr, 
				0, NULL, 
				0, NULL);
	String_AppendString(trace, &str);
	Logger_Log(&str);
}
#elif defined CC_BUILD_IRIX
/* IRIX doesn't expose a nice interface for dladdr */
static void DumpFrame(cc_string* trace, void* addr) {
	cc_uintptr addr_ = (cc_uintptr)addr;
	String_Format1(trace, "%x", &addr_);
}
#elif defined CC_BUILD_POSIX && !defined CC_BUILD_OS2
/* need to define __USE_GNU for dladdr */
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <dlfcn.h>
#undef __USE_GNU

static void DumpFrame(cc_string* trace, void* addr) {
	cc_string str; char strBuffer[384];
	Dl_info s = { 0 };

	String_InitArray(str, strBuffer);
	dladdr(addr, &s);

	PrintFrame(&str, (cc_uintptr)addr, 
				(cc_uintptr)s.dli_saddr, s.dli_sname, 
				(cc_uintptr)s.dli_fbase, s.dli_fname);
	String_AppendString(trace, &str);
	Logger_Log(&str);
}
#elif defined CC_BUILD_OS2
static void DumpFrame(cc_string* trace, void* addr) {
// TBD
}
#else
/* No backtrace support implemented */
#endif


/*########################################################################################################################*
*-------------------------------------------------------Backtracing-------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_UWP
void Logger_Backtrace(cc_string* trace, void* ctx) {
	String_AppendConst(trace, "-- backtrace unimplemented --");
}
#elif defined CC_BUILD_WIN

static PVOID WINAPI FunctionTableAccessCallback(HANDLE process, _DWORD_PTR addr) {
	if (!_SymFunctionTableAccess) return NULL;
	return _SymFunctionTableAccess(process, addr);
}

static _DWORD_PTR WINAPI GetModuleBaseCallback(HANDLE process, _DWORD_PTR addr) {
	if (!_SymGetModuleBase) return 0;
	return _SymGetModuleBase(process, addr);
}

/* This callback function is used so stack Walking works using StackWalk properly on Windows 9x: */
/*  - on Windows 9x process ID is passed instead of process handle as the "process" argument */
/*  - the SymXYZ functions expect a process ID on Windows 9x, so that works fine */
/*  - if NULL is passed as the "ReadMemory" argument, the default callback using ReadProcessMemory is used */
/*  - however, ReadProcessMemory expects a process handle, and so that will fail since it's given a process ID */
/* So to work around this, instead manually call ReadProcessMemory with the current process handle */
static BOOL WINAPI ReadMemCallback(HANDLE process, _DWORD_PTR baseAddress, PVOID buffer, DWORD size, DWORD* numBytesRead) {
	SIZE_T numRead = 0;
	BOOL ok = ReadProcessMemory(CUR_PROCESS_HANDLE, (LPCVOID)baseAddress, buffer, size, &numRead);
	
	*numBytesRead = (DWORD)numRead; /* DWORD always 32 bits */
	return ok;
}

static int GetFrames(CONTEXT* ctx, cc_uintptr* addrs, int max) {
	STACKFRAME frame = { 0 };
	int count, type;
	HANDLE thread;

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
	frame.AddrFrame.Offset = ctx->Rbp;
	frame.AddrStack.Offset = ctx->Rsp;
#elif defined _M_ARM64
	type = IMAGE_FILE_MACHINE_ARM64;
	frame.AddrPC.Offset    = ctx->Pc;
	frame.AddrFrame.Offset = ctx->Fp;
	frame.AddrStack.Offset = ctx->Sp;
#elif defined _M_ARM
	type = IMAGE_FILE_MACHINE_ARM;
	frame.AddrPC.Offset    = ctx->Pc;
	frame.AddrFrame.Offset = ctx->R11;
	frame.AddrStack.Offset = ctx->Sp;
#else
	/* Always available after XP, so use that */
	return RtlCaptureStackBackTrace(0, max, (void**)addrs, NULL);
#endif
	spRegister = frame.AddrStack.Offset;
	thread     = GetCurrentThread();
	if (!_StackWalk) return 0;

	for (count = 0; count < max; count++) 
	{
		if (!_StackWalk(type, curProcess, thread, &frame, ctx, ReadMemCallback, 
						FunctionTableAccessCallback, GetModuleBaseCallback, NULL)) break;
		if (!frame.AddrFrame.Offset) break;
		addrs[count] = frame.AddrPC.Offset;
	}
	return count;
}

void Logger_Backtrace(cc_string* trace, void* ctx) {
	cc_uintptr addrs[MAX_BACKTRACE_FRAMES];
	int i, frames;

	if (_SymInitialize) {
		_SymInitialize(curProcess, NULL, TRUE); /* TODO only in MSVC.. */
	}
	frames  = GetFrames((CONTEXT*)ctx, addrs, MAX_BACKTRACE_FRAMES);

	for (i = 0; i < frames; i++) {
		DumpFrame(curProcess, trace, addrs[i]);
	}
	String_AppendConst(trace, _NL);
}
#elif defined CC_BUILD_ANDROID
	#define CC_BACKTRACE_UNWIND
#elif defined CC_BUILD_DARWIN
/* backtrace is only available on macOS since 10.5 */
void Logger_Backtrace(cc_string* trace, void* ctx) {
	void* addrs[MAX_BACKTRACE_FRAMES];
	unsigned i, frames;
	/* See lldb/tools/debugserver/source/MacOSX/stack_logging.h */
	/*  backtrace uses this internally too, and exists since macOS 10.1 */
	extern void thread_stack_pcs(void** buffer, unsigned max, unsigned* nb);

	thread_stack_pcs(addrs, MAX_BACKTRACE_FRAMES, &frames);
	/* Skip frames don't want to include in backtrace */
	/*  frame 0 = thread_stack_pcs */
	/*  frame 1 = Logger_Backtrace */
	for (i = 2; i < frames; i++) 
	{
		DumpFrame(trace, addrs[i]);
	}
	String_AppendConst(trace, _NL);
}
#elif defined CC_BUILD_SERENITY
void Logger_Backtrace(cc_string* trace, void* ctx) {
	String_AppendConst(trace, "-- backtrace unimplemented --");
	/* TODO: Backtrace using LibSymbolication */
}
#elif defined CC_BUILD_IRIX
void Logger_Backtrace(cc_string* trace, void* ctx) {
	String_AppendConst(trace, "-- backtrace unimplemented --");
	/* TODO implement backtrace using exc_unwind https://nixdoc.net/man-pages/IRIX/man3/exception.3.html */
}
#elif defined CC_BACKTRACE_BUILTIN
/* Implemented later at end of the file */
#elif defined CC_BUILD_POSIX && (defined __GLIBC__ || defined CC_BUILD_OPENBSD)
#include <execinfo.h>

void Logger_Backtrace(cc_string* trace, void* ctx) {
	void* addrs[MAX_BACKTRACE_FRAMES];
	int i, frames = backtrace(addrs, MAX_BACKTRACE_FRAMES);

	for (i = 0; i < frames; i++) 
	{
		DumpFrame(trace, addrs[i]);
	}
	String_AppendConst(trace, _NL);
}
#elif defined CC_BUILD_POSIX
/* musl etc - rely on unwind from GCC instead */
	#define CC_BACKTRACE_UNWIND
#else
void Logger_Backtrace(cc_string* trace, void* ctx) { }
#endif


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
String_Format3(str, "ip =%x sp =%x lr =%x" _NL, REG_GET_IP(), REG_GET_SP(), REG_GET_LR());\
String_Format1(str, "pc =%x"               _NL, REG_GET_PC());

#define Dump_ARM64() \
String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format4(str, "r28=%x fp =%x lr =%x sp =%x" _NL, REG_GNUM(28), REG_GET_FP(), REG_GET_LR(), REG_GET_SP()); \
String_Format1(str, "pc =%x" _NL,                      REG_GET_PC());

#define Dump_Alpha() \
String_Format4(str, "v0 =%x t0 =%x t1 =%x t2 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "t3 =%x t4 =%x t5 =%x t6 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "t7 =%x s0 =%x s1 =%x s2 =%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "s3 =%x s4 =%x s5 =%x a0 =%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(16)); \
String_Format4(str, "a1 =%x a2 =%x a3 =%x a4 =%x" _NL, REG_GNUM(17), REG_GNUM(18), REG_GNUM(19), REG_GNUM(20)); \
String_Format4(str, "a5 =%x t8 =%x t9 =%x t10=%x" _NL, REG_GNUM(21), REG_GNUM(22), REG_GNUM(23), REG_GNUM(24)); \
String_Format4(str, "t11=%x ra =%x pv =%x at =%x" _NL, REG_GNUM(25), REG_GNUM(26), REG_GNUM(27), REG_GNUM(28)); \
String_Format4(str, "gp =%x fp =%x sp =%x pc =%x" _NL, REG_GNUM(29), REG_GET_FP(), REG_GET_SP(), REG_GET_PC());

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

#define Dump_MIPS() \
String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format4(str, "r28=%x sp =%x fp =%x ra =%x" _NL, REG_GNUM(28), REG_GNUM(29), REG_GNUM(30), REG_GNUM(31)); \
String_Format3(str, "pc =%x lo =%x hi =%x" _NL,        REG_GET_PC(), REG_GET_LO(), REG_GET_HI());

#define Dump_SH() \
String_Format4(str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format3(str, "mal=%x mah=%x gbr=%x"        _NL, REG_GET_MACL(), REG_GET_MACH(), REG_GET_GBR()); \
String_Format2(str, "pc =%x ra =%x"               _NL, REG_GET_PC(), REG_GET_RA());

#if defined CC_BUILD_WIN
/* See CONTEXT in WinNT.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	CONTEXT* r = (CONTEXT*)ctx;
#if defined _M_IX86
	#define REG_GET(reg, ign) &r->E ## reg
	Dump_X86()
#elif defined _M_X64
	#define REG_GET(reg, ign) &r->R ## reg
	Dump_X64()
#elif defined _M_ARM64
	#define REG_GNUM(num)     &r->X[num]
	#define REG_GET_FP()      &r->Fp
	#define REG_GET_LR()      &r->Lr
	#define REG_GET_SP()      &r->Sp
	#define REG_GET_PC()      &r->Pc
	Dump_ARM64()
#elif defined _M_ARM
	#define REG_GNUM(num)     &r->R ## num
	#define REG_GET_FP()      &r->R11
	#define REG_GET_IP()      &r->R12
	#define REG_GET_SP()      &r->Sp
	#define REG_GET_LR()      &r->Lr
	#define REG_GET_PC()      &r->Pc
	Dump_ARM32()
#elif defined _M_PPC
	#define REG_GNUM(num) &r->Gpr ## num
	#define REG_GET_PC()  &r->Iar
	#define REG_GET_LR()  &r->Lr
	#define REG_GET_CTR() &r->Ctr
	Dump_PPC()
#elif defined _M_ALPHA
	#define REG_GNUM(num) &r->IntV0 + num
	#define REG_GET_FP()  &r->IntFp
	#define REG_GET_SP()  &r->IntSp
	#define REG_GET_PC()  &r->Fir
	Dump_Alpha()
#elif defined _MIPS_
	#define REG_GNUM(num) &r->IntZero + num
	#define REG_GET_PC()  &r->Fir
	#define REG_GET_LO()  &r->IntLo
	#define REG_GET_HI()  &r->IntHi
	Dump_MIPS()
#elif defined SHx
	#define REG_GNUM(num)  &r->R ## num
	#define REG_GET_MACL() &r->MACL
	#define REG_GET_MACH() &r->MACH
	#define REG_GET_GBR()  &r->GBR
	#define REG_GET_PC()   &r->Fir
	#define REG_GET_RA()   &r->PR
	Dump_SH()
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
#elif defined __aarch64__
	#define REG_GNUM(num)     &r->__ss.__x[num]
	#define REG_GET_FP()      &r->__ss.__fp
	#define REG_GET_LR()      &r->__ss.__lr
	#define REG_GET_SP()      &r->__ss.__sp
	#define REG_GET_PC()      &r->__ss.__pc
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->__ss.__r[num]
	#define REG_GET_FP()      &r->__ss.__r[11]
	#define REG_GET_IP()      &r->__ss.__r[12]
	#define REG_GET_SP()      &r->__ss.__sp
	#define REG_GET_LR()      &r->__ss.__lr
	#define REG_GET_PC()      &r->__ss.__pc
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
	mcontext_t* r = ((ucontext_t*)ctx)->uc_mcontext.uc_regs;
#else
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#endif

#if defined __i386__
	#define REG_GET(ign, reg) &r->gregs[REG_E##reg]
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(ign, reg) &r->gregs[REG_R##reg]
	Dump_X64()
#elif defined __aarch64__
	#define REG_GNUM(num)     &r->regs[num]
	#define REG_GET_FP()      &r->regs[29]
	#define REG_GET_LR()      &r->regs[30]
	#define REG_GET_SP()      &r->sp
	#define REG_GET_PC()      &r->pc
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->arm_r##num
	#define REG_GET_FP()      &r->arm_fp
	#define REG_GET_IP()      &r->arm_ip
	#define REG_GET_SP()      &r->arm_sp
	#define REG_GET_LR()      &r->arm_lr
	#define REG_GET_PC()      &r->arm_pc
	Dump_ARM32()
#elif defined __alpha__
	#define REG_GNUM(num)     &r->sc_regs[num]
	#define REG_GET_FP()      &r->sc_regs[15]
	#define REG_GET_PC()      &r->sc_pc
	#define REG_GET_SP()      &r->sc_regs[30]
	Dump_Alpha()
#elif defined __sparc__
	#define REG_GET(ign, reg) &r->gregs[REG_##reg]
	Dump_SPARC()
#elif defined __PPC__ && __WORDSIZE == 32
	#define REG_GNUM(num)     &r->gregs[num]
	#define REG_GET_PC()      &r->gregs[32]
	#define REG_GET_LR()      &r->gregs[35]
	#define REG_GET_CTR()     &r->gregs[34]
	Dump_PPC()
#elif defined __PPC__
	#define REG_GNUM(num)     &r->gp_regs[num]
	#define REG_GET_PC()      &r->gp_regs[32]
	/* TODO this might be wrong, compare with PT_LNK in */
	/* https://elixir.bootlin.com/linux/v4.19.122/source/arch/powerpc/include/uapi/asm/ptrace.h#L102 */
	#define REG_GET_LR()      &r->gp_regs[35]
	#define REG_GET_CTR()     &r->gp_regs[34]
	Dump_PPC()
#elif defined __mips__
	#define REG_GNUM(num)     &r->gregs[num]
	#define REG_GET_PC()      &r->pc
	#define REG_GET_LO()      &r->mdlo
	#define REG_GET_HI()      &r->mdhi
	Dump_MIPS()
#elif defined __riscv
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_PC()      &r->__gregs[REG_PC]
	Dump_RISCV()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_SOLARIS
/* See /usr/include/sys/regset.h */
/* -> usr/src/uts/[ARCH]/sys/mcontext.h */
/* -> usr/src/uts/[ARCH]/sys/regset.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

#if defined __i386__
	#define REG_GET(ign, reg) &r->gregs[E##reg]
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(ign, reg) &r->gregs[REG_R##reg]
	Dump_X64()
#elif defined __sparc__
	#define REG_GET(ign, reg) &r->gregs[REG_##reg]
	Dump_SPARC()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_NETBSD
/* See /usr/include/[ARCH]/mcontext.h */
/* -> src/sys/arch/[ARCH]/include/mcontext.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(ign, reg) &r->__gregs[_REG_E##reg]
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(ign, reg) &r->__gregs[_REG_R##reg]
	Dump_X64()
#elif defined __aarch64__
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_FP()      &r->__gregs[_REG_FP]
	#define REG_GET_LR()      &r->__gregs[_REG_LR]
	#define REG_GET_SP()      &r->__gregs[_REG_SP]
	#define REG_GET_PC()      &r->__gregs[_REG_PC]
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_FP()      &r->__gregs[_REG_FP]
	#define REG_GET_IP()      &r->__gregs[12]
	#define REG_GET_SP()      &r->__gregs[_REG_SP]
	#define REG_GET_LR()      &r->__gregs[_REG_LR]
	#define REG_GET_PC()      &r->__gregs[_REG_PC]
	Dump_ARM32()
#elif defined __powerpc__
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_PC()      &r->__gregs[_REG_PC]
	#define REG_GET_LR()      &r->__gregs[_REG_LR]
	#define REG_GET_CTR()     &r->__gregs[_REG_CTR]
	Dump_PPC()
#elif defined __mips__
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_PC()      &r->__gregs[_REG_EPC]
	#define REG_GET_LO()      &r->__gregs[_REG_MDLO]
	#define REG_GET_HI()      &r->__gregs[_REG_MDHI]
	Dump_MIPS()
#elif defined __riscv
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_PC()      &r->__gregs[_REG_PC]
	Dump_RISCV()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_FREEBSD
/* See /usr/include/machine/ucontext.h */
/* -> src/sys/[ARCH]/include/ucontext.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r->mc_e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r->mc_r##reg
	Dump_X64()
#elif defined __aarch64__
	#define REG_GNUM(num)     &r->mc_gpregs.gp_x[num]
	#define REG_GET_FP()      &r->mc_gpregs.gp_x[29]
	#define REG_GET_LR()      &r->mc_gpregs.gp_lr
	#define REG_GET_SP()      &r->mc_gpregs.gp_sp
	#define REG_GET_PC()      &r->mc_gpregs.gp_elr
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->__gregs[num]
	#define REG_GET_FP()      &r->__gregs[_REG_FP]
	#define REG_GET_IP()      &r->__gregs[12]
	#define REG_GET_SP()      &r->__gregs[_REG_SP]
	#define REG_GET_LR()      &r->__gregs[_REG_LR]
	#define REG_GET_PC()      &r->__gregs[_REG_PC]
	Dump_ARM32()
#elif defined __powerpc__
	#define REG_GNUM(num)     &r->mc_frame[##num]
	#define REG_GET_PC()      &r->mc_srr0
	#define REG_GET_LR()      &r->mc_lr
	#define REG_GET_CTR()     &r->mc_ctr
	Dump_PPC()
#elif defined __mips__
	#define REG_GNUM(num)     &r->mc_regs[num]
	#define REG_GET_PC()      &r->mc_pc
	#define REG_GET_LO()      &r->mullo
	#define REG_GET_HI()      &r->mulhi
	Dump_MIPS()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_OPENBSD
/* See /usr/include/machine/signal.h */
/* -> src/sys/arch/[ARCH]/include/signal.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	ucontext_t* r = (ucontext_t*)ctx;
#if defined __i386__
	#define REG_GET(reg, ign) &r->sc_e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r->sc_r##reg
	Dump_X64()
#elif defined __aarch64__
	#define REG_GNUM(num)     &r->sc_x[num]
	#define REG_GET_FP()      &r->sc_x[29]
	#define REG_GET_LR()      &r->sc_lr
	#define REG_GET_SP()      &r->sc_sp
	#define REG_GET_PC()      &r->sc_elr
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->sc_r##num
	#define REG_GET_FP()      &r->sc_r11
	#define REG_GET_IP()      &r->sc_r12
	#define REG_GET_SP()      &r->sc_usr_sp
	#define REG_GET_LR()      &r->sc_usr_lr
	#define REG_GET_PC()      &r->sc_pc
	Dump_ARM32()
#elif defined __powerpc__
	#define REG_GNUM(num)     &r->sc_frame.fixreg[num]
	#define REG_GET_PC()      &r->sc_frame.srr0
	#define REG_GET_LR()      &r->sc_frame.lr
	#define REG_GET_CTR()     &r->sc_frame.ctr
	Dump_PPC()
#elif defined __mips__
	#define REG_GNUM(num)     &r->sc_regs[num]
	#define REG_GET_PC()      &r->sc_pc
	#define REG_GET_LO()      &r->mullo
	#define REG_GET_HI()      &r->mulhi
	Dump_MIPS()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_HAIKU
/* See /headers/posix/arch/[ARCH]/signal.h */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r->e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r->r##reg
	Dump_X64()
#elif defined __aarch64__
	#define REG_GNUM(num)     &r->x[num]
	#define REG_GET_FP()      &r->x[29]
	#define REG_GET_LR()      &r->lr
	#define REG_GET_SP()      &r->sp
	#define REG_GET_PC()      &r->elr
	Dump_ARM64()
#elif defined __arm__
	#define REG_GNUM(num)     &r->r##num
	#define REG_GET_FP()      &r->r11
	#define REG_GET_IP()      &r->r12
	#define REG_GET_SP()      &r->r13
	#define REG_GET_LR()      &r->r14
	#define REG_GET_PC()      &r->r15
	Dump_ARM32()
#elif defined __riscv
	#define REG_GNUM(num)     &r->x[num - 1]
	#define REG_GET_PC()      &r->pc
	Dump_RISCV()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_SERENITY
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;
#if defined __i386__
	#define REG_GET(reg, ign) &r->e##reg
	Dump_X86()
#elif defined __x86_64__
	#define REG_GET(reg, ign) &r->r##reg
	Dump_X64()
#else
	#error "Unknown CPU architecture"
#endif
}
#elif defined CC_BUILD_IRIX
/* See /usr/include/sys/ucontext.h */
/* https://nixdoc.net/man-pages/IRIX/man5/UCONTEXT.5.html */
static void PrintRegisters(cc_string* str, void* ctx) {
	mcontext_t* r = &((ucontext_t*)ctx)->uc_mcontext;

	#define REG_GNUM(num) &r->__gregs[CTX_EPC]
	#define REG_GET_PC()  &r->__gregs[CTX_MDLO]
	#define REG_GET_LO()  &r->__gregs[CTX_MDLO]
	#define REG_GET_HI()  &r->__gregs[CTX_MDHI]
	Dump_MIPS()
}
#else
static void PrintRegisters(cc_string* str, void* ctx) {
	/* Register dumping not implemented */
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
#if defined CC_BUILD_UWP
static void DumpMisc(void) { }

#elif defined CC_BUILD_WIN
static void DumpStack(void) {
	static const cc_string stack = String_FromConst("-- stack --\r\n");
	cc_string str; char strBuffer[128];
	cc_uint8 buffer[0x10];
	SIZE_T numRead;
	int i, j;

	Logger_Log(&stack);
	spRegister &= ~0x0F;
	spRegister -= 0x40;

	/* Dump 128 bytes near stack pointer */
	for (i = 0; i < 8; i++, spRegister += 0x10) 
	{
		String_InitArray(str, strBuffer);
		String_Format1(&str, "0x%x)", &spRegister);
		ReadProcessMemory(CUR_PROCESS_HANDLE, (LPCVOID)spRegister, buffer, 0x10, &numRead);

		for (j = 0; j < 0x10; j++)
		{
			if ((j & 0x03) == 0) String_Append(&str, ' ');
			String_AppendHex(&str, buffer[j]);
			String_Append(&str,    ' ');
		}
		String_AppendConst(&str, "\r\n");
		Logger_Log(&str);
	}
}

static BOOL WINAPI DumpModule(const char* name, ULONG_PTR base, ULONG size, void* userCtx) {
	cc_string str; char strBuffer[256];
	cc_uintptr beg, end;

	beg = base; end = base + (size - 1);
	String_InitArray(str, strBuffer);

	String_Format3(&str, "%c = %x-%x\r\n", name, &beg, &end);
	Logger_Log(&str);
	return true;
}

static void DumpMisc(void) {
	static const cc_string modules = String_FromConst("-- modules --\r\n");
	if (spRegister >= 0xFFFF) DumpStack();

	if (!_EnumerateLoadedModules) return;
	Logger_Log(&modules);
	_EnumerateLoadedModules(curProcess, DumpModule, NULL);
}

#elif defined CC_BUILD_LINUX || defined CC_BUILD_SOLARIS || defined CC_BUILD_ANDROID
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef CC_BUILD_ANDROID
static int SkipRange(const cc_string* str) {
	/* Android has a lot of ranges in /maps, which produces 100-120 kb of logs for one single crash! */
	/* Example of different types of ranges:
		7a2df000-7a2eb000 r-xp 00000000 fd:01 419        /vendor/lib/libdrm.so
		7a2eb000-7a2ec000 r--p 0000b000 fd:01 419        /vendor/lib/libdrm.so
		7a2ec000-7a2ed000 rw-p 0000c000 fd:01 419        /vendor/lib/libdrm.so
		7a3d5000-7a4d1000 rw-p 00000000 00:00 0
		7a4d1000-7a4d2000 ---p 00000000 00:00 0          [anon:thread stack guard]
	To cut down crash logs to more relevant information, unnecessary '/' entries are ignored */
	cc_string path;

	/* Always include ranges without a / */
	int idx = String_IndexOf(str, '/');
	if (idx == -1) return false;
	path = String_UNSAFE_SubstringAt(str, idx);

	return
		/* Ignore fonts */
		String_ContainsConst(&path, "/system/fonts/")
		/* Ignore shared memory (e.g. '/dev/ashmem/dalvik-thread local mark stack (deleted)') */
		|| String_ContainsConst(&path, "/dev/ashmem/")
		/* Ignore /dev/mali0 ranges (~200 entries on some devices) */
		|| String_ContainsConst(&path, "/dev/mali0")
		/* Ignore /system/lib/ (~350 entries) and /system/lib64 (~700 entries) */
		|| String_ContainsConst(&path, "/system/lib")
		/* Ignore /system/framework (~130 to ~160 entries) */
		|| String_ContainsConst(&path, "/system/framework/")
		/* Ignore /apex/com.android.art/javalib/ (~240 entries) */
		|| String_ContainsConst(&path, "/apex/com.")
		/* Ignore /dev/dri/renderD128 entries (~100 to ~120 entries) */
		|| String_ContainsConst(&path, "/dri/renderD128")
		/* Ignore /data/dalvik-cache entries (~80 entries) */
		|| String_ContainsConst(&path, "/data/dalvik-cache/")
		/* Ignore /vendor/lib entries (~80 entries) */
		|| String_ContainsConst(&path, "/vendor/lib");
}
#else
static int SkipRange(const cc_string* str) { 
	return
		/* Ignore GPU iris driver i915 GEM buffers (~60,000 entries for one user) */
		String_ContainsConst(str, "anon_inode:i915.gem");
}
#endif

static void DumpMisc(void) {
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

static void DumpMisc(void) {
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
static void DumpMisc(void) { }
#endif


/*########################################################################################################################*
*--------------------------------------------------Unhandled error logging------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN && !defined CC_BUILD_UWP
void Logger_Hook(void) {
	OSVERSIONINFOA osInfo;
	ImageHlp_LoadDynamicFuncs();

	/* Windows 9x requires process IDs instead - see old DBGHELP docs */
	/*   https://documentation.help/DbgHelp/documentation.pdf */
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	osInfo.dwPlatformId        = 0;
	GetVersionExA(&osInfo);

	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		curProcess = (HANDLE)((cc_uintptr)GetCurrentProcessId());
	}
}
#else
void Logger_Hook(void) { }
#endif


/*########################################################################################################################*
*----------------------------------------------------------Common---------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_MINFILES
void Logger_Log(const cc_string* msg) {
	Platform_Log(msg->buffer, msg->length);
}
static void LogCrashHeader(void) { }
static void CloseLogFile(void)   { }
#else
static struct Stream logStream;
static cc_bool logOpen;

void Logger_Log(const cc_string* msg) {
	static const cc_string path = String_FromConst("client.log");
	if (!logOpen) {
		logOpen = true;
		Stream_AppendFile(&logStream, &path);
	}

	if (!logStream.meta.file) return;
	Stream_Write(&logStream, (const cc_uint8*)msg->buffer, msg->length);
}

static void LogCrashHeader(void) {
	cc_string msg; char msgBuffer[96];
	struct cc_datetime now;

	String_InitArray(msg, msgBuffer);
	String_AppendConst(&msg, _NL "----------------------------------------" _NL);
	Logger_Log(&msg);
	msg.length = 0;

	DateTime_CurrentLocal(&now);
	String_Format3(&msg, "Crash time: %p2/%p2/%p4 ", &now.day,  &now.month,  &now.year);
	String_Format3(&msg, "%p2:%p2:%p2" _NL,          &now.hour, &now.minute, &now.second);
	Logger_Log(&msg);
}

static void CloseLogFile(void) { 
	if (logStream.meta.file) logStream.Close(&logStream);
}
#endif

#if CC_GFX_BACKEND == CC_GFX_BACKEND_D3D11
	#define GFX_BACKEND " (Direct3D11)"
#elif CC_GFX_BACKEND == CC_GFX_BACKEND_D3D9
	#define GFX_BACKEND " (Direct3D9)"
#elif CC_GFX_BACKEND == CC_GFX_BACKEND_GL2
	#define GFX_BACKEND " (ModernGL)"
#elif CC_GFX_BACKEND == CC_GFX_BACKEND_GL1
	#define GFX_BACKEND " (OpenGL)"
#else
	#define GFX_BACKEND " (Unknown)"
#endif

void Logger_DoAbort(cc_result result, const char* raw_msg, void* ctx) {
	static const cc_string backtrace = String_FromConst("-- backtrace --" _NL);
	cc_string msg; char msgBuffer[3070 + 1];
	String_InitArray_NT(msg, msgBuffer);

	String_AppendConst(&msg, "ClassiCube crashed." _NL);
	if (raw_msg) String_Format1(&msg, "Reason: %c" _NL, raw_msg);
	#ifdef CC_COMMIT_SHA
	String_AppendConst(&msg, "Commit SHA: " CC_COMMIT_SHA GFX_BACKEND _NL);
	#endif

	if (result) {
		String_Format1(&msg, "%h" _NL, &result);
	} else { result = 1; }

	LogCrashHeader();
	Logger_Log(&msg);

	String_AppendConst(&msg, "Full details of the crash have been logged to 'client.log'.\n");
	String_AppendConst(&msg, "Please report this on the ClassiCube forums or Discord.\n\n");
	if (ctx) DumpRegisters(ctx);

	/* These two function calls used to be in a separate DumpBacktrace function */
	/*  However that was not always inlined by the compiler, which resulted in a */
	/*  useless strackframe being logged - so manually inline Logger_Backtrace call */
	Logger_Log(&backtrace);
	if (ctx) Logger_Backtrace(&msg, ctx);

	DumpMisc();
	CloseLogFile();

	msg.buffer[msg.length] = '\0';
	Window_ShowDialog("We're sorry", msg.buffer);
	Process_Exit(result);
}

void Logger_FailToStart(const char* raw_msg) {
	cc_string msg = String_FromReadonly(raw_msg);

	Window_ShowDialog("Failed to start ClassiCube", raw_msg);
	LogCrashHeader();
	Logger_Log(&msg);
	Process_Exit(1);
}


#if defined CC_BACKTRACE_UNWIND
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
#endif

#if defined CC_BACKTRACE_BUILTIN
static CC_NOINLINE void* GetReturnAddress(int level) {
	/* "... a value of 0 yields the return address of the current function, a value of 1 yields the return address of the caller of the current function" */
	switch(level) {
	case 0:  return __builtin_return_address(1);
	case 1:  return __builtin_return_address(2);
	case 2:  return __builtin_return_address(3);
	case 3:  return __builtin_return_address(4);
	case 4:  return __builtin_return_address(5);
	case 5:  return __builtin_return_address(6);
	case 6:  return __builtin_return_address(7);
	case 7:  return __builtin_return_address(8);
	case 8:  return __builtin_return_address(9);
	case 9:  return __builtin_return_address(10);
	case 10: return __builtin_return_address(11);
	case 11: return __builtin_return_address(12);
	case 12: return __builtin_return_address(13);
	case 13: return __builtin_return_address(14);
	case 14: return __builtin_return_address(15);
	case 15: return __builtin_return_address(16);
	case 16: return __builtin_return_address(17);
	case 17: return __builtin_return_address(18);
	case 18: return __builtin_return_address(19);
	case 19: return __builtin_return_address(20);
	case 20: return __builtin_return_address(21);
	default: return NULL;
	}
}

static CC_NOINLINE void* GetFrameAddress(int level) {
	/* "... a value of 0 yields the frame address of the current function, a value of 1 yields the frame address of the caller of the current function, and so forth." */
	switch(level) {
	case 0:  return __builtin_frame_address(1);
	case 1:  return __builtin_frame_address(2);
	case 2:  return __builtin_frame_address(3);
	case 3:  return __builtin_frame_address(4);
	case 4:  return __builtin_frame_address(5);
	case 5:  return __builtin_frame_address(6);
	case 6:  return __builtin_frame_address(7);
	case 7:  return __builtin_frame_address(8);
	case 8:  return __builtin_frame_address(9);
	case 9:  return __builtin_frame_address(10);
	case 10: return __builtin_frame_address(11);
	case 11: return __builtin_frame_address(12);
	case 12: return __builtin_frame_address(13);
	case 13: return __builtin_frame_address(14);
	case 14: return __builtin_frame_address(15);
	case 15: return __builtin_frame_address(16);
	case 16: return __builtin_frame_address(17);
	case 17: return __builtin_frame_address(18);
	case 18: return __builtin_frame_address(19);
	case 19: return __builtin_frame_address(20);
	case 20: return __builtin_frame_address(21);
	default: return NULL;
	}
}

void Logger_Backtrace(cc_string* trace, void* ctx) {
	void* addrs[MAX_BACKTRACE_FRAMES];
	int i, frames;
	
	/* See https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html */
	/*   Note "Calling this function with a nonzero argument can have unpredictable effects, including crashing the calling program" */
	/*   So this probably only works on x86/x86_64 */
	for (i = 0; GetFrameAddress(i + 1) && i < MAX_BACKTRACE_FRAMES; i++)
	{
		addrs[i] = GetReturnAddress(i);
		if (!addrs[i]) break;
	}	

	frames = i;
	for (i = 0; i < frames; i++) {
		DumpFrame(trace, addrs[i]);
	}
	String_AppendConst(trace, _NL);
}
#endif
