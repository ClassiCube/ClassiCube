#include "Platform.h"
#include "Logger.h"
#include "Stream.h"
#include "ExtMath.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "Http.h"
#include "Bitmap.h"
#include "Window.h"

#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmodapi.h"

static void Platform_InitDisplay(void);
static void Platform_InitStopwatch(void);
struct DisplayDevice DisplayDevice_Default;
void* DisplayDevice_Meta;

#ifdef CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WIN32_IE    0x0400
#define WINVER       0x0500
#define _WIN32_WINNT 0x0500
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <wincrypt.h>

#define HTTP_QUERY_ETAG 54 /* Missing from some old MingW32 headers */
#define Socket__Error() WSAGetLastError()

static HANDLE heap;
char* Platform_NewLine    = "\r\n";

const ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
const ReturnCode ReturnCode_FileNotFound = ERROR_FILE_NOT_FOUND;
const ReturnCode ReturnCode_NotSupported = ERROR_NOT_SUPPORTED;
const ReturnCode ReturnCode_InvalidArg   = ERROR_INVALID_PARAMETER;
const ReturnCode ReturnCode_SocketInProgess  = WSAEINPROGRESS;
const ReturnCode ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
#endif
/* POSIX is mainly shared between Linux and OSX */
#ifdef CC_BUILD_POSIX
#include <curl/curl.h>
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
#include <utime.h>
#include <poll.h>

#define Socket__Error() errno
char* Platform_NewLine    = "\n";
pthread_mutex_t event_mutex;

const ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
const ReturnCode ReturnCode_FileNotFound = ENOENT;
const ReturnCode ReturnCode_NotSupported = EPERM;
const ReturnCode ReturnCode_InvalidArg   = EINVAL;
const ReturnCode ReturnCode_SocketInProgess = EINPROGRESS;
const ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;
#endif
#ifdef CC_BUILD_NIX
#include <X11/Xlib.h>
#include <AL/al.h>
#include <AL/alc.h>
#endif
#ifdef CC_BUILD_SOLARIS
#include <X11/Xlib.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <sys/filio.h>
#endif
#ifdef CC_BUILD_OSX
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif


/*########################################################################################################################*
*------------------------------------------------------GraphicsMode-------------------------------------------------------*
*#########################################################################################################################*/
void GraphicsMode_Make(struct GraphicsMode* m, int bpp, int depth, int stencil, int buffers) {
	m->DepthBits    = depth;
	m->StencilBits  = stencil;
	m->Buffers      = buffers;
	m->IsIndexed    = bpp < 15;
	m->BitsPerPixel = bpp;

	m->A = 0;
	switch (bpp) {
	case 32:
		m->R = 8; m->G = 8; m->B = 8; m->A = 8; break;
	case 24:
		m->R = 8; m->G = 8; m->B = 8; break;
	case 16:
		m->R = 5; m->G = 6; m->B = 5; break;
	case 15:
		m->R = 5; m->G = 5; m->B = 5; break;
	case 8:
		m->R = 3; m->G = 3; m->B = 2; break;
	case 4:
		m->R = 2; m->G = 2; m->B = 1; break;
	default:
		/* mode->R = 0; mode->G = 0; mode->B = 0; */
		Logger_Abort2(bpp, "Unsupported bits per pixel"); break;
	}
}
void GraphicsMode_MakeDefault(struct GraphicsMode* m) {
	int bpp = DisplayDevice_Default.BitsPerPixel;
	GraphicsMode_Make(m, bpp, 24, 0, 2);
}


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

#ifdef CC_BUILD_WIN
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
#endif
#ifdef CC_BUILD_POSIX
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
int Stopwatch_ElapsedMicroseconds(uint64_t beg, uint64_t end) {
	uint64_t delta;
	if (end < beg) return 0;

	delta = ((end - beg) * sw_freqMul) / sw_freqDiv;
	return (int)delta;
}

