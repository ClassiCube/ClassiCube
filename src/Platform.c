#include "Platform.h"
#include "String.h"
#include "Logger.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "Window.h"
#include "Utils.h"
#include "Errors.h"

#if defined CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>
#include <wincrypt.h>

#define Socket__Error() WSAGetLastError()
static HANDLE heap;
const cc_result ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const cc_result ReturnCode_FileNotFound     = ERROR_FILE_NOT_FOUND;
const cc_result ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = ERROR_ALREADY_EXISTS;
#elif defined CC_BUILD_POSIX
/* POSIX can be shared between Linux/BSD/macOS */
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <signal.h>
#include <stdio.h>

#define Socket__Error() errno
static char* defaultDirectory;
const cc_result ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const cc_result ReturnCode_FileNotFound     = ENOENT;
const cc_result ReturnCode_SocketInProgess  = EINPROGRESS;
const cc_result ReturnCode_SocketWouldBlock = EWOULDBLOCK;
const cc_result ReturnCode_DirectoryExists  = EEXIST;
#endif
/* Platform specific include files (Try to share for UNIX-ish) */
#if defined CC_BUILD_DARWIN
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#if defined CC_BUILD_MACOS
#include <ApplicationServices/ApplicationServices.h>
#endif
#elif defined CC_BUILD_SOLARIS
#include <sys/filio.h>
#elif defined CC_BUILD_BSD
#include <sys/sysctl.h>
#elif defined CC_BUILD_HAIKU
/* TODO: Use load_image/resume_thread instead of fork */
/* Otherwise opening browser never works because fork fails */
#include <kernel/image.h>
#elif defined CC_BUILD_WEB
#include <emscripten.h>
#include "Chat.h"
#endif


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void*  dst, cc_uint8 value,  cc_uint32 numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, const void* src, cc_uint32 numBytes) { memcpy(dst, src,   numBytes); }

int Mem_Equal(const void* a, const void* b, cc_uint32 numBytes) {
	const cc_uint8* src = (const cc_uint8*)a;
	const cc_uint8* dst = (const cc_uint8*)b;

	while (numBytes--) { 
		if (*src++ != *dst++) return false; 
	}
	return true;
}

CC_NOINLINE static void AbortOnAllocFailed(const char* place) {	
	cc_string log; char logBuffer[STRING_SIZE+20 + 1];
	String_InitArray_NT(log, logBuffer);

	String_Format1(&log, "Out of memory! (when allocating %c)", place);
	log.buffer[log.length] = '\0';
	Logger_Abort(log.buffer);
}

void* Mem_Alloc(cc_uint32 numElems, cc_uint32 elemsSize, const char* place) {
	void* ptr = Mem_TryAlloc(numElems, elemsSize);
	if (!ptr) AbortOnAllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(cc_uint32 numElems, cc_uint32 elemsSize, const char* place) {
	void* ptr = Mem_TryAllocCleared(numElems, elemsSize);
	if (!ptr) AbortOnAllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize, const char* place) {
	void* ptr = Mem_TryRealloc(mem, numElems, elemsSize);
	if (!ptr) AbortOnAllocFailed(place);
	return ptr;
}

static CC_NOINLINE cc_uint32 CalcMemSize(cc_uint32 numElems, cc_uint32 elemsSize) {
	if (!numElems) return 1; /* treat 0 size as 1 byte */
	cc_uint32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	if (numBytes < numElems) return 0; /* TODO: Use proper overflow checking */
	return numBytes;
}

#if defined CC_BUILD_WIN
void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? HeapAlloc(heap, 0, size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? HeapAlloc(heap, HEAP_ZERO_MEMORY, size) : NULL;
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? HeapReAlloc(heap, 0, mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) HeapFree(heap, 0, mem);
}
#elif defined CC_BUILD_POSIX
void* Mem_TryAlloc(cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? malloc(size) : NULL;
}

void* Mem_TryAllocCleared(cc_uint32 numElems, cc_uint32 elemsSize) {
	return calloc(numElems, elemsSize);
}

void* Mem_TryRealloc(void* mem, cc_uint32 numElems, cc_uint32 elemsSize) {
	cc_uint32 size = CalcMemSize(numElems, elemsSize);
	return size ? realloc(mem, size) : NULL;
}

void Mem_Free(void* mem) {
	if (mem) free(mem);
}
#endif


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log1(const char* format, const void* a1) {
	Platform_Log4(format, a1, NULL, NULL, NULL);
}
void Platform_Log2(const char* format, const void* a1, const void* a2) {
	Platform_Log4(format, a1, a2, NULL, NULL);
}
void Platform_Log3(const char* format, const void* a1, const void* a2, const void* a3) {
	Platform_Log4(format, a1, a2, a3, NULL);
}

void Platform_Log4(const char* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	cc_string msg; char msgBuffer[512];
	String_InitArray(msg, msgBuffer);

	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(msg.buffer, msg.length);
}

void Platform_LogConst(const char* message) {
	Platform_Log(message, String_Length(message));
}

/* TODO: check this is actually accurate */
static cc_uint64 sw_freqMul = 1, sw_freqDiv = 1;
cc_uint64 Stopwatch_ElapsedMicroseconds(cc_uint64 beg, cc_uint64 end) {
	if (end < beg) return 0;
	return ((end - beg) * sw_freqMul) / sw_freqDiv;
}

int Stopwatch_ElapsedMS(cc_uint64 beg, cc_uint64 end) {
	cc_uint64 raw = Stopwatch_ElapsedMicroseconds(beg, end);
	if (raw > Int32_MaxValue) return Int32_MaxValue / 1000;
	return (int)raw / 1000;
}

#if defined CC_BUILD_WIN
static HANDLE conHandle;
static BOOL hasDebugger;

void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	DWORD wrote;

	if (conHandle) {
		WriteFile(conHandle, msg,  len, &wrote, NULL);
		WriteFile(conHandle, "\n",   1, &wrote, NULL);
	}

	if (!hasDebugger) return;
	len = min(len, 2048);
	Mem_Copy(tmp, msg, len); tmp[len] = '\0';

	OutputDebugStringA(tmp);
	OutputDebugStringA("\n");
}

#define FILETIME_EPOCH 50491123200000ULL
#define FILETIME_UNIX_EPOCH 11644473600LL
#define FileTime_TotalMS(time)  ((time / 10000)    + FILETIME_EPOCH)
#define FileTime_UnixTime(time) ((time / 10000000) - FILETIME_UNIX_EPOCH)
TimeMS DateTime_CurrentUTC_MS(void) {
	FILETIME ft; 
	cc_uint64 raw;
	
	GetSystemTimeAsFileTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	raw = ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
	return FileTime_TotalMS(raw);
}

void DateTime_CurrentLocal(struct DateTime* t) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);

	t->year   = localTime.wYear;
	t->month  = localTime.wMonth;
	t->day    = localTime.wDay;
	t->hour   = localTime.wHour;
	t->minute = localTime.wMinute;
	t->second = localTime.wSecond;
}

static cc_bool sw_highRes;
cc_uint64 Stopwatch_Measure(void) {
	LARGE_INTEGER t;
	FILETIME ft;

	if (sw_highRes) {
		QueryPerformanceCounter(&t);
		return (cc_uint64)t.QuadPart;
	} else {		
		GetSystemTimeAsFileTime(&ft);
		return (cc_uint64)ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
	}
}
#elif defined CC_BUILD_POSIX
/* log to android logcat */
#ifdef CC_BUILD_ANDROID
#include <android/log.h>
void Platform_Log(const char* msg, int len) {
	char tmp[2048 + 1];
	len = min(len, 2048);

	Mem_Copy(tmp, msg, len); tmp[len] = '\0';
	__android_log_write(ANDROID_LOG_DEBUG, "ClassiCube", tmp);
}
#else
void Platform_Log(const char* msg, int len) {
	write(STDOUT_FILENO, msg,  len);
	write(STDOUT_FILENO, "\n",   1);
}
#endif

#define UnixTime_TotalMS(time) ((cc_uint64)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
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

