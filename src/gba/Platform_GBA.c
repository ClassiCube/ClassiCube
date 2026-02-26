#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING
#define OVERRIDE_MEM_FUNCTIONS

#include "../Stream.h"
#include "../ExtMath.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../Options.h"
#include "../Animations.h"

#include <stdint.h>
#include <string.h>
#include "gbadefs.h"

#include "../../third_party/tinyalloc/tinyalloc.c"

typedef volatile uint8_t   vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = -1;
const cc_result ReturnCode_PathNotFound     = -1;
const cc_result ReturnCode_DirectoryExists  = -1;
const cc_result ReturnCode_SocketInProgess  = -1;
const cc_result ReturnCode_SocketWouldBlock = -1;
const cc_result ReturnCode_SocketDropped    = -1;

const char* Platform_AppNameSuffix = " GBA";
cc_bool Platform_ReadonlyFilesystem;
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS | PLAT_FLAG_APP_EXIT;
#include "../_PlatformBase.h"


/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

int main(int argc, char** argv) {
	SetupProgram(argc, argv);
	while (Window_Main.Exists) { 
		RunGame();
	}
	
	Window_Free();
	return 0;
}


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	void* ptr = size ? ta_alloc(size) : NULL;
	Platform_Log2("MALLOCED: %x (%i bytes)", &ptr, &size);
    return ptr;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	void* ptr = size ? ta_alloc(size) : NULL;
	Platform_Log2("CALLOCED: %x (%i bytes)", &ptr, &size);

	if (ptr) Mem_Set(ptr, 0, size);
    return ptr;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	return NULL; // TODO
}

void Mem_Free(void* mem) {
	if (mem) ta_free(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
static uint32_t GetTimerValues(void) {
	uint16_t lo = REG_TMR2_DATA;
    uint16_t hi = REG_TMR3_DATA;

	// Lo timer can possibly overflow between reading lo and hi
	uint16_t lo_again = REG_TMR2_DATA;
	uint16_t hi_again = REG_TMR3_DATA;

	// Check if lo timer has overflowed
	if (lo_again < lo) {
		// If so, use known stable timer read values instead
		lo = lo_again;
		hi = hi_again;
	}
	return lo | (hi << 16);
}

static void Stopwatch_Init(void) {
	// Turn off both timers
	REG_TMR2_CTRL = 0;
	REG_TMR3_CTRL = 0;

	// Reset timer values to 0
	REG_TMR2_DATA = 0;
	REG_TMR3_DATA = 0;

	// Turn on timer 3, with timer 3 incrementing timer 2 on overflow
	REG_TMR3_CTRL = TMR_CASCADE | TMR_ENABLE;
	REG_TMR2_CTRL = TMR_ENABLE;
}

static uint32_t last_raw;
static uint64_t base_time;

#define US_PER_SEC 1000000
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;

	cc_uint64 delta = end - beg;
	return (delta * US_PER_SEC) / SYS_CLOCK;
}

cc_uint64 Stopwatch_Measure(void) {
	uint32_t raw = GetTimerValues();
	// Since counter is only a 32 bit integer, it overflows after a minute or two
	// TODO use IRQ instead
	// TODO lower frequency ?
	if (last_raw > 0xF0000000 && raw < 0x10000000) {
		base_time += 0x100000000ULL;
	}

	last_raw = raw;
	return base_time + raw;
}

#define MGBA_LOG_DEBUG 4
#define REG_DEBUG_ENABLE (vu16*)0x4FFF780
#define REG_DEBUG_FLAGS  (vu16*)0x4FFF700
#define REG_DEBUG_STRING (char*)0x4FFF600

static void Log_mgba(const char* msg, int len) {
	*REG_DEBUG_ENABLE = 0xC0DE;
	// Check if actually emulated or not
	if (*REG_DEBUG_ENABLE != 0x1DEA) return;

	while (len)
	{
    	// Can only be up to 120 bytes total
		int bit   = min(len, 119);
		char* dst = REG_DEBUG_STRING;

		Mem_Copy(dst, msg, bit);
		dst[bit] = '\0';

		*REG_DEBUG_FLAGS = MGBA_LOG_DEBUG | 0x100;
		msg += bit; len -= bit;
	}
}

// Log to nocash debugger
extern char nocash_msg[82];
extern void nocash_log(void);

static void Log_Nocash(const char* msg, int len) {
	while (len)
	{
    	// Can only be up to 80 bytes total
		int bit   = min(len, 80);
		char* dst = nocash_msg;

		Mem_Copy(dst, msg, bit);
		dst[bit + 0] = '\0';

		nocash_log();
		msg += bit; len -= bit;
	}
}


void Platform_Log(const char* msg, int len) {
	Log_mgba(msg, len);
    Log_Nocash(msg, len);
}

TimeMS DateTime_CurrentUTC(void) {
	return 0;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	Mem_Set(t, 0, sizeof(*t));
}


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) {
}


void Process_Abort2(cc_result result, const char* raw_msg) {
	Platform_LogConst(raw_msg);
	Process_Exit(0);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	int len = String_CopyToRaw(dst->buffer, sizeof(dst->buffer) - 1, path);
	dst->buffer[len] = '\0'; // Always null terminate just in case
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	String_AppendConst(dst, path->buffer);
}

void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	return ERR_NOT_SUPPORTED;
}

int File_Exists(const cc_filepath* path) {
	return false;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Close(cc_file file) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	return ERR_NOT_SUPPORTED;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
// !!! NOTE: PSP uses cooperative multithreading (not preemptive multithreading) !!!
void Thread_Sleep(cc_uint32 milliseconds) { 
	Stopwatch_Measure();
	//swiDelay(8378 * milliseconds); // TODO probably wrong
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
extern void __ewram_end; // end of USED ewram
#define EWRAM_END 0x0203FFFF

void Platform_Init(void) {
	void* heap_beg = &__ewram_end;
	void* heap_end = (void*)EWRAM_END;
	ta_init(heap_beg, heap_end, 256, 16, 4);

	int size = (int)(heap_end - heap_beg);
	Platform_Log3("HEAP SIZE: %i bytes (%x -> %x)", &size, &heap_beg, &heap_end);
	Stopwatch_Init();
}

void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return false;
}

cc_bool Process_OpenSupported = false;
cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return 0;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return 0;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) { 
	return 0; 
}

extern void bios_soft_reset(void);
void Process_Exit(cc_result code) { 
	//*(vu8*)0x03007FFA = 0x00; // controls reset address
	// TODO jump to start_vector instead?
	for (;;) { __asm__ volatile(""); }
}

