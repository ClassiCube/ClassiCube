#include "../_PlatformBase.h"
#include "../Stream.h"
#include "../ExtMath.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../Options.h"
#include "../Animations.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gbadefs.h"

#define OVERRIDE_MEM_FUNCTIONS
#include "../_PlatformConsole.h"

#include "../../third_party/tinyalloc/tinyalloc.c"

typedef volatile uint8_t   vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;

const cc_result ReturnCode_FileShareViolation = 1000000000; // not used
const cc_result ReturnCode_FileNotFound     = -1;
const cc_result ReturnCode_DirectoryExists  = -1;
const cc_result ReturnCode_SocketInProgess  = -1;
const cc_result ReturnCode_SocketWouldBlock = -1;
const cc_result ReturnCode_SocketDropped    = -1;

const char* Platform_AppNameSuffix = " GBA";
cc_bool Platform_ReadonlyFilesystem;


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	Platform_Log1("  MALLOC: %i", &size);
	void* ptr = size ? ta_alloc(size) : NULL;
	Platform_Log1("MALLOCED: %x", &ptr);
    return ptr;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	Platform_Log1("  CALLOC: %i", &size);
	void* ptr = size ? ta_alloc(size) : NULL;
	Platform_Log1("CALLOCED: %x", &ptr);

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

	// Did lo timer possibly overflow between reading lo and hi?
	uint16_t lo_again = REG_TMR2_DATA;
	uint16_t hi_again = REG_TMR3_DATA;

	if (lo_again < lo) {
		// If so, use known safe timer read values instead
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

extern int nocash_puts(const char *str);
static void Log_Nocash(char* buffer) {
	nocash_puts(buffer);
}

#define MGBA_LOG_DEBUG 4
#define REG_DEBUG_ENABLE (vu16*)0x4FFF780
#define REG_DEBUG_FLAGS  (vu16*)0x4FFF700
#define REG_DEBUG_STRING (char*)0x4FFF600

static void Log_mgba(char* buffer, int len) {
	*REG_DEBUG_ENABLE = 0xC0DE;
	// Check if actually emulated or not
	if (*REG_DEBUG_ENABLE != 0x1DEA) return;

	Mem_Copy(REG_DEBUG_STRING, buffer, len);
	*REG_DEBUG_FLAGS = MGBA_LOG_DEBUG | 0x100;
}

void Platform_Log(const char* msg, int len) {
    // Can only be up to 120 bytes total
	char buffer[120];
	len = min(len, 118);
	
	Mem_Copy(buffer, msg, len);
	buffer[len + 0] = '\n';
	buffer[len + 1] = '\0';
	Log_Nocash(buffer);
	Log_mgba(buffer, len);
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
	_exit(0);
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* str = dst->buffer;
	String_EncodeUtf8(str, path);
}

cc_result Directory_Create(const cc_filepath* path) {
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

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	*handle = NULL;
}

void Thread_Detach(void* handle) {
}

void Thread_Join(void* handle) {
}

void* Mutex_Create(const char* name) {
	return NULL;
}

void Mutex_Free(void* handle) {
}

void Mutex_Lock(void* handle) {
}

void Mutex_Unlock(void* handle) {
}

void* Waitable_Create(const char* name) {
	return NULL;
}

void Waitable_Free(void* handle) {
}

void Waitable_Signal(void* handle) {
}

void Waitable_Wait(void* handle) {
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Create(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Connect(cc_socket s, cc_sockaddr* addr) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}

void Socket_Close(cc_socket s) {
}

cc_result Socket_CheckReadable(cc_socket s, cc_bool* readable) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_CheckWritable(cc_socket s, cc_bool* writable) {
	return ERR_NOT_SUPPORTED;
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