#define NS_PER_SEC 1000000000ULL
#endif
/* clock_gettime is optional, see http://pubs.opengroup.org/onlinepubs/009696899/functions/clock_getres.html */
/* "... These functions are part of the Timers option and need not be available on all implementations..." */
#if defined CC_BUILD_WEB
cc_uint64 Stopwatch_Measure(void) {
	/* time is a milliseconds double */
	return (cc_uint64)(emscripten_get_now() * 1000);
}
#elif defined CC_BUILD_DARWIN
cc_uint64 Stopwatch_Measure(void) { return mach_absolute_time(); }
#elif defined CC_BUILD_SOLARIS
cc_uint64 Stopwatch_Measure(void) { return gethrtime(); }
#elif defined CC_BUILD_POSIX
cc_uint64 Stopwatch_Measure(void) {
	struct timespec t;
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (cc_uint64)t.tv_sec * NS_PER_SEC + t.tv_nsec;
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
cc_result Directory_Create(const cc_string* path) {
	WCHAR str[NATIVE_STR_LEN];
	cc_result res;

	Platform_EncodeUtf16(str, path);
	if (CreateDirectoryW(str, NULL)) return 0;
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	Platform_Utf16ToAnsi(str);
	return CreateDirectoryA((LPCSTR)str, NULL) ? 0 : GetLastError();
}

int File_Exists(const cc_string* path) {
	WCHAR str[NATIVE_STR_LEN];
	DWORD attribs;

	Platform_EncodeUtf16(str, path);
	attribs = GetFileAttributesW(str);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[MAX_PATH + 10];
	WCHAR str[NATIVE_STR_LEN];
	WCHAR* src;
	WIN32_FIND_DATAW entry;
	HANDLE find;
	cc_result res;	
	int i;

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_EncodeUtf16(str, &path);
	
	find = FindFirstFileW(str, &entry);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		src = entry.cFileName;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;
		
		for (i = 0; i < MAX_PATH && src[i]; i++) {
			/* TODO: UTF16 to codepoint conversion */
			String_Append(&path, Convert_CodepointToCP437(src[i]));
		}

		if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			res = Directory_Enum(&path, obj, callback);
			if (res) { FindClose(find); return res; }
		} else {
			callback(&path, obj);
		}
	}  while (FindNextFileW(find, &entry));

	res = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return res == ERROR_NO_MORE_FILES ? 0 : GetLastError();
}

static cc_result DoFile(cc_file* file, const cc_string* path, DWORD access, DWORD createMode) {
	WCHAR str[NATIVE_STR_LEN];
	cc_result res;
	Platform_EncodeUtf16(str, path);

	*file = CreateFileW(str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	if (*file && *file != INVALID_HANDLE_VALUE) return 0;
	if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	/* Windows 9x does not support W API functions */
	Platform_Utf16ToAnsi(str);
	*file = CreateFileA((LPCSTR)str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return *file != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

cc_result File_Open(cc_file* file, const cc_string* path) {
	return DoFile(file, path, GENERIC_READ, OPEN_EXISTING);
}
cc_result File_Create(cc_file* file, const cc_string* path) {
	return DoFile(file, path, GENERIC_WRITE | GENERIC_READ, CREATE_ALWAYS);
}
cc_result File_OpenOrCreate(cc_file* file, const cc_string* path) {
	return DoFile(file, path, GENERIC_WRITE | GENERIC_READ, OPEN_ALWAYS);
}

cc_result File_Read(cc_file file, void* data, cc_uint32 count, cc_uint32* bytesRead) {
	BOOL success = ReadFile(file, data, count, bytesRead, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Write(cc_file file, const void* data, cc_uint32 count, cc_uint32* bytesWrote) {
	BOOL success = WriteFile(file, data, count, bytesWrote, NULL);
	return success ? 0 : GetLastError();
}

cc_result File_Close(cc_file file) {
	return CloseHandle(file) ? 0 : GetLastError();
}

cc_result File_Seek(cc_file file, int offset, int seekType) {
	static cc_uint8 modes[3] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
	DWORD pos = SetFilePointer(file, offset, NULL, modes[seekType]);
	return pos != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
}

cc_result File_Position(cc_file file, cc_uint32* pos) {
	*pos = SetFilePointer(file, 0, NULL, FILE_CURRENT);
	return *pos != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
}

cc_result File_Length(cc_file file, cc_uint32* len) {
	*len = GetFileSize(file, NULL);
	return *len != INVALID_FILE_SIZE ? 0 : GetLastError();
}
#elif defined CC_BUILD_POSIX
cc_result Directory_Create(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

int File_Exists(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	struct stat sb;
	Platform_EncodeUtf8(str, path);
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

cc_result Directory_Enum(const cc_string* dirPath, void* obj, Directory_EnumCallback callback) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	char str[NATIVE_STR_LEN];
	DIR* dirPtr;
	struct dirent* entry;
	char* src;
	int len, res;

	Platform_EncodeUtf8(str, dirPath);
	dirPtr = opendir(str);
	if (!dirPtr) return errno;

	/* POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed." */
	/* errno is sometimes leftover from previous calls, so always reset it before readdir gets called */
	errno = 0;
	String_InitArray(path, pathBuffer);

	while ((entry = readdir(dirPtr))) {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		len = String_Length(src);
		String_AppendUtf8(&path, src, len);

		/* TODO: fallback to stat when this fails */
		if (entry->d_type == DT_DIR) {
			res = Directory_Enum(&path, obj, callback);
			if (res) { closedir(dirPtr); return res; }
		} else {
			callback(&path, obj);
		}
		errno = 0;
	}

	res = errno; /* return code from readdir */
	closedir(dirPtr);
	return res;
}

static cc_result File_Do(cc_file* file, const cc_string* path, int mode) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, path);
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
#ifndef CC_BUILD_WEB
	return close(file) == -1 ? errno : 0;
#else
	int ret = close(file) == -1 ? errno : 0;
	EM_ASM( FS.syncfs(false, function(err) { if (err) console.log(err); }); );
	return ret;
#endif
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
#endif


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
void Thread_Sleep(cc_uint32 milliseconds) { Sleep(milliseconds); }
static DWORD WINAPI ExecThread(void* param) {
	Thread_StartFunc func = (Thread_StartFunc)param;
	func();
	return 0;
}

void* Thread_Start(Thread_StartFunc func) {
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, ExecThread, (void*)func, 0, &threadID);
	if (!handle) {
		Logger_Abort2(GetLastError(), "Creating thread");
	}
	return handle;
}

void Thread_Detach(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		Logger_Abort2(GetLastError(), "Freeing thread handle");
	}
}

void Thread_Join(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
	Thread_Detach(handle);
}

void* Mutex_Create(void) {
	CRITICAL_SECTION* ptr = (CRITICAL_SECTION*)Mem_Alloc(1, sizeof(CRITICAL_SECTION), "mutex");
	InitializeCriticalSection(ptr);
	return ptr;
}

void Mutex_Free(void* handle)   { 
	DeleteCriticalSection((CRITICAL_SECTION*)handle); 
	Mem_Free(handle);
}
void Mutex_Lock(void* handle)   { EnterCriticalSection((CRITICAL_SECTION*)handle); }
void Mutex_Unlock(void* handle) { LeaveCriticalSection((CRITICAL_SECTION*)handle); }

void* Waitable_Create(void) {
	void* handle = CreateEventA(NULL, false, false, NULL);
	if (!handle) {
		Logger_Abort2(GetLastError(), "Creating waitable");
	}
	return handle;
}

void Waitable_Free(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		Logger_Abort2(GetLastError(), "Freeing waitable");
	}
}

void Waitable_Signal(void* handle) { SetEvent((HANDLE)handle); }
void Waitable_Wait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	WaitForSingleObject((HANDLE)handle, milliseconds);
}
#elif defined CC_BUILD_WEB
/* No real threading support with emscripten backend */
void Thread_Sleep(cc_uint32 milliseconds) { }
void* Thread_Start(Thread_StartFunc func) { func(); return NULL; }
void Thread_Detach(void* handle) { }
void Thread_Join(void* handle) { }

void* Mutex_Create(void) { return NULL; }
void Mutex_Free(void* handle) { }
void Mutex_Lock(void* handle) { }
void Mutex_Unlock(void* handle) { }

void* Waitable_Create(void) { return NULL; }
void Waitable_Free(void* handle) { }
void Waitable_Signal(void* handle) { }
void Waitable_Wait(void* handle) { }
void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) { }
#elif defined CC_BUILD_POSIX
void Thread_Sleep(cc_uint32 milliseconds) { usleep(milliseconds * 1000); }

