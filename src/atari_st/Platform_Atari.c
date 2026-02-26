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

#include <string.h>
#include <stdlib.h>
#include <tos.h>

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

const char* Platform_AppNameSuffix = " Atari";
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
		//RunProgram(argc, argv);
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
// Number of ticks 200 HZ system timer
// Must be called when in supervisor mode
static LONG Read_200HZ(void) { return *((LONG*)0x04BA); }

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;

	// 200 ticks a second
	// 1,000,000 microseconds a second
	// > one tick = 10^6/200 = 5000 microseconds
	return (end - beg) * 5000;
}

cc_uint64 Stopwatch_Measure(void) {
	return Supexec(Read_200HZ);
}

void Platform_Log(const char* msg, int len) {
    for (int i = 0; i < len; i++) 
	{
        Bconout(2, msg[i]);
    }
	Bconout(2, '\r');
	Bconout(2, '\n');
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
	exit(0);
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
	// TODO probably wrong
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Init(void) {
	LONG size = (LONG)Malloc(-1);
	char* ptr = Malloc(size);

	char* heap_beg = ptr;
	char* heap_end = ptr + size;
	ta_init(heap_beg, heap_end, 256, 16, 4);

	int size_ = (int)(heap_end - heap_beg);
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

void Process_Exit(cc_result code) { exit(code); }

