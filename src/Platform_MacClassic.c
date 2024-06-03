#include "Core.h"
#if defined CC_BUILD_MACCLASSIC

#include "_PlatformBase.h"
#include "Stream.h"
#include "ExtMath.h"
#include "SystemFonts.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"
#include "PackedCol.h"
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#undef true
#undef false
#include <MacMemory.h>

const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;

const char* Platform_AppNameSuffix = "MAC68k";
cc_bool Platform_SingleProcess;

/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void* Mem_Set(void*  dst, cc_uint8 value,  unsigned numBytes) { return memset( dst, value, numBytes); }
void* Mem_Copy(void* dst, const void* src, unsigned numBytes) { return memcpy( dst, src,   numBytes); }
void* Mem_Move(void* dst, const void* src, unsigned numBytes) { return memmove(dst, src,   numBytes); }

void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? NewPtr(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? NewPtrClear(size) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	if (!size) return NULL;
	if (!mem)  return NewPtr(size);

	// Try to resize in place
	MemError();
	SetPtrSize(mem, size);
	if (!MemError()) return mem;

	void* newMem = NewPtr(size);
	if (!newMem) return NULL;

	Mem_Copy(newMem, mem, GetPtrSize(mem));
	return newMem;
}

void Mem_Free(void* mem) {
	if (mem) DisposePtr(mem);
}


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log(const char* msg, int len) {
	write(STDOUT_FILENO, msg,  len);
	write(STDOUT_FILENO, "\n",   1);
}

TimeMS DateTime_CurrentUTC(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return (cc_uint64)cur.tv_sec + UNIX_EPOCH_SECONDS;
}

void DateTime_CurrentLocal(struct DateTime* t) {
	struct timeval cur;
	struct tm loc_time;
	gettimeofday(&cur, NULL);
	localtime_r(&cur.tv_sec, &loc_time);

	t->year   = loc_time.tm_year + 1900;
	t->month  = loc_time.tm_mon  + 1;
	t->day    = loc_time.tm_mday;
	t->hour   = loc_time.tm_hour;
	t->minute = loc_time.tm_min;
	t->second = loc_time.tm_sec;
}


/*########################################################################################################################*
*--------------------------------------------------------Stopwatch--------------------------------------------------------*
*#########################################################################################################################*/
#define NS_PER_SEC 1000000000ULL

cc_uint64 Stopwatch_Measure(void) {
	return clock();
}

cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return (end - beg) / 1000;
}

void Stopwatch_Init(void) {
    //TODO
	cc_uint64 doSomething = Stopwatch_Measure();
}



/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
void Directory_GetCachePath(cc_string* path) { }

cc_result Directory_Create(const cc_string* path) {
	return ERR_NOT_SUPPORTED;
}

int File_Exists(const cc_string* path) {
	return 0;
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	return ERR_NOT_SUPPORTED;
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	char str[NATIVE_STR_LEN];
	String_EncodeUtf8(str, path);

	*file = open(str, mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	return *file == -1 ? errno : 0;
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return File_Do(file, path, O_RDONLY);
}

cc_result File_Create(cc_file* file, const cc_string* path) {
	return File_Do(file, path, O_RDWR | O_CREAT | O_TRUNC);
}

cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return File_Do(file, path, O_RDWR | O_CREAT);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	*bytesRead = read(file, data, count);
	return *bytesRead == -1 ? errno : 0;
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	*bytesWrote = write(file, data, count);
	return *bytesWrote == -1 ? errno : 0;
}