#ifdef CC_BUILD_ANDROID
/* All threads using JNI must detach BEFORE they exit */
/* (see https://developer.android.com/training/articles/perf-jni */
static void* ExecThread(void* param) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);

	((Thread_StartFunc)param)();
	(*VM_Ptr)->DetachCurrentThread(VM_Ptr);
	return NULL;
}
#else
static void* ExecThread(void* param) {
	((Thread_StartFunc)param)(); 
	return NULL;
}
#endif

void* Thread_Start(Thread_StartFunc func) {
	pthread_t* ptr = (pthread_t*)Mem_Alloc(1, sizeof(pthread_t), "thread");
	int res = pthread_create(ptr, NULL, ExecThread, (void*)func);
	if (res) Logger_Abort2(res, "Creating thread");
	return ptr;
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_detach(*ptr);
	if (res) Logger_Abort2(res, "Detaching thread");
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	pthread_t* ptr = (pthread_t*)handle;
	int res = pthread_join(*ptr, NULL);
	if (res) Logger_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(void) {
	pthread_mutex_t* ptr = (pthread_mutex_t*)Mem_Alloc(1, sizeof(pthread_mutex_t), "mutex");
	int res = pthread_mutex_init(ptr, NULL);
	if (res) Logger_Abort2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = pthread_mutex_destroy((pthread_mutex_t*)handle);
	if (res) Logger_Abort2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = pthread_mutex_lock((pthread_mutex_t*)handle);
	if (res) Logger_Abort2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = pthread_mutex_unlock((pthread_mutex_t*)handle);
	if (res) Logger_Abort2(res, "Unlocking mutex");
}

struct WaitData {
	pthread_cond_t  cond;
	pthread_mutex_t mutex;
	int signalled;
};

void* Waitable_Create(void) {
	struct WaitData* ptr = (struct WaitData*)Mem_Alloc(1, sizeof(struct WaitData), "waitable");
	int res;
	
	res = pthread_cond_init(&ptr->cond, NULL);
	if (res) Logger_Abort2(res, "Creating waitable");
	res = pthread_mutex_init(&ptr->mutex, NULL);
	if (res) Logger_Abort2(res, "Creating waitable mutex");

	ptr->signalled = false;
	return ptr;
}

void Waitable_Free(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;
	
	res = pthread_cond_destroy(&ptr->cond);
	if (res) Logger_Abort2(res, "Destroying waitable");
	res = pthread_mutex_destroy(&ptr->mutex);
	if (res) Logger_Abort2(res, "Destroying waitable mutex");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	ptr->signalled = true;
	Mutex_Unlock(&ptr->mutex);

	res = pthread_cond_signal(&ptr->cond);
	if (res) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	struct WaitData* ptr = (struct WaitData*)handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = pthread_cond_wait(&ptr->cond, &ptr->mutex);
		if (res) Logger_Abort2(res, "Waitable wait");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
}

void Waitable_WaitFor(void* handle, cc_uint32 milliseconds) {
	struct WaitData* ptr = (struct WaitData*)handle;
	struct timeval tv;
	struct timespec ts;
	int res;
	gettimeofday(&tv, NULL);

	/* absolute time for some silly reason */
	ts.tv_sec  = tv.tv_sec + milliseconds / 1000;
	ts.tv_nsec = 1000 * (tv.tv_usec + 1000 * (milliseconds % 1000));

	/* statement above might exceed max nsec, so adjust for that */
	while (ts.tv_nsec >= NS_PER_SEC) {
		ts.tv_sec++;
		ts.tv_nsec -= NS_PER_SEC;
	}

	Mutex_Lock(&ptr->mutex);
	if (!ptr->signalled) {
		res = pthread_cond_timedwait(&ptr->cond, &ptr->mutex, &ts);
		if (res && res != ETIMEDOUT) Logger_Abort2(res, "Waitable wait for");
	}
	ptr->signalled = false;
	Mutex_Unlock(&ptr->mutex);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WEB
void Platform_LoadSysFonts(void) { }
#else
static void FontDirCallback(const cc_string* path, void* obj) {
	static const cc_string fonExt = String_FromConst(".fon");
	/* Completely skip windows .FON files */
	if (String_CaselessEnds(path, &fonExt)) return;
	SysFonts_Register(path);
}

void Platform_LoadSysFonts(void) { 
	int i;
#if defined CC_BUILD_WIN
	char winFolder[FILENAME_SIZE];
	WCHAR winTmp[FILENAME_SIZE];
	UINT winLen;
	/* System folder path may not be C:/Windows */
	cc_string dirs[1];
	String_InitArray(dirs[0], winFolder);

	winLen = GetWindowsDirectoryW(winTmp, FILENAME_SIZE);
	if (winLen) {
		String_AppendUtf16(&dirs[0], winTmp, winLen * 2);
	} else {
		String_AppendConst(&dirs[0], "C:/Windows");
	}
	String_AppendConst(&dirs[0], "/fonts");
	
#elif defined CC_BUILD_ANDROID
	static const cc_string dirs[3] = {
		String_FromConst("/system/fonts"),
		String_FromConst("/system/font"),
		String_FromConst("/data/fonts"),
	};
#elif defined CC_BUILD_NETBSD
	static const cc_string dirs[3] = {
		String_FromConst("/usr/X11R7/lib/X11/fonts"),
		String_FromConst("/usr/pkg/lib/X11/fonts"),
		String_FromConst("/usr/pkg/share/fonts")
	};
#elif defined CC_BUILD_HAIKU
	static const cc_string dirs[1] = {
		String_FromConst("/system/data/fonts")
	};
#elif defined CC_BUILD_DARWIN
	static const cc_string dirs[2] = {
		String_FromConst("/System/Library/Fonts"),
		String_FromConst("/Library/Fonts")
	};
#elif defined CC_BUILD_POSIX
	static const cc_string dirs[2] = {
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};
#endif
	for (i = 0; i < Array_Elems(dirs); i++) {
		Directory_Enum(&dirs[i], NULL, FontDirCallback);
	}
}
#endif


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Socket_Create(cc_socket* s) {
	*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return *s == -1 ? Socket__Error() : 0;
}

static cc_result Socket_ioctl(cc_socket s, cc_uint32 cmd, int* data) {
#if defined CC_BUILD_WIN
	return ioctlsocket(s, cmd, data);
#else
	return ioctl(s, cmd, data);
#endif
}

cc_result Socket_Available(cc_socket s, cc_uint32* available) {
	return Socket_ioctl(s, FIONREAD, available);
}
cc_result Socket_SetBlocking(cc_socket s, cc_bool blocking) {
#if defined CC_BUILD_WEB
	return ERR_NOT_SUPPORTED; /* sockets always async */
#else
	int blocking_raw = blocking ? 0 : -1;
	return Socket_ioctl(s, FIONBIO, &blocking_raw);
#endif
}

cc_result Socket_GetError(cc_socket s, cc_result* result) {
	socklen_t resultSize = sizeof(cc_result);
	return getsockopt(s, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

cc_result Socket_Connect(cc_socket s, const cc_string* ip, int port) {
	struct sockaddr addr;
	cc_result res;

	addr.sa_family = AF_INET;
	Stream_SetU16_BE( (cc_uint8*)&addr.sa_data[0], port);
	Utils_ParseIP(ip, (cc_uint8*)&addr.sa_data[2]);

	res = connect(s, &addr, sizeof(addr));
	return res == -1 ? Socket__Error() : 0;
}

cc_result Socket_Read(cc_socket s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
#ifdef CC_BUILD_WEB
	/* recv only reads one WebSocket frame at most, hence call it multiple times */
	int recvCount = 0, pending;
	*modified = 0;

	while (count && !Socket_Available(s, &pending) && pending) {
		recvCount = recv(s, data, count, 0);
		if (recvCount == -1) return Socket__Error();

		*modified += recvCount;
		data      += recvCount; count -= recvCount;
	}
	return 0;
#else
	int recvCount = recv(s, data, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return Socket__Error();
#endif
}

cc_result Socket_Write(cc_socket s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	int sentCount = send(s, data, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return Socket__Error();
}

cc_result Socket_Close(cc_socket s) {
	cc_result res = 0;
	cc_result res1, res2;

#if defined CC_BUILD_WEB
	res1 = 0;
#elif defined CC_BUILD_WIN
	res1 = shutdown(s, SD_BOTH);
#else
	res1 = shutdown(s, SHUT_RDWR);
#endif
	if (res1 == -1) res = Socket__Error();

#if defined CC_BUILD_WIN
	res2 = closesocket(s);
#else
	res2 = close(s);
#endif
	if (res2 == -1) res = Socket__Error();
	return res;
}

/* Alas, a simple cross-platform select() is not good enough */
#if defined CC_BUILD_WIN
cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	set.fd_count    = 1;
	set.fd_array[0] = s;

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return Socket__Error(); }

	*success = set.fd_count != 0; return 0;
}
#elif defined CC_BUILD_DARWIN
/* poll is broken on old OSX apparently https://daniel.haxx.se/docs/poll-vs-select.html */
cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	FD_ZERO(&set);
	FD_SET(s, &set);

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(s + 1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(s + 1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return Socket__Error(); }
	*success = FD_ISSET(s, &set) != 0; return 0;
}
#else
#include <poll.h>
cc_result Socket_Poll(cc_socket s, int mode, cc_bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = s;
	pfd.events = mode == SOCKET_POLL_READ ? POLLIN : POLLOUT;
	if (poll(&pfd, 1, 0) == -1) { *success = false; return Socket__Error(); }
	
	/* to match select, closed socket still counts as readable */
	flags    = mode == SOCKET_POLL_READ ? (POLLIN | POLLHUP) : POLLOUT;
	*success = (pfd.revents & flags) != 0;
	return 0;
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Process/Module------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
static cc_result Process_RawStart(WCHAR* path, WCHAR* args) {
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	cc_result res;
	si.cb = sizeof(STARTUPINFOW);

	if (CreateProcessW(path, args, NULL, NULL, 
			false, 0, NULL, NULL, &si, &pi)) goto success;
	//if ((res = GetLastError()) != ERROR_CALL_NOT_IMPLEMENTED) return res;

	//Platform_Utf16ToAnsi(path);
	//if (CreateProcessA((LPCSTR)path, args, NULL, NULL, 
	//		false, 0, NULL, NULL, &si, &pi)) goto success;
	return GetLastError();

success:
	/* Don't leak memory for process return code */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

static cc_result Process_RawGetExePath(WCHAR* path, int* len) {
	*len = GetModuleFileNameW(NULL, path, NATIVE_STR_LEN);
	return *len ? 0 : GetLastError();
}

cc_result Process_StartGame(const cc_string* args) {
	cc_string argv; char argvBuffer[NATIVE_STR_LEN];
	WCHAR raw[NATIVE_STR_LEN], path[NATIVE_STR_LEN + 1];
	int len;

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';

	String_InitArray(argv, argvBuffer);
	String_Format1(&argv, "ClassiCube.exe %s", args);
	Platform_EncodeUtf16(raw, &argv);
	return Process_RawStart(path, raw);
}
void Process_Exit(cc_result code) { ExitProcess(code); }

void Process_StartOpen(const cc_string* args) {
	WCHAR str[NATIVE_STR_LEN];
	Platform_EncodeUtf16(str, args);
	ShellExecuteW(NULL, NULL, str, NULL, NULL, SW_SHOWNORMAL);
}
#elif defined CC_BUILD_WEB
cc_result Process_StartGame(const cc_string* args) { return ERR_NOT_SUPPORTED; }
void Process_Exit(cc_result code) { exit(code); }

void Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, args);
	EM_ASM_({ window.open(UTF8ToString($0)); }, str);
}
#elif defined CC_BUILD_ANDROID
static char gameArgsBuffer[512];
static cc_string gameArgs = String_FromArray(gameArgsBuffer);

cc_result Process_StartGame(const cc_string* args) {
	String_Copy(&gameArgs, args);
	return 0; /* TODO: Is there a clean way of handling an error */
}
void Process_Exit(cc_result code) { exit(code); }

void Process_StartOpen(const cc_string* args) {
	JavaCall_String_Void("startOpen", args);
}
#elif defined CC_BUILD_POSIX
static cc_result Process_RawStart(const char* path, char** argv) {
	pid_t pid = fork();
	if (pid == -1) return errno;

	if (pid == 0) {
		/* Executed in child process */
		execvp(path, argv);
		_exit(127); /* "command not found" */
	} else {
		/* Executed in parent process */
		/* We do nothing here.. */
		return 0;
	}
}

static cc_result Process_RawGetExePath(char* path, int* len);

cc_result Process_StartGame(const cc_string* args) {
	char path[NATIVE_STR_LEN], raw[NATIVE_STR_LEN];
	int i, j, len = 0;
	char* argv[15];

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';

	Platform_EncodeUtf8(raw, args);
	argv[0] = path; argv[1] = raw;

	/* need to null-terminate multiple arguments */
	for (i = 0, j = 2; raw[i] && i < Array_Elems(raw); i++) {
		if (raw[i] != ' ') continue;

		/* null terminate previous argument */
		raw[i] = '\0';
		argv[j++] = &raw[i + 1];
	}

	if (defaultDirectory) { argv[j++] = defaultDirectory; }
	argv[j] = NULL;
	return Process_RawStart(path, argv);
}

void Process_Exit(cc_result code) { exit(code); }

/* Opening browser/starting shell is not really standardised */
#if defined CC_BUILD_MACOS
void Process_StartOpen(const cc_string* args) {
	UInt8 str[NATIVE_STR_LEN];
	CFURLRef urlCF;
	int len;
	
	len   = Platform_EncodeUtf8(str, args);
	urlCF = CFURLCreateWithBytes(kCFAllocatorDefault, str, len, kCFStringEncodingUTF8, NULL);
	LSOpenCFURLRef(urlCF, NULL);
	CFRelease(urlCF);
}
#elif defined CC_BUILD_HAIKU
void Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	char* cmd[3];
	Platform_EncodeUtf8(str, args);

	cmd[0] = "open"; cmd[1] = str; cmd[2] = NULL;
	Process_RawStart("open", cmd);
}
#else
void Process_StartOpen(const cc_string* args) {
	char str[NATIVE_STR_LEN];
	char* cmd[3];
	Platform_EncodeUtf8(str, args);

	/* TODO: Can xdg-open be used on original Solaris, or is it just an OpenIndiana thing */
	cmd[0] = "xdg-open"; cmd[1] = str; cmd[2] = NULL;
	Process_RawStart("xdg-open", cmd);
}
#endif

/* Retrieving exe path is completely OS dependant */
#if defined CC_BUILD_DARWIN
static cc_result Process_RawGetExePath(char* path, int* len) {
	Mem_Set(path, '\0', NATIVE_STR_LEN);
	cc_uint32 size = NATIVE_STR_LEN;
	if (_NSGetExecutablePath(path, &size)) return ERR_INVALID_ARGUMENT;

	/* despite what you'd assume, size is NOT changed to length of path */
	*len = String_CalcLen(path, NATIVE_STR_LEN);
	return 0;
}
#elif defined CC_BUILD_LINUX
static cc_result Process_RawGetExePath(char* path, int* len) {
	*len = readlink("/proc/self/exe", path, NATIVE_STR_LEN);
	return *len == -1 ? errno : 0;
}
#elif defined CC_BUILD_FREEBSD
static cc_result Process_RawGetExePath(char* path, int* len) {
	static int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
	size_t size       = NATIVE_STR_LEN;

	if (sysctl(mib, 4, path, &size, NULL, 0) == -1) return errno;
	*len = String_CalcLen(path, NATIVE_STR_LEN);
	return 0;
}
#elif defined CC_BUILD_OPENBSD
static cc_result Process_RawGetExePath(char* path, int* len) {
	static int mib[4] = { CTL_KERN, KERN_PROC_ARGS, 0, KERN_PROC_ARGV };
	char tmp[NATIVE_STR_LEN];
	size_t size;
	char* argv[100];
	char* str;

	/* NOTE: OpenBSD doesn't seem to let us get executable's location, so fallback to argv[0] */
	/* See OpenBSD sysctl manpage for why argv array is so large */
	/*... The buffer pointed to by oldp is filled with an array of char pointers followed by the strings themselves... */
	mib[2] = getpid();
	size   = 100 * sizeof(char*);
	if (sysctl(mib, 4, argv, &size, NULL, 0) == -1) return errno;

	str = argv[0];
	if (str[0] != '/') {
		/* relative path */
		if (!realpath(str, tmp)) return errno;
		str = tmp;
	}

	*len = String_CalcLen(str, NATIVE_STR_LEN);
	Mem_Copy(path, str, *len);
	return 0;
}
#elif defined CC_BUILD_NETBSD
static cc_result Process_RawGetExePath(char* path, int* len) {
	static int mib[4] = { CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME };
	size_t size       = NATIVE_STR_LEN;

	if (sysctl(mib, 4, path, &size, NULL, 0) == -1) return errno;
	*len = String_CalcLen(path, NATIVE_STR_LEN);
	return 0;
}
#elif defined CC_BUILD_SOLARIS
static cc_result Process_RawGetExePath(char* path, int* len) {
	*len = readlink("/proc/self/path/a.out", path, NATIVE_STR_LEN);
	return *len == -1 ? errno : 0;
}
#elif defined CC_BUILD_HAIKU
static cc_result Process_RawGetExePath(char* path, int* len) {
	image_info info;
	int32 cookie = 0;

	cc_result res = get_next_image_info(B_CURRENT_TEAM, &cookie, &info);
	if (res != B_OK) return res;

	*len = String_CalcLen(info.name, NATIVE_STR_LEN);
	Mem_Copy(path, info.name, *len);
	return 0;
}
#endif
#endif /* CC_BUILD_POSIX */


/*########################################################################################################################*
*--------------------------------------------------------Updater----------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
#define UPDATE_TMP TEXT("CC_prev.exe")
#define UPDATE_SRC TEXT(UPDATE_FILE)

#if _WIN64
const char* const Updater_D3D9 = "ClassiCube.64.exe";
const char* const Updater_OGL  = "ClassiCube.64-opengl.exe";
#else
const char* const Updater_D3D9 = "ClassiCube.exe";
const char* const Updater_OGL  = "ClassiCube.opengl.exe";
#endif

cc_bool Updater_Clean(void) {
	return DeleteFile(UPDATE_TMP) || GetLastError() == ERROR_FILE_NOT_FOUND;
}

cc_result Updater_Start(const char** action) {
	WCHAR path[NATIVE_STR_LEN + 1];
	WCHAR args[2] = { 'a', '\0' }; /* don't actually care about arguments */
	cc_result res;
	int len = 0;

	*action = "Getting executable path";
	if ((res = Process_RawGetExePath(path, &len))) return res;
	path[len] = '\0';

	*action = "Moving executable to CC_prev.exe";
	if (!MoveFileExW(path, UPDATE_TMP, MOVEFILE_REPLACE_EXISTING)) return GetLastError();
	*action = "Replacing executable";
	if (!MoveFileExW(UPDATE_SRC, path, MOVEFILE_REPLACE_EXISTING)) return GetLastError();

	*action = "Restarting game";
	return Process_RawStart(path, args);
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	WCHAR path[NATIVE_STR_LEN + 1];
	cc_file file;
	FILETIME ft;
	cc_uint64 raw;
	int len = 0;

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';

	file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE) return GetLastError();

	if (GetFileTime(file, NULL, NULL, &ft)) {
		raw        = ft.dwLowDateTime | ((cc_uint64)ft.dwHighDateTime << 32);
		*timestamp = FileTime_UnixTime(raw);
	} else {
		res = GetLastError();
	}

	File_Close(file);
	return res;
}

/* Don't need special execute permission on windows */
cc_result Updater_MarkExecutable(void) { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	static const cc_string path = String_FromConst(UPDATE_FILE);
	cc_file file;
	FILETIME ft;
	cc_uint64 raw;
	cc_result res = File_OpenOrCreate(&file, &path);
	if (res) return res;

	raw = 10000000 * (timestamp + FILETIME_UNIX_EPOCH);
	ft.dwLowDateTime  = (cc_uint32)raw;
	ft.dwHighDateTime = (cc_uint32)(raw >> 32);

	if (!SetFileTime(file, NULL, NULL, &ft)) res = GetLastError();
	File_Close(file);
	return res;
}

#elif defined CC_BUILD_WEB || defined CC_BUILD_ANDROID
const char* const Updater_D3D9 = NULL;
const char* const Updater_OGL  = NULL;

#if defined CC_BUILD_WEB
cc_result Updater_GetBuildTime(cc_uint64* t) { return ERR_NOT_SUPPORTED; }
#else
cc_result Updater_GetBuildTime(cc_uint64* t) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	
	/* https://developer.android.com/reference/java/io/File#lastModified() */
	/* lastModified is returned in milliseconds */
	*t = JavaCallLong(env, "getApkUpdateTime", "()J", NULL) / 1000;
	return 0;
}
#endif

