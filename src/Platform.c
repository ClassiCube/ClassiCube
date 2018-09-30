#include "Platform.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "AsyncDownloader.h"
#include "Bitmap.h"

#include "freetype/ft2build.h"
#include "freetype/freetype.h"

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WIN32_IE    0x0400
#define WINVER       0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <mmsystem.h>
#include <shellapi.h>

#define HTTP_QUERY_ETAG 54 /* Missing from some old MingW32 headers */
#define Socket__Error() WSAGetLastError()
#define Win_Return(success) ((success) ? 0 : GetLastError())

HDC hdc;
HANDLE heap;
char* Platform_NewLine = "\r\n";
char Directory_Separator = '\\';

ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
ReturnCode ReturnCode_FileNotFound = ERROR_FILE_NOT_FOUND;
ReturnCode ReturnCode_NotSupported = ERROR_NOT_SUPPORTED;
ReturnCode ReturnCode_InvalidArg = ERROR_INVALID_PARAMETER;
ReturnCode ReturnCode_SocketInProgess = WSAEINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
#elif CC_BUILD_NIX
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
#include <X11/Xlib.h>
#include <curl/curl.h>
#include <AL/al.h>
#include <AL/alc.h>
#if CC_BUILD_SOLARIS
#include <sys/filio.h>
#endif

#define Socket__Error() errno
#define Nix_Return(success) ((success) ? 0 : errno)

pthread_mutex_t event_mutex;

char* Platform_NewLine = "\n";
char Directory_Separator = '/';
ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
ReturnCode ReturnCode_FileNotFound = ENOENT;
ReturnCode ReturnCode_NotSupported = EPERM;
ReturnCode ReturnCode_InvalidArg = EINVAL;
ReturnCode ReturnCode_SocketInProgess = EINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;
#endif


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void* dst, UInt8 value, UInt32 numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, void* src,  UInt32 numBytes) { memcpy(dst, src,   numBytes); }

NOINLINE_ static void Platform_AllocFailed(const char* place) {
	char logBuffer[STRING_SIZE+20 + 1];
	String log = String_NT_Array(logBuffer);
	String_Format1(&log, "Failed allocating memory for: %c", place);

	log.buffer[log.length] = '\0';
	ErrorHandler_Fail(log.buffer);
}

