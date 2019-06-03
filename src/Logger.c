#include "Logger.h"
#include "Platform.h"
#include "Chat.h"
#include "Window.h"
#include "Funcs.h"
#include "Stream.h"
#include "Errors.h"

#ifdef CC_BUILD_WEB
#undef CC_BUILD_POSIX /* Can't see native CPU state with javascript */
#endif
#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <imagehlp.h>
#endif

/* POSIX can be shared between unix-ish systems */
#ifdef CC_BUILD_POSIX
#if defined CC_BUILD_OPENBSD
/* OpenBSD doesn't provide ucontext.h */
#elif defined CC_BUILD_LINUX
/* Need to define this to get REG_ constants */
#define __USE_GNU
#include <ucontext.h>
#undef  __USE_GNU
#else
#include <ucontext.h>
#endif
#include <execinfo.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#endif


/*########################################################################################################################*
*----------------------------------------------------------Warning--------------------------------------------------------*
*#########################################################################################################################*/
void Logger_DialogWarn(const String* msg) {
	String dst; char dstBuffer[512];
	String_InitArray_NT(dst, dstBuffer);

	String_AppendString(&dst, msg);
	dst.buffer[dst.length] = '\0';
	Window_ShowDialog(Logger_DialogTitle, dst.buffer);
}
const char* Logger_DialogTitle = "Error";
Logger_DoWarn Logger_WarnFunc  = Logger_DialogWarn;

void Logger_OldWarn(ReturnCode res, const char* place) {
	String msg; char msgBuffer[128];
	String_InitArray(msg, msgBuffer);

	String_Format2(&msg, "Error %h when %c", &res, place);
	Logger_WarnFunc(&msg);
}

void Logger_OldWarn2(ReturnCode res, const char* place, const String* path) {
	String msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);

	String_Format3(&msg, "Error %h when %c '%s'", &res, place, path);
	Logger_WarnFunc(&msg);
}