cc_bool Updater_Clean(void)   { return true; }
cc_result Updater_Start(const char** action) { *action = "Updating game"; return ERR_NOT_SUPPORTED; }
cc_result Updater_MarkExecutable(void)         { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 t) { return ERR_NOT_SUPPORTED; }
#elif defined CC_BUILD_POSIX
cc_bool Updater_Clean(void) { return true; }

const char* const Updater_D3D9 = NULL;
#if defined CC_BUILD_LINUX
#if __x86_64__
const char* const Updater_OGL = "ClassiCube";
#elif __i386__
const char* const Updater_OGL = "ClassiCube.32";
#elif CC_BUILD_RPI
const char* const Updater_OGL = "ClassiCube.rpi";
#else
const char* const Updater_OGL = NULL;
#endif
#elif defined CC_BUILD_DARWIN
#if __x86_64__
const char* const Updater_OGL = "ClassiCube.64.osx";
#elif __i386__
const char* const Updater_OGL = "ClassiCube.osx";
#else
const char* const Updater_OGL = NULL;
#endif
#else
const char* const Updater_OGL = NULL;
#endif

cc_result Updater_Start(const char** action) {
	char path[NATIVE_STR_LEN + 1];
	char* argv[2];
	cc_result res;
	int len = 0;

	*action = "Getting executable path";
	if ((res = Process_RawGetExePath(path, &len))) return res;
	path[len] = '\0';
	
	/* Because the process is only referenced by inode, we can */
	/* just unlink current filename and rename updated file to it */
	*action = "Deleting executable";
	if (unlink(path) == -1) return errno;
	*action = "Replacing executable";
	if (rename(UPDATE_FILE, path) == -1) return errno;

	argv[0] = path; argv[1] = NULL;
	*action = "Restarting game";
	return Process_RawStart(path, argv);
}