cc_result File_Close(cc_file file) {
	return close(file) == -1 ? errno : 0;
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return lseek(file, offset, modes[seekType]) == -1 ? errno : 0;
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	*pos = lseek(file, 0, SEEK_CUR);
	return *pos == -1 ? errno : 0;
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	struct stat st;
	if (fstat(file, &st) == -1) { *len = -1; return errno; }
	*len = st.st_size; return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
void Thread_Sleep(cc_uint32 milliseconds) { 
	// TODO Delay API
}

void Thread_Run(void** handle, Thread_StartFunc func, int stackSize, const char* name) {
	*handle = NULL;
	// TODO
}

void Thread_Detach(void* handle) {
	// TODO
}

void Thread_Join(void* handle) {
	// TODO
}

void* Mutex_Create(void) {
	// TODO
	return 1;
}

void Mutex_Free(void* handle) {
	// TODO
}

void Mutex_Lock(void* handle) {
	// TODO
}

void Mutex_Unlock(void* handle) {
	// TODO
}

void* Waitable_Create(void) {
	return 1;
	// TODO
}

void Waitable_Free(void* handle) {
	// TODO
}

void Waitable_Signal(void* handle) {
	// TODO
}

void Waitable_Wait(void* handle) {
	// TODO
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	// TODO
}


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static void FontDirCallback(const cc_string* path, void* obj) {
	SysFonts_Register(path, NULL);
}

void Platform_LoadSysFonts(void) {
	int i;
	static const cc_string dirs[] = {
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};

	for (i = 0; i < Array_Elems(dirs); i++) {
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Socket_ParseAddress(const cc_string* address, int port, cc_sockaddr* addrs, int* numValidAddrs) {
	return ERR_NOT_SUPPORTED;
}

cc_result Socket_Connect(cc_socket* s, cc_sockaddr* addr, cc_bool nonblocking) {
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
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Process_OpenSupported = true;

static cc_result Process_RawStart(const char* path, char** argv) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

static cc_result Process_RawGetExePath(char* path, int* len);

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
	char raw[GAME_MAX_CMDARGS][NATIVE_STR_LEN];
	char path[NATIVE_STR_LEN];
	int i, j, len = 0;
	char* argv[15];

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';
	argv[0]   = path;

	for (i = 0, j = 1; i < numArgs; i++, j++) {
		String_EncodeUtf8(raw[i], &args[i]);
		argv[j] = raw[i];
	}

	argv[j] = NULL;
	return Process_RawStart(path, argv);
}
void Process_Exit(cc_result code) { exit(code); }

cc_result Process_StartOpen(const cc_string* args) {
	return ERR_NOT_SUPPORTED;
}

static cc_result Process_RawGetExePath(char* path, int* len) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Updater_Supported = false;
cc_bool Updater_Clean(void) { return true; }

const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };

cc_result Updater_Start(const char** action) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_MarkExecutable(void) {
	return ERR_NOT_SUPPORTED;
}

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
const cc_string DynamicLib_Ext = String_FromConst(".dylib");

void* DynamicLib_Load2(const cc_string* path) {
	return NULL;
}

void* DynamicLib_Get2(void* lib, const char* name) {
	return NULL;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	return false;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Free(void) { }

#ifdef CC_BUILD_IRIX
cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	const char* err = strerror(res);
	if (!err || res >= 1000) return false;

	String_AppendUtf8(dst, err, String_Length(err));
	return true;
}
#else
cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	char chars[NATIVE_STR_LEN];
	int len;

	/* For unrecognised error codes, strerror_r might return messages */
	/*  such as 'No error information', which is not very useful */
	/* (could check errno here but quicker just to skip entirely) */
	if (res >= 1000) return false;

	len = strerror_r(res, chars, NATIVE_STR_LEN);
	if (len == -1) return false;

	len = String_CalcLen(chars, NATIVE_STR_LEN);
	String_AppendUtf8(dst, chars, len);
	return true;
}
#endif

void Platform_Init(void) {
	printf("Macintosh ClassiCube has started to init.\n");	// Test, just to see if it's actually *running* at all.
	Platform_LoadSysFonts();
	Stopwatch_Init();
}

cc_result Platform_Encrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}

cc_result Platform_Decrypt(const void* data, int len, cc_string* dst) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i]);
	}
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return 0;
}
#endif
