#define CC_NO_UPDATER
#define CC_NO_DYNLIB
#define CC_NO_SOCKETS
#define CC_NO_THREADING
#define OVERRIDE_MEM_FUNCTIONS

#include "../Stream.h"
#include "../ExtMath.h"
#include "../SystemFonts.h"
#include "../Funcs.h"
#include "../Window.h"
#include "../Utils.h"
#include "../Errors.h"
#include "../PackedCol.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <string.h>

const cc_result ReturnCode_FileShareViolation = 1000000000;
const cc_result ReturnCode_FileNotFound     = 1000000;
const cc_result ReturnCode_PathNotFound     = 99999;
const cc_result ReturnCode_DirectoryExists  = 1000000;
const cc_result ReturnCode_SocketInProgess  = 1000000;
const cc_result ReturnCode_SocketWouldBlock = 1000000;
const cc_result ReturnCode_SocketDropped    = 1000000;

const char* Platform_AppNameSuffix = " Amiga";
cc_uint8 Platform_Flags = PLAT_FLAG_SINGLE_PROCESS;
cc_bool  Platform_ReadonlyFilesystem;
#include "../_PlatformBase.h"

#ifdef __GNUC__
static const char __attribute__((used)) min_stack[] = "$STACK:102400";
#else
size_t __stack = 65536; // vbcc
#endif

/*########################################################################################################################*
*-----------------------------------------------------Main entrypoint-----------------------------------------------------*
*#########################################################################################################################*/
#include "../main_impl.h"

int main(int argc, char** argv) {
	cc_result res;
	SetupProgram(argc, argv);

	do {
		res = RunProgram(argc, argv);
	} while (Window_Main.Exists);

	Window_Free();
	Process_Exit(res);
	return res;
}

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? AllocVec(size, MEMF_ANY) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? AllocVec(size, MEMF_ANY | MEMF_CLEAR) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	return NULL; // TODO
}

void Mem_Free(void* mem) {
	if (mem) FreeVec(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	Write(Output(), msg, len);
	Write(Output(), "\n", 1);
}

TimeMS DateTime_CurrentUTC(void) {
	ULONG secs, micro;
	//CurrentTime(&secs, &micro);
	// TODO epoch adjustment
	//return secs;
	return 0;
}

void DateTime_CurrentLocal(struct cc_datetime* t) {
	// TODO
}


/*########################################################################################################################*
*-------------------------------------------------------Crash handling----------------------------------------------------*
*#########################################################################################################################*/
void CrashHandler_Install(void) { }

void Process_Abort2(cc_result result, const char* raw_msg) {
	Logger_DoAbort(result, raw_msg, NULL);
}


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#define US_PER_SEC 1000000ULL

cc_uint64 Stopwatch_Measure(void) {
#ifdef __GNUC__
	ULONG secs, micro;
	CurrentTime(&secs, &micro);
	return secs * US_PER_SEC + micro;
#else
	// TODO
	return 10;
#endif
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return end - beg;
}


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Platform_EncodePath(cc_filepath* dst, const cc_string* path) {
	char* buf = dst->buffer;
	char* str = dst->buffer;
	
	// Amiga OS uses : to separate directories
	for (int i = 0; i < path->length; i++) 
	{
		char c = (char)path->buffer[i];
		if (c == '/') c = ':';
		*str++ = c;
	}
	*str = '\0';
	// TODO
}

void Platform_DecodePath(cc_string* dst, const cc_filepath* path) {
	String_AppendConst(dst, path->buffer);
}


void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create2(const cc_filepath* path) {
	return ERR_NOT_SUPPORTED; // TODO
}

int File_Exists(const cc_filepath* path) {
	return false; // TODO
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Open(cc_file* file, const cc_filepath* path) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Create(cc_file* file, const cc_filepath* path) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_OpenOrCreate(cc_file* file, const cc_filepath* path) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Close(cc_file file) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	return ERR_NOT_SUPPORTED; // TODO
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	return ERR_NOT_SUPPORTED; // TODO
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) {
	cc_uint32 ticks = milliseconds * 50 / 1000;
	// per documentation, Delay works in 50 ticks/second
	Delay(ticks);
}


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = false;

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	return GetGameArgs(args);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	return SetGameArgs(args, numArgs);
}

void Process_Exit(cc_result code) { 
	Exit(code);
    for(;;) { }
}

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Free(void) { }

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	// TODO
	return false;
}

void Platform_Init(void) {
	SysBase = *((struct Library **)4UL);
	DOSBase = OpenLibrary("dos.library", 0);
}

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_GetEntropy(void* data, int len) {
	return ERR_NOT_SUPPORTED;
}