cc_result Updater_GetBuildTime(cc_uint64* timestamp) {
	char path[NATIVE_STR_LEN + 1];
	struct stat sb;
	int len = 0;

	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;
	path[len] = '\0';

	if (stat(path, &sb) == -1) return errno;
	*timestamp = (cc_uint64)sb.st_mtime;
	return 0;
}

cc_result Updater_MarkExecutable(void) {
	struct stat st;
	if (stat(UPDATE_FILE, &st) == -1) return errno;

	st.st_mode |= S_IXUSR;
	return chmod(UPDATE_FILE, st.st_mode) == -1 ? errno : 0;
}

cc_result Updater_SetNewBuildTime(cc_uint64 timestamp) {
	struct utimbuf times = { 0 };
	times.modtime = timestamp;
	return utime(UPDATE_FILE, &times) == -1 ? errno : 0;
}
#endif


/*########################################################################################################################*
*-------------------------------------------------------Dynamic lib-------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
const cc_string DynamicLib_Ext = String_FromConst(".dll");

void* DynamicLib_Load2(const cc_string* path) {
	WCHAR str[NATIVE_STR_LEN];
	Platform_EncodeUtf16(str, path);
	return LoadLibraryW(str);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	return GetProcAddress((HMODULE)lib, name);
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	cc_result res = GetLastError();
	Platform_DescribeError(res, dst);
	String_Format1(dst, " (error %i)", &res);
	return true;
}
#elif defined CC_BUILD_WEB
void* DynamicLib_Load2(const cc_string* path)         { return NULL; }
void* DynamicLib_Get2(void* lib, const char* name) { return NULL; }
cc_bool DynamicLib_DescribeError(cc_string* dst)      { return false; }
#elif defined MAC_OS_X_VERSION_MIN_REQUIRED && (MAC_OS_X_VERSION_MIN_REQUIRED < 1040)
/* Really old mac OS versions don't have the dlopen/dlsym API */
const cc_string DynamicLib_Ext = String_FromConst(".dylib");