#ifdef CC_BUILD_WIN
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
#endif
#ifdef CC_BUILD_POSIX
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
#ifdef CC_BUILD_NIX
uint64_t Stopwatch_Measure(void) {
	struct timespec t;
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (uint64_t)t.tv_sec * NS_PER_SEC + t.tv_nsec;
}
#endif
#ifdef CC_BUILD_SOLARIS
uint64_t Stopwatch_Measure(void) {
	return gethrtime();
}
#endif
#ifdef CC_BUILD_OSX
uint64_t Stopwatch_Measure(void) {
	return mach_absolute_time();
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
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
#endif
#ifdef CC_BUILD_POSIX
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
#ifdef CC_BUILD_WIN
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

static CRITICAL_SECTION mutexList[3]; int mutexIndex;
void* Mutex_Create(void) {
	if (mutexIndex == Array_Elems(mutexList)) Logger_Abort("Cannot allocate mutex");
	CRITICAL_SECTION* ptr = &mutexList[mutexIndex];
	InitializeCriticalSection(ptr); mutexIndex++;
	return ptr;
}

void Mutex_Free(void* handle)   { DeleteCriticalSection((CRITICAL_SECTION*)handle); }
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
#endif
#ifdef CC_BUILD_POSIX
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

void* Waitable_Create(void) {
	pthread_cond_t* ptr = Mem_Alloc(1, sizeof(pthread_cond_t), "allocating waitable");
	int res = pthread_cond_init(ptr, NULL);
	if (res) Logger_Abort2(res, "Creating event");
	return ptr;
}

void Waitable_Free(void* handle) {
	int res = pthread_cond_destroy((pthread_cond_t*)handle);
	if (res) Logger_Abort2(res, "Destroying event");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	int res = pthread_cond_signal((pthread_cond_t*)handle);
	if (res) Logger_Abort2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int res = pthread_cond_wait((pthread_cond_t*)handle, &event_mutex);
	if (res) Logger_Abort2(res, "Waiting event");
}

void Waitable_WaitFor(void* handle, uint32_t milliseconds) {
	struct timeval tv;
	struct timespec ts;
	int res;
	gettimeofday(&tv, NULL);

	ts.tv_sec = tv.tv_sec + milliseconds / 1000;
	ts.tv_nsec = 1000 * (tv.tv_usec + 1000 * (milliseconds % 1000));
	ts.tv_sec += ts.tv_nsec / NS_PER_SEC;
	ts.tv_nsec %= NS_PER_SEC;

	res = pthread_cond_timedwait((pthread_cond_t*)handle, &event_mutex, &ts);
	if (res == ETIMEDOUT) return;
	if (res) Logger_Abort2(res, "Waiting timed event");
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
#define DPI_DEVICE 96 /* TODO: GetDeviceCaps(hdc, LOGPIXELSY) in Platform_InitDisplay ? */


static unsigned long Font_ReadWrapper(FT_Stream s, unsigned long offset, unsigned char* buffer, unsigned long count) {
	struct Stream* stream;
	ReturnCode res;

	if (!count && offset > s->size) return 1;
	stream = s->descriptor.pointer;
	if (s->pos != offset) stream->Seek(stream, offset);

	res = Stream_Read(stream, buffer, count);
	return res ? 0 : count;
}

static void Font_CloseWrapper(FT_Stream s) {
	struct Stream* stream;
	struct Stream* source;

	stream = s->descriptor.pointer;
	if (!stream) return;
	source = stream->Meta.Buffered.Source;

	/* Close the actual file stream */
	source->Close(source);
	Mem_Free(s->descriptor.pointer);
	s->descriptor.pointer = NULL;
}

static bool Font_MakeArgs(const String* path, FT_Stream stream, FT_Open_Args* args) {
	struct Stream* data;
	uint8_t* buffer;
	FileHandle file;
	uint32_t size;

	if (File_Open(&file, path)) return false;
	if (File_Length(file, &size)) { File_Close(file); return false; }
	stream->size = size;

	data   = Mem_Alloc(1, sizeof(struct Stream) * 2 + 8192, "Font_MakeArgs");
	buffer = (uint8_t*)&data[2];
	Stream_FromFile(&data[1], file);
	Stream_ReadonlyBuffered(&data[0], &data[1], buffer, 8192);

	stream->descriptor.pointer = data;
	stream->memory = &ft_mem;
	stream->read   = Font_ReadWrapper;
	stream->close  = Font_CloseWrapper;

	args->flags    = FT_OPEN_STREAM;
	args->pathname = NULL;
	args->stream   = stream;
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
	String path;
	FT_Stream stream;
	FT_Open_Args args;
	FT_Face face;
	FT_Error err;

	desc->Size  = size;
	desc->Style = style;

	path = Font_Lookup(fontName, style);
	if (!path.length) Logger_Abort("Unknown font");

	stream = Mem_AllocCleared(1, sizeof(FT_StreamRec), "leaky font"); /* TODO: LEAKS MEMORY!!! */
	if (!Font_MakeArgs(&path, stream, &args)) return;

	/* For OSX font suitcase files */
#ifdef CC_BUILD_OSX
	char filenameBuffer[FILENAME_SIZE + 1];
	String filename = String_NT_Array(filenameBuffer);
	String_Copy(&filename, &path);
	filename.buffer[filename.length] = '\0';
	args.pathname = filename.buffer;
#endif

	err = FT_New_Face(ft_lib, &args, 0, &face);
	if (err) Logger_Abort2(err, "Creating font failed");
	desc->Handle = face;

	err = FT_Set_Char_Size(face, size * 64, 0, DPI_DEVICE, 0);
	if (err) Logger_Abort2(err, "Resizing font failed");
}

void Font_Free(FontDesc* desc) {
	FT_Face face;
	FT_Error err;

	desc->Size  = 0;
	desc->Style = 0;
	/* NULL for fonts created by Drawer2D_MakeFont and bitmapped text mode is on */
	if (!desc->Handle) return;

	face = desc->Handle;
	err  = FT_Done_Face(face);
	if (err) Logger_Abort2(err, "Deleting font failed");
	desc->Handle = NULL;
}

static void Font_Add(const String* path, FT_Face face, char type, const char* defStyle) {
	String name; char nameBuffer[STRING_SIZE];
	String style;

	if (!face->family_name || !(face->face_flags & FT_FACE_FLAG_SCALABLE)) return;
	String_InitArray(name, nameBuffer);
	String_AppendConst(&name, face->family_name);

	/* don't want 'Arial Regular' or 'Arial Bold' */
	if (face->style_name) {
		style = String_FromReadonly(face->style_name);
		if (!String_CaselessEqualsConst(&style, defStyle)) {
			String_Format1(&name, " %c", face->style_name);
		}
	}

	Platform_Log1("Face: %s", &name);
	String_Append(&name, ' '); String_Append(&name, type);
	EntryList_Set(&font_list, &name, path);
	font_list_changed = true;
}

static void Font_DirCallback(const String* path, void* obj) {
	const static String fonExt = String_FromConst(".fon");
	String entry, name, fontPath;
	FT_StreamRec stream = { 0 };
	FT_Open_Args args;
	FT_Face face;
	FT_Error err;
	int i, flags;

	/* Completely skip windows .FON files */
	if (String_CaselessEnds(path, &fonExt)) return;

	/* If font is already known good, skip it */
	for (i = 0; i < font_list.Entries.Count; i++) {
		entry = StringsBuffer_UNSAFE_Get(&font_list.Entries, i);
		String_UNSAFE_Separate(&entry, font_list.Separator, &name, &fontPath);
		if (String_CaselessEquals(path, &fontPath)) return;
	}

	if (!Font_MakeArgs(path, &stream, &args)) return;

	/* For OSX font suitcase files */
#ifdef CC_BUILD_OSX
	char filenameBuffer[FILENAME_SIZE + 1];
	String filename = String_NT_Array(filenameBuffer);
	String_Copy(&filename, path);
	filename.buffer[filename.length] = '\0';
	args.pathname = filename.buffer;
#endif

	err = FT_New_Face(ft_lib, &args, 0, &face);
	if (err) { stream.close(&stream); return; }
	flags = face->style_flags;

	if (flags == (FT_STYLE_FLAG_BOLD | FT_STYLE_FLAG_ITALIC)) {
		Font_Add(path, face, 'Z', "Bold Italic");
	} else if (flags == FT_STYLE_FLAG_BOLD) {
		Font_Add(path, face, 'B', "Bold");
	} else if (flags == FT_STYLE_FLAG_ITALIC) {
		Font_Add(path, face, 'I', "Italic");
	} else if (flags == 0) {
		Font_Add(path, face, 'R', "Regular");
	}
	FT_Done_Face(face);
}

#define TEXT_CEIL(x) (((x) + 63) >> 6)
int Platform_TextWidth(struct DrawTextArgs* args) {
	FT_Face face = args->Font.Handle;
	String text  = args->Text;
	int i, width = 0;
	Codepoint cp;

	for (i = 0; i < text.length; i++) {
		cp = Convert_CP437ToUnicode(text.buffer[i]);
		FT_Load_Char(face, cp, 0); /* TODO: Check error */
		width += face->glyph->advance.x;
	}
	return TEXT_CEIL(width);
}

int Platform_FontHeight(const FontDesc* font) {
	FT_Face face = font->Handle;
	int height   = face->size->metrics.height;
	return TEXT_CEIL(height);
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

int Platform_TextDraw(struct DrawTextArgs* args, Bitmap* bmp, int x, int y, BitmapCol col) {
	FT_Face face = args->Font.Handle;
	String text  = args->Text;
	int descender, height, begX = x;

	/* glyph state */
	FT_Bitmap* img;
	int i, offset;
	Codepoint cp;

	height    = TEXT_CEIL(face->size->metrics.height);
	descender = TEXT_CEIL(face->size->metrics.descender);

	for (i = 0; i < text.length; i++) {
		cp = Convert_CP437ToUnicode(text.buffer[i]);
		FT_Load_Char(face, cp, FT_LOAD_RENDER); /* TODO: Check error */

		offset = (height + descender) - face->glyph->bitmap_top;
		x += face->glyph->bitmap_left; y += offset;

		img = &face->glyph->bitmap;
		if (img->num_grays == 2) {
			Platform_BlackWhiteGlyph(img, bmp, x, y, col);
		} else {
			Platform_GrayscaleGlyph(img, bmp, x, y, col);
		}

		x += TEXT_CEIL(face->glyph->advance.x);
		x -= face->glyph->bitmap_left; y -= offset;
	}

	if (args->Font.Style == FONT_STYLE_UNDERLINE) {
		int ul_pos   = FT_MulFix(face->underline_position,  face->size->metrics.y_scale);
		int ul_thick = FT_MulFix(face->underline_thickness, face->size->metrics.y_scale);

		int ulHeight = TEXT_CEIL(ul_thick);
		int ulY      = height + TEXT_CEIL(ul_pos);
		Drawer2D_Underline(bmp, begX, ulY + y, x - begX, ulHeight, col);
	}

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

#define FONT_CACHE_FILE "fontcache.txt"
static void Font_Init(void) {
#ifdef CC_BUILD_WIN
	const static String dir = String_FromConst("C:\\Windows\\Fonts");
#endif
#ifdef CC_BUILD_NIX
	const static String dir = String_FromConst("/usr/share/fonts");
#endif
#ifdef CC_BUILD_SOLARIS
	const static String dir = String_FromConst("/usr/share/fonts");
#endif
#ifdef CC_BUILD_OSX
	const static String dir = String_FromConst("/Library/Fonts");
#endif
	const static String cachePath = String_FromConst(FONT_CACHE_FILE);
	FT_Error err;

	ft_mem.alloc   = FT_AllocWrapper;
	ft_mem.free    = FT_FreeWrapper;
	ft_mem.realloc = FT_ReallocWrapper;

	err = FT_New_Library(&ft_mem, &ft_lib);
	if (err) Logger_Abort2(err, "Failed to init freetype");

	FT_Add_Default_Modules(ft_lib);
	FT_Set_Default_Properties(ft_lib);

	if (!File_Exists(&cachePath)) {
		Window_ShowDialog("One time load", "Initialising font cache, this can take several seconds.");
	}

	EntryList_Init(&font_list, NULL, FONT_CACHE_FILE, '=');
	Directory_Enum(&dir, NULL, Font_DirCallback);
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
#ifdef CC_BUILD_WIN
	return ioctlsocket(socket, cmd, data);
#else
	return ioctl(socket, cmd, data);
#endif
}

ReturnCode Socket_Available(SocketHandle socket, uint32_t* available) {
	return Socket_ioctl(socket, FIONREAD, available);
}
ReturnCode Socket_SetBlocking(SocketHandle socket, bool blocking) {
	int blocking_raw = blocking ? 0 : -1;
	return Socket_ioctl(socket, FIONBIO, &blocking_raw);
}

ReturnCode Socket_GetError(SocketHandle socket, ReturnCode* result) {
	int resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Socket_Connect(SocketHandle socket, const String* ip, int port) {
	struct { int16_t Family; uint8_t Port[2], IP[4], Pad[8]; } addr;
	ReturnCode res;

	addr.Family = AF_INET;
	Stream_SetU16_BE(addr.Port, port);
	Utils_ParseIP(ip, addr.IP);

	res = connect(socket, (struct sockaddr*)(&addr), sizeof(addr));
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

#ifdef CC_BUILD_WIN
	res1 = shutdown(socket, SD_BOTH);
#else
	res1 = shutdown(socket, SHUT_RDWR);
#endif
	if (res1 == -1) res = Socket__Error();

#ifdef CC_BUILD_WIN
	res2 = closesocket(socket);
#else
	res2 = close(socket);
#endif
	if (res2 == -1) res = Socket__Error();
	return res;
}

/* Alas, a simple cross-platform select() is not good enough */
#ifdef CC_BUILD_WIN
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
#else
#ifdef CC_BUILD_OSX
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
#endif


/*########################################################################################################################*
*----------------------------------------------------------Audio----------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Audio_AllCompleted(AudioHandle handle, bool* finished);
#ifdef CC_BUILD_WIN
struct AudioContext {
	HWAVEOUT Handle;
	WAVEHDR Headers[AUDIO_MAX_BUFFERS];
	struct AudioFormat Format;
	int Count;
};
static struct AudioContext Audio_Contexts[20];

void Audio_Init(AudioHandle* handle, int buffers) {
	struct AudioContext* ctx;
	int i, j;

	for (i = 0; i < Array_Elems(Audio_Contexts); i++) {
		ctx = &Audio_Contexts[i];
		if (ctx->Count) continue;

		for (j = 0; j < buffers; j++) {
			ctx->Headers[j].dwFlags = WHDR_DONE;
		}

		*handle    = i;
		ctx->Count = buffers;
		return;
	}
	Logger_Abort("No free audio contexts");
}

ReturnCode Audio_Free(AudioHandle handle) {
	struct AudioFormat fmt = { 0 };
	struct AudioContext* ctx;
	ReturnCode res;
	ctx = &Audio_Contexts[handle];

	ctx->Count  = 0;
	ctx->Format = fmt;
	if (!ctx->Handle) return 0;

	res = waveOutClose(ctx->Handle);
	ctx->Handle = NULL;
	return res;
}

ReturnCode Audio_SetFormat(AudioHandle handle, struct AudioFormat* format) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	struct AudioFormat*  cur = &ctx->Format;
	ReturnCode res;

	if (AudioFormat_Eq(cur, format)) return 0;
	if (ctx->Handle && (res = waveOutClose(ctx->Handle))) return res;

	int sampleSize = format->Channels * format->BitsPerSample / 8;
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

ReturnCode Audio_BufferData(AudioHandle handle, int idx, void* data, uint32_t dataSize) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];
	ReturnCode res;

	Mem_Set(hdr, 0, sizeof(WAVEHDR));
	hdr->lpData         = data;
	hdr->dwBufferLength = dataSize;
	hdr->dwLoops        = 1;
	
	if ((res = waveOutPrepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR)))) return res;
	if ((res = waveOutWrite(ctx->Handle, hdr, sizeof(WAVEHDR))))         return res;
	return 0;
}

ReturnCode Audio_Play(AudioHandle handle) { return 0; }

ReturnCode Audio_Stop(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	if (!ctx->Handle) return 0;
	return waveOutReset(ctx->Handle);
}

ReturnCode Audio_IsCompleted(AudioHandle handle, int idx, bool* completed) {
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
#endif
#ifdef CC_BUILD_POSIX
struct AudioContext {
	ALuint Source;
	ALuint Buffers[AUDIO_MAX_BUFFERS];
	bool Completed[AUDIO_MAX_BUFFERS];
	struct AudioFormat Format;
	int Count;
	ALenum DataFormat;
};
static struct AudioContext Audio_Contexts[20];

static pthread_mutex_t audio_lock;
static ALCdevice* audio_device;
static ALCcontext* audio_context;
static volatile int audio_refs;

static void Audio_CheckContextErrors(void) {
	ALenum err = alcGetError(audio_device);
	if (err) Logger_Abort2(err, "Error creating OpenAL context");
}

static void Audio_CreateContext(void) {
	audio_device = alcOpenDevice(NULL);
	if (!audio_device) Logger_Abort("Failed to create OpenAL device");
	Audio_CheckContextErrors();

	audio_context = alcCreateContext(audio_device, NULL);
	if (!audio_context) {
		alcCloseDevice(audio_device);
		Logger_Abort("Failed to create OpenAL context");
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
	ALenum err;
	if (ctx->Source == -1) return 0;

	alDeleteSources(1, &ctx->Source);
	ctx->Source = -1;
	if ((err = alGetError())) return err;

	alDeleteBuffers(ctx->Count, ctx->Buffers);
	if ((err = alGetError())) return err;
	return 0;
}

void Audio_Init(AudioHandle* handle, int buffers) {
	ALenum err;
	int i, j;

	Mutex_Lock(&audio_lock);
	{
		if (!audio_context) Audio_CreateContext();
		audio_refs++;
	}
	Mutex_Unlock(&audio_lock);

	alDistanceModel(AL_NONE);
	err = alGetError();
	if (err) { Logger_Abort2(err, "DistanceModel"); }

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
	Logger_Abort("No free audio contexts");
}

ReturnCode Audio_Free(AudioHandle handle) {
	struct AudioFormat fmt = { 0 };
	struct AudioContext* ctx;
	ALenum err;
	ctx = &Audio_Contexts[handle];

	if (!ctx->Count) return 0;
	ctx->Count  = 0;
	ctx->Format = fmt;

	err = Audio_FreeSource(ctx);
	if (err) return err;

	Mutex_Lock(&audio_lock);
	{
		audio_refs--;
		if (audio_refs == 0) Audio_DestroyContext();
	}
	Mutex_Unlock(&audio_lock);
	return 0;
}

static ALenum GetALFormat(int channels, int bitsPerSample) {
    if (bitsPerSample == 16) {
        if (channels == 1) return AL_FORMAT_MONO16;
        if (channels == 2) return AL_FORMAT_STEREO16;
	} else if (bitsPerSample == 8) {
        if (channels == 1) return AL_FORMAT_MONO8;
        if (channels == 2) return AL_FORMAT_STEREO8;
	}
	Logger_Abort("Unsupported audio format"); return 0;
}

ReturnCode Audio_SetFormat(AudioHandle handle, struct AudioFormat* format) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	struct AudioFormat*  cur = &ctx->Format;
	ALenum err;

	if (AudioFormat_Eq(cur, format)) return 0;
	ctx->DataFormat = GetALFormat(format->Channels, format->BitsPerSample);
	ctx->Format     = *format;
	
	if ((err = Audio_FreeSource(ctx))) return err;
	alGenSources(1, &ctx->Source);
	if ((err = alGetError())) return err;

	alGenBuffers(ctx->Count, ctx->Buffers);
	if ((err = alGetError())) return err;
	return 0;
}

ReturnCode Audio_BufferData(AudioHandle handle, int idx, void* data, uint32_t dataSize) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	ALuint buffer = ctx->Buffers[idx];
	ALenum err;
	ctx->Completed[idx] = false;

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

ReturnCode Audio_IsCompleted(AudioHandle handle, int idx, bool* completed) {
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
	ALint state = 0;
	ReturnCode res;

	if (ctx->Source == -1) { *finished = true; return 0; }
	res = Audio_AllCompleted(handle, finished);
	if (res) return res;
	
	alGetSourcei(ctx->Source, AL_SOURCE_STATE, &state);
	*finished = state != AL_PLAYING; return 0;
}
#endif
static ReturnCode Audio_AllCompleted(AudioHandle handle, bool* finished) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	ReturnCode res;
	int i;
	*finished = false;

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
	bool finished;
	Audio_Stop(handle);
	Audio_IsFinished(handle, &finished); /* unqueue buffers */
	return Audio_Free(handle);
}


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
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

void Platform_Init(void) {	
	WSADATA wsaData;
	ReturnCode res;

	Platform_InitDisplay();
	Platform_InitStopwatch();
	heap = GetProcessHeap();
	
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res) Logger_Abort2(res, "WSAStartup failed");
}

void Platform_Free(void) {
	WSACleanup();
	HeapDestroy(heap);
}

static void Platform_InitDisplay(void) {
	HDC hdc = GetDC(NULL);

	DisplayDevice_Default.Bounds.X = 0;
	DisplayDevice_Default.Bounds.Y = 0;
	DisplayDevice_Default.Bounds.Width  = GetSystemMetrics(SM_CXSCREEN);
	DisplayDevice_Default.Bounds.Height = GetSystemMetrics(SM_CYSCREEN);
	DisplayDevice_Default.BitsPerPixel  = GetDeviceCaps(hdc, BITSPIXEL);

	ReleaseDC(NULL, hdc);
}

static void Platform_InitStopwatch(void) {
	LARGE_INTEGER freq;
	sw_highRes = QueryPerformanceFrequency(&freq);

	if (sw_highRes) {
		sw_freqMul = 1000 * 1000;
		sw_freqDiv = freq.QuadPart;
	} else { sw_freqDiv = 10; }
}

ReturnCode Platform_SetCurrentDirectory(const String* path) {
	TCHAR str[300];
	Platform_ConvertString(str, path);
	return SetCurrentDirectory(str) ? 0 : GetLastError();
}

void Platform_Exit(ReturnCode code) { ExitProcess(code); }

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
		end = String_IndexOf(args, '"', 0);
	} else {
		end = String_IndexOf(args, ' ', 0);
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

ReturnCode Platform_GetExePath(String* path) {
	TCHAR chars[FILENAME_SIZE + 1];
	DWORD len = GetModuleFileName(NULL, chars, FILENAME_SIZE);
	if (!len) return GetLastError();

#ifdef UNICODE
	Convert_DecodeUtf16(path, chars, len * 2);
#else
	Convert_DecodeAscii(path, chars, len);
#endif
	return 0;
}

ReturnCode Platform_StartProcess(const String* path, const String* args) {
	String file, argv; char argvBuffer[300];
	TCHAR str[300], raw[300];
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	BOOL ok;

	file = *path; Utils_UNSAFE_GetFilename(&file);
	String_InitArray(argv, argvBuffer);
	String_Format2(&argv, "%s %s", &file, args);
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

ReturnCode Platform_StartOpen(const String* args) {
	TCHAR str[300];
	HINSTANCE instance;
	Platform_ConvertString(str, args);
	instance = ShellExecute(NULL, NULL, str, NULL, NULL, SW_SHOWNORMAL);
	return instance > 32 ? 0 : (ReturnCode)instance;
}
/* Don't need special execute permission on windows */
ReturnCode Platform_MarkExecutable(const String* path) { return 0; }

ReturnCode Platform_LoadLibrary(const String* path, void** lib) {
	TCHAR str[300];
	Platform_ConvertString(str, path);
	*lib = LoadLibrary(str);
	return *lib ? 0 : GetLastError();
}

ReturnCode Platform_GetSymbol(void* lib, const char* name, void** symbol) {
	*symbol = GetProcAddress(lib, name);
	return *symbol ? 0 : GetLastError();
}
#else
ReturnCode Platform_Encrypt(const uint8_t* data, int len, uint8_t** enc, int* encLen) {
	return ReturnCode_NotSupported;
}
ReturnCode Platform_Decrypt(const uint8_t* data, int len, uint8_t** dec, int* decLen) {
	return ReturnCode_NotSupported;
}
#endif
#ifdef CC_BUILD_POSIX
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

void Platform_Init(void) {
	Platform_InitDisplay();
	Platform_InitStopwatch();
	pthread_mutex_init(&event_mutex, NULL);
	pthread_mutex_init(&audio_lock,  NULL);
}

void Platform_Free(void) {
	pthread_mutex_destroy(&event_mutex);
	pthread_mutex_destroy(&audio_lock);
}

ReturnCode Platform_SetCurrentDirectory(const String* path) {
	char str[600];
	Platform_ConvertString(str, path);
	return chdir(str) == -1 ? errno : 0;
}

void Platform_Exit(ReturnCode code) { exit(code); }

int Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args) {
	int i, count;
	argc--; /* skip executable path argument */
	count = min(argc, GAME_MAX_CMDARGS);

	for (i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i + 1]);
	}
	return count;
}

ReturnCode Platform_StartProcess(const String* path, const String* args) {
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

ReturnCode Platform_MarkExecutable(const String* path) {
	char str[600];
	struct stat st;
	Platform_ConvertString(str, path);

	if (stat(str, &st) == -1) return errno;
	st.st_mode |= S_IXUSR;
	return chmod(str, st.st_mode) == -1 ? errno : 0;
}

ReturnCode Platform_LoadLibrary(const String* path, void** lib) {
	char str[600];
	Platform_ConvertString(str, path);
	*lib = dlopen(str, RTLD_NOW);
	return *lib == NULL;
}

ReturnCode Platform_GetSymbol(void* lib, const char* name, void** symbol) {
	*symbol = dlsym(lib, name);
	return *symbol == NULL; /* dlerror would be proper, but eh */
}
#endif
#ifdef CC_BUILD_X11
static void Platform_InitDisplay(void) {
	Display* display = XOpenDisplay(NULL);
	int screen;
	if (!display) Logger_Abort("Failed to open display");

	DisplayDevice_Meta = display;
	screen = DefaultScreen(display);

	/* TODO: Use Xinerama and XRandR for querying these */
	DisplayDevice_Default.Bounds.X = 0;
	DisplayDevice_Default.Bounds.Y = 0;
	DisplayDevice_Default.Bounds.Width  = DisplayWidth(display,  screen);
	DisplayDevice_Default.Bounds.Height = DisplayHeight(display, screen);
	DisplayDevice_Default.BitsPerPixel  = DefaultDepth(display,  screen);
}
#endif
#ifdef CC_BUILD_NIX
ReturnCode Platform_StartOpen(const String* args) {
	const static String path = String_FromConst("xdg-open");
	return Platform_StartProcess(&path, args);
}
static void Platform_InitStopwatch(void) { sw_freqDiv = 1000; }

ReturnCode Platform_GetExePath(String* path) {
	char str[600];
	int len = readlink("/proc/self/exe", str, 600);
	if (len == -1) return errno;

	Convert_DecodeUtf8(path, str, len);
	return 0;
}
#endif
#ifdef CC_BUILD_SOLARIS
ReturnCode Platform_StartOpen(const String* args) {
	/* TODO: Is this on solaris, or just an OpenIndiana thing */
	const static String path = String_FromConst("xdg-open");
	return Platform_StartProcess(&path, args);
}
static void Platform_InitStopwatch(void) { sw_freqDiv = 1000; }

ReturnCode Platform_GetExePath(String* path) {
	char str[600];
	int len = readlink("/proc/self/path/a.out", str, 600);
	if (len == -1) return errno;

	Convert_DecodeUtf8(path, str, len);
	return 0;
}
#endif
#ifdef CC_BUILD_OSX
ReturnCode Platform_StartOpen(const String* args) {
	const static String path = String_FromConst("/usr/bin/open");
	return Platform_StartProcess(&path, args);
}
ReturnCode Platform_GetExePath(String* path) {
	char str[600];
	int len = 600;

	if (_NSGetExecutablePath(str, &len) != 0) return ReturnCode_InvalidArg;
	Convert_DecodeUtf8(path, str, len);
	return 0;
}

static void Platform_InitDisplay(void) {
	CGDirectDisplayID display = CGMainDisplayID();
	CGRect bounds = CGDisplayBounds(display);

	DisplayDevice_Default.Bounds.X = (int)bounds.origin.x;
	DisplayDevice_Default.Bounds.Y = (int)bounds.origin.y;
	DisplayDevice_Default.Bounds.Width  = (int)bounds.size.width;
	DisplayDevice_Default.Bounds.Height = (int)bounds.size.height;
	DisplayDevice_Default.BitsPerPixel  = CGDisplayBitsPerPixel(display);
}

static void Platform_InitStopwatch(void) {
	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);

	sw_freqMul = tb.numer;
	sw_freqDiv = tb.denom * 1000;
}
#endif
