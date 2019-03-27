#include "Platform.h"
#include "Logger.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Window.h"
#include "Utils.h"

#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmodapi.h"
#include "freetype/ftglyph.h"

#if defined CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#ifdef UNICODE
#define Platform_DecodeString(dst, src, len) Convert_DecodeUtf16(dst, src, (len) * 2)
#else
#define Platform_DecodeString(dst, src, len) Convert_DecodeAscii(dst, src, len)
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <wincrypt.h>

#define Socket__Error() WSAGetLastError()
static HANDLE heap;
const char* Platform_NewLine = "\r\n";

const ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const ReturnCode ReturnCode_FileNotFound = ERROR_FILE_NOT_FOUND;
const ReturnCode ReturnCode_NotSupported = ERROR_NOT_SUPPORTED;
const ReturnCode ReturnCode_InvalidArg   = ERROR_INVALID_PARAMETER;
const ReturnCode ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const ReturnCode ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
#elif defined CC_BUILD_POSIX
/* POSIX can be shared between Linux/BSD/OSX */
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <poll.h>
#include <signal.h>

#define Socket__Error() errno
const char* Platform_NewLine = "\n";

const ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const ReturnCode ReturnCode_FileNotFound = ENOENT;
const ReturnCode ReturnCode_NotSupported = EPERM;
const ReturnCode ReturnCode_InvalidArg   = EINVAL;
const ReturnCode ReturnCode_SocketInProgess  = EINPROGRESS;
const ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;
#endif
/* Platform specific include files (Try to share for UNIX-ish) */
#if defined CC_BUILD_LINUX
#define CC_BUILD_UNIX
#elif defined CC_BUILD_FREEBSD
#define CC_BUILD_UNIX
#include <sys/sysctl.h>
#elif defined CC_BUILD_OPENBSD
#define CC_BUILD_UNIX
#include <sys/sysctl.h>
#elif defined CC_BUILD_NETBSD
#define CC_BUILD_UNIX
#include <sys/sysctl.h>
#elif defined CC_BUILD_SOLARIS
#define CC_BUILD_UNIX
#include <sys/filio.h>
#elif defined CC_BUILD_OSX
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#elif defined CC_BUILD_WEB
#include <emscripten.h>
#endif


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void* dst, uint8_t value,    uint32_t numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, const void* src, uint32_t numBytes) { memcpy(dst, src,   numBytes); }

CC_NOINLINE static void Platform_AllocFailed(const char* place) {	
	String log; char logBuffer[STRING_SIZE+20 + 1];
	String_InitArray_NT(log, logBuffer);

	String_Format1(&log, "Out of memory! (when allocating %c)", place);
	log.buffer[log.length] = '\0';
	Logger_Abort(log.buffer);
}