/* Returns a description for ClassiCube specific error codes */
static const char* Logger_GetCCErrorDesc(ReturnCode res) {
	switch (res) {
	case ERR_END_OF_STREAM:    return "End of stream";
	case ERR_NOT_SUPPORTED:    return "Operation not supported";
	case ERR_INVALID_ARGUMENT: return "Invalid argument";
	case OGG_ERR_INVALID_SIG:  return "Invalid OGG signature";
	case OGG_ERR_VERSION:      return "Invalid OGG format version";

	case WAV_ERR_STREAM_HDR:  return "Invalid WAV header";
	case WAV_ERR_STREAM_TYPE: return "Invalid WAV type";
	case WAV_ERR_DATA_TYPE:   return "Unsupported WAV audio format";
	case WAV_ERR_NO_DATA:     return "No audio in WAV";

			/* Vorbis audio decoding errors */ /*
			VORBIS_ERR_HEADER,
			VORBIS_ERR_WRONG_HEADER,
			VORBIS_ERR_FRAMING,
			VORBIS_ERR_VERSION, 
			VORBIS_ERR_BLOCKSIZE, 
			VORBIS_ERR_CHANS,
			VORBIS_ERR_TIME_TYPE,
			VORBIS_ERR_FLOOR_TYPE, 
			VORBIS_ERR_RESIDUE_TYPE,
			VORBIS_ERR_MAPPING_TYPE, 
			VORBIS_ERR_MODE_TYPE,
			VORBIS_ERR_CODEBOOK_SYNC, 
			VORBIS_ERR_CODEBOOK_ENTRY, 
			VORBIS_ERR_CODEBOOK_LOOKUP,
			VORBIS_ERR_MODE_WINDOW, 
			VORBIS_ERR_MODE_TRANSFORM,
			VORBIS_ERR_MAPPING_CHANS, 
			VORBIS_ERR_MAPPING_RESERVED,
			VORBIS_ERR_FRAME_TYPE,

			/* PNG image decoding errors */
	case PNG_ERR_INVALID_SIG:      return "Invalid PNG signature";
	case PNG_ERR_INVALID_HDR_SIZE: return "Invalid PNG header size";
	case PNG_ERR_TOO_WIDE:         return "PNG image too wide";
	case PNG_ERR_TOO_TALL:         return "PNG image too tall";
	case PNG_ERR_INVALID_COL_BPP:  return "Invalid color type in PNG";
	case PNG_ERR_COMP_METHOD:      return "Invalid compression in PNG";
	case PNG_ERR_FILTER:           return "Invalid filter in PNG"; 
	case PNG_ERR_INTERLACED:       return "Interlaced PNGs unsupported";
	case PNG_ERR_PAL_SIZE:         return "Invalid size of palette data";
	case PNG_ERR_TRANS_COUNT:      return "Invalid number of transparency entries";
	case PNG_ERR_TRANS_INVALID:    return "Transparency invalid for color type";
	case PNG_ERR_INVALID_END_SIZE: return "Non-empty IEND chunk";
	case PNG_ERR_NO_DATA:          return "No image in PNG";
	case PNG_ERR_INVALID_SCANLINE: return "Invalid PNG scanline type";

			/* ZIP archive decoding errors */ /*
			ZIP_ERR_TOO_MANY_ENTRIES, 
			ZIP_ERR_SEEK_END_OF_CENTRAL_DIR, 
			ZIP_ERR_NO_END_OF_CENTRAL_DIR,
			ZIP_ERR_SEEK_CENTRAL_DIR, 
			ZIP_ERR_INVALID_CENTRAL_DIR,
			ZIP_ERR_SEEK_LOCAL_DIR, 
			ZIP_ERR_INVALID_LOCAL_DIR, 
			ZIP_ERR_FILENAME_LEN,
			/* GZIP header decoding errors */ /*
			GZIP_ERR_HEADER1, 
			GZIP_ERR_HEADER2, 
			GZIP_ERR_METHOD, 
			GZIP_ERR_FLAGS,
			/* ZLIB header decoding errors */ /*
			ZLIB_ERR_METHOD, 
			ZLIB_ERR_WINDOW_SIZE,
			ZLIB_ERR_FLAGS,
			/* FCM map decoding errors */ /*
			FCM_ERR_IDENTIFIER, 
			FCM_ERR_REVISION,
			/* LVL map decoding errors */ /*
			LVL_ERR_VERSION,
			/* DAT map decoding errors */ /*
			DAT_ERR_IDENTIFIER,
			DAT_ERR_VERSION, 
			DAT_ERR_JIDENTIFIER,
			DAT_ERR_JVERSION,
			DAT_ERR_ROOT_TYPE,
			DAT_ERR_JSTRING_LEN,
			DAT_ERR_JFIELD_CLASS_NAME,
			DAT_ERR_JCLASS_TYPE, 
			DAT_ERR_JCLASS_FIELDS,
			DAT_ERR_JCLASS_ANNOTATION,
			DAT_ERR_JOBJECT_TYPE, 
			DAT_ERR_JARRAY_TYPE,
			DAT_ERR_JARRAY_CONTENT,
			*/

	case NBT_ERR_INT32S:    return "I32_Array NBT tag unsupported";
	case NBT_ERR_UNKNOWN:   return "Unknown NBT tag type";
	case CW_ERR_ROOT_TAG:   return "Invalid root NBT tag";
	case CW_ERR_STRING_LEN: return "NBT string too long";
	}
	return NULL;
}

/* Appends more detailed information about an error if possible */
static void Logger_AppendErrorDesc(String* msg, ReturnCode res, Logger_DescribeError describeErr) {
	const char* cc_err;
	String err; char errBuffer[128];
	String_InitArray(err, errBuffer);

	cc_err = Logger_GetCCErrorDesc(res);
	if (cc_err) {
		String_Format1(msg, "\n  Error meaning: %c", cc_err);
	} else if (describeErr(res, &err)) {
		String_Format1(msg, "\n  Error meaning: %s", &err);
	}
}