void* DynamicLib_Load2(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, path);
	return NSAddImage(str, NSADDIMAGE_OPTION_WITH_SEARCHING | 
							NSADDIMAGE_OPTION_RETURN_ON_ERROR);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	cc_string tmp; char tmpBuffer[128];
	NSSymbol sym;
	String_InitArray_NT(tmp, tmpBuffer);

	/* NS linker api rquires symbols to have a _ prefix */
	String_Append(&tmp, '_');
	String_AppendConst(&tmp, name);
	tmp.buffer[tmp.length] = '\0';

	sym = NSLookupSymbolInImage(lib, tmp.buffer, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW |
												NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
	return sym ? NSAddressOfSymbol(sym) : NULL;
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	NSLinkEditErrors err = 0;
	const char* name = "";
	const char* msg  = "";
	int errNum = 0;

	NSLinkEditError(&err, &errNum, &name, &msg);
	String_Format4(dst, "%c in %c (%i, sys %i)", msg, name, &err, &errNum);
	return true;
}
#elif defined CC_BUILD_POSIX
#include <dlfcn.h>
/* TODO: Should we use .bundle instead of .dylib? */

#ifdef CC_BUILD_DARWIN
const cc_string DynamicLib_Ext = String_FromConst(".dylib");
#else
const cc_string DynamicLib_Ext = String_FromConst(".so");
#endif

void* DynamicLib_Load2(const cc_string* path) {
	char str[NATIVE_STR_LEN];
	Platform_EncodeUtf8(str, path);
	return dlopen(str, RTLD_NOW);
}

void* DynamicLib_Get2(void* lib, const char* name) {
	return dlsym(lib, name);
}

cc_bool DynamicLib_DescribeError(cc_string* dst) {
	char* err = dlerror();
	if (err) String_AppendConst(dst, err);
	return err && err[0];
}
#endif

cc_result DynamicLib_Load(const cc_string* path, void** lib) {
	*lib = DynamicLib_Load2(path);
	return *lib == NULL;
}
cc_result DynamicLib_Get(void* lib, const char* name, void** symbol) {
	*symbol = DynamicLib_Get2(lib, name);
	return *symbol == NULL;
}


cc_bool DynamicLib_GetAll(void* lib, const struct DynamicLibSym* syms, int count) {
	int i, loaded = 0;
	void* addr;

	for (i = 0; i < count; i++) {
		addr = DynamicLib_Get2(lib, syms[i].name);
		if (addr) loaded++;
		*syms[i].symAddr = addr;
	}
	return loaded == count;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
int Platform_EncodeUtf16(void* data, const cc_string* src) {
	WCHAR* dst = (WCHAR*)data;
	int i;
	if (src->length > FILENAME_SIZE) Logger_Abort("String too long to expand");

	for (i = 0; i < src->length; i++) {
		*dst++ = Convert_CP437ToUnicode(src->buffer[i]);
	}
	*dst = '\0';
	return src->length * 2;
}

void Platform_Utf16ToAnsi(void* data) {
	WCHAR* src = (WCHAR*)data;
	char* dst  = (char*)data;

	while (*src) { *dst++ = *src++; }
	*dst = '\0';
}

static void Platform_InitStopwatch(void) {
	LARGE_INTEGER freq;
	sw_highRes = QueryPerformanceFrequency(&freq);

	if (sw_highRes) {
		sw_freqMul = 1000 * 1000;
		sw_freqDiv = freq.QuadPart;
	} else { sw_freqDiv = 10; }
}

typedef BOOL (WINAPI *FUNC_AttachConsole)(DWORD dwProcessId);
static void AttachParentConsole(void) {
	static const cc_string kernel32 = String_FromConst("KERNEL32.DLL");
	FUNC_AttachConsole attach;
	void* lib;

	/* NOTE: Need to dynamically load, not supported on Windows 2000 */
	if ((lib = DynamicLib_Load2(&kernel32))) {
		attach = (FUNC_AttachConsole)DynamicLib_Get2(lib, "AttachConsole");
		if (attach) attach((DWORD)-1); /* ATTACH_PARENT_PROCESS */
	}
}

void Platform_Init(void) {
	WSADATA wsaData;
	cc_result res;

	Platform_InitStopwatch();
	heap = GetProcessHeap();
	
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_SysWarn(res, "starting WSA");

	hasDebugger = IsDebuggerPresent();
	/* For when user runs from command prompt */
	AttachParentConsole();

	conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (conHandle == INVALID_HANDLE_VALUE) conHandle = NULL;
}

void Platform_Free(void) {
	WSACleanup();
	HeapDestroy(heap);
}

cc_bool Platform_DescribeErrorExt(cc_result res, cc_string* dst, void* lib) {
	WCHAR chars[NATIVE_STR_LEN];
	DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	if (lib) flags |= FORMAT_MESSAGE_FROM_HMODULE;

	res = FormatMessageW(flags, lib, res, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						 chars, NATIVE_STR_LEN, NULL);
	if (!res) return false;

	String_AppendUtf16(dst, chars, res * 2);
	return true;
}

cc_bool Platform_DescribeError(cc_result res, cc_string* dst) {
	return Platform_DescribeErrorExt(res, dst, NULL);
}
#elif defined CC_BUILD_POSIX
int Platform_EncodeUtf8(void* data, const cc_string* src) {
	cc_uint8* dst = (cc_uint8*)data;
	cc_uint8* cur;
	int i, len = 0;
	if (src->length > FILENAME_SIZE) Logger_Abort("String too long to expand");

	for (i = 0; i < src->length; i++) {
		cur = dst + len;
		len += Convert_CP437ToUtf8(src->buffer[i], cur);
	}
	dst[len] = '\0';
	return len;
}

static void Platform_InitPosix(void) {
	signal(SIGCHLD, SIG_IGN);
	/* So writing to closed socket doesn't raise SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	/* Assume stopwatch is in nanoseconds */
	/* Some platforms (e.g. macOS) override this */
	sw_freqDiv = 1000;
}
void Platform_Free(void) { }

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

#if defined CC_BUILD_DARWIN
static void Platform_InitStopwatch(void) {
	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);

	sw_freqMul = tb.numer;
	/* tb.denom may be large, so multiplying by 1000 overflows 32 bits */
	/* (one powerpc system had tb.denom of 33329426) */
	sw_freqDiv = (cc_uint64)tb.denom * 1000;
}