#if CC_BUILD_WIN
void* Mem_Alloc(UInt32 numElems, UInt32 elemsSize, const char* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, 0, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(UInt32 numElems, UInt32 elemsSize, const char* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, HEAP_ZERO_MEMORY, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, UInt32 numElems, UInt32 elemsSize, const char* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapReAlloc(heap, 0, mem, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void Mem_Free(void* mem) {
	if (mem) HeapFree(heap, 0, mem);
}
#elif CC_BUILD_NIX
void* Mem_Alloc(UInt32 numElems, UInt32 elemsSize, const char* place) {
	void* ptr = malloc(numElems * elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(UInt32 numElems, UInt32 elemsSize, const char* place) {
	void* ptr = calloc(numElems, elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, UInt32 numElems, UInt32 elemsSize, const char* place) {
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
	char msgBuffer[512];
	String msg = String_FromArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(&msg);
}

void Platform_LogConst(const char* message) {
	String msg = String_FromReadonly(message);
	Platform_Log(&msg);
}

/* TODO: check this is actually accurate */
UInt64 sw_freqMul = 1, sw_freqDiv = 1;
Int32 Stopwatch_ElapsedMicroseconds(UInt64* timer) {
	UInt64 beg = *timer;
	Stopwatch_Measure(timer);
	UInt64 end = *timer;

	if (end < beg) return 0;
	UInt64 delta = ((end - beg) * sw_freqMul) / sw_freqDiv;
	return (Int32)delta;
}

#if CC_BUILD_WIN
void Platform_Log(const String* message) {
	/* TODO: log to console */
	OutputDebugStringA(message->buffer);
	OutputDebugStringA("\n");
}

#define FILETIME_EPOCH 50491123200000ULL
#define FileTime_TotalMS(time) ((time / 10000) + FILETIME_EPOCH)
TimeMS DateTime_CurrentUTC_MS(void) {
	FILETIME ft; GetSystemTimeAsFileTime(&ft);
	/* in 100 nanosecond units, since Jan 1 1601 */
	UInt64 raw = ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
	return FileTime_TotalMS(raw);
}

static void Platform_FromSysTime(DateTime* time, SYSTEMTIME* sysTime) {
	time->Year   = (UInt16)sysTime->wYear;
	time->Month  =  (UInt8)sysTime->wMonth;
	time->Day    =  (UInt8)sysTime->wDay;
	time->Hour   =  (UInt8)sysTime->wHour;
	time->Minute =  (UInt8)sysTime->wMinute;
	time->Second =  (UInt8)sysTime->wSecond;
	time->Milli  = (UInt16)sysTime->wMilliseconds;
}

void DateTime_CurrentUTC(DateTime* time) {
	SYSTEMTIME utcTime;
	GetSystemTime(&utcTime);
	Platform_FromSysTime(time, &utcTime);
}

void DateTime_CurrentLocal(DateTime* time) {
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	Platform_FromSysTime(time, &localTime);
}

bool sw_highRes;
void Stopwatch_Measure(UInt64* timer) {
	if (sw_highRes) {
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		*timer = (UInt64)t.QuadPart;
	} else {
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		*timer = (UInt64)ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
	}
}
#elif CC_BUILD_NIX
void Platform_Log(const String* message) {
	write(STDOUT_FILENO, message->buffer, message->length);
	write(STDOUT_FILENO, "\n",            1);
}

#define UNIX_EPOCH 62135596800000ULL
#define UnixTime_TotalMS(time) ((UInt64)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
TimeMS DateTime_CurrentUTC_MS(void) {
	struct timeval cur;
	gettimeofday(&cur, NULL);
	return UnixTime_TotalMS(cur);
}

static void Platform_FromSysTime(DateTime* time, struct tm* sysTime) {
	time->Year   = sysTime->tm_year + 1900;
	time->Month  = sysTime->tm_mon + 1;
	time->Day    = sysTime->tm_mday;
	time->Hour   = sysTime->tm_hour;
	time->Minute = sysTime->tm_min;
	time->Second = sysTime->tm_sec;
}

void DateTime_CurrentUTC(DateTime* time_) {
	struct timeval cur; struct tm utc_time;
	gettimeofday(&cur, NULL);
	gmtime_r(&cur.tv_sec, &utc_time);

	Platform_FromSysTime(time_, &utc_time);
	time_->Milli = cur.tv_usec / 1000;
}

void DateTime_CurrentLocal(DateTime* time_) {
	struct timeval cur; struct tm loc_time;
	gettimeofday(&cur, NULL);
	localtime_r(&cur.tv_sec, &loc_time);

	Platform_FromSysTime(time_, &loc_time);
	time_->Milli = cur.tv_usec / 1000;
}

#define NS_PER_SEC 1000000000ULL
void Stopwatch_Measure(UInt64* timer) {
	struct timespec t;
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &t);
	*timer = (UInt64)t.tv_sec * NS_PER_SEC + t.tv_nsec;
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
bool Directory_Exists(const String* path) {
	WCHAR str[300]; Platform_ConvertString(str, path);
	DWORD attribs = GetFileAttributesW(str);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Create(const String* path) {
	WCHAR str[300]; Platform_ConvertString(str, path);
	BOOL success = CreateDirectoryW(str, NULL);
	return Win_Return(success);
}

bool File_Exists(const String* path) {
	WCHAR str[300]; Platform_ConvertString(str, path);
	DWORD attribs = GetFileAttributesW(str);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Enum(const String* path, void* obj, Directory_EnumCallback callback) {
	char fileBuffer[MAX_PATH + 10];
	String file = String_FromArray(fileBuffer);
	/* Need to append \* to search for files in directory */
	String_Format1(&file, "%s\\*", path);
	WCHAR str[300]; Platform_ConvertString(str, &file);

	WIN32_FIND_DATAW entry;
	HANDLE find = FindFirstFileW(str, &entry);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		file.length = 0;
		Int32 i;

		for (i = 0; i < MAX_PATH && entry.cFileName[i]; i++) {
			String_Append(&file, Convert_UnicodeToCP437(entry.cFileName[i]));
		}

		Utils_UNSAFE_GetFilename(&file);
		callback(&file, obj);
	}  while (FindNextFileW(find, &entry));

	ReturnCode result = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return Win_Return(result == ERROR_NO_MORE_FILES);
}

ReturnCode File_GetModifiedTime_MS(const String* path, TimeMS* time) {
	void* file; ReturnCode result = File_Open(&file, path);
	if (result) return result;

	FILETIME ft;
	if (GetFileTime(file, NULL, NULL, &ft)) {
		UInt64 raw = ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
		*time = FileTime_TotalMS(raw);
	} else {
		result = GetLastError();
	}

	File_Close(file);
	return result;
}

ReturnCode File_Do(void** file, const String* path, DWORD access, DWORD createMode) {
	WCHAR str[300]; Platform_ConvertString(str, path);
	*file = CreateFileW(str, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return Win_Return(*file != INVALID_HANDLE_VALUE);
}

ReturnCode File_Open(void** file, const String* path) {
	return File_Do(file, path, GENERIC_READ, OPEN_EXISTING);
}
ReturnCode File_Create(void** file, const String* path) {
	return File_Do(file, path, GENERIC_WRITE, CREATE_ALWAYS);
}
ReturnCode File_Append(void** file, const String* path) {
	ReturnCode result = File_Do(file, path, GENERIC_WRITE, OPEN_ALWAYS);
	if (result) return result;
	return File_Seek(*file, 0, STREAM_SEEKFROM_END);
}

ReturnCode File_Read(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead) {
	BOOL success = ReadFile((HANDLE)file, buffer, count, bytesRead, NULL);
	return Win_Return(success);
}

ReturnCode File_Write(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWrote) {
	BOOL success = WriteFile((HANDLE)file, buffer, count, bytesWrote, NULL);
	return Win_Return(success);
}

ReturnCode File_Close(void* file) {
	return Win_Return(CloseHandle((HANDLE)file));
}

ReturnCode File_Seek(void* file, Int32 offset, Int32 seekType) {
	static UInt8 modes[3] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
	DWORD pos = SetFilePointer(file, offset, NULL, modes[seekType]);
	return Win_Return(pos != INVALID_SET_FILE_POINTER);
}

ReturnCode File_Position(void* file, UInt32* position) {
	*position = SetFilePointer(file, 0, NULL, 1); /* SEEK_CUR */
	return Win_Return(*position != INVALID_SET_FILE_POINTER);
}

ReturnCode File_Length(void* file, UInt32* length) {
	*length = GetFileSize(file, NULL);
	return Win_Return(*length != INVALID_FILE_SIZE);
}
#elif CC_BUILD_NIX
bool Directory_Exists(const String* path) {
	char str[600]; Platform_ConvertString(str, path);
	struct stat sb;
	return stat(str, &sb) == 0 && S_ISDIR(sb.st_mode);
}

ReturnCode Directory_Create(const String* path) {
	char str[600]; Platform_ConvertString(str, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return Nix_Return(mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1);
}

bool File_Exists(const String* path) {
	char str[600]; Platform_ConvertString(str, path);
	struct stat sb;
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

ReturnCode Directory_Enum(const String* path, void* obj, Directory_EnumCallback callback) {
	char str[600]; Platform_ConvertString(str, path);
	DIR* dirPtr = opendir(str);
	if (!dirPtr) return errno;

	char fileBuffer[FILENAME_SIZE];
	String file = String_FromArray(fileBuffer);
	struct dirent* entry;

	/* TODO: does this also include subdirectories */
	while (entry = readdir(dirPtr)) {
		UInt16 len = String_CalcLen(entry->d_name, UInt16_MaxValue);
		file.length = 0;
		String_DecodeUtf8(&file, entry->d_name, len);

		/* exlude . and .. paths */
		if (file.length == 1 && file.buffer[0] == '.')                          continue;
		if (file.length == 2 && file.buffer[0] == '.' && file.buffer[1] == '.') continue;

		Utils_UNSAFE_GetFilename(&file);
		callback(&file, obj);
	}

	int result = errno; /* return code from readdir */
	closedir(dirPtr);
	return result;
}

ReturnCode File_GetModifiedTime_MS(const String* path, TimeMS* time) {
	char str[600]; Platform_ConvertString(str, path);
	struct stat sb;
	if (stat(str, &sb) == -1) return errno;

	*time = (UInt64)sb.st_mtime * 1000 + UNIX_EPOCH;
	return 0;
}

ReturnCode File_Do(void** file, const String* path, int mode) {
	char str[600]; Platform_ConvertString(str, path);
	*file = open(str, mode, (6 << 6) | (4 << 3) | 4); /* rw|r|r */
	return Nix_Return(*file != -1);
}

ReturnCode File_Open(void** file, const String* path) {
	return File_Do(file, path, O_RDONLY);
}
ReturnCode File_Create(void** file, const String* path) {
	return File_Do(file, path, O_WRONLY | O_CREAT | O_TRUNC);
}
ReturnCode File_Append(void** file, const String* path) {
	ReturnCode result = File_Do(file, path, O_WRONLY | O_CREAT);
	if (result) return result;
	return File_Seek(*file, 0, STREAM_SEEKFROM_END);
}

ReturnCode File_Read(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead) {
	*bytesRead = read((int)file, buffer, count);
	return Nix_Return(*bytesRead != -1);
}

ReturnCode File_Write(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWrote) {
	*bytesWrote = write((int)file, buffer, count);
	return Nix_Return(*bytesWrote != -1);
}

ReturnCode File_Close(void* file) {
	return Nix_Return(close((int)file) != -1);
}

ReturnCode File_Seek(void* file, Int32 offset, Int32 seekType) {
	static UInt8 modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return Nix_Return(lseek((int)file, offset, modes[seekType]) != -1);
}

ReturnCode File_Position(void* file, UInt32* position) {
	*position = lseek((int)file, 0, SEEK_CUR);
	return Nix_Return(*position != -1);
}

ReturnCode File_Length(void* file, UInt32* length) {
	struct stat st;
	if (fstat((int)file, &st) == -1) { *length = -1; return errno; }
	*length = st.st_size; return 0;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Threading--------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
void Thread_Sleep(UInt32 milliseconds) { Sleep(milliseconds); }
DWORD WINAPI Thread_StartCallback(void* param) {
	Thread_StartFunc* func = (Thread_StartFunc*)param;
	(*func)();
	return 0;
}

void* Thread_Start(Thread_StartFunc* func, bool detach) {
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, Thread_StartCallback, func, 0, &threadID);
	if (!handle) {
		ErrorHandler_Fail2(GetLastError(), "Creating thread");
	}

	if (detach) Thread_Detach(handle);
	return handle;
}

void Thread_Detach(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_Fail2(GetLastError(), "Freeing thread handle");
	}
}

void Thread_Join(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
	Thread_Detach(handle);
}

CRITICAL_SECTION mutexList[3]; Int32 mutexIndex;
void* Mutex_Create(void) {
	if (mutexIndex == Array_Elems(mutexList)) ErrorHandler_Fail("Cannot allocate mutex");
	CRITICAL_SECTION* ptr = &mutexList[mutexIndex];
	InitializeCriticalSection(ptr); mutexIndex++;
	return ptr;
}

void Mutex_Free(void* handle)   { DeleteCriticalSection((CRITICAL_SECTION*)handle); }
void Mutex_Lock(void* handle)   { EnterCriticalSection((CRITICAL_SECTION*)handle); }
void Mutex_Unlock(void* handle) { LeaveCriticalSection((CRITICAL_SECTION*)handle); }

void* Waitable_Create(void) {
	void* handle = CreateEventW(NULL, false, false, NULL);
	if (!handle) {
		ErrorHandler_Fail2(GetLastError(), "Creating waitable");
	}
	return handle;
}

void Waitable_Free(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_Fail2(GetLastError(), "Freeing waitable");
	}
}

void Waitable_Signal(void* handle) { SetEvent((HANDLE)handle); }
void Waitable_Wait(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Waitable_WaitFor(void* handle, UInt32 milliseconds) {
	WaitForSingleObject((HANDLE)handle, milliseconds);
}
#elif CC_BUILD_NIX
void Thread_Sleep(UInt32 milliseconds) { usleep(milliseconds * 1000); }
void* Thread_StartCallback(void* lpParam) {
	Thread_StartFunc* func = (Thread_StartFunc*)lpParam;
	(*func)();
	return NULL;
}

pthread_t threadList[3]; Int32 threadIndex;
void* Thread_Start(Thread_StartFunc* func, bool detach) {
	if (threadIndex == Array_Elems(threadList)) ErrorHandler_Fail("Cannot allocate thread");
	pthread_t* ptr = &threadList[threadIndex];
	int result = pthread_create(ptr, NULL, Thread_StartCallback, func);
	ErrorHandler_CheckOrFail(result, "Creating thread");

	if (detach) Thread_Detach(ptr);
	threadIndex++; return ptr;
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = handle;
	int result = pthread_detach(*ptr);
	ErrorHandler_CheckOrFail(result, "Detaching thread");
}

void Thread_Join(void* handle) {
	pthread_t* ptr = handle;
	int result = pthread_join(*ptr, NULL);
	ErrorHandler_CheckOrFail(result, "Joining thread");
}

pthread_mutex_t mutexList[3]; Int32 mutexIndex;
void* Mutex_Create(void) {
	if (mutexIndex == Array_Elems(mutexList)) ErrorHandler_Fail("Cannot allocate mutex");
	pthread_mutex_t* ptr = &mutexList[mutexIndex];
	int result = pthread_mutex_init(ptr, NULL);

	ErrorHandler_CheckOrFail(result, "Creating mutex");
	mutexIndex++; return ptr;
}

void Mutex_Free(void* handle) {
	int result = pthread_mutex_destroy((pthread_mutex_t*)handle);
	ErrorHandler_CheckOrFail(result, "Destroying mutex");
}

void Mutex_Lock(void* handle) {
	int result = pthread_mutex_lock((pthread_mutex_t*)handle);
	ErrorHandler_CheckOrFail(result, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int result = pthread_mutex_unlock((pthread_mutex_t*)handle);
	ErrorHandler_CheckOrFail(result, "Unlocking mutex");
}

pthread_cond_t condList[2]; Int32 condIndex;
void* Waitable_Create(void) {
	if (condIndex == Array_Elems(condList)) ErrorHandler_Fail("Cannot allocate event");
	pthread_cond_t* ptr = &condList[condIndex];
	int result = pthread_cond_init(ptr, NULL);

	ErrorHandler_CheckOrFail(result, "Creating event");
	condIndex++; return ptr;
}

void Waitable_Free(void* handle) {
	int result = pthread_cond_destroy((pthread_cond_t*)handle);
	ErrorHandler_CheckOrFail(result, "Destroying event");
}

void Waitable_Signal(void* handle) {
	int result = pthread_cond_signal((pthread_cond_t*)handle);
	ErrorHandler_CheckOrFail(result, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int result = pthread_cond_wait((pthread_cond_t*)handle, &event_mutex);
	ErrorHandler_CheckOrFail(result, "Waiting event");
}

void Waitable_WaitFor(void* handle, UInt32 milliseconds) {
	struct timeval tv;
	struct timespec ts;
	gettimeofday(&tv, NULL);

	ts.tv_sec = tv.tv_sec + milliseconds / 1000;
	ts.tv_nsec = 1000 * (tv.tv_usec + 1000 * (milliseconds % 1000));
	ts.tv_sec += ts.tv_nsec / NS_PER_SEC;
	ts.tv_nsec %= NS_PER_SEC;

	int result = pthread_cond_timedwait((pthread_cond_t*)handle, &event_mutex, &ts);
	if (result == ETIMEDOUT) return;
	ErrorHandler_CheckOrFail(result, "Waiting timed event");
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
#include "freetype\ftsnames.h"
FT_Library lib;
StringsBuffer fonts_list;
static void Font_Init(void);

#define DPI_PIXEL  72
#define DPI_DEVICE 96 /* TODO: GetDeviceCaps(hdc, LOGPIXELSY) */

static Int32 Font_Find(const String* name) {
	Int32 i;
	for (i = 1; i < fonts_list.Count; i += 2) {
		String faceName = StringsBuffer_UNSAFE_Get(&fonts_list, i);
		if (String_CaselessEquals(&faceName, name)) return i;
	}
	return -1;
}

void Font_GetNames(StringsBuffer* buffer) {
	if (!fonts_list.Count) Font_Init();

	Int32 i;
	for (i = 1; i < fonts_list.Count; i += 2) {
		String faceName = StringsBuffer_UNSAFE_Get(&fonts_list, i);
		StringsBuffer_Add(buffer, &faceName);
	}
}

void Font_Make(FontDesc* desc, const String* fontName, UInt16 size, UInt16 style) {
	desc->Size  = size;
	desc->Style = style;
	
	if (!fonts_list.Count) Font_Init();
	Int32 idx = Font_Find(fontName);
	if (idx == -1) ErrorHandler_Fail("Unknown font");
	
	char pathBuffer[FILENAME_SIZE + 1];
	String path = String_NT_Array(pathBuffer);
	StringsBuffer_Get(&fonts_list, idx - 1, &path);
	path.buffer[path.length] = '\0';

	FT_Face face;
	FT_Error err = FT_New_Face(lib, path.buffer, 0, &face);
	if (err) ErrorHandler_Fail2(err, "Creating font failed");
	desc->Handle = face;

	err = FT_Set_Char_Size(face, size * 64, 0, DPI_DEVICE, 0); /* TODO: Check error */
	if (err) ErrorHandler_Fail2(err, "Resizing font failed");
}

void Font_Free(FontDesc* desc) {
	desc->Size  = 0;
	desc->Style = 0;
	/* NULL for fonts created by Drawer2D_MakeFont and bitmapped text mode is on */
	if (!desc->Handle) return;

	FT_Face face = desc->Handle;
	FT_Error err = FT_Done_Face(face);
	if (err) ErrorHandler_Fail2(err, "Deleting font failed");
	desc->Handle = NULL;
}

static void Font_DirCallback(const String* filename, void* obj) {
	char pathBuffer[MAX_PATH * 5 + 1];
	String path = String_NT_Array(pathBuffer);
	String* dir = obj;

	String_Format2(&path, "%s%s", dir, filename);
	path.buffer[path.length] = '\0';

	FT_Face face;
	FT_Error error = FT_New_Face(lib, path.buffer, 0, &face);
	if (error) return;

	bool styled = (face->style_flags & FT_STYLE_FLAG_BOLD) || (face->style_flags & FT_STYLE_FLAG_ITALIC);
	if (!styled) {
		StringsBuffer_Add(&fonts_list, &path);
		path.length = 0;

		String_AppendConst(&path, face->family_name);
		/* don't want 'Arial Regular' */
		if (face->style_name) {
			String style = String_FromReadonly(face->style_name);
			if (!String_CaselessEqualsConst(&style, "Regular")) {
				String_Format1(&path, " %c", face->style_name);
			}
		}

		Platform_Log1("Face: %s", &path);
		StringsBuffer_Add(&fonts_list, &path);
	}
	FT_Done_Face(face);
}

#define TEXT_CEIL(x) (((x) + 63) >> 6)
Size2D Platform_TextMeasure(struct DrawTextArgs* args) {
	FT_Face face = args->Font.Handle;
	String text = args->Text;
	Size2D s = { 0, face->height };
	Int32 i;

	for (i = 0; i < text.length; i++) {
		UInt16 c = Convert_CP437ToUnicode(text.buffer[i]);
		FT_Load_Char(face, c, 0); /* TODO: Check error */
		s.Width += face->glyph->advance.x;
	}

	s.Width  = TEXT_CEIL(s.Width);
	s.Height = TEXT_CEIL(s.Height);
	return s;
}

Size2D Platform_TextDraw(struct DrawTextArgs* args, Bitmap* bmp, Int32 x, Int32 y, PackedCol col) {
	FT_Face face = args->Font.Handle;
	String text = args->Text;
	Size2D s = { 0, face->height };
	Int32 i;

	for (i = 0; i < text.length; i++) {
		UInt16 c = Convert_CP437ToUnicode(text.buffer[i]);
		FT_Load_Char(face, c, FT_LOAD_RENDER); /* TODO: Check error */

		FT_Bitmap* img = &face->glyph->bitmap;
		Int32 xx, yy, offset = TEXT_CEIL(s.Height) + TEXT_CEIL(face->descender) - face->glyph->bitmap_top;
		y += offset;

		for (yy = 0; yy < img->rows; yy++) {
			if ((y + yy) < 0|| (y + yy) >= bmp->Height) continue;
			UInt8* src = img->buffer + (yy * img->width);
			UInt8* dst = (UInt8*)Bitmap_GetRow(bmp, y + yy) + (x * BITMAP_SIZEOF_PIXEL);

			for (xx = 0; xx < img->width; xx++) {
				if ((x + xx) < 0 || (x + xx) >= bmp->Width) continue;

				UInt8 intensity = *src, invIntensity = UInt8_MaxValue - intensity;
				dst[0] = ((col.B * intensity) >> 8) + ((dst[0] * invIntensity) >> 8);
				dst[1] = ((col.G * intensity) >> 8) + ((dst[1] * invIntensity) >> 8);
				dst[2] = ((col.R * intensity) >> 8) + ((dst[2] * invIntensity) >> 8);
				//dst[3] = ((col.A * intensity) >> 8) + ((dst[3] * invIntensity) >> 8);
				dst[3] = intensity + ((dst[3] * invIntensity) >> 8);
				src++; dst += BITMAP_SIZEOF_PIXEL;
			}
		}
		x += TEXT_CEIL(face->glyph->advance.x);
		y -= offset;
	}

	s.Width  = TEXT_CEIL(s.Width);
	s.Height = TEXT_CEIL(s.Height);
	return s;
}

#if CC_BUILD_WIN
static void Font_Init(void) {
	FT_Error err = FT_Init_FreeType(&lib);
	String dir   = String_FromConst("C:\\Windows\\fonts\\");
	Directory_Enum(&dir, &dir, Font_DirCallback);
}
#elif CC_BUILD_NIX
#endif


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
void Socket_Create(SocketPtr* socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		ErrorHandler_Fail2(Socket__Error(), "Failed to create socket");
	}
}

static ReturnCode Socket_ioctl(SocketPtr socket, UInt32 cmd, Int32* data) {
#if CC_BUILD_WIN
	return ioctlsocket(socket, cmd, data);
#else
	return ioctl(socket, cmd, data);
#endif
}

ReturnCode Socket_Available(SocketPtr socket, UInt32* available) {
	return Socket_ioctl(socket, FIONREAD, available);
}
ReturnCode Socket_SetBlocking(SocketPtr socket, bool blocking) {
	Int32 blocking_raw = blocking ? 0 : -1;
	return Socket_ioctl(socket, FIONBIO, &blocking_raw);
}

ReturnCode Socket_GetError(SocketPtr socket, ReturnCode* result) {
	Int32 resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Socket_Connect(SocketPtr socket, const String* ip, Int32 port) {
	struct RAW_IPV4_ADDR { Int16 Family; UInt8 Port[2], IP[4], Pad[8]; } addr;
	addr.Family = AF_INET;

	Stream_SetU16_BE(addr.Port, port);
	Utils_ParseIP(ip, addr.IP);

	ReturnCode result = connect(socket, (struct sockaddr*)(&addr), sizeof(addr));
	return result == -1 ? Socket__Error() : 0;
}

ReturnCode Socket_Read(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 recvCount = recv(socket, buffer, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Socket_Write(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 sentCount = send(socket, buffer, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Socket_Close(SocketPtr socket) {
	ReturnCode result = 0;
#if CC_BUILD_WIN
	ReturnCode result1 = shutdown(socket, SD_BOTH);
#else
	ReturnCode result1 = shutdown(socket, SHUT_RDWR);
#endif
	if (result1 == -1) result = Socket__Error();

#if CC_BUILD_WIN
	ReturnCode result2 = closesocket(socket);
#else
	ReturnCode result2 = close(socket);
#endif
	if (result2 == -1) result = Socket__Error();
	return result;
}

ReturnCode Socket_Select(SocketPtr socket, Int32 selectMode, bool* success) {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(socket, &set);

	struct timeval time = { 0 };
	Int32 selectCount = -1;

	#if CC_BUILD_WIN
	int nfds = 1;
	#else
	int nfds = socket + 1;
	#endif

	if (selectMode == SOCKET_SELECT_READ) {
		selectCount = select(nfds, &set, NULL, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_WRITE) {
		selectCount = select(nfds, NULL, &set, NULL, &time);
	}

	if (selectCount == -1) { *success = false; return Socket__Error(); }
#if CC_BUILD_WIN
	*success = set.fd_count != 0; return 0;
#else
	*success = FD_ISSET(socket, &set); return 0;
#endif
}


/*########################################################################################################################*
*----------------------------------------------------------Http-----------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
HINTERNET hInternet;
/* TODO: Test last modified and etag even work */
#define FLAG_STATUS  HTTP_QUERY_STATUS_CODE    | HTTP_QUERY_FLAG_NUMBER
#define FLAG_LENGTH  HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER
#define FLAG_LASTMOD HTTP_QUERY_LAST_MODIFIED  | HTTP_QUERY_FLAG_SYSTEMTIME

void Http_Init(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(PROGRAM_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) ErrorHandler_Fail2(GetLastError(), "Failed to init WinINet");
}

static ReturnCode Http_Make(struct AsyncRequest* req, HINTERNET* handle) {
	String url = String_FromRawArray(req->URL);
	char urlStr[STRING_SIZE + 1];
	Mem_Copy(urlStr, url.buffer, url.length);

	urlStr[url.length] = '\0';
	char headersBuffer[STRING_SIZE * 3];
	String headers = String_FromArray(headersBuffer);

	/* https://stackoverflow.com/questions/25308488/c-wininet-custom-http-headers */
	if (req->Etag[0] || req->LastModified) {
		if (req->LastModified) {
			String_AppendConst(&headers, "If-Modified-Since: ");
			DateTime_HttpDate(req->LastModified, &headers);
			String_AppendConst(&headers, "\r\n");
		}

		if (req->Etag[0]) {
			String etag = String_FromRawArray(req->Etag);
			String_AppendConst(&headers, "If-None-Match: ");
			String_AppendString(&headers, &etag);
			String_AppendConst(&headers, "\r\n");
		}

		String_AppendConst(&headers, "\r\n\r\n");
		headers.buffer[headers.length] = '\0';
	} else { headers.buffer = NULL; }

	*handle = InternetOpenUrlA(hInternet, urlStr, headers.buffer, headers.length,
		INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0);
	return Win_Return(*handle);
}

static ReturnCode Http_GetHeaders(struct AsyncRequest* req, HINTERNET handle) {
	DWORD len;

	len = sizeof(DWORD);
	if (!HttpQueryInfoA(handle, FLAG_STATUS, &req->StatusCode, &len, NULL)) return GetLastError();

	len = sizeof(DWORD);
	HttpQueryInfoA(handle, FLAG_LENGTH, &req->ResultSize, &len, NULL);

	SYSTEMTIME sysTime;
	len = sizeof(SYSTEMTIME);
	if (HttpQueryInfoA(handle, FLAG_LASTMOD, &sysTime, &len, NULL)) {
		DateTime time;
		Platform_FromSysTime(&time, &sysTime);
		req->LastModified = DateTime_TotalMs(&time);
	}

	String etag = String_ClearedArray(req->Etag);
	len = etag.capacity;
	HttpQueryInfoA(handle, HTTP_QUERY_ETAG, etag.buffer, &len, NULL);

	return 0;
}

static ReturnCode Http_GetData(struct AsyncRequest* req, HINTERNET handle, volatile Int32* progress) {
	UInt32 size = req->ResultSize;
	if (!size) return ERROR_NOT_SUPPORTED;
	*progress = 0;

	UInt8* buffer = Mem_Alloc(size, sizeof(UInt8), "http get data");
	UInt32 left = size, read, totalRead = 0;
	req->ResultData = buffer;

	while (left) {
		UInt32 toRead = left, avail = 0;
		/* only read as much data that is pending */
		if (InternetQueryDataAvailable(handle, &avail, 0, 0)) {
			toRead = min(toRead, avail);
		}

		bool success = InternetReadFile(handle, buffer, toRead, &read);
		if (!success) { Mem_Free(buffer); return GetLastError(); }

		if (!read) break;
		buffer += read; totalRead += read; left -= read;
		*progress = (Int32)(100.0f * totalRead / size);
	}

	*progress = 100;
	return 0;
}

ReturnCode Http_Do(struct AsyncRequest* req, volatile Int32* progress) {
	HINTERNET handle;
	ReturnCode res = Http_Make(req, &handle);
	if (res) return res;

	*progress = ASYNC_PROGRESS_FETCHING_DATA;
	res = Http_GetHeaders(req, handle);
	if (res) { InternetCloseHandle(handle); return res; }

	if (req->RequestType != REQUEST_TYPE_CONTENT_LENGTH && req->StatusCode == 200) {
		res = Http_GetData(req, handle, progress);
		if (res) { InternetCloseHandle(handle); return res; }
	}

	return Win_Return(InternetCloseHandle(handle));
}

ReturnCode Http_Free(void) { return Win_Return(InternetCloseHandle(hInternet)); }
#elif CC_BUILD_NIX
CURL* curl;

void Http_Init(void) {
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) ErrorHandler_Fail2(res, "Failed to init curl");

	curl = curl_easy_init();
	if (!curl) ErrorHandler_Fail("Failed to init easy curl");
}

static int Http_Progress(Int32* progress, double total, double received, double a, double b) {
	if (total == 0) return 0;
	*progress = (Int32)(100 * received / total);
	return 0;
}

static struct curl_slist* Http_Make(struct AsyncRequest* req) {
	struct curl_slist* list = NULL;
	char buffer1[STRING_SIZE + 1] = { 0 };
	char buffer2[STRING_SIZE + 1] = { 0 };

	if (req->Etag[0]) {
		String tmp = { buffer1, 0, STRING_SIZE };
		String_AppendConst(&tmp, "If-None-Match: ");

		String etag = String_FromRawArray(req->Etag);
		String_AppendString(&tmp, &etag);
		list = curl_slist_append(list, tmp.buffer);
	}

	if (req->LastModified) {
		String tmp = { buffer2, 0, STRING_SIZE };
		String_AppendConst(&tmp, "Last-Modified: ");

		DateTime_HttpDate(req->LastModified, &tmp);
		list = curl_slist_append(list, tmp.buffer);
	}
	return list;
}

static size_t Http_GetHeaders(char *buffer, size_t size, size_t nitems, struct AsyncRequest* req) {
	size_t total = size * nitems;
	if (size != 1) return total; /* non byte header */
	String line = String_Init(buffer, nitems, nitems), name, value;
	if (!String_UNSAFE_Separate(&line, ':', &name, &value)) return total;

	/* value usually has \r\n at end */
	if (value.length && value.buffer[value.length - 1] == '\n') value.length--;
	if (value.length && value.buffer[value.length - 1] == '\r') value.length--;
	if (!value.length) return total;

	if (String_CaselessEqualsConst(&name, "ETag")) {
		String etag = String_ClearedArray(req->Etag);
		String_AppendString(&etag, &value);
	} else if (String_CaselessEqualsConst(&name, "Content-Length")) {
		Convert_TryParseInt32(&value, &req->ResultSize);
	} else if (String_CaselessEqualsConst(&name, "Last-Modified")) {
		char tmpBuffer[STRING_SIZE + 1] = { 0 };
		String tmp = { tmpBuffer, 0, STRING_SIZE };
		String_AppendString(&tmp, &value);

		time_t time = curl_getdate(tmp.buffer, NULL);
		if (time == -1) return total;
		req->LastModified = (UInt64)time * 1000 + UNIX_EPOCH;
	}
	return total;
}

static size_t Http_GetData(char *buffer, size_t size, size_t nitems, struct AsyncRequest* req) {
	UInt32 total = req->ResultSize;
	if (!total || req->RequestType == REQUEST_TYPE_CONTENT_LENGTH) return 0;
	if (!req->ResultData) req->ResultData = Mem_Alloc(total, 1, "http get data");

	/* reuse Result as an offset */
	UInt32 left = total - req->Result;
	left        = min(left, nitems);

	UInt8* dst = (UInt8*)req->ResultData + req->Result;
	Mem_Copy(dst, buffer, left);
	req->Result += left;

	return nitems;
}

ReturnCode Http_Do(struct AsyncRequest* req, volatile Int32* progress) {
	curl_easy_reset(curl);
	String url = String_FromRawArray(req->URL);
	char urlStr[600]; Platform_ConvertString(urlStr, &url);

	struct curl_slist* list = Http_Make(req);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

	curl_easy_setopt(curl, CURLOPT_URL,            urlStr);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,      PROGRAM_APP_NAME);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, Http_Progress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,     progress);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_GetHeaders);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_GetData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);

	*progress = ASYNC_PROGRESS_FETCHING_DATA;
	CURLcode res = curl_easy_perform(curl);
	*progress = 100;

	long status = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	req->StatusCode = status;

	curl_slist_free_all(list);
	return res;
}

ReturnCode Http_Free(void) {
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------Audio----------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Audio_AllCompleted(AudioHandle handle, bool* finished);
#if CC_BUILD_WIN
struct AudioContext {
	HWAVEOUT Handle;
	WAVEHDR Headers[AUDIO_MAX_CHUNKS];
	struct AudioFormat Format;
	Int32 Count;
};
struct AudioContext Audio_Contexts[20];

void Audio_Init(AudioHandle* handle, Int32 buffers) {
	Int32 i, j;
	for (i = 0; i < Array_Elems(Audio_Contexts); i++) {
		struct AudioContext* ctx = &Audio_Contexts[i];
		if (ctx->Count) continue;
	
		for (j = 0; j < buffers; j++) {
			ctx->Headers[j].dwFlags = WHDR_DONE;
		}

		*handle    = i;
		ctx->Count = buffers;
		return;
	}
	ErrorHandler_Fail("No free audio contexts");
}

ReturnCode Audio_Free(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	if (!ctx->Count) return 0;

	ReturnCode res = waveOutClose(ctx->Handle);
	ctx->Handle = NULL;
	ctx->Count  = 0;

	struct AudioFormat fmt = { 0 };
	ctx->Format = fmt;
	return res;
}

ReturnCode Audio_SetFormat(AudioHandle handle, struct AudioFormat* format) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	struct AudioFormat*  cur = &ctx->Format;
	if (AudioFormat_Eq(cur, format)) return 0;

	ReturnCode res;
	if (ctx->Handle && (res = waveOutClose(ctx->Handle))) return res;

	Int32 sampleSize = format->Channels * format->BitsPerSample / 8;
	WAVEFORMATEX fmt;
	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = format->Channels;
	fmt.nSamplesPerSec  = format->SampleRate;
	fmt.nAvgBytesPerSec = format->SampleRate * sampleSize;
	fmt.nBlockAlign     = sampleSize;
	fmt.wBitsPerSample  = format->BitsPerSample;
	fmt.cbSize          = 0;

	ctx->Format = *format;
	return waveOutOpen(&ctx->Handle, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
}

ReturnCode Audio_BufferData(AudioHandle handle, Int32 idx, void* data, UInt32 dataSize) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];
	Mem_Set(hdr, 0, sizeof(WAVEHDR));

	hdr->lpData         = data;
	hdr->dwBufferLength = dataSize;
	hdr->dwLoops        = 1;

	ReturnCode res;
	if ((res = waveOutPrepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR)))) return res;
	if ((res = waveOutWrite(ctx->Handle, hdr, sizeof(WAVEHDR))))         return res;
	return 0;
}

ReturnCode Audio_Play(AudioHandle handle) { return 0; }

ReturnCode Audio_Stop(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	return waveOutReset(ctx->Handle);
}

ReturnCode Audio_IsCompleted(AudioHandle handle, Int32 idx, bool* completed) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];

	*completed = false;
	if (!(hdr->dwFlags & WHDR_DONE)) return 0;
	ReturnCode res = 0;

	if (hdr->dwFlags & WHDR_PREPARED) {
		res = waveOutUnprepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR));
	}
	*completed = true; return res;
}

ReturnCode Audio_IsFinished(AudioHandle handle, bool* finished) { return Audio_AllCompleted(handle, finished); }
#elif CC_BUILD_NIX
struct AudioContext {
	ALuint Source;
	ALuint Buffers[AUDIO_MAX_CHUNKS];
	bool Completed[AUDIO_MAX_CHUNKS];
	struct AudioFormat Format;
	Int32 Count;
	ALenum DataFormat;
};
struct AudioContext Audio_Contexts[20];

pthread_mutex_t audio_lock;
ALCdevice* audio_device;
ALCcontext* audio_context;
volatile int audio_refs;

static void Audio_CheckContextErrors(void) {
	ALenum err = alcGetError(audio_device);
	if (err) ErrorHandler_Fail2(err, "Error creating OpenAL context");
}

static void Audio_CreateContext(void) {
	audio_device = alcOpenDevice(NULL);
	if (!audio_device) ErrorHandler_Fail("Failed to create OpenAL device");
	Audio_CheckContextErrors();

	audio_context = alcCreateContext(audio_device, NULL);
	if (!audio_context) {
		alcCloseDevice(audio_device);
		ErrorHandler_Fail("Failed to create OpenAL context");
	}
	Audio_CheckContextErrors();

	alcMakeContextCurrent(audio_context);
	Audio_CheckContextErrors();
}

static void Audio_DestroyContext(void) {
	if (!audio_device) return;
	alcMakeContextCurrent(NULL);

	if (audio_context) alcDestroyContext(audio_context);
	if (audio_device)  alcCloseDevice(audio_device);

	audio_context = NULL;
	audio_device  = NULL;
}

static ALenum Audio_FreeSource(struct AudioContext* ctx) {
	if (ctx->Source == -1) return 0;
	ALenum err;

	alDeleteSources(1, &ctx->Source);
	ctx->Source = -1;
	if ((err = alGetError())) return err;

	alDeleteBuffers(ctx->Count, ctx->Buffers);
	if ((err = alGetError())) return err;
	return 0;
}

void Audio_Init(AudioHandle* handle, Int32 buffers) {
	Mutex_Lock(&audio_lock);
	{
		if (!audio_context) Audio_CreateContext();
		audio_refs++;
	}
	Mutex_Unlock(&audio_lock);

	alDistanceModel(AL_NONE);
	ALenum err = alGetError();
	if (err) { ErrorHandler_Fail2(err, "DistanceModel"); }

	Int32 i, j;
	for (i = 0; i < Array_Elems(Audio_Contexts); i++) {
		struct AudioContext* ctx = &Audio_Contexts[i];
		if (ctx->Count) continue;

		for (j = 0; j < buffers; j++) {
			ctx->Completed[j] = true;
		}

		*handle     = i;
		ctx->Count  = buffers;
		ctx->Source = -1;
		return;
	}
	ErrorHandler_Fail("No free audio contexts");
}

ReturnCode Audio_Free(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	if (!ctx->Count) return 0;

	ctx->Count  = 0;
	struct AudioFormat fmt = { 0 };
	ctx->Format = fmt;

	ALenum err = Audio_FreeSource(ctx);
	if (err) return err;

	Mutex_Lock(&audio_lock);
	{
		audio_refs--;
		if (audio_refs == 0) Audio_DestroyContext();
	}
	Mutex_Unlock(&audio_lock);
	return 0;
}

static ALenum GetALFormat(Int32 channels, Int32 bitsPerSample) {
    if (bitsPerSample == 16) {
        if (channels == 1) return AL_FORMAT_MONO16;
        if (channels == 2) return AL_FORMAT_STEREO16;
	} else if (bitsPerSample == 8) {
        if (channels == 1) return AL_FORMAT_MONO8;
        if (channels == 2) return AL_FORMAT_STEREO8;
	}
	ErrorHandler_Fail("Unsupported audio format"); return 0;
}

ReturnCode Audio_SetFormat(AudioHandle handle, struct AudioFormat* format) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	struct AudioFormat*  cur = &ctx->Format;
	if (AudioFormat_Eq(cur, format)) return 0;

	ctx->DataFormat = GetALFormat(format->Channels, format->BitsPerSample);
	ctx->Format     = *format;

	ALenum err;
	if ((err = Audio_FreeSource(ctx))) return err;
	alGenSources(1, &ctx->Source);
	if ((err = alGetError())) return err;

	alGenBuffers(ctx->Count, ctx->Buffers);
	if ((err = alGetError())) return err;
	return 0;
}

ReturnCode Audio_BufferData(AudioHandle handle, Int32 idx, void* data, UInt32 dataSize) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	ALuint buffer = ctx->Buffers[idx];
	ctx->Completed[idx] = false;
	ALenum err;

	alBufferData(buffer, ctx->DataFormat, data, dataSize, ctx->Format.SampleRate);
	if ((err = alGetError())) return err;
	alSourceQueueBuffers(ctx->Source, 1, &buffer);
	if ((err = alGetError())) return err;
	return 0;
}

ReturnCode Audio_Play(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	alSourcePlay(ctx->Source);
	return alGetError();
}

ReturnCode Audio_Stop(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	alSourceStop(ctx->Source);
	return alGetError();
}

ReturnCode Audio_IsCompleted(AudioHandle handle, Int32 idx, bool* completed) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	ALint i, processed = 0;
	ALuint buffer;
	ALenum err;

	alGetSourcei(ctx->Source, AL_BUFFERS_PROCESSED, &processed);
	if ((err = alGetError())) return err;

	if (processed > 0) {
		alSourceUnqueueBuffers(ctx->Source, 1, &buffer);
		if ((err = alGetError())) return err;

		for (i = 0; i < ctx->Count; i++) {
			if (ctx->Buffers[i] == buffer) ctx->Completed[i] = true;
		}
	}
	*completed = ctx->Completed[idx]; return 0;
}

ReturnCode Audio_IsFinished(AudioHandle handle, bool* finished) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	if (ctx->Source == -1) { *finished = true; return 0; }

	ReturnCode res = Audio_AllCompleted(handle, finished);
	if (res) return res;

	ALint state = 0;
	alGetSourcei(ctx->Source, AL_SOURCE_STATE, &state);
	*finished = state != AL_PLAYING; return 0;
}
#endif
static ReturnCode Audio_AllCompleted(AudioHandle handle, bool* finished) {
	struct AudioContext* ctx = &Audio_Contexts[handle];	
	ReturnCode res;
	*finished = false;
	Int32 i;

	for (i = 0; i < ctx->Count; i++) {
		res = Audio_IsCompleted(handle, i, finished);
		if (res) return res;
		if (!(*finished)) return 0;
	}

	*finished = true;
	return 0;
}

struct AudioFormat* Audio_GetFormat(AudioHandle handle) {
	return &Audio_Contexts[handle].Format;
}

ReturnCode Audio_StopAndFree(AudioHandle handle) {
	Audio_Stop(handle);
	bool finished;
	Audio_IsFinished(handle, &finished); /* unqueue buffers */
	return Audio_Free(handle);
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
void Platform_ConvertString(void* dstPtr, const String* src) {
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");
	WCHAR* dst = dstPtr;

	Int32 i;
	for (i = 0; i < src->length; i++) {
		*dst = Convert_CP437ToUnicode(src->buffer[i]); dst++;
	}
	*dst = '\0';
}

static void Platform_InitDisplay(void) {
	HDC hdc = GetDC(NULL);
	struct DisplayDevice device = { 0 };

	device.Bounds.Width   = GetSystemMetrics(SM_CXSCREEN);
	device.Bounds.Height  = GetSystemMetrics(SM_CYSCREEN);
	device.BitsPerPixel   = GetDeviceCaps(hdc, BITSPIXEL);
	DisplayDevice_Default = device;

	ReleaseDC(NULL, hdc);
}

void Platform_Init(void) {
	Platform_InitDisplay();
	heap = GetProcessHeap(); /* TODO: HeapCreate instead? probably not */
	hdc = CreateCompatibleDC(NULL);
	if (!hdc) ErrorHandler_Fail("Failed to get screen DC");

	SetTextColor(hdc, 0x00FFFFFF);
	SetBkColor(hdc, 0x00000000);
	SetBkMode(hdc, OPAQUE);

	LARGE_INTEGER freq;
	sw_highRes = QueryPerformanceFrequency(&freq);
	if (sw_highRes) {
		sw_freqMul = 1000 * 1000;
		sw_freqDiv = freq.QuadPart;
	} else { sw_freqDiv = 10; }

	WSADATA wsaData;
	ReturnCode wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	ErrorHandler_CheckOrFail(wsaResult, "WSAStartup failed");
}

void Platform_Free(void) {
	DeleteDC(hdc);
	WSACleanup();
	HeapDestroy(heap);
}

void Platform_SetWorkingDir(void) {
	WCHAR dirName[FILENAME_SIZE + 1];
	DWORD len = GetModuleFileNameW(NULL, dirName, FILENAME_SIZE);
	if (!len) return;

	/* get rid of filename at end of directory */
	for (; len > 0; len--) {
		if (dirName[len] == '/' || dirName[len] == '\\') break;
	}

	dirName[len] = '\0';
	SetCurrentDirectoryW(dirName);
}

void Platform_Exit(ReturnCode code) { ExitProcess(code); }

ReturnCode Platform_StartShell(const String* args) {
	WCHAR str[300]; Platform_ConvertString(str, args);
	HINSTANCE instance = ShellExecuteW(NULL, NULL, str, NULL, NULL, SW_SHOWNORMAL);
	return instance > 32 ? 0 : (ReturnCode)instance;
}

static String Platform_NextArg(STRING_REF String* args) {
	/* get rid of leading spaces before arg */
	while (args->length && args->buffer[0] == ' ') {
		*args = String_UNSAFE_SubstringAt(args, 1);
	}

	Int32 end;
	if (args->length && args->buffer[0] == '"') {
		/* "xy za" is used for arg with spaces */
		*args = String_UNSAFE_SubstringAt(args, 1);
		end = String_IndexOf(args, '"', 0);
	} else {
		end = String_IndexOf(args, ' ', 0);
	}

	String arg;
	if (end == -1) {
		arg   = *args;
		args->length = 0;
	} else {
		arg   = String_UNSAFE_Substring(args, 0, end);
		*args = String_UNSAFE_SubstringAt(args, end + 1);
	}
	return arg;
}

Int32 Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args) {
	String cmdArgs = String_FromReadonly(GetCommandLineA());
	Platform_NextArg(&cmdArgs); /* skip exe path */

	Int32 count;
	for (count = 0; count < PROGRAM_MAX_CMDARGS; count++) {
		args[count] = Platform_NextArg(&cmdArgs);

		if (!args[count].length) break;
	}
	return count;
}
#elif CC_BUILD_NIX
void Platform_ConvertString(void* dstPtr, const String* src) {
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");
	UInt8* dst = dstPtr;

	Int32 i;
	for (i = 0; i < src->length; i++) {
		UInt16 codepoint = Convert_CP437ToUnicode(src->buffer[i]);
		Int32 len = Stream_WriteUtf8(dst, codepoint); dst += len;
	}
	*dst = '\0';
}

static void Platform_InitDisplay(void) {
	Display* display = XOpenDisplay(NULL);
	if (!display) ErrorHandler_Fail("Failed to open display");

	int screen = XDefaultScreen(display);
	Window rootWin = XRootWindow(display, screen);
	sw_freqDiv = 1000;

	/* TODO: Use Xinerama and XRandR for querying these */
	struct DisplayDevice device = { 0 };
	device.Bounds.Width   = DisplayWidth(display,  screen);
	device.Bounds.Height  = DisplayHeight(display, screen);
	device.BitsPerPixel   = DefaultDepth(display,  screen);
	DisplayDevice_Default = device;

	DisplayDevice_Meta[0] = display;
	DisplayDevice_Meta[1] = screen;
	DisplayDevice_Meta[2] = rootWin;
}

void Platform_Init(void) {
	Platform_InitDisplay();
	pthread_mutex_init(&event_mutex, NULL);
	pthread_mutex_init(&audio_lock,  NULL);
}

void Platform_Free(void) {
	pthread_mutex_destroy(&event_mutex);
	pthread_mutex_destroy(&audio_lock);
}

void Platform_SetWorkingDir(void) {
	char path[FILENAME_SIZE + 1] = { 0 };
	int len = readlink("/proc/self/exe", path, FILENAME_SIZE);
	if (len <= 0) return;

	/* get rid of filename at end of directory*/
	for (; len > 0; len--) {
		if (path[len] == '/' || path[len] == '\\') break;
		path[len] = '\0';
	}
	chdir(path);
}

void Platform_Exit(ReturnCode code) { exit(code); }

ReturnCode Platform_StartShell(const String* args) {
	char pathBuffer[FILENAME_SIZE + 10];
	String path = String_FromArray(pathBuffer);
	String_Format1(&path, "xdg-open %s", args);
	char str[300]; Platform_ConvertString(str, &path);

	FILE* fp = popen(str, "r");
	if (!fp) return errno;
	return Nix_Return(pclose(fp));
}

Int32 Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args) {
	argc--; /* skip path argument*/
	Int32 i, count = min(argc, PROGRAM_MAX_CMDARGS);

	for (i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i + 1]);
	}
	return count;
}
#endif