void Logger_SysWarn(ReturnCode res, const char* place, Logger_DescribeError describeErr) {
	String msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);

	String_Format2(&msg, "Error %h when %c", &res, place);
	Logger_AppendErrorDesc(&msg, res, describeErr);
	Logger_WarnFunc(&msg);
}

void Logger_SysWarn2(ReturnCode res, const char* place, const String* path, Logger_DescribeError describeErr) {
	String msg; char msgBuffer[256];
	String_InitArray(msg, msgBuffer);

	String_Format3(&msg, "Error %h when %c '%s'", &res, place, path);
	Logger_AppendErrorDesc(&msg, res, describeErr);
	Logger_WarnFunc(&msg);
}

void Logger_DynamicLibWarn2(ReturnCode res, const char* place, const String* path) {
	Logger_SysWarn2(res, place, path, DynamicLib_DescribeError);
}
void Logger_Warn(ReturnCode res, const char* place) {
	Logger_SysWarn(res, place,  Platform_DescribeError);
}
void Logger_Warn2(ReturnCode res, const char* place, const String* path) {
	Logger_SysWarn2(res, place, path, Platform_DescribeError);
}


/*########################################################################################################################*
*-------------------------------------------------------Info dumping------------------------------------------------------*
*#########################################################################################################################*/
/* Unfortunately, operating systems vary wildly in how they name and access registers for dumping */
/* So this is the simplest way to avoid duplicating code on each platform */
#define Logger_Dump_X86() \
String_Format3(&str, "eax=%x ebx=%x ecx=%x" _NL, REG_GET(ax,AX), REG_GET(bx,BX), REG_GET(cx,CX));\
String_Format3(&str, "edx=%x esi=%x edi=%x" _NL, REG_GET(dx,DX), REG_GET(si,SI), REG_GET(di,DI));\
String_Format3(&str, "eip=%x ebp=%x esp=%x" _NL, REG_GET(ip,IP), REG_GET(bp,BP), REG_GET(sp,SP));

#define Logger_Dump_X64() \
String_Format3(&str, "rax=%x rbx=%x rcx=%x" _NL, REG_GET(ax,AX), REG_GET(bx,BX), REG_GET(cx,CX));\
String_Format3(&str, "rdx=%x rsi=%x rdi=%x" _NL, REG_GET(dx,DX), REG_GET(si,SI), REG_GET(di,DI));\
String_Format3(&str, "rip=%x rbp=%x rsp=%x" _NL, REG_GET(ip,IP), REG_GET(bp,BP), REG_GET(sp,SP));\
String_Format3(&str, "r8 =%x r9 =%x r10=%x" _NL, REG_GET(8,8),   REG_GET(9,9),   REG_GET(10,10));\
String_Format3(&str, "r11=%x r12=%x r13=%x" _NL, REG_GET(11,11), REG_GET(12,12), REG_GET(13,13));\
String_Format2(&str, "r14=%x r15=%x" _NL,        REG_GET(14,14), REG_GET(15,15));