#if defined CC_BUILD_MACOS
static void Platform_InitSpecific(void) {
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	/* NOTE: Call as soon as possible, otherwise can't click on dialog boxes or create windows */
	/* NOTE: TransformProcessType is macOS 10.3 or later */
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}
#else
/* Always foreground process on iOS */
static void Platform_InitSpecific(void) { }
#endif

void Platform_Init(void) {
	Platform_InitPosix();
	Platform_InitStopwatch();
	Platform_InitSpecific();
}
#elif defined CC_BUILD_WEB
void Platform_Init(void) {
	char tmp[64+1] = { 0 };
	EM_ASM( Module['websocket']['subprotocol'] = 'ClassiCube'; );
	/* Check if an error occurred when pre-loading IndexedDB */
	EM_ASM_({ if (window.cc_idbErr) stringToUTF8(window.cc_idbErr, $0, 64); }, tmp);

	EM_ASM({
		Module.saveBlob = function(blob, name) {
			if (window.navigator.msSaveBlob) {
				window.navigator.msSaveBlob(blob, name); return;
			}
			var url  = window.URL.createObjectURL(blob);
			var elem = document.createElement('a');

			elem.href     = url;
			elem.download = name;
			elem.style.display = 'none';

			document.body.appendChild(elem);
			elem.click();
			document.body.removeChild(elem);
			window.URL.revokeObjectURL(url);
		}
	});

	if (!tmp[0]) return;
	Chat_Add1("&cError preloading IndexedDB: %c", tmp);
	Chat_AddRaw("&cPreviously saved settings/maps will be lost");

	/* NOTE: You must pre-load IndexedDB before main() */
	/* (because pre-loading only works asynchronously) */
	/* If you don't, you'll get errors later trying to sync local to remote */
	/* See doc/hosting-webclient.md for example preloading IndexedDB code */
}
#else
void Platform_Init(void) { Platform_InitPosix(); }
#endif
#endif /* CC_BUILD_POSIX */


/*########################################################################################################################*
*-------------------------------------------------------Encryption--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
cc_result Platform_Encrypt(const cc_string* key, const void* data, int len, cc_string* dst) {
	DATA_BLOB input, output;
	int i;
	input.cbData = len; input.pbData = (BYTE*)data;
	if (!CryptProtectData(&input, NULL, NULL, NULL, NULL, 0, &output)) return GetLastError();

	for (i = 0; i < output.cbData; i++) {
		String_Append(dst, output.pbData[i]);
	}
	LocalFree(output.pbData);
	return 0;
}
cc_result Platform_Decrypt(const cc_string* key, const void* data, int len, cc_string* dst) {
	DATA_BLOB input, output;
	int i;
	input.cbData = len; input.pbData = (BYTE*)data;
	if (!CryptUnprotectData(&input, NULL, NULL, NULL, NULL, 0, &output)) return GetLastError();

	for (i = 0; i < output.cbData; i++) {
		String_Append(dst, output.pbData[i]);
	}
	LocalFree(output.pbData);
	return 0;
}
#elif defined CC_BUILD_LINUX || defined CC_BUILD_MACOS
/* Encrypts data using XTEA block cipher, with OS specific method to get machine-specific key */

static void EncipherBlock(cc_uint32* v, const cc_uint32* key, cc_string* dst) {
	cc_uint32 v0 = v[0], v1 = v[1], sum = 0, delta = 0x9E3779B9;
	int i;

    for (i = 0; i < 12; i++) {
        v0  += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1  += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0] = v0; v[1] = v1;
	String_AppendAll(dst, v, 8);
}

static void DecipherBlock(cc_uint32* v, const cc_uint32* key) {
	cc_uint32 v0 = v[0], v1 = v[1], delta = 0x9E3779B9, sum = delta * 12;
	int i;

    for (i = 0; i < 12; i++) {
        v1  -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
        sum -= delta;
        v0  -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
    }
    v[0] = v0; v[1] = v1;
}

#define ENC1 0xCC005EC0
#define ENC2 0x0DA4A0DE 
#define ENC3 0xC0DED000
#define MACHINEID_LEN 32
#define ENC_SIZE 8 /* 2 32 bit ints per block */

/* "b3c5a0d9" --> 0xB3C5A0D9 */
static void DecodeMachineID(char* tmp, cc_uint8* key) {
	int hex[MACHINEID_LEN], i;
	PackedCol_Unhex(tmp, hex, MACHINEID_LEN);

	for (i = 0; i < MACHINEID_LEN / 2; i++) {
		key[i] = (hex[i * 2] << 4) | hex[i * 2 + 1];
	}
}

#if defined CC_BUILD_LINUX
/* Read /var/lib/dbus/machine-id for the key */
static void GetMachineID(cc_uint32* key) {
	const cc_string idFile = String_FromConst("/var/lib/dbus/machine-id");
	char tmp[MACHINEID_LEN];
	struct Stream s;
	int i;

	for (i = 0; i < 4; i++) key[i] = 0;
	if (Stream_OpenFile(&s, &idFile)) return;

	if (!Stream_Read(&s, tmp, MACHINEID_LEN)) {
		DecodeMachineID(tmp, (cc_uint8*)key);
	}
	s.Close(&s);
}
#elif defined CC_BUILD_MACOS
static void GetMachineID(cc_uint32* key) {
	io_registry_entry_t registry;
	CFStringRef uuid;
	char tmp[MACHINEID_LEN] = { 0 };
	const char* src;
	struct Stream s;
	int i;

	for (i = 0; i < 4; i++) key[i] = 0;

	registry = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
	if (!registry) return;

	uuid = IORegistryEntryCreateCFProperty(registry, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
	if (uuid && (src = CFStringGetCStringPtr(uuid, kCFStringEncodingUTF8))) {
		for (i = 0; *src && i < MACHINEID_LEN; src++) {
			if (*src == '-') continue;
			tmp[i++] = *src;
		}
		DecodeMachineID(tmp, (cc_uint8*)key);	
	}
	CFRelease(uuid);
	IOObjectRelease(registry);
}
#endif

cc_result Platform_Encrypt(const cc_string* key_, const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4];
	header[0] = ENC1; header[1] = ENC2;
	header[2] = ENC3; header[3] = len;

	GetMachineID(key);
	EncipherBlock(header + 0, key, dst);
	EncipherBlock(header + 2, key, dst);

	for (; len > 0; len -= ENC_SIZE, src += ENC_SIZE) {
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));
		EncipherBlock(header, key, dst);
	}
	return 0;
}
cc_result Platform_Decrypt(const cc_string* key__, const void* data, int len, cc_string* dst) {
	const cc_uint8* src = (const cc_uint8*)data;
	cc_uint32 header[4], key[4];
	int dataLen;
	/* Total size must be >= header size */
	if (len < 16) return ERR_END_OF_STREAM;

	GetMachineID(key);
	Mem_Copy(header, src, 16);
	DecipherBlock(header + 0, key);
	DecipherBlock(header + 2, key);

	if (header[0] != ENC1 || header[1] != ENC2 || header[2] != ENC3) return ERR_INVALID_ARGUMENT;
	len -= 16; src += 16;

	if (header[3] > len) return ERR_INVALID_ARGUMENT;
	dataLen = header[3];

	for (; dataLen > 0; len -= ENC_SIZE, src += ENC_SIZE, dataLen -= ENC_SIZE) {
		header[0] = 0; header[1] = 0;
		Mem_Copy(header, src, min(len, ENC_SIZE));

		DecipherBlock(header, key);
		String_AppendAll(dst, header, min(dataLen, ENC_SIZE));
	}
	return 0;
}
#elif defined CC_BUILD_POSIX
cc_result Platform_Encrypt(const cc_string* key, const void* data, int len, cc_string* dst) {
	/* TODO: Is there a similar API for macOS/Linux? */
	/* Fallback to NOT SECURE XOR. Prevents simple reading from options.txt */
	const cc_uint8* src = data;
	cc_uint8 c;
	int i;

	for (i = 0; i < len; i++) {
		c = (cc_uint8)(src[i] ^ key->buffer[i % key->length] ^ 0x43);
		String_Append(dst, c);
	}
	return 0;
}
cc_result Platform_Decrypt(const cc_string* key, const void* data, int len, cc_string* dst) {
	/* TODO: Is there a similar API for macOS/Linux? */
	return Platform_Encrypt(key, data, len, dst);
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Configuration-------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
static cc_string Platform_NextArg(STRING_REF cc_string* args) {
	cc_string arg;
	int end;

	/* get rid of leading spaces before arg */
	while (args->length && args->buffer[0] == ' ') {
		*args = String_UNSAFE_SubstringAt(args, 1);
	}

	if (args->length && args->buffer[0] == '"') {
		/* "xy za" is used for arg with spaces */
		*args = String_UNSAFE_SubstringAt(args, 1);
		end = String_IndexOf(args, '"');
	} else {
		end = String_IndexOf(args, ' ');
	}

	if (end == -1) {
		arg   = *args;
		args->length = 0;
	} else {
		arg   = String_UNSAFE_Substring(args, 0, end);
		*args = String_UNSAFE_SubstringAt(args, end + 1);
	}
	return arg;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	cc_string cmdArgs = String_FromReadonly(GetCommandLineA());
	int i;
	Platform_NextArg(&cmdArgs); /* skip exe path */

	for (i = 0; i < GAME_MAX_CMDARGS; i++) {
		args[i] = Platform_NextArg(&cmdArgs);

		if (!args[i].length) break;
	}
	return i;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	WCHAR path[NATIVE_STR_LEN + 1];
	int i, len;
	cc_result res = Process_RawGetExePath(path, &len);
	if (res) return res;

	/* Get rid of filename at end of directory */
	for (i = len - 1; i >= 0; i--, len--) {
		if (path[i] == '/' || path[i] == '\\') break;
	}

	path[len] = '\0';
	return SetCurrentDirectoryW(path) ? 0 : GetLastError();
}
#elif defined CC_BUILD_WEB
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) { args[i] = String_FromReadonly(argv[i]); }
	return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	return chdir("/classicube") == -1 ? errno : 0;
}
#elif defined CC_BUILD_ANDROID
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	if (!gameArgs.length) return 0;
	return String_UNSAFE_Split(&gameArgs, ' ', args, GAME_MAX_CMDARGS);
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	cc_string dir; char dirBuffer[FILENAME_SIZE + 1];
	String_InitArray_NT(dir, dirBuffer);

	JavaCall_Void_String("getExternalAppDir", &dir);
	dir.buffer[dir.length] = '\0';
	Platform_Log1("EXTERNAL DIR: %s|", &dir);
	return chdir(dir.buffer) == -1 ? errno : 0;
}
#elif defined CC_BUILD_POSIX
int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
	int i, count;
	argc--; argv++; /* skip executable path argument */

