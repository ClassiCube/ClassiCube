#include "Platform.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "AsyncDownloader.h"
#include "Bitmap.h"

#include "freetype/ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftmodapi.h"

static void Platform_InitDisplay(void);
static void Platform_InitStopwatch(void);

#ifdef CC_BUILD_WIN
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

static HANDLE heap;
char* Platform_NewLine    = "\r\n";
char  Directory_Separator = '\\';
char* Font_DefaultName    = "Arial";

ReturnCode ReturnCode_FileShareViolation = ERROR_SHARING_VIOLATION;
ReturnCode ReturnCode_FileNotFound = ERROR_FILE_NOT_FOUND;
ReturnCode ReturnCode_NotSupported = ERROR_NOT_SUPPORTED;
ReturnCode ReturnCode_InvalidArg   = ERROR_INVALID_PARAMETER;
ReturnCode ReturnCode_SocketInProgess  = WSAEINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = WSAEWOULDBLOCK;
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define Socket__Error() errno
#define Nix_Return(success) ((success) ? 0 : errno)

char* Platform_NewLine    = "\n";
char  Directory_Separator = '/'; /* TODO: Is this right for old OSX though?? */
pthread_mutex_t event_mutex;

ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
ReturnCode ReturnCode_FileNotFound = ENOENT;
ReturnCode ReturnCode_NotSupported = EPERM;
ReturnCode ReturnCode_InvalidArg   = EINVAL;
ReturnCode ReturnCode_SocketInProgess = EINPROGRESS;
ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;
#endif
#ifdef CC_BUILD_NIX
#include <X11/Xlib.h>
#include <AL/al.h>
#include <AL/alc.h>
char* Font_DefaultName = "Century Schoolbook L Roman";
#endif
#ifdef CC_BUILD_SOLARIS
#include <X11/Xlib.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <sys/filio.h>
char* Font_DefaultName = "Century Schoolbook L Roman";
#endif
#ifdef CC_BUILD_OSX
#include <mach/mach_time.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
char* Font_DefaultName = "Arial";
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
		ErrorHandler_Fail2(bpp, "Unsupported bits per pixel"); break;
	}
}
void GraphicsMode_MakeDefault(struct GraphicsMode* m) {
	int bpp = DisplayDevice_Default.BitsPerPixel;
	GraphicsMode_Make(m, bpp, 24, 0, 2);
}


/*########################################################################################################################*
*---------------------------------------------------------Memory----------------------------------------------------------*
*#########################################################################################################################*/
void Mem_Set(void* dst, uint8_t value, uint32_t numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, void* src,  uint32_t numBytes)   { memcpy(dst, src,   numBytes); }