#define Logger_Dump_PPC() \
String_Format4(&str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(&str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(&str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(&str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(&str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(&str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(&str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format4(&str, "r28=%x r29=%x r30=%x r31=%x" _NL, REG_GNUM(28), REG_GNUM(29), REG_GNUM(30), REG_GNUM(31)); \
String_Format3(&str, "pc =%x lr =%x ctr=%x" _NL,  REG_GET(srr0, SRR0), REG_GET(lr, LR), REG_GET(ctr,CTR));

#define Logger_Dump_ARM32() \
String_Format3(&str, "r0 =%x r1 =%x r2 =%x" _NL, REG_GNUM(0), REG_GNUM(1),  REG_GNUM(2));\
String_Format3(&str, "r3 =%x r4 =%x r5 =%x" _NL, REG_GNUM(3), REG_GNUM(4),  REG_GNUM(5));\
String_Format3(&str, "r6 =%x r7 =%x r8 =%x" _NL, REG_GNUM(6), REG_GNUM(7),  REG_GNUM(8));\
String_Format3(&str, "r9 =%x r10=%x fp =%x" _NL, REG_GNUM(9), REG_GNUM(10), REG_GET(fp,FP));\
String_Format3(&str, "sp =%x lr =%x pc =%x" _NL, REG_GET(sp,SP), REG_GET(lr,LR),  REG_GET(pc,PC));

#define Logger_Dump_ARM64() \
String_Format4(&str, "r0 =%x r1 =%x r2 =%x r3 =%x" _NL, REG_GNUM(0),  REG_GNUM(1),  REG_GNUM(2),  REG_GNUM(3)); \
String_Format4(&str, "r4 =%x r5 =%x r6 =%x r7 =%x" _NL, REG_GNUM(4),  REG_GNUM(5),  REG_GNUM(6),  REG_GNUM(7)); \
String_Format4(&str, "r8 =%x r9 =%x r10=%x r11=%x" _NL, REG_GNUM(8),  REG_GNUM(9),  REG_GNUM(10), REG_GNUM(11)); \
String_Format4(&str, "r12=%x r13=%x r14=%x r15=%x" _NL, REG_GNUM(12), REG_GNUM(13), REG_GNUM(14), REG_GNUM(15)); \
String_Format4(&str, "r16=%x r17=%x r18=%x r19=%x" _NL, REG_GNUM(16), REG_GNUM(17), REG_GNUM(18), REG_GNUM(19)); \
String_Format4(&str, "r20=%x r21=%x r22=%x r23=%x" _NL, REG_GNUM(20), REG_GNUM(21), REG_GNUM(22), REG_GNUM(23)); \
String_Format4(&str, "r24=%x r25=%x r26=%x r27=%x" _NL, REG_GNUM(24), REG_GNUM(25), REG_GNUM(26), REG_GNUM(27)); \
String_Format3(&str, "r28=%x r29=%x r30=%x" _NL,        REG_GNUM(28), REG_GNUM(29), REG_GNUM(30)); \
String_Format2(&str, "sp =%x pc =%x" _NL,               REG_GET(sp,SP), REG_GET(pc,PC));

#define Logger_Dump_SPARC() \
String_Format4(&str, "o0=%x o1=%x o2=%x o3=%x" _NL, REG_GET(o0,O0), REG_GET(o1,O1), REG_GET(o2,O2), REG_GET(o3,O3)); \
String_Format4(&str, "o4=%x o5=%x o6=%x o7=%x" _NL, REG_GET(o4,O4), REG_GET(o5,O5), REG_GET(o6,O6), REG_GET(o7,O7)); \
String_Format4(&str, "g1=%x g2=%x g3=%x g4=%x" _NL, REG_GET(g1,G1), REG_GET(g2,G2), REG_GET(g3,G3), REG_GET(g4,G4)); \
String_Format4(&str, "g5=%x g6=%x g7=%x y =%x" _NL, REG_GET(g5,G5), REG_GET(g6,G6), REG_GET(g7,G7), REG_GET( y, Y)); \
String_Format2(&str, "pc=%x nc=%x" _NL,             REG_GET(pc,PC), REG_GET(npc,nPC));

#if defined CC_BUILD_WEB
static void Logger_DumpBacktrace(String* str, void* ctx) { }
static void Logger_DumpRegisters(void* ctx) { }
static void Logger_DumpMisc(void* ctx) { }
#elif defined CC_BUILD_WIN
struct StackPointers { uintptr_t Instruction, Frame, Stack; };
struct SymbolAndName { IMAGEHLP_SYMBOL Symbol; char Name[256]; };

static int Logger_GetFrames(CONTEXT* ctx, struct StackPointers* pointers, int max) {
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

static void Logger_Backtrace(String* backtrace, void* ctx) {
	struct SymbolAndName sym = { 0 };
	sym.Symbol.MaxNameLength = 255;
	sym.Symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);

	HANDLE process = GetCurrentProcess();
	struct StackPointers pointers[40];
	int i, frames = Logger_GetFrames((CONTEXT*)ctx, pointers, 40);

	for (i = 0; i < frames; i++) {
		int number = i + 1;
		uintptr_t addr = pointers[i].Instruction;

		char strBuffer[512];
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
		Logger_Log(&str);
	}
	String_AppendConst(backtrace, "\r\n");
}

static void Logger_DumpBacktrace(String* str, void* ctx) {
	const static String backtrace = String_FromConst("-- backtrace --\r\n");
	HANDLE process = GetCurrentProcess();

	SymInitialize(process, NULL, TRUE);
	Logger_Log(&backtrace);
	Logger_Backtrace(str, ctx);
}

static void Logger_DumpRegisters(void* ctx) {
	String str; char strBuffer[512];
	CONTEXT* r = (CONTEXT*)ctx;

	String_InitArray(str, strBuffer);
	String_AppendConst(&str, "-- registers --\r\n");

#if defined _M_IX86
	#define REG_GET(reg, ign) &r->E ## reg
	Logger_Dump_X86()
#elif defined _M_X64
	#define REG_GET(reg, ign) &r->R ## reg
	Logger_Dump_X64()
#else
#error "Unknown machine type"
#endif
	Logger_Log(&str);
}

static BOOL CALLBACK Logger_DumpModule(const char* name, ULONG_PTR base, ULONG size, void* ctx) {
	String str; char strBuffer[256];
	uintptr_t beg, end;

	beg = base; end = base + (size - 1);
	String_InitArray(str, strBuffer);

	String_Format3(&str, "%c = %x-%x\r\n", name, &beg, &end);
	Logger_Log(&str);
	return true;
}

static void Logger_DumpMisc(void* ctx) {
	const static String modules = String_FromConst("-- modules --\r\n");
	HANDLE process = GetCurrentProcess();

	Logger_Log(&modules);
	EnumerateLoadedModules(process, Logger_DumpModule, NULL);
}

#elif defined CC_BUILD_POSIX
static void Logger_Backtrace(String* backtrace_, void* ctx) {
	String str; char strBuffer[384];
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
		Logger_Log(&str);
	}

	String_AppendConst(backtrace_, "\n");
	free(strings);
}

static void Logger_DumpBacktrace(String* str, void* ctx) {
	const static String backtrace = String_FromConst("-- backtrace --\n");
	Logger_Log(&backtrace);
	Logger_Backtrace(str, ctx);
}

static void Logger_DumpRegisters(void* ctx) {
	String str; char strBuffer[512];
#ifdef CC_BUILD_OPENBSD
	struct sigcontext r;
	r = *((ucontext_t*)ctx);
#else
	mcontext_t r;
	r = ((ucontext_t*)ctx)->uc_mcontext;
#endif

	String_InitArray(str, strBuffer);
	String_AppendConst(&str, "-- registers --\n");

	/* Linux:   See /usr/include/sys/ucontext.h */
	/* OSX:     See /usr/include/mach/i386/_structs.h */
	/* Solaris: See /usr/include/sys/regset.h */
	/* NetBSD:  See /usr/include/i386/mcontext.h */
	/* OpenBSD: See /usr/include/machine/signal.h */

#if defined __i386__
	#if defined CC_BUILD_LINUX
		#define REG_GET(ign, reg) &r.gregs[REG_E##reg]
	#elif defined CC_BUILD_OSX
		#define REG_GET(reg, ign) &r->__ss.__e##reg
	#elif defined CC_BUILD_SOLARIS
		#define REG_GET(ign, reg) &r.gregs[E##reg]
	#elif defined CC_BUILD_FREEBSD
		#define REG_GET(reg, ign) &r.mc_e##reg
	#elif defined CC_BUILD_OPENBSD
		#define REG_GET(reg, ign) &r.sc_e##reg
	#elif defined CC_BUILD_NETBSD
		#define REG_GET(ign, reg) &r.__gregs[_REG_E##reg]
	#endif
	Logger_Dump_X86()
#elif defined __x86_64__
	#if defined CC_BUILD_LINUX
		#define REG_GET(ign, reg) &r.gregs[REG_R##reg]
	#elif defined CC_BUILD_OSX
		#define REG_GET(reg, ign) &r->__ss.__r##reg
	#elif defined CC_BUILD_SOLARIS
		#define REG_GET(ign, reg) &r.gregs[REG_R##reg]
	#elif defined CC_BUILD_FREEBSD
		#define REG_GET(reg, ign) &r.mc_r##reg
	#elif defined CC_BUILD_OPENBSD
		#define REG_GET(reg, ign) &r.sc_r##reg
	#elif defined CC_BUILD_NETBSD
		#define REG_GET(ign, reg) &r.__gregs[_REG_R##reg]
	#endif
	Logger_Dump_X64()
#elif defined __ppc__
	#if defined CC_BUILD_OSX
		#define REG_GNUM(num)     &r->__ss.__r##num
		#define REG_GET(reg, ign) &r->__ss.__##reg
	#endif
	Logger_Dump_PPC()
#elif defined __aarch64__
	#if defined CC_BUILD_LINUX
		#define REG_GNUM(num)     &r.regs[num]
		#define REG_GET(reg, ign) &r.##reg
	#endif
	Logger_Dump_ARM64()
#elif defined __arm__
	#if defined CC_BUILD_LINUX
		#define REG_GNUM(num)     &r.arm_r##num
		#define REG_GET(reg, ign) &r.arm_##reg
	#endif
	Logger_Dump_ARM32()
#elif defined __sparc__
	#if defined CC_BUILD_LINUX
		#define REG_GET(ign, reg) &r.gregs[REG_##reg]
	#endif
	Logger_Dump_SPARC()
#else
#error "Unknown ISA/architecture"
#endif

	Logger_Log(&str);
}

/* OS specific stuff */
#if defined CC_BUILD_LINUX || defined CC_BUILD_SOLARIS
static void Logger_DumpMisc(void* ctx) {
	const static String memMap = String_FromConst("-- memory map --\n");
	String str; char strBuffer[320];
	int n, fd;

	Logger_Log(&memMap);
	/* dumps all known ranges of memory */
	fd = open("/proc/self/maps", O_RDONLY);
	if (fd < 0) return;
	String_InitArray(str, strBuffer);

	while ((n = read(fd, str.buffer, str.capacity)) > 0) {
		str.length = n;
		Logger_Log(&str);
	}

	close(fd);
}
#else
static void Logger_DumpMisc(void* ctx) { }
#endif
#endif

/*########################################################################################################################*
*------------------------------------------------------Error handling-----------------------------------------------------*
*#########################################################################################################################*/
static void Logger_AbortCommon(ReturnCode result, const char* raw_msg, void* ctx);

#if defined CC_BUILD_WEB
void Logger_Hook(void) { }
void Logger_Abort2(ReturnCode result, const char* raw_msg) {
	Logger_AbortCommon(result, raw_msg, NULL);
}
#elif defined CC_BUILD_WIN
static LONG WINAPI Logger_UnhandledFilter(struct _EXCEPTION_POINTERS* pInfo) {
	String msg; char msgBuffer[128 + 1];
	uint32_t code;
	uintptr_t addr;

	code = (uint32_t)pInfo->ExceptionRecord->ExceptionCode;
	addr = (uintptr_t)pInfo->ExceptionRecord->ExceptionAddress;

	String_InitArray_NT(msg, msgBuffer);
	String_Format2(&msg, "Unhandled exception 0x%h at 0x%x", &code, &addr);
	msg.buffer[msg.length] = '\0';

	Logger_AbortCommon(0, msg.buffer, pInfo->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER; /* TODO: different flag */
}
void Logger_Hook(void) { SetUnhandledExceptionFilter(Logger_UnhandledFilter); }

/* Don't want compiler doing anything fancy with registers */
#if _MSC_VER
#pragma optimize ("", off)
#endif
void Logger_Abort2(ReturnCode result, const char* raw_msg) {
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
	__asm__(
		"mov 0(%%ebp), %%eax \n\t"
		"mov %%eax, %0       \n\t"
		"mov 4(%%ebp), %%eax \n\t"
		"mov %%eax, %1       \n\t"
		"lea 8(%%ebp), %%eax \n\t"
		"mov %%eax, %2"
		: "=m" (ctx.Ebp), "=m" (ctx.Eip), "=m" (ctx.Esp)
		:
		: "eax", "memory"
	);
	ctx.ContextFlags = CONTEXT_CONTROL;
#endif

	Logger_AbortCommon(result, raw_msg, &ctx);
}
#if _MSC_VER
#pragma optimize ("", on)
#endif
#elif defined CC_BUILD_POSIX
static void Logger_SignalHandler(int sig, siginfo_t* info, void* ctx) {
	String msg; char msgBuffer[128 + 1];
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

	Logger_AbortCommon(0, msg.buffer, ctx);
}

void Logger_Hook(void) {
	struct sigaction sa, old;
	sa.sa_sigaction = Logger_SignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, &old);
	sigaction(SIGBUS,  &sa, &old);
	sigaction(SIGILL,  &sa, &old);
	sigaction(SIGABRT, &sa, &old);
	sigaction(SIGFPE,  &sa, &old);
}

void Logger_Abort2(ReturnCode result, const char* raw_msg) {
#ifdef CC_BUILD_OPENBSD
	/* getcontext is absent on OpenBSD */
	Logger_AbortCommon(result, raw_msg, NULL);
#else
	ucontext_t ctx;
	getcontext(&ctx);
	Logger_AbortCommon(result, raw_msg, &ctx);
#endif
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Common---------------------------------------------------------*
*#########################################################################################################################*/
static FileHandle logFile;
static struct Stream logStream;
static bool logOpen;

void Logger_Log(const String* msg) {
	const static String path = String_FromConst("client.log");
	ReturnCode res;

	if (!logOpen) {
		logOpen = true;
		res     = File_Append(&logFile, &path);
		if (!res) Stream_FromFile(&logStream, logFile);
	}

	if (!logStream.Meta.File) return;
	Stream_Write(&logStream, (const uint8_t*)msg->buffer, msg->length);
}

static void Logger_LogCrashHeader(void) {
	String msg; char msgBuffer[96];
	struct DateTime now;

	String_InitArray(msg, msgBuffer);
	String_AppendConst(&msg, _NL "----------------------------------------" _NL);
	Logger_Log(&msg);
	msg.length = 0;

	DateTime_CurrentLocal(&now);
	String_Format3(&msg, "Crash time: %p2/%p2/%p4 ", &now.Day,  &now.Month,  &now.Year);
	String_Format3(&msg, "%p2:%p2:%p2" _NL,          &now.Hour, &now.Minute, &now.Second);
	Logger_Log(&msg);
}

static void Logger_AbortCommon(ReturnCode result, const char* raw_msg, void* ctx) {	
	String msg; char msgBuffer[3070 + 1];
	String_InitArray_NT(msg, msgBuffer);

	String_Format1(&msg, "ClassiCube crashed." _NL "Reason: %c" _NL, raw_msg);
	#ifdef CC_COMMIT_SHA
	String_Format1(&msg, "Commit SHA: %c" _NL, CC_COMMIT_SHA);
	#endif

	if (result) {
		String_Format1(&msg, "%h" _NL, &result);
	} else { result = 1; }

	Logger_LogCrashHeader();
	Logger_Log(&msg);

	String_AppendConst(&msg, "Full details of the crash have been logged to 'client.log'.\n");
	String_AppendConst(&msg, "Please report this on the ClassiCube forums or to UnknownShadow200.\n\n");

	if (ctx) Logger_DumpRegisters(ctx);
	Logger_DumpBacktrace(&msg, ctx);
	Logger_DumpMisc(ctx);
	if (logStream.Meta.File) File_Close(logFile);

	msg.buffer[msg.length] = '\0';
	Window_ShowDialog("We're sorry", msg.buffer);
	Process_Exit(result);
}

void Logger_Abort(const char* raw_msg) { Logger_Abort2(0, raw_msg); }