#ifdef CC_BUILD_DARWIN
	if (argc) {
		static const cc_string psn = String_FromConst("-psn_0_");
		cc_string arg0 = String_FromReadonly(argv[0]);
		if (String_CaselessStarts(&arg0, &psn)) { argc--; argv++; }
	}
#endif

	count = min(argc, GAME_MAX_CMDARGS);
	for (i = 0; i < count; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'd' && argv[i][2]) {
			--count;
			continue;
		}
		args[i] = String_FromReadonly(argv[i]);
	}
	return count;
}


cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
	char path[NATIVE_STR_LEN];
	int i, len = 0;
	cc_result res;

	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] == 'd' && argv[i][2]) {
			defaultDirectory = argv[i];
			break;
		}
	}

	if (defaultDirectory) {
		return chdir(defaultDirectory + 2) == -1 ? errno : 0;
	}

	res = Process_RawGetExePath(path, &len);
	if (res) return res;

	/* get rid of filename at end of directory */
	for (i = len - 1; i >= 0; i--, len--) {
		if (path[i] == '/') break;
	}

#ifdef CC_BUILD_DARWIN
	static const cc_string bundle = String_FromConst(".app/Contents/MacOS/");
	cc_string raw = String_Init(path, len, 0);

	if (String_CaselessEnds(&raw, &bundle)) {
		len -= bundle.length;

		for (i = len - 1; i >= 0; i--, len--) {
			if (path[i] == '/') break;
		}
	}
#endif

	path[len] = '\0';
	return chdir(path) == -1 ? errno : 0;
}
#endif

/* Android java interop stuff */
#if defined CC_BUILD_ANDROID
jclass  App_Class;
jobject App_Instance;
JavaVM* VM_Ptr;

/* JNI helpers */
cc_string JavaGetString(JNIEnv* env, jstring str, char* buffer) {
	const char* src; int len;
	cc_string dst;
	src = (*env)->GetStringUTFChars(env, str, NULL);
	len = (*env)->GetStringUTFLength(env, str);

	dst.buffer   = buffer;
	dst.length   = 0;
	dst.capacity = NATIVE_STR_LEN;
	String_AppendUtf8(&dst, src, len);

	(*env)->ReleaseStringUTFChars(env, str, src);
	return dst;
}

jobject JavaMakeString(JNIEnv* env, const cc_string* str) {
	cc_uint8 tmp[2048 + 4];
	cc_uint8* cur;
	int i, len = 0;

	for (i = 0; i < str->length && len < 2048; i++) {
		cur = tmp + len;
		len += Convert_CP437ToUtf8(str->buffer[i], cur);
	}
	tmp[len] = '\0';
	return (*env)->NewStringUTF(env, (const char*)tmp);
}

jbyteArray JavaMakeBytes(JNIEnv* env, const cc_uint8* src, cc_uint32 len) {
	if (!len) return NULL;
	jbyteArray arr = (*env)->NewByteArray(env, len);
	(*env)->SetByteArrayRegion(env, arr, 0, len, src);
	return arr;
}

void JavaCallVoid(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	(*env)->CallVoidMethodA(env, App_Instance, method, args);
}

jint JavaCallInt(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	return (*env)->CallIntMethodA(env, App_Instance, method, args);
}

jlong JavaCallLong(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	return (*env)->CallLongMethodA(env, App_Instance, method, args);
}

jfloat JavaCallFloat(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	return (*env)->CallFloatMethodA(env, App_Instance, method, args);
}

jobject JavaCallObject(JNIEnv* env, const char* name, const char* sig, jvalue* args) {
	jmethodID method = (*env)->GetMethodID(env, App_Class, name, sig);
	return (*env)->CallObjectMethodA(env, App_Instance, method, args);
}

void JavaCall_String_Void(const char* name, const cc_string* value) {
	JNIEnv* env;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, value);
	JavaCallVoid(env, name, "(Ljava/lang/String;)V", args);
	(*env)->DeleteLocalRef(env, args[0].l);
}

static void ReturnString(JNIEnv* env, jobject obj, cc_string* dst) {
	const jchar* src;
	jsize len;
	if (!obj) return;

	src = (*env)->GetStringChars(env,  obj, NULL);
	len = (*env)->GetStringLength(env, obj);
	String_AppendUtf16(dst, src, len * 2);
	(*env)->ReleaseStringChars(env, obj, src);
	(*env)->DeleteLocalRef(env,     obj);
}

void JavaCall_Void_String(const char* name, cc_string* dst) {
	JNIEnv* env;
	jobject obj;
	JavaGetCurrentEnv(env);

	obj = JavaCallObject(env, name, "()Ljava/lang/String;", NULL);
	ReturnString(env, obj, dst);
}

void JavaCall_String_String(const char* name, const cc_string* arg, cc_string* dst) {
	JNIEnv* env;
	jobject obj;
	jvalue args[1];
	JavaGetCurrentEnv(env);

	args[0].l = JavaMakeString(env, arg);
	obj       = JavaCallObject(env, name, "(Ljava/lang/String;)Ljava/lang/String;", args);
	ReturnString(env, obj, dst);
	(*env)->DeleteLocalRef(env, args[0].l);
}
#endif