#if defined CC_BUILD_WIN
void* Mem_Alloc(uint32_t numElems, uint32_t elemsSize, const char* place) {
	uint32_t numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, 0, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(uint32_t numElems, uint32_t elemsSize, const char* place) {
	uint32_t numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, HEAP_ZERO_MEMORY, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, uint32_t numElems, uint32_t elemsSize, const char* place) {
	uint32_t numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapReAlloc(heap, 0, mem, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void Mem_Free(void* mem) {
	if (mem) HeapFree(heap, 0, mem);
}
#elif defined CC_BUILD_POSIX
void* Mem_Alloc(uint32_t numElems, uint32_t elemsSize, const char* place) {
	void* ptr = malloc(numElems * elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(uint32_t numElems, uint32_t elemsSize, const char* place) {
	void* ptr = calloc(numElems, elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, uint32_t numElems, uint32_t elemsSize, const char* place) {
	void* ptr = realloc(mem, numElems * elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
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
	String msg; char msgBuffer[512];
	String_InitArray(msg, msgBuffer);

	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(&msg);
}

void Platform_LogConst(const char* message) {
	String msg = String_FromReadonly(message);
	Platform_Log(&msg);
}

/* TODO: check this is actually accurate */
static uint64_t sw_freqMul = 1, sw_freqDiv = 1;
uint64_t Stopwatch_ElapsedMicroseconds(uint64_t beg, uint64_t end) {
	if (end < beg) return 0;
	return ((end - beg) * sw_freqMul) / sw_freqDiv;
}

#if defined CC_BUILD_WIN
static HANDLE conHandle;
static BOOL hasDebugger;

void Platform_Log(const String* message) {
	DWORD wrote;
	if (conHandle) {
		WriteFile(conHandle, message->buffer, message->length, &wrote, NULL);
		WriteFile(conHandle, "\n",            1,               &wrote, NULL);
	}
	if (hasDebugger) {
		/* TODO: This reads past the end of the buffer */
		OutputDebugStringA(message->buffer);
		OutputDebugStringA("\n");
	}
}

#define FILETIME_EPOCH 50491123200000ULL
#define FileTime_TotalMS(time) ((time / 10000) + FILETIME_EPOCH)
TimeMS DateTime_CurrentUTC_MS(void) {
	FILETIME ft; GetSystemTimeAsFileTime(&ft); 
	/* in 100 nanosecond units, since Jan 1 1601 */
	uint64_t raw = ft.dwLowDateTime | ((uint64_t)ft.dwHighDateTime << 32);
	return FileTime_TotalMS(raw);
}

static void Platform_FromSysTime(struct DateTime* time, SYSTEMTIME* sysTime) {
	time->Year   = sysTime->wYear;
	time->Month  = sysTime->wMonth;
	time->Day    = sysTime->wDay;
	time->Hour   = sysTime->wHour;
	time->Minute = sysTime->wMinute;
	time->Second = sysTime->wSecond;
	time->Milli  = sysTime->wMilliseconds;
}

void DateTime_CurrentUTC(struct DateTime* time) {
	SYSTEMTIME utcTime;
	GetSystemTime(&utcTime);
	Platform_FromSysTime(time, &utcTime);
}

void DateTime_CurrentLocal(struct DateTime* time) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	Platform_FromSysTime(time, &localTime);
}

static bool sw_highRes;
uint64_t Stopwatch_Measure(void) {
	LARGE_INTEGER t;
	FILETIME ft;

	if (sw_highRes) {		
		QueryPerformanceCounter(&t);
		return (uint64_t)t.QuadPart;
	} else {		
		GetSystemTimeAsFileTime(&ft);
		return (uint64_t)ft.dwLowDateTime | ((uint64_t)ft.dwHighDateTime << 32);
	}
}
#elif defined CC_BUILD_POSIX
void Platform_Log(const String* message) {
	write(STDOUT_FILENO, message->buffer, message->length);
	write(STDOUT_FILENO, "\n",            1);
}

#define UnixTime_TotalMS(time) ((uint64_t)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

static void Platform_FromSysTime(struct DateTime* time, struct tm* sysTime) {
	time->Year   = sysTime->tm_year + 1900;
	time->Month  = sysTime->tm_mon + 1;
	time->Day    = sysTime->tm_mday;
	time->Hour   = sysTime->tm_hour;
	time->Minute = sysTime->tm_min;
	time->Second = sysTime->tm_sec;
}

void DateTime_CurrentUTC(struct DateTime* time_) {
	struct timeval cur; 
	struct tm utc_time;

	gettimeofday(&cur, NULL);
	gmtime_r(&cur.tv_sec, &utc_time);

	Platform_FromSysTime(time_, &utc_time);
	time_->Milli = cur.tv_usec / 1000;
}

void DateTime_CurrentLocal(struct DateTime* time_) {
	struct timeval cur; 
	struct tm loc_time;

	gettimeofday(&cur, NULL);
	localtime_r(&cur.tv_sec, &loc_time);

	Platform_FromSysTime(time_, &loc_time);
	time_->Milli = cur.tv_usec / 1000;
}

#define NS_PER_SEC 1000000000ULL
#endif
/* clock_gettime is optional, see http://pubs.opengroup.org/onlinepubs/009696899/functions/clock_getres.html */
/* "... These functions are part of the Timers option and need not be available on all implementations..." */
#if defined CC_BUILD_WEB
uint64_t Stopwatch_Measure(void) {
	/* time is a milliseconds double */
	return (uint64_t)(emscripten_get_now() * 1000);
}
#elif defined CC_BUILD_OSX
uint64_t Stopwatch_Measure(void) { return mach_absolute_time(); }
#elif defined CC_BUILD_SOLARIS
uint64_t Stopwatch_Measure(void) { return gethrtime(); }
#elif defined CC_BUILD_UNIX
uint64_t Stopwatch_Measure(void) {
	struct timespec t;
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (uint64_t)t.tv_sec * NS_PER_SEC + t.tv_nsec;
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
bool Directory_Exists(const String* path) {
	TCHAR str[300];
	DWORD attribs;

	Platform_ConvertString(str, path);
	attribs = GetFileAttributes(str);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Create(const String* path) {
	TCHAR str[300];
	BOOL success;

	Platform_ConvertString(str, path);
	success = CreateDirectory(str, NULL);
	return success ? 0 : GetLastError();
}

bool File_Exists(const String* path) {
	TCHAR str[300];
	DWORD attribs;

	Platform_ConvertString(str, path);
	attribs = GetFileAttributes(str);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Enum(const String* dirPath, void* obj, Directory_EnumCallback callback) {
	String path; char pathBuffer[MAX_PATH + 10];
	TCHAR str[300];
	WIN32_FIND_DATA entry;
	HANDLE find;
	ReturnCode res;	

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_ConvertString(str, &path);
	
	find = FindFirstFile(str, &entry);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		path.length = 0;
		String_Format1(&path, "%s/", dirPath);

		/* ignore . and .. entry */
		TCHAR* src = entry.cFileName;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		int i;
		for (i = 0; i < MAX_PATH && src[i]; i++) {
			String_Append(&path, Convert_UnicodeToCP437(src[i]));
		}

		if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			res = Directory_Enum(&path, obj, callback);
			if (res) { FindClose(find); return res; }
		} else {
			callback(&path, obj);
		}
	}  while (FindNextFile(find, &entry));

	res = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return res == ERROR_NO_MORE_FILES ? 0 : GetLastError();
}

ReturnCode File_GetModifiedTime(const String* path, TimeMS* time) {
	FileHandle file; 
	ReturnCode res = File_Open(&file, path);
	if (res) return res;

	FILETIME ft;
	if (GetFileTime(file, NULL, NULL, &ft)) {
		uint64_t raw = ft.dwLowDateTime | ((uint64_t)ft.dwHighDateTime << 32);
		*time = FileTime_TotalMS(raw);
	} else {
		res = GetLastError();
	}

	File_Close(file);
	return res;
}

ReturnCode File_SetModifiedTime(const String* path, TimeMS time) {
	FileHandle file;
	ReturnCode res = File_Append(&file, path);
	if (res) return res;

	FILETIME ft;
	uint64_t raw = 10000 * (time - FILETIME_EPOCH);
	ft.dwLowDateTime  = (uint32_t)raw;
	ft.dwHighDateTime = (uint32_t)(raw >> 32);

	if (!SetFileTime(file, NULL, NULL, &ft)) res = GetLastError();
	File_Close(file);
	return res;
}

static ReturnCode File_Do(FileHandle* file, const String* path, DWORD access, DWORD createMode) {
	TCHAR str[300]; 
	Platform_ConvertString(str, path);
	*file = CreateFile(str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return *file != INVALID_HANDLE_VALUE ? 0 : GetLastError();
}

ReturnCode File_Open(FileHandle* file, const String* path) {
	return File_Do(file, path, GENERIC_READ, OPEN_EXISTING);
}
ReturnCode File_Create(FileHandle* file, const String* path) {
	return File_Do(file, path, GENERIC_WRITE | GENERIC_READ, CREATE_ALWAYS);
}
ReturnCode File_Append(FileHandle* file, const String* path) {
	ReturnCode res = File_Do(file, path, GENERIC_WRITE | GENERIC_READ, OPEN_ALWAYS);
	if (res) return res;
	return File_Seek(*file, 0, FILE_SEEKFROM_END);
}

ReturnCode File_Read(FileHandle file, uint8_t* buffer, uint32_t count, uint32_t* bytesRead) {
	BOOL success = ReadFile(file, buffer, count, bytesRead, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode File_Write(FileHandle file, const uint8_t* buffer, uint32_t count, uint32_t* bytesWrote) {
	BOOL success = WriteFile(file, buffer, count, bytesWrote, NULL);
	return success ? 0 : GetLastError();
}

ReturnCode File_Close(FileHandle file) {
	return CloseHandle(file) ? 0 : GetLastError();
}

ReturnCode File_Seek(FileHandle file, int offset, int seekType) {
	static uint8_t modes[3] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
	DWORD pos = SetFilePointer(file, offset, NULL, modes[seekType]);
	return pos != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
}

ReturnCode File_Position(FileHandle file, uint32_t* pos) {
	*pos = SetFilePointer(file, 0, NULL, FILE_CURRENT);
	return *pos != INVALID_SET_FILE_POINTER ? 0 : GetLastError();
}

ReturnCode File_Length(FileHandle file, uint32_t* len) {
	*len = GetFileSize(file, NULL);
	return *len != INVALID_FILE_SIZE ? 0 : GetLastError();
}
#elif defined CC_BUILD_POSIX
bool Directory_Exists(const String* path) {
	char str[600]; 
	struct stat sb;
	Platform_ConvertString(str, path);
	return stat(str, &sb) == 0 && S_ISDIR(sb.st_mode);
}

ReturnCode Directory_Create(const String* path) {
	char str[600]; 
	Platform_ConvertString(str, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

bool File_Exists(const String* path) {
	char str[600]; 
	struct stat sb;
	Platform_ConvertString(str, path);
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

ReturnCode Directory_Enum(const String* dirPath, void* obj, Directory_EnumCallback callback) {
	String path; char pathBuffer[FILENAME_SIZE];
	char str[600];
	DIR* dirPtr;
	struct dirent* entry;
	char* src;
	int len, res;

	Platform_ConvertString(str, dirPath);
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

		len = String_CalcLen(src, UInt16_MaxValue);
		Convert_DecodeUtf8(&path, src, len);

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

ReturnCode File_GetModifiedTime(const String* path, TimeMS* time) {
	char str[600]; 
	struct stat sb;
	Platform_ConvertString(str, path);
	if (stat(str, &sb) == -1) return errno;

	*time = (uint64_t)sb.st_mtime * 1000 + UNIX_EPOCH;
	return 0;
}

ReturnCode File_SetModifiedTime(const String* path, TimeMS time) {
	char str[600];
	struct utimbuf times = { 0 };

	times.modtime = (time - UNIX_EPOCH) / 1000;
	Platform_ConvertString(str, path);
	return utime(str, &times) == -1 ? errno : 0;
}

static ReturnCode File_Do(FileHandle* file, const String* path, int mode) {
	char str[600]; 
	Platform_ConvertString(str, path);
	*file = open(str, mode, (6 << 6) | (4 << 3) | 4); /* rw|r|r */
	return *file == -1 ? errno : 0;
}

ReturnCode File_Open(FileHandle* file, const String* path) {
	return File_Do(file, path, O_RDONLY);
}
ReturnCode File_Create(FileHandle* file, const String* path) {
	return File_Do(file, path, O_RDWR | O_CREAT | O_TRUNC);
}
ReturnCode File_Append(FileHandle* file, const String* path) {
	ReturnCode res = File_Do(file, path, O_RDWR | O_CREAT);
	if (res) return res;
	return File_Seek(*file, 0, FILE_SEEKFROM_END);
}

ReturnCode File_Read(FileHandle file, uint8_t* buffer, uint32_t count, uint32_t* bytesRead) {
	*bytesRead = read(file, buffer, count);
	return *bytesRead == -1 ? errno : 0;
}

ReturnCode File_Write(FileHandle file, const uint8_t* buffer, uint32_t count, uint32_t* bytesWrote) {
	*bytesWrote = write(file, buffer, count);
	return *bytesWrote == -1 ? errno : 0;
}

ReturnCode File_Close(FileHandle file) {
	return close(file) == -1 ? errno : 0;
}

ReturnCode File_Seek(FileHandle file, int offset, int seekType) {
	static uint8_t modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return lseek(file, offset, modes[seekType]) == -1 ? errno : 0;
}

ReturnCode File_Position(FileHandle file, uint32_t* pos) {
	*pos = lseek(file, 0, SEEK_CUR);
	return *pos == -1 ? errno : 0;
}

ReturnCode File_Length(FileHandle file, uint32_t* len) {
	struct stat st;
	if (fstat(file, &st) == -1) { *len = -1; return errno; }
	*len = st.st_size; return 0;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
void Thread_Sleep(uint32_t milliseconds) { Sleep(milliseconds); }
DWORD WINAPI Thread_StartCallback(void* param) {
	Thread_StartFunc* func = (Thread_StartFunc*)param;
	(*func)();
	return 0;
}

void* Thread_Start(Thread_StartFunc* func, bool detach) {
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, Thread_StartCallback, func, 0, &threadID);
	if (!handle) {
		Logger_Abort2(GetLastError(), "Creating thread");
	}

	if (detach) Thread_Detach(handle);
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
	CRITICAL_SECTION* ptr = Mem_Alloc(1, sizeof(CRITICAL_SECTION), "mutex");
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
	void* handle = CreateEvent(NULL, false, false, NULL);
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

void Waitable_WaitFor(void* handle, uint32_t milliseconds) {
	WaitForSingleObject((HANDLE)handle, milliseconds);
}
#elif defined CC_BUILD_WEB
/* No true multithreading support with emscripten backend */
void Thread_Sleep(uint32_t milliseconds) { usleep(milliseconds * 1000); }
void* Thread_Start(Thread_StartFunc* func, bool detach) { (*func)(); return NULL; }
void Thread_Detach(void* handle) { }
void Thread_Join(void* handle) { }

void* Mutex_Create(void) { return NULL; }
void Mutex_Free(void* handle) { }
void Mutex_Lock(void* handle) { }
void Mutex_Unlock(void* handle) { }

void* Waitable_Create(void) { eeturn NULL; }
void Waitable_Free(void* handle) { }
void Waitable_Signal(void* handle) { }
void Waitable_Wait(void* handle) { }
void Waitable_WaitFor(void* handle, uint32_t milliseconds) { }
#elif defined CC_BUILD_POSIX
void Thread_Sleep(uint32_t milliseconds) { usleep(milliseconds * 1000); }
void* Thread_StartCallback(void* lpParam) {
	Thread_StartFunc* func = (Thread_StartFunc*)lpParam;
	(*func)();
	return NULL;
}

void* Thread_Start(Thread_StartFunc* func, bool detach) {
	pthread_t* ptr = Mem_Alloc(1, sizeof(pthread_t), "allocating thread");
	int res = pthread_create(ptr, NULL, Thread_StartCallback, func);
	if (res) Logger_Abort2(res, "Creating thread");

	if (detach) Thread_Detach(ptr);
	return ptr;
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = handle;
	int res = pthread_detach(*ptr);
	if (res) Logger_Abort2(res, "Detaching thread");
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	pthread_t* ptr = handle;
	int res = pthread_join(*ptr, NULL);
	if (res) Logger_Abort2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(void) {
	pthread_mutex_t* ptr = Mem_Alloc(1, sizeof(pthread_mutex_t), "allocating mutex");
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
};

void* Waitable_Create(void) {
	struct WaitData* ptr = Mem_Alloc(1, sizeof(struct WaitData), "allocating waitable");
	int res;
	
	res = pthread_cond_init(&ptr->cond, NULL);
	if (res) Logger_Abort2(res, "Creating waitable");
	res = pthread_mutex_init(&ptr->mutex, NULL);
	if (res) Logger_Abort2(res, "Creating waitable mutex");
	return ptr;
}

void Waitable_Free(void* handle) {
	struct WaitData* ptr = handle;
	int res;
	
	res = pthread_cond_destroy(&ptr->cond);
	if (res) Logger_Abort2(res, "Destroying waitable");
	res = pthread_mutex_destroy(&ptr->mutex);
	if (res) Logger_Abort2(res, "Destroying waitable mutex");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	struct WaitData* ptr = handle;
	int res = pthread_cond_signal(&ptr->cond);
	if (res) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	struct WaitData* ptr = handle;
	int res;

	Mutex_Lock(&ptr->mutex);
	res = pthread_cond_wait(&ptr->cond, &ptr->mutex);
	if (res) Logger_Abort2(res, "Waitable wait");
	Mutex_Unlock(&ptr->mutex);
}

void Waitable_WaitFor(void* handle, uint32_t milliseconds) {
	struct WaitData* ptr = handle;
	struct timeval tv;
	struct timespec ts;
	int res;
	gettimeofday(&tv, NULL);

	/* absolute time for some silly reason */
	ts.tv_sec = tv.tv_sec + milliseconds / 1000;
	ts.tv_nsec = 1000 * (tv.tv_usec + 1000 * (milliseconds % 1000));
	ts.tv_sec += ts.tv_nsec / NS_PER_SEC;
	ts.tv_nsec %= NS_PER_SEC;

	Mutex_Lock(&ptr->mutex);
	res = pthread_cond_timedwait(&ptr->cond, &ptr->mutex, &ts);
	if (res && res != ETIMEDOUT) Logger_Abort2(res, "Waitable wait for");
	Mutex_Unlock(&ptr->mutex);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static FT_Library ft_lib;
static struct FT_MemoryRec_ ft_mem;
static struct EntryList font_list;
static bool font_list_changed;
static void Font_Init(void);

#define DPI_PIXEL  72
#define DPI_DEVICE 96 /* TODO: GetDeviceCaps(hdc, LOGPIXELSY) in Window_Init ? */

struct FontData {
	FT_Face face;
	struct Stream src, file;
	FT_StreamRec stream;
	uint8_t buffer[8192]; /* small buffer to minimise disk I/O */
	uint16_t widths[256]; /* cached width of each character glyph */
	FT_Glyph glyphs[256]; /* cached glyphs */
	FT_Glyph shadow_glyphs[256]; /* cached glyphs (for back layer shadow) */
#ifdef CC_BUILD_OSX
	char filename[FILENAME_SIZE + 1];
#endif
};

static unsigned long FontData_Read(FT_Stream s, unsigned long offset, unsigned char* buffer, unsigned long count) {
	struct FontData* data;
	ReturnCode res;

	if (!count && offset > s->size) return 1;
	data = s->descriptor.pointer;
	if (s->pos != offset) data->src.Seek(&data->src, offset);

	res = Stream_Read(&data->src, buffer, count);
	return res ? 0 : count;
}

static void FontData_Free(struct FontData* font) {
	int i;

	/* Close the actual underlying file */
	struct Stream* source = &font->file;
	if (!source->Meta.File) return;
	source->Close(source);

	for (i = 0; i < 256; i++) {
		if (!font->glyphs[i]) continue;
		FT_Done_Glyph(font->glyphs[i]);
	}
	for (i = 0; i < 256; i++) {
		if (!font->shadow_glyphs[i]) continue;
		FT_Done_Glyph(font->shadow_glyphs[i]);
	}
}

static void FontData_Close(FT_Stream stream) {
	struct FontData* data = stream->descriptor.pointer;
	FontData_Free(data);
}

static bool FontData_Init(const String* path, struct FontData* data, FT_Open_Args* args) {
	FileHandle file;
	uint32_t size;

	if (File_Open(&file, path)) return false;
	if (File_Length(file, &size)) { File_Close(file); return false; }

	data->stream.base = NULL;
	data->stream.size = size;
	data->stream.pos  = 0;

	data->stream.descriptor.pointer = data;
	data->stream.read   = FontData_Read;
	data->stream.close  = FontData_Close;

	data->stream.memory = &ft_mem;
	data->stream.cursor = NULL;
	data->stream.limit  = NULL;

	args->flags    = FT_OPEN_STREAM;
	args->pathname = NULL;
	args->stream   = &data->stream;

	Stream_FromFile(&data->file, file);
	Stream_ReadonlyBuffered(&data->src, &data->file, data->buffer, sizeof(data->buffer));

	/* For OSX font suitcase files */
#ifdef CC_BUILD_OSX
	String filename = String_NT_Array(data->filename);
	String_Copy(&filename, path);
	data->filename[filename.length] = '\0';
	args->pathname = data->filename;
#endif
	Mem_Set(data->widths,        0xFF, sizeof(data->widths));
	Mem_Set(data->glyphs,        0x00, sizeof(data->glyphs));
	Mem_Set(data->shadow_glyphs, 0x00, sizeof(data->shadow_glyphs));
	return true;
}

void Font_GetNames(StringsBuffer* buffer) {
	String entry, name, path;
	int i;
	if (!font_list.Entries.Count) Font_Init();

	for (i = 0; i < font_list.Entries.Count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&font_list.Entries, i);
		String_UNSAFE_Separate(&entry, font_list.Separator, &name, &path);

		/* Only want Regular fonts here */
		if (name.length < 2 || name.buffer[name.length - 1] != 'R') continue;
		name.length -= 2;
		StringsBuffer_Add(buffer, &name);
	}
}

static String Font_LookupOf(const String* fontName, const char type) {
	String name; char nameBuffer[STRING_SIZE + 2];
	String_InitArray(name, nameBuffer);

	String_Format2(&name, "%s %r", fontName, &type);
	return EntryList_UNSAFE_Get(&font_list, &name);
}

String Font_Lookup(const String* fontName, int style) {
	String path;
	if (!font_list.Entries.Count) Font_Init();
	path = String_Empty;

	if (style & FONT_STYLE_BOLD)   path = Font_LookupOf(fontName, 'B');
	if (style & FONT_STYLE_ITALIC) path = Font_LookupOf(fontName, 'I');

	return path.length ? path : Font_LookupOf(fontName, 'R');
}

void Font_Make(FontDesc* desc, const String* fontName, int size, int style) {
	struct FontData* data;
	String value, path, index;
	int faceIndex;
	FT_Open_Args args;
	FT_Error err;

	desc->Size  = size;
	desc->Style = style;

	value = Font_Lookup(fontName, style);
	if (!value.length) Logger_Abort("Unknown font");
	String_UNSAFE_Separate(&value, ',', &path, &index);
	Convert_ParseInt(&index, &faceIndex);

	data = Mem_Alloc(1, sizeof(struct FontData), "FontData");
	if (!FontData_Init(&path, data, &args)) return;
	desc->Handle = data;

	err = FT_New_Face(ft_lib, &args, faceIndex, &data->face);
	if (err) Logger_Abort2(err, "Creating font failed");
	err = FT_Set_Char_Size(data->face, size * 64, 0, DPI_DEVICE, 0);
	if (err) Logger_Abort2(err, "Resizing font failed");
}

void Font_Free(FontDesc* desc) {
	struct FontData* data;
	FT_Error err;

	desc->Size  = 0;
	desc->Style = 0;
	/* NULL for fonts created by Drawer2D_MakeFont and bitmapped text mode is on */
	if (!desc->Handle) return;

	data = desc->Handle;
	err  = FT_Done_Face(data->face);
	if (err) Logger_Abort2(err, "Deleting font failed");

	Mem_Free(data);
	desc->Handle = NULL;
}

static void Font_Add(const String* path, FT_Face face, int index, char type, const char* defStyle) {
	String key;   char keyBuffer[STRING_SIZE];
	String value; char valueBuffer[FILENAME_SIZE];
	String style = String_Empty;

	if (!face->family_name || !(face->face_flags & FT_FACE_FLAG_SCALABLE)) return;
	/* don't want 'Arial Regular' or 'Arial Bold' */
	if (face->style_name) {
		style = String_FromReadonly(face->style_name);
		if (String_CaselessEqualsConst(&style, defStyle)) style.length = 0;
	}

	String_InitArray(key, keyBuffer);
	if (style.length) {
		String_Format3(&key, "%c %c %r", face->family_name, face->style_name, &type);
	} else {
		String_Format2(&key, "%c %r", face->family_name, &type);
	}

	String_InitArray(value, valueBuffer);
	String_Format2(&value, "%s,%i", path, &index);

	Platform_Log2("Face: %s = %s", &key, &value);
	EntryList_Set(&font_list, &key, &value);
	font_list_changed = true;
}

static int Font_Register(const String* path, int faceIndex) {
	struct FontData data;
	FT_Open_Args args;
	FT_Error err;
	int flags, count;

	if (!FontData_Init(path, &data, &args)) return 0;
	err = FT_New_Face(ft_lib, &args, faceIndex, &data.face);
	if (err) { FontData_Free(&data); return 0; }

	flags = data.face->style_flags;
	count = data.face->num_faces;

	if (flags == (FT_STYLE_FLAG_BOLD | FT_STYLE_FLAG_ITALIC)) {
		Font_Add(path, data.face, faceIndex, 'Z', "Bold Italic");
	} else if (flags == FT_STYLE_FLAG_BOLD) {
		Font_Add(path, data.face, faceIndex, 'B', "Bold");
	} else if (flags == FT_STYLE_FLAG_ITALIC) {
		Font_Add(path, data.face, faceIndex, 'I', "Italic");
	} else if (flags == 0) {
		Font_Add(path, data.face, faceIndex, 'R', "Regular");
	}

	FT_Done_Face(data.face);
	return count;
}

static void Font_DirCallback(const String* path, void* obj) {
	const static String fonExt = String_FromConst(".fon");
	String entry, name, value;
	String fontPath, index;
	int i, count;

	/* Completely skip windows .FON files */
	if (String_CaselessEnds(path, &fonExt)) return;

	/* If font is already known good, skip it */
	for (i = 0; i < font_list.Entries.Count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&font_list.Entries, i);
		String_UNSAFE_Separate(&entry, font_list.Separator, &name, &value);

		String_UNSAFE_Separate(&value, ',', &fontPath, &index);
		if (String_CaselessEquals(path, &fontPath)) return;
	}

	count = Font_Register(path, 0);
	/* There may be more than one font in a font file */
	for (i = 1; i < count; i++) {
		Font_Register(path, i);
	}
}

#define TEXT_CEIL(x) (((x) + 63) >> 6)
int Platform_TextWidth(struct DrawTextArgs* args) {
	struct FontData* data = args->Font.Handle;
	FT_Face face = data->face;
	String text  = args->Text;
	int i, width = 0, charWidth;
	FT_Error res;
	Codepoint cp;

	for (i = 0; i < text.length; i++) {
		charWidth = data->widths[(uint8_t)text.buffer[i]];
		/* need to calculate glyph width */
		if (charWidth == UInt16_MaxValue) {
			cp  = Convert_CP437ToUnicode(text.buffer[i]);
			res = FT_Load_Char(face, cp, 0);

			if (res) {
				Platform_Log2("Error %i measuring width of %r", &res, &text.buffer[i]);
				charWidth = 0;
			} else {
				charWidth = face->glyph->advance.x;		
			}

			data->widths[(uint8_t)text.buffer[i]] = charWidth;
		}
		width += charWidth;
	}
	return TEXT_CEIL(width);
}

int Platform_FontHeight(const FontDesc* font) {
	struct FontData* data = font->Handle;
	FT_Face face = data->face;
	return TEXT_CEIL(face->size->metrics.height);
}

static void Platform_GrayscaleGlyph(FT_Bitmap* img, Bitmap* bmp, int x, int y, BitmapCol col) {
	uint8_t* src;
	BitmapCol* dst;
	uint8_t intensity, invIntensity;
	int xx, yy;

	for (yy = 0; yy < img->rows; yy++) {
		if ((unsigned)(y + yy) >= (unsigned)bmp->Height) continue;
		src = img->buffer + (yy * img->pitch);
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < img->width; xx++, src++, dst++) {
			if ((unsigned)(x + xx) >= (unsigned)bmp->Width) continue;
			intensity = *src; invIntensity = UInt8_MaxValue - intensity;

			dst->B = ((col.B * intensity) >> 8) + ((dst->B * invIntensity) >> 8);
			dst->G = ((col.G * intensity) >> 8) + ((dst->G * invIntensity) >> 8);
			dst->R = ((col.R * intensity) >> 8) + ((dst->R * invIntensity) >> 8);
			/*dst->A = ((col.A * intensity) >> 8) + ((dst->A * invIntensity) >> 8);*/
			dst->A = intensity + ((dst->A * invIntensity) >> 8);
		}
	}
}

static void Platform_BlackWhiteGlyph(FT_Bitmap* img, Bitmap* bmp, int x, int y, BitmapCol col) {
	uint8_t* src;
	BitmapCol* dst;
	uint8_t intensity;
	int xx, yy;

	for (yy = 0; yy < img->rows; yy++) {
		if ((unsigned)(y + yy) >= (unsigned)bmp->Height) continue;
		src = img->buffer + (yy * img->pitch);
		dst = Bitmap_GetRow(bmp, y + yy) + x;

		for (xx = 0; xx < img->width; xx++, dst++) {
			if ((unsigned)(x + xx) >= (unsigned)bmp->Width) continue;
			intensity = src[xx >> 3];

			if (intensity & (1 << (7 - (xx & 7)))) {
				dst->B = col.B; dst->G = col.G; dst->R = col.R;
				/*dst->A = col.A*/
				dst->A = 255;
			}
		}
	}
}

int Platform_TextDraw(struct DrawTextArgs* args, Bitmap* bmp, int x, int y, BitmapCol col, bool shadow) {
	struct FontData* data = args->Font.Handle;
	FT_Face face     = data->face;
	String text      = args->Text;
	FT_Glyph* glyphs = data->glyphs;
	int descender, height, begX = x;
	
	/* glyph state */
	FT_BitmapGlyph glyph;
	FT_Bitmap* img;
	int i, offset;
	FT_Error res;
	Codepoint cp;

	if (shadow) {
		glyphs = data->shadow_glyphs;
		FT_Vector delta = { 83, -83 };
		FT_Set_Transform(face, NULL, &delta);
	}

	height    = TEXT_CEIL(face->size->metrics.height);
	descender = TEXT_CEIL(face->size->metrics.descender);

	for (i = 0; i < text.length; i++) {
		glyph = glyphs[(uint8_t)text.buffer[i]];
		if (!glyph) {
			cp  = Convert_CP437ToUnicode(text.buffer[i]);
			res = FT_Load_Char(face, cp, FT_LOAD_RENDER);

			if (res) {
				Platform_Log2("Error %i drawing %r", &res, &text.buffer[i]);
				continue;
			}

			FT_Get_Glyph(face->glyph, &glyph); /* TODO: Check error */
			glyphs[(uint8_t)text.buffer[i]] = glyph;
		}

		offset = (height + descender) - glyph->top;
		x += glyph->left; y += offset;

		img = &glyph->bitmap;
		if (img->num_grays == 2) {
			Platform_BlackWhiteGlyph(img, bmp, x, y, col);
		} else {
			Platform_GrayscaleGlyph(img, bmp, x, y, col);
		}

		x += TEXT_CEIL(glyph->root.advance.x >> 10);
		x -= glyph->left; y -= offset;
	}

	if (args->Font.Style & FONT_FLAG_UNDERLINE) {
		int ul_pos   = FT_MulFix(face->underline_position,  face->size->metrics.y_scale);
		int ul_thick = FT_MulFix(face->underline_thickness, face->size->metrics.y_scale);

		int ulHeight = TEXT_CEIL(ul_thick);
		int ulY      = height + TEXT_CEIL(ul_pos);
		Drawer2D_Underline(bmp, begX, ulY + y, x - begX, ulHeight, col);
	}

	if (shadow) FT_Set_Transform(face, NULL, NULL);
	return x - begX;
}

static void* FT_AllocWrapper(FT_Memory memory, long size) {
	return Mem_Alloc(size, 1, "Freetype data");
}

static void FT_FreeWrapper(FT_Memory memory, void* block) {
	Mem_Free(block);
}

static void* FT_ReallocWrapper(FT_Memory memory, long cur_size, long new_size, void* block) {
	return Mem_Realloc(block, new_size, 1, "Freetype data");
}

#define FONT_CACHE_FILE "fontscache.txt"
static void Font_Init(void) {
#if defined CC_BUILD_WIN
	const static String dirs[2] = {
		String_FromConst("C:/Windows/Fonts"),
		String_FromConst("C:/WINNT/Fonts")
	};
#elif defined CC_BUILD_NETBSD
	const static String dirs[3] = {
		String_FromConst("/usr/X11R7/lib/X11/fonts"),
		String_FromConst("/usr/pkg/lib/X11/fonts"),
		String_FromConst("/usr/pkg/share/fonts")
	};
#elif defined CC_BUILD_UNIX
	const static String dirs[2] = {
		String_FromConst("/usr/share/fonts"),
		String_FromConst("/usr/local/share/fonts")
	};
#elif defined CC_BUILD_OSX
	const static String dirs[2] = {
		String_FromConst("/System/Library/Fonts"),
		String_FromConst("/Library/Fonts")
	};
#elif defined CC_BUILD_WEB
	/* TODO: Implement fonts */
	const static String dirs[1] = { String_FromConst("Fonts") };
#endif
	const static String cachePath = String_FromConst(FONT_CACHE_FILE);
	FT_Error err;
	int i;

	ft_mem.alloc   = FT_AllocWrapper;
	ft_mem.free    = FT_FreeWrapper;
	ft_mem.realloc = FT_ReallocWrapper;

	err = FT_New_Library(&ft_mem, &ft_lib);
	if (err) Logger_Abort2(err, "Failed to init freetype");
	FT_Add_Default_Modules(ft_lib);

	if (!File_Exists(&cachePath)) {
		Window_ShowDialog("One time load", "Initialising font cache, this can take several seconds.");
	}

	EntryList_Init(&font_list, NULL, FONT_CACHE_FILE, '=');
	for (i = 0; i < Array_Elems(dirs); i++) {
		Directory_Enum(&dirs[i], NULL, Font_DirCallback);
	}
	if (font_list_changed) EntryList_Save(&font_list);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
void Socket_Create(SocketHandle* socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		Logger_Abort2(Socket__Error(), "Failed to create socket");
	}
}

static ReturnCode Socket_ioctl(SocketHandle socket, uint32_t cmd, int* data) {
#if defined CC_BUILD_WIN
	return ioctlsocket(socket, cmd, data);
#else
	return ioctl(socket, cmd, data);
#endif
}

ReturnCode Socket_Available(SocketHandle socket, uint32_t* available) {
	return Socket_ioctl(socket, FIONREAD, available);
}
ReturnCode Socket_SetBlocking(SocketHandle socket, bool blocking) {
#if defined CC_BUILD_WEB
	return ReturnCode_NotSupported; /* sockets always async */
#else
	int blocking_raw = blocking ? 0 : -1;
	return Socket_ioctl(socket, FIONBIO, &blocking_raw);
#endif
}


ReturnCode Socket_GetError(SocketHandle socket, ReturnCode* result) {
	int resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Socket_Connect(SocketHandle socket, const String* ip, int port) {
	struct sockaddr addr;
	ReturnCode res;

	addr.sa_family = AF_INET;
	Stream_SetU16_BE(&addr.sa_data[0], port);
	Utils_ParseIP(ip, &addr.sa_data[2]);

	res = connect(socket, &addr, sizeof(addr));
	return res == -1 ? Socket__Error() : 0;
}

ReturnCode Socket_Read(SocketHandle socket, uint8_t* buffer, uint32_t count, uint32_t* modified) {
	int recvCount = recv(socket, buffer, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Socket_Write(SocketHandle socket, uint8_t* buffer, uint32_t count, uint32_t* modified) {
	int sentCount = send(socket, buffer, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Socket_Close(SocketHandle socket) {
	ReturnCode res = 0;
	ReturnCode res1, res2;

#if defined CC_BUILD_WEB
	res1 = 0;
#elif defined CC_BUILD_WIN
	res1 = shutdown(socket, SD_BOTH);
#else
	res1 = shutdown(socket, SHUT_RDWR);
#endif
	if (res1 == -1) res = Socket__Error();

#if defined CC_BUILD_WIN
	res2 = closesocket(socket);
#else
	res2 = close(socket);
#endif
	if (res2 == -1) res = Socket__Error();
	return res;
}

/* Alas, a simple cross-platform select() is not good enough */
#if defined CC_BUILD_WIN
ReturnCode Socket_Poll(SocketHandle socket, int mode, bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	set.fd_count    = 1;
	set.fd_array[0] = socket;

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return Socket__Error(); }

	*success = set.fd_count != 0; return 0;
}
#elif defined CC_BUILD_OSX
/* poll is broken on old OSX apparently https://daniel.haxx.se/docs/poll-vs-select.html */
ReturnCode Socket_Poll(SocketHandle socket, int mode, bool* success) {
	fd_set set;
	struct timeval time = { 0 };
	int selectCount;

	FD_ZERO(&set);
	FD_SET(socket, &set);

	if (mode == SOCKET_POLL_READ) {
		selectCount = select(socket + 1, &set, NULL, NULL, &time);
	} else {
		selectCount = select(socket + 1, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return Socket__Error(); }
	*success = FD_ISSET(socket, &set); return 0;
}
#else
ReturnCode Socket_Poll(SocketHandle socket, int mode, bool* success) {
	struct pollfd pfd;
	int flags;

	pfd.fd     = socket;
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
ReturnCode Process_GetExePath(String* path) {
	TCHAR chars[FILENAME_SIZE + 1];
	DWORD len = GetModuleFileName(NULL, chars, FILENAME_SIZE);
	if (!len) return GetLastError();

	Platform_DecodeString(path, chars, len);
	return 0;
}

ReturnCode Process_Start(const String* path, const String* args) {
	String file, argv; char argvBuffer[300];
	TCHAR str[300], raw[300];
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	BOOL ok;

	file = *path; Utils_UNSAFE_GetFilename(&file);
	String_InitArray(argv, argvBuffer);
	String_Format2(&argv, "\"%s\" %s", &file, args);
	Platform_ConvertString(str, path);
	Platform_ConvertString(raw, &argv);

	si.cb = sizeof(STARTUPINFO);
	ok    = CreateProcess(str, raw, NULL, NULL, false, 0, NULL, NULL, &si, &pi);
	if (!ok) return GetLastError();

	/* Don't leak memory for proess return code */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

ReturnCode Process_StartOpen(const String* args) {
	TCHAR str[300];
	HINSTANCE instance;
	Platform_ConvertString(str, args);
	instance = ShellExecute(NULL, NULL, str, NULL, NULL, SW_SHOWNORMAL);
	return instance > 32 ? 0 : (ReturnCode)instance;
}

void Process_Exit(ReturnCode code) { ExitProcess(code); }

ReturnCode DynamicLib_Load(const String* path, void** lib) {
	TCHAR str[300];
	Platform_ConvertString(str, path);
	*lib = LoadLibrary(str);
	return *lib ? 0 : GetLastError();
}

ReturnCode DynamicLib_Get(void* lib, const char* name, void** symbol) {
	*symbol = GetProcAddress(lib, name);
	return *symbol ? 0 : GetLastError();
}

bool DynamicLib_DescribeError(ReturnCode res, String* dst) {
	return Platform_DescribeError(res, dst);
}
#elif defined CC_BUILD_WEB
ReturnCode Process_GetExePath(String* path) { return ReturnCode_NotSupported; }
ReturnCode Process_Start(const String* path, const String* args) { return ReturnCode_NotSupported; }

ReturnCode Process_StartOpen(const String* args) {
	char str[600];
	Platform_ConvertString(str, args);
	EM_ASM_({ window.open(UTF8ToString($0)); }, str);
}
void Process_Exit(ReturnCode code) { exit(code); }

ReturnCode DynamicLib_Load(const String* path, void** lib) { return ReturnCode_NotSupported; }
ReturnCode DynamicLib_Get(void* lib, const char* name, void** symbol) { return ReturnCode_NotSupported; }
bool DynamicLib_DescribeError(ReturnCode res, String* dst) { return false; }
#elif defined CC_BUILD_POSIX
ReturnCode Process_Start(const String* path, const String* args) {
	char str[600], raw[600];
	pid_t pid;
	int i, j;
	Platform_ConvertString(str, path);
	Platform_ConvertString(raw, args);

	pid = fork();
	if (pid == -1) return errno;

	if (pid == 0) {
		/* Executed in child process */
		char* argv[15];
		argv[0] = str; argv[1] = raw;

		/* need to null-terminate multiple arguments */
		for (i = 0, j = 2; raw[i] && i < Array_Elems(raw); i++) {
			if (raw[i] != ' ') continue;

			/* null terminate previous argument */
			raw[i]    = '\0';
			argv[j++] = &raw[i + 1];
		}
		argv[j] = NULL;

		execvp(str, argv);
		_exit(127); /* "command not found" */
	} else {
		/* Executed in parent process */
		/* We do nothing here.. */
		return 0;
	}
}

void Process_Exit(ReturnCode code) { exit(code); }

ReturnCode DynamicLib_Load(const String* path, void** lib) {
	char str[600];
	Platform_ConvertString(str, path);
	*lib = dlopen(str, RTLD_NOW);
	return *lib == NULL;
}

ReturnCode DynamicLib_Get(void* lib, const char* name, void** symbol) {
	*symbol = dlsym(lib, name);
	return *symbol == NULL; /* dlerror would be proper, but eh */
}

bool DynamicLib_DescribeError(ReturnCode res, String* dst) {
	char* err = dlerror();
	if (err) String_AppendConst(dst, err);
	return err && err[0];
}
#endif
/* Opening browser and retrieving exe path is not standardised at all */
#if defined CC_BUILD_OSX
ReturnCode Process_StartOpen(const String* args) {
	const static String path = String_FromConst("/usr/bin/open");
	return Process_Start(&path, args);
}
ReturnCode Process_GetExePath(String* path) {
	char str[600];
	int len = 600;

	if (_NSGetExecutablePath(str, &len) != 0) return ReturnCode_InvalidArg;
	Convert_DecodeUtf8(path, str, len);
	return 0;
}
#elif defined CC_BUILD_UNIX
ReturnCode Process_StartOpen(const String* args) {
	/* TODO: Can this be used on original Solaris, or is it just an OpenIndiana thing */
	const static String path = String_FromConst("xdg-open");
	return Process_Start(&path, args);
}
#if defined CC_BUILD_LINUX
ReturnCode Process_GetExePath(String* path) {
	char str[600];
	int len = readlink("/proc/self/exe", str, 600);
	if (len == -1) return errno;

	Convert_DecodeUtf8(path, str, len);
	return 0;
}
#elif defined CC_BUILD_FREEBSD
ReturnCode Process_GetExePath(String* path) {
	static int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
	char str[600];
	size_t size = 600;
	if (sysctl(mib, 4, str, &size, NULL, 0) == -1) return errno;

	size = String_CalcLen(str, 600);
	Convert_DecodeUtf8(path, str, size);
	return 0;
}
#elif defined CC_BUILD_OPENBSD
ReturnCode Process_GetExePath(String* path) {
	static int mib[4] = { CTL_KERN, KERN_PROC_ARGS, 0, KERN_PROC_ARGV };
	char tmp[600];	
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

	size = String_CalcLen(str, 600);
	Convert_DecodeUtf8(path, str, size);
	return 0;
}
#elif defined CC_BUILD_NETBSD
ReturnCode Process_GetExePath(String* path) {
	static int mib[4] = { CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME };
	char str[600];
	size_t size = 600;	
	if (sysctl(mib, 4, str, &size, NULL, 0) == -1) return errno;

	size = String_CalcLen(str, 600);
	Convert_DecodeUtf8(path, str, size);
	return 0;
}
#elif defined CC_BUILD_SOLARIS
ReturnCode Process_GetExePath(String* path) {
	char str[600];
	int len = readlink("/proc/self/path/a.out", str, 600);
	if (len == -1) return errno;

	Convert_DecodeUtf8(path, str, len);
	return 0;
}
#endif
#endif

void* DynamicLib_GetFrom(const char* filename, const char* name) {
	void* symbol;
	void* lib;
	String path;
	ReturnCode res;

	path = String_FromReadonly(filename);
	res = DynamicLib_Load(&path, &lib);
	if (res) return NULL;

	res = DynamicLib_Get(lib, name, &symbol);
	return res ? NULL : symbol;
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WIN
int Platform_ConvertString(void* data, const String* src) {
	TCHAR* dst = data;
	int i;
	if (src->length > FILENAME_SIZE) Logger_Abort("String too long to expand");

	for (i = 0; i < src->length; i++) {
		*dst = Convert_CP437ToUnicode(src->buffer[i]); dst++;
	}
	*dst = '\0';
	return src->length * 2;
}

static void Platform_InitStopwatch(void) {
	LARGE_INTEGER freq;
	sw_highRes = QueryPerformanceFrequency(&freq);

	if (sw_highRes) {
		sw_freqMul = 1000 * 1000;
		sw_freqDiv = freq.QuadPart;
	} else { sw_freqDiv = 10; }
}

typedef BOOL (WINAPI *AttachConsoleFunc)(DWORD dwProcessId);
void Platform_Init(void) {	
	WSADATA wsaData;
	ReturnCode res;

	Platform_InitStopwatch();
	heap = GetProcessHeap();
	
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_Warn(res, "starting WSA");

	hasDebugger = IsDebuggerPresent();
	/* For when user runs from command prompt */
	/* NOTE: Need to dynamically load, not supported on Windows 2000 */
	AttachConsoleFunc attach = DynamicLib_GetFrom("KERNEL32.DLL", "AttachConsole");
	/* -1 is ATTACH_PARENT_PROCESS (define doesn't exist when compiling for win2000) */
	if (attach) attach((DWORD)(-1));

	conHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (conHandle == INVALID_HANDLE_VALUE) conHandle = NULL;
}

void Platform_Free(void) {
	WSACleanup();
	HeapDestroy(heap);
}

ReturnCode Platform_SetCurrentDirectory(const String* path) {
	TCHAR str[300];
	Platform_ConvertString(str, path);
	return SetCurrentDirectory(str) ? 0 : GetLastError();
}

/* Don't need special execute permission on windows */
ReturnCode Platform_MarkExecutable(const String* path) { return 0; }

static String Platform_NextArg(STRING_REF String* args) {
	String arg;
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

int Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args) {
	String cmdArgs = String_FromReadonly(GetCommandLineA());
	int i;
	Platform_NextArg(&cmdArgs); /* skip exe path */

	for (i = 0; i < GAME_MAX_CMDARGS; i++) {
		args[i] = Platform_NextArg(&cmdArgs);

		if (!args[i].length) break;
	}
	return i;
}

ReturnCode Platform_Encrypt(const uint8_t* data, int len, uint8_t** enc, int* encLen) {
	DATA_BLOB dataIn, dataOut;
	dataIn.cbData = len; dataIn.pbData = data;
	if (!CryptProtectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) return GetLastError();

	/* copy to memory we can free */
	*enc    = Mem_Alloc(dataOut.cbData, 1, "encrypt data");
	*encLen = dataOut.cbData;
	Mem_Copy(*enc, dataOut.pbData, dataOut.cbData);
	LocalFree(dataOut.pbData);
	return 0;
}
ReturnCode Platform_Decrypt(const uint8_t* data, int len, uint8_t** dec, int* decLen) {
	DATA_BLOB dataIn, dataOut;
	dataIn.cbData = len; dataIn.pbData = data;
	if (!CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) return GetLastError();

	/* copy to memory we can free */
	*dec    = Mem_Alloc(dataOut.cbData, 1, "decrypt data");
	*decLen = dataOut.cbData;
	Mem_Copy(*dec, dataOut.pbData, dataOut.cbData);
	LocalFree(dataOut.pbData);
	return 0;
}

bool Platform_DescribeError(ReturnCode res, String* dst) {
	TCHAR chars[600];
	res = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, res, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), chars, 600, NULL);
	if (!res) return false;

	Platform_DecodeString(dst, chars, res);
	return true;
}
#elif defined CC_BUILD_POSIX
int Platform_ConvertString(void* data, const String* src) {
	uint8_t* dst = data;
	uint8_t* cur;

	Codepoint cp;
	int i, len = 0;
	if (src->length > FILENAME_SIZE) Logger_Abort("String too long to expand");

	for (i = 0; i < src->length; i++) {
		cur = dst + len;
		cp  = Convert_CP437ToUnicode(src->buffer[i]);	
		len += Convert_UnicodeToUtf8(cp, cur);
	}
	dst[len] = '\0';
	return len;
}

static void Platform_InitCommon(void) {
	signal(SIGCHLD, SIG_IGN);
	/* So writing to closed socket doesn't raise SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
}
void Platform_Free(void) { }

ReturnCode Platform_SetCurrentDirectory(const String* path) {
	char str[600];
	Platform_ConvertString(str, path);
	return chdir(str) == -1 ? errno : 0;
}

ReturnCode Platform_MarkExecutable(const String* path) {
	char str[600];
	struct stat st;
	Platform_ConvertString(str, path);

	if (stat(str, &st) == -1) return errno;
	st.st_mode |= S_IXUSR;
	return chmod(str, st.st_mode) == -1 ? errno : 0;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args) {
	int i, count;
	argc--; /* skip executable path argument */
	count = min(argc, GAME_MAX_CMDARGS);

	for (i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i + 1]);
	}
	return count;
}

ReturnCode Platform_Encrypt(const uint8_t* data, int len, uint8_t** enc, int* encLen) {
	return ReturnCode_NotSupported;
}
ReturnCode Platform_Decrypt(const uint8_t* data, int len, uint8_t** dec, int* decLen) {
	return ReturnCode_NotSupported;
}

bool Platform_DescribeError(ReturnCode res, String* dst) {
	char chars[600];
	int len;

	len = strerror_r(res, chars, 600);
	if (len == -1) return false;

	len = String_CalcLen(chars, 600);
	Convert_DecodeUtf8(dst, chars, len);
	return true;
}
#endif
#if defined CC_BUILD_UNIX
void Platform_Init(void) {
	Platform_InitCommon();
	/* stopwatch always in nanoseconds */
	sw_freqDiv = 1000;
}
#elif defined CC_BUILD_OSX
static void Platform_InitStopwatch(void) {
	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);

	sw_freqMul = tb.numer;
	sw_freqDiv = tb.denom * 1000;
}

void Platform_Init(void) {
	ProcessSerialNumber psn;
	Platform_InitCommon();
	Platform_InitStopwatch();
	
	/* NOTE: Call as soon as possible, otherwise can't click on dialog boxes. */
	GetCurrentProcess(&psn);
	/* NOTE: TransformProcessType is OSX 10.3 or later */
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}
#elif defined CC_BUILD_WEB
void Platform_Init(void) { }
#endif