NOINLINE_ static void Platform_AllocFailed(const char* place) {
	char logBuffer[STRING_SIZE+20 + 1];
	String log = String_NT_Array(logBuffer);
	String_Format1(&log, "Failed allocating memory for: %c", place);

	log.buffer[log.length] = '\0';
	ErrorHandler_Fail(log.buffer);
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

static void Platform_FromSysTime(DateTime* time, SYSTEMTIME* sysTime) {
	time->Year   = sysTime->wYear;
	time->Month  = sysTime->wMonth;
	time->Day    = sysTime->wDay;
	time->Hour   = sysTime->wHour;
	time->Minute = sysTime->wMinute;
	time->Second = sysTime->wSecond;
	time->Milli  = sysTime->wMilliseconds;
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

#define UNIX_EPOCH 62135596800000ULL
#define UnixTime_TotalMS(time) ((uint64_t)time.tv_sec * 1000 + UNIX_EPOCH + (time.tv_usec / 1000))
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
	struct timeval cur; 
	struct tm utc_time;

	gettimeofday(&cur, NULL);
	gmtime_r(&cur.tv_sec, &utc_time);

	Platform_FromSysTime(time_, &utc_time);
	time_->Milli = cur.tv_usec / 1000;
}

void DateTime_CurrentLocal(DateTime* time_) {
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
	WCHAR str[300];
	DWORD attribs;

	Platform_ConvertString(str, path);
	attribs = GetFileAttributesW(str);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Create(const String* path) {
	WCHAR str[300];
	BOOL success;

	Platform_ConvertString(str, path);
	success = CreateDirectoryW(str, NULL);
	return Win_Return(success);
}

bool File_Exists(const String* path) {
	WCHAR str[300];
	DWORD attribs;

	Platform_ConvertString(str, path);
	attribs = GetFileAttributesW(str);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Enum(const String* dirPath, void* obj, Directory_EnumCallback callback) {
	WCHAR str[300];
	WIN32_FIND_DATAW entry;
	HANDLE find;
	ReturnCode res;
	String path; char pathBuffer[MAX_PATH + 10];

	/* Need to append \* to search for files in directory */
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "%s\\*", dirPath);
	Platform_ConvertString(str, &path);
	
	find = FindFirstFileW(str, &entry);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		path.length = 0;
		String_Format2(&path, "%s%r", dirPath, &Directory_Separator);

		/* ignore . and .. entry */
		WCHAR* src = entry.cFileName;
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
	}  while (FindNextFileW(find, &entry));

	res = GetLastError(); /* return code from FindNextFile */
	FindClose(find);
	return Win_Return(res == ERROR_NO_MORE_FILES);
}

ReturnCode File_GetModifiedTime_MS(const String* path, TimeMS* time) {
	void* file; 
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

ReturnCode File_Do(void** file, const String* path, DWORD access, DWORD createMode) {
	WCHAR str[300]; 
	Platform_ConvertString(str, path);
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
	ReturnCode res = File_Do(file, path, GENERIC_WRITE, OPEN_ALWAYS);
	if (res) return res;
	return File_Seek(*file, 0, FILE_SEEKFROM_END);
}

ReturnCode File_Read(void* file, uint8_t* buffer, uint32_t count, uint32_t* bytesRead) {
	BOOL success = ReadFile((HANDLE)file, buffer, count, bytesRead, NULL);
	return Win_Return(success);
}

ReturnCode File_Write(void* file, uint8_t* buffer, uint32_t count, uint32_t* bytesWrote) {
	BOOL success = WriteFile((HANDLE)file, buffer, count, bytesWrote, NULL);
	return Win_Return(success);
}

ReturnCode File_Close(void* file) {
	return Win_Return(CloseHandle((HANDLE)file));
}

ReturnCode File_Seek(void* file, int offset, int seekType) {
	static uint8_t modes[3] = { FILE_BEGIN, FILE_CURRENT, FILE_END };
	DWORD pos = SetFilePointer(file, offset, NULL, modes[seekType]);
	return Win_Return(pos != INVALID_SET_FILE_POINTER);
}

ReturnCode File_Position(void* file, uint32_t* position) {
	*position = SetFilePointer(file, 0, NULL, 1); /* SEEK_CUR */
	return Win_Return(*position != INVALID_SET_FILE_POINTER);
}

ReturnCode File_Length(void* file, uint32_t* length) {
	*length = GetFileSize(file, NULL);
	return Win_Return(*length != INVALID_FILE_SIZE);
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
	return Nix_Return(mkdir(str, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1);
}

bool File_Exists(const String* path) {
	char str[600]; 
	struct stat sb;
	Platform_ConvertString(str, path);
	return stat(str, &sb) == 0 && S_ISREG(sb.st_mode);
}

ReturnCode Directory_Enum(const String* dirPath, void* obj, Directory_EnumCallback callback) {
	char str[600];
	DIR* dirPtr;
	struct dirent* entry;
	char* src;
	int len, res;
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);

	Platform_ConvertString(str, dirPath);
	dirPtr = opendir(str);
	if (!dirPtr) return errno;

	/* POSIX docs: "When the end of the directory is encountered, a null pointer is returned and errno is not changed." */
	/* errno is sometimes leftover from previous calls, so always reset it before readdir gets called */
	errno = 0;
	while (entry = readdir(dirPtr)) {
		path.length = 0;
		String_Format2(&path, "%s%r", dirPath, &Directory_Separator);

		/* ignore . and .. entry */
		src = entry->d_name;
		if (src[0] == '.' && src[1] == '\0') continue;
		if (src[0] == '.' && src[1] == '.' && src[2] == '\0') continue;

		len = String_CalcLen(src, UInt16_MaxValue);
		String_DecodeUtf8(&path, src, len);

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

ReturnCode File_GetModifiedTime_MS(const String* path, TimeMS* time) {
	char str[600]; 
	struct stat sb;
	Platform_ConvertString(str, path);
	if (stat(str, &sb) == -1) return errno;

	*time = (uint64_t)sb.st_mtime * 1000 + UNIX_EPOCH;
	return 0;
}

ReturnCode File_Do(void** file, const String* path, int mode) {
	char str[600]; 
	Platform_ConvertString(str, path);
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
	ReturnCode res = File_Do(file, path, O_WRONLY | O_CREAT);
	if (res) return res;
	return File_Seek(*file, 0, FILE_SEEKFROM_END);
}

ReturnCode File_Read(void* file, uint8_t* buffer, uint32_t count, uint32_t* bytesRead) {
	*bytesRead = read((int)file, buffer, count);
	return Nix_Return(*bytesRead != -1);
}

ReturnCode File_Write(void* file, uint8_t* buffer, uint32_t count, uint32_t* bytesWrote) {
	*bytesWrote = write((int)file, buffer, count);
	return Nix_Return(*bytesWrote != -1);
}

ReturnCode File_Close(void* file) {
	return Nix_Return(close((int)file) != -1);
}

ReturnCode File_Seek(void* file, int offset, int seekType) {
	static uint8_t modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
	return Nix_Return(lseek((int)file, offset, modes[seekType]) != -1);
}

ReturnCode File_Position(void* file, uint32_t* position) {
	*position = lseek((int)file, 0, SEEK_CUR);
	return Nix_Return(*position != -1);
}

ReturnCode File_Length(void* file, uint32_t* length) {
	struct stat st;
	if (fstat((int)file, &st) == -1) { *length = -1; return errno; }
	*length = st.st_size; return 0;
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

static CRITICAL_SECTION mutexList[3]; int mutexIndex;
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
	if (res) ErrorHandler_Fail2(res, "Creating thread");

	if (detach) Thread_Detach(ptr);
	return ptr;
}

void Thread_Detach(void* handle) {
	pthread_t* ptr = handle;
	int res = pthread_detach(*ptr);
	if (res) ErrorHandler_Fail2(res, "Detaching thread");
	Mem_Free(ptr);
}

void Thread_Join(void* handle) {
	pthread_t* ptr = handle;
	int res = pthread_join(*ptr, NULL);
	if (res) ErrorHandler_Fail2(res, "Joining thread");
	Mem_Free(ptr);
}

void* Mutex_Create(void) {
	pthread_mutex_t* ptr = Mem_Alloc(1, sizeof(pthread_mutex_t), "allocating mutex");
	int res = pthread_mutex_init(ptr, NULL);
	if (res) ErrorHandler_Fail2(res, "Creating mutex");
	return ptr;
}

void Mutex_Free(void* handle) {
	int res = pthread_mutex_destroy((pthread_mutex_t*)handle);
	if (res) ErrorHandler_Fail2(res, "Destroying mutex");
	Mem_Free(handle);
}

void Mutex_Lock(void* handle) {
	int res = pthread_mutex_lock((pthread_mutex_t*)handle);
	if (res) ErrorHandler_Fail2(res, "Locking mutex");
}

void Mutex_Unlock(void* handle) {
	int res = pthread_mutex_unlock((pthread_mutex_t*)handle);
	if (res) ErrorHandler_Fail2(res, "Unlocking mutex");
}

void* Waitable_Create(void) {
	pthread_cond_t* ptr = Mem_Alloc(1, sizeof(pthread_cond_t), "allocating waitable");
	int res = pthread_cond_init(ptr, NULL);
	if (res) ErrorHandler_Fail2(res, "Creating event");
	return ptr;
}

void Waitable_Free(void* handle) {
	int res = pthread_cond_destroy((pthread_cond_t*)handle);
	if (res) ErrorHandler_Fail2(res, "Destroying event");
	Mem_Free(handle);
}

void Waitable_Signal(void* handle) {
	int res = pthread_cond_signal((pthread_cond_t*)handle);
	if (res) ErrorHandler_Fail2(res, "Signalling event");
}

void Waitable_Wait(void* handle) {
	int res = pthread_cond_wait((pthread_cond_t*)handle, &event_mutex);
	if (res) ErrorHandler_Fail2(res, "Waiting event");
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
	if (res) ErrorHandler_Fail2(res, "Waiting timed event");
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
static FT_Library ft_lib;
static struct FT_MemoryRec_ ft_mem;
static StringsBuffer norm_fonts, bold_fonts;
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
	void* file;
	uint32_t size;

	if (File_Open(&file, path)) return false;
	if (File_Length(file, &size)) { File_Close(file); return false; }
	stream->size = size;

	struct Stream* data    = Mem_Alloc(1, sizeof(struct Stream) * 2 + 8192, "Font_MakeArgs");
	struct Stream* wrapper = &data[0];
	struct Stream* fileSrm = &data[1];

	/* TODO: Increase buffer size to 8192, better Seek impl (want 11,000 I/O reads) */
	uint8_t* buffer = (uint8_t*)&data[2];
	Stream_FromFile(fileSrm, file);
	Stream_ReadonlyBuffered(wrapper, fileSrm, buffer, 8192);

	stream->descriptor.pointer = data;
	stream->memory = &ft_mem;
	stream->read   = Font_ReadWrapper;
	stream->close  = Font_CloseWrapper;

	args->flags    = FT_OPEN_STREAM;
	args->pathname = NULL;
	args->stream   = stream;
	return true;
}

static int Font_Find(const String* name, StringsBuffer* entries) {
	String faceName;
	int i;

	for (i = 1; i < entries->Count; i += 2) {
		faceName = StringsBuffer_UNSAFE_Get(entries, i);
		if (String_CaselessEquals(&faceName, name)) return i;
	}
	return -1;
}

void Font_GetNames(StringsBuffer* buffer) {
	String faceName;
	int i;
	if (!norm_fonts.Count) Font_Init();

	for (i = 1; i < norm_fonts.Count; i += 2) {
		faceName = StringsBuffer_UNSAFE_Get(&norm_fonts, i);
		StringsBuffer_Add(buffer, &faceName);
	}
}

void Font_Make(FontDesc* desc, const String* fontName, int size, int style) {
	FT_Stream stream;
	FT_Open_Args args;
	FT_Face face;
	FT_Error err;

	desc->Size  = size;
	desc->Style = style;
	if (!norm_fonts.Count) Font_Init();

	int idx = -1;
	StringsBuffer* entries = &bold_fonts;
	if (style & FONT_STYLE_BOLD) { idx = Font_Find(fontName, entries); }

	if (idx == -1) {
		entries = &norm_fonts;
		idx = Font_Find(fontName, entries);
	}

	if (idx == -1) ErrorHandler_Fail("Unknown font");
	String path = StringsBuffer_UNSAFE_Get(entries, idx - 1);

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
	if (err) ErrorHandler_Fail2(err, "Creating font failed");
	desc->Handle = face;

	err = FT_Set_Char_Size(face, size * 64, 0, DPI_DEVICE, 0);
	if (err) ErrorHandler_Fail2(err, "Resizing font failed");
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
	if (err) ErrorHandler_Fail2(err, "Deleting font failed");
	desc->Handle = NULL;
}

static void Font_Add(const String* path, FT_Face face, StringsBuffer* entries, const char* defStyle) {
	String name; char nameBuffer[STRING_SIZE];
	String style;

	if (!face->family_name || !(face->face_flags & FT_FACE_FLAG_SCALABLE)) return;
	StringsBuffer_Add(entries, path);
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
	StringsBuffer_Add(entries, &name);
}

static void Font_DirCallback(const String* path, void* obj) {
	FT_StreamRec stream = { 0 };
	FT_Open_Args args;
	FT_Face face;
	FT_Error err;

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

	if (face->style_flags == FT_STYLE_FLAG_BOLD) {
		Font_Add(path, face, &bold_fonts, "Bold");
	} else if (face->style_flags == 0) {
		Font_Add(path, face, &norm_fonts, "Regular");
	}
	FT_Done_Face(face);
}

#define TEXT_CEIL(x) (((x) + 63) >> 6)
Size2D Platform_TextMeasure(struct DrawTextArgs* args) {
	FT_Face face = args->Font.Handle;
	String text  = args->Text;
	Size2D s = { 0, face->size->metrics.height };
	int i;

	for (i = 0; i < text.length; i++) {
		Codepoint cp = Convert_CP437ToUnicode(text.buffer[i]);
		FT_Load_Char(face, cp, 0); /* TODO: Check error */
		s.Width += face->glyph->advance.x;
	}

	s.Width  = TEXT_CEIL(s.Width);
	s.Height = TEXT_CEIL(s.Height);
	return s;
}

Size2D Platform_TextDraw(struct DrawTextArgs* args, Bitmap* bmp, int x, int y, PackedCol col) {
	FT_Face face = args->Font.Handle;
	String text = args->Text;
	Size2D s = { x, TEXT_CEIL(face->size->metrics.height) };
	int i, descender = TEXT_CEIL(face->size->metrics.descender);

	for (i = 0; i < text.length; i++) {
		Codepoint cp = Convert_CP437ToUnicode(text.buffer[i]);
		FT_Load_Char(face, cp, FT_LOAD_RENDER); /* TODO: Check error */

		FT_Bitmap* img = &face->glyph->bitmap;
		int xx, yy, offset = s.Height + descender - face->glyph->bitmap_top;
		x += face->glyph->bitmap_left; y += offset;

		for (yy = 0; yy < img->rows; yy++) {
			if ((y + yy) < 0 || (y + yy) >= bmp->Height) continue;
			uint8_t* src = img->buffer + (yy * img->width);
			uint8_t* dst = (uint8_t*)Bitmap_GetRow(bmp, y + yy) + (x * BITMAP_SIZEOF_PIXEL);

			for (xx = 0; xx < img->width; xx++) {
				if ((x + xx) < 0 || (x + xx) >= bmp->Width) continue;

				uint8_t intensity = *src, invIntensity = UInt8_MaxValue - intensity;
				dst[0] = ((col.B * intensity) >> 8) + ((dst[0] * invIntensity) >> 8);
				dst[1] = ((col.G * intensity) >> 8) + ((dst[1] * invIntensity) >> 8);
				dst[2] = ((col.R * intensity) >> 8) + ((dst[2] * invIntensity) >> 8);
				//dst[3] = ((col.A * intensity) >> 8) + ((dst[3] * invIntensity) >> 8);
				dst[3] = intensity + ((dst[3] * invIntensity) >> 8);
				src++; dst += BITMAP_SIZEOF_PIXEL;
			}
		}

		x += TEXT_CEIL(face->glyph->advance.x);
		x -= face->glyph->bitmap_left; y -= offset;
	}

	int begX = s.Width;
	if (args->Font.Style == FONT_STYLE_UNDERLINE) {
		int ul_pos   = FT_MulFix(face->underline_position,  face->size->metrics.y_scale);
		int ul_thick = FT_MulFix(face->underline_thickness, face->size->metrics.y_scale);

		int ulHeight = TEXT_CEIL(ul_thick);
		int ulY      = s.Height + TEXT_CEIL(ul_pos);
		Drawer2D_Underline(bmp, begX, ulY + y, x - begX, ulHeight, col);
	}
	s.Width = x - begX; return s;
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

static void Font_Init(void) {
#ifdef CC_BUILD_WIN
	static String dir = String_FromConst("C:\\Windows\\fonts");
#endif
#ifdef CC_BUILD_NIX
	static String dir = String_FromConst("/usr/share/fonts");
#endif
#ifdef CC_BUILD_SOLARIS
	static String dir = String_FromConst("/usr/share/fonts");
#endif
#ifdef CC_BUILD_OSX
	static String dir = String_FromConst("/Library/Fonts");
#endif
	FT_Error err;

	ft_mem.alloc   = FT_AllocWrapper;
	ft_mem.free    = FT_FreeWrapper;
	ft_mem.realloc = FT_ReallocWrapper;

	err = FT_New_Library(&ft_mem, &ft_lib);
	if (err) ErrorHandler_Fail2(err, "Failed to init freetype");

	FT_Add_Default_Modules(ft_lib);
	FT_Set_Default_Properties(ft_lib);
	Directory_Enum(&dir, NULL, Font_DirCallback);
}


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
void Socket_Create(SocketPtr* socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		ErrorHandler_Fail2(Socket__Error(), "Failed to create socket");
	}
}

static ReturnCode Socket_ioctl(SocketPtr socket, uint32_t cmd, int* data) {
#ifdef CC_BUILD_WIN
	return ioctlsocket(socket, cmd, data);
#else
	return ioctl(socket, cmd, data);
#endif
}

ReturnCode Socket_Available(SocketPtr socket, uint32_t* available) {
	return Socket_ioctl(socket, FIONREAD, available);
}
ReturnCode Socket_SetBlocking(SocketPtr socket, bool blocking) {
	int blocking_raw = blocking ? 0 : -1;
	return Socket_ioctl(socket, FIONBIO, &blocking_raw);
}

ReturnCode Socket_GetError(SocketPtr socket, ReturnCode* result) {
	int resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Socket_Connect(SocketPtr socket, const String* ip, int port) {
	struct { int16_t Family; uint8_t Port[2], IP[4], Pad[8]; } addr;
	ReturnCode res;

	addr.Family = AF_INET;
	Stream_SetU16_BE(addr.Port, port);
	Utils_ParseIP(ip, addr.IP);

	res = connect(socket, (struct sockaddr*)(&addr), sizeof(addr));
	return res == -1 ? Socket__Error() : 0;
}

ReturnCode Socket_Read(SocketPtr socket, uint8_t* buffer, uint32_t count, uint32_t* modified) {
	int recvCount = recv(socket, buffer, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Socket_Write(SocketPtr socket, uint8_t* buffer, uint32_t count, uint32_t* modified) {
	int sentCount = send(socket, buffer, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return Socket__Error();
}

ReturnCode Socket_Close(SocketPtr socket) {
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

ReturnCode Socket_Select(SocketPtr socket, int selectMode, bool* success) {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(socket, &set);

	struct timeval time = { 0 };
	int selectCount = -1;

	#ifdef CC_BUILD_WIN
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
#ifdef CC_BUILD_WIN
	*success = set.fd_count != 0; return 0;
#else
	*success = FD_ISSET(socket, &set); return 0;
#endif
}


/*########################################################################################################################*
*----------------------------------------------------------Http-----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WIN
static HINTERNET hInternet;
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

static ReturnCode Http_GetData(struct AsyncRequest* req, HINTERNET handle, volatile int* progress) {
	uint32_t size = req->ResultSize;
	if (!size) return ERROR_NOT_SUPPORTED;
	*progress = 0;

	uint8_t* buffer = Mem_Alloc(size, 1, "http get data");
	uint32_t left = size, read, totalRead = 0;
	req->ResultData = buffer;

	while (left) {
		uint32_t toRead = left, avail = 0;
		/* only read as much data that is pending */
		if (InternetQueryDataAvailable(handle, &avail, 0, 0)) {
			toRead = min(toRead, avail);
		}

		bool success = InternetReadFile(handle, buffer, toRead, &read);
		if (!success) { Mem_Free(buffer); return GetLastError(); }

		if (!read) break;
		buffer += read; totalRead += read; left -= read;
		*progress = (int)(100.0f * totalRead / size);
	}

	*progress = 100;
	return 0;
}

ReturnCode Http_Do(struct AsyncRequest* req, volatile int* progress) {
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
#endif
#ifdef CC_BUILD_POSIX
CURL* curl;

void Http_Init(void) {
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) ErrorHandler_Fail2(res, "Failed to init curl");

	curl = curl_easy_init();
	if (!curl) ErrorHandler_Fail("Failed to init easy curl");
}

static int Http_Progress(int* progress, double total, double received, double a, double b) {
	if (total == 0) return 0;
	*progress = (int)(100 * received / total);
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
		Convert_TryParseInt(&value, &req->ResultSize);
	} else if (String_CaselessEqualsConst(&name, "Last-Modified")) {
		char tmpBuffer[STRING_SIZE + 1] = { 0 };
		String tmp = { tmpBuffer, 0, STRING_SIZE };
		String_AppendString(&tmp, &value);

		time_t time = curl_getdate(tmp.buffer, NULL);
		if (time == -1) return total;
		req->LastModified = (uint64_t)time * 1000 + UNIX_EPOCH;
	}
	return total;
}

static size_t Http_GetData(char *buffer, size_t size, size_t nitems, struct AsyncRequest* req) {
	uint32_t total = req->ResultSize;
	if (!total || req->RequestType == REQUEST_TYPE_CONTENT_LENGTH) return 0;
	if (!req->ResultData) req->ResultData = Mem_Alloc(total, 1, "http get data");

	/* reuse Result as an offset */
	uint32_t left = total - req->Result;
	left        = min(left, nitems);

	uint8_t* dst = (uint8_t*)req->ResultData + req->Result;
	Mem_Copy(dst, buffer, left);
	req->Result += left;

	return nitems;
}

ReturnCode Http_Do(struct AsyncRequest* req, volatile int* progress) {
	struct curl_slist* list;
	String url = String_FromRawArray(req->URL);
	char urlStr[600];
	long status = 0;

	Platform_ConvertString(urlStr, &url);
	curl_easy_reset(curl);
	list = Http_Make(req);
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
	ErrorHandler_Fail("No free audio contexts");
}

ReturnCode Audio_Free(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	ReturnCode res;
	if (!ctx->Count) return 0;

	res = waveOutClose(ctx->Handle);
	ctx->Handle = NULL;
	ctx->Count  = 0;

	struct AudioFormat fmt = { 0 };
	ctx->Format = fmt;
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
	if (err) { ErrorHandler_Fail2(err, "DistanceModel"); }

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
	ALenum err;
	if (!ctx->Count) return 0;

	ctx->Count  = 0;
	struct AudioFormat fmt = { 0 };
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
	ErrorHandler_Fail("Unsupported audio format"); return 0;
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
	WCHAR* dst = data;
	int i;
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");

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
	if (res) ErrorHandler_Fail2(res, "WSAStartup failed");
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
	WCHAR str[300];
	HINSTANCE instance;
	Platform_ConvertString(str, args);
	instance = ShellExecuteW(NULL, NULL, str, NULL, NULL, SW_SHOWNORMAL);
	return instance > 32 ? 0 : (ReturnCode)instance;
}

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

	for (i = 0; i < PROGRAM_MAX_CMDARGS; i++) {
		args[i] = Platform_NextArg(&cmdArgs);

		if (!args[i].length) break;
	}
	return i;
}
#endif
#ifdef CC_BUILD_POSIX
int Platform_ConvertString(void* data, const String* src) {
	uint8_t* dst = data;
	uint8_t* cur;

	Codepoint cp;
	int i, len = 0;
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");

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

void Platform_Exit(ReturnCode code) { exit(code); }

int Platform_GetCommandLineArgs(int argc, STRING_REF const char** argv, String* args) {
	int i, count;
	argc--; /* skip executable path argument */
	count = min(argc, PROGRAM_MAX_CMDARGS);

	for (i = 0; i < count; i++) {
		args[i] = String_FromReadonly(argv[i + 1]);
	}
	return count;
}

static ReturnCode Platform_RunOpen(const char* format, const String* args) {
	char str[300];
	FILE* fp;
	String path; char pathBuffer[FILENAME_SIZE + 10];

	String_InitArray(path, pathBuffer);
	String_Format1(&path, format, args);
	Platform_ConvertString(str, &path);

	fp = popen(str, "r");
	if (!fp) return errno;
	return Nix_Return(pclose(fp));
}

static void Platform_TrimFilename(char* path, int len) {
	/* get rid of filename at end of directory */
	for (; len > 0; len--) {
		if (path[len] == '/' || path[len] == '\\') break;
		path[len] = '\0';
	}
}
#endif
#ifdef CC_BUILD_X11
static void Platform_InitDisplay(void) {
	Display* display = XOpenDisplay(NULL);
	int screen;
	if (!display) ErrorHandler_Fail("Failed to open display");

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
ReturnCode Platform_StartShell(const String* args) {
	return Platform_RunOpen("xdg-open %s", args);
}
static void Platform_InitStopwatch(void) { sw_freqDiv = 1000; }

void Platform_SetWorkingDir(void) {
	char path[FILENAME_SIZE + 1] = { 0 };
	int len = readlink("/proc/self/exe", path, FILENAME_SIZE);
	if (len <= 0) return;

	Platform_TrimFilename(path, len);
	chdir(path);
}
#endif
#ifdef CC_BUILD_SOLARIS
ReturnCode Platform_StartShell(const String* args) {
	/* TODO: Is this on solaris, or just an OpenIndiana thing */
	return Platform_RunOpen("xdg-open %s", args);
}
static void Platform_InitStopwatch(void) { sw_freqDiv = 1000; }

void Platform_SetWorkingDir(void) {
	char path[FILENAME_SIZE + 1] = { 0 };
	int len = readlink("/proc/self/path/a.out", path, FILENAME_SIZE);
	if (len <= 0) return;

	Platform_TrimFilename(path, len);
	chdir(path);
}
#endif
#ifdef CC_BUILD_OSX
ReturnCode Platform_StartShell(const String* args) {
	return Platform_RunOpen("/usr/bin/open %s", args);
}
void Platform_SetWorkingDir(void) {
	char path[1024];
	CFBundleRef bundle;
	CFURLRef bundleUrl;
	CFStringRef cfPath;
	int len;

	bundle = CFBundleGetMainBundle();
	if (!bundle) return;
	bundleUrl = CFBundleCopyBundleURL(bundle);
	if (!bundleUrl) return;

	cfPath = CFURLCopyFileSystemPath(bundleUrl, kCFURLPOSIXPathStyle);
	if (!cfPath) return;
	CFStringGetCString(cfPath, path, Array_Elems(path), kCFStringEncodingUTF8);

	CFRelease(bundleUrl);
	CFRelease(cfPath);

	len = String_CalcLen(path, Array_Elems(path));
	Platform_TrimFilename(path, len);
	chdir(path);
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
