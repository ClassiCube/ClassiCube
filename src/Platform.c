#include "Platform.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Drawer2D.h"
#include "Funcs.h"
#include "AsyncDownloader.h"

#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <mmsystem.h>

#define HTTP_QUERY_ETAG 54 /* Missing from some old MingW32 headers */
#define Socket__Error() WSAGetLastError()
#define Win_Return(success) ((success) ? 0 : GetLastError())

HDC hdc;
HANDLE heap;
UChar* Platform_NewLine = "\r\n";
UChar Directory_Separator = '\\';

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

#define UNIX_EPOCH 62135596800
#define Socket__Error() errno
#define Nix_Return(success) ((success) ? 0 : errno)

pthread_mutex_t event_mutex;

UChar* Platform_NewLine = "\n";
UChar Directory_Separator = '/';
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
static void Platform_AllocFailed(const UChar* place) {
	UChar logBuffer[String_BufferSize(STRING_SIZE + 20)];
	String log = String_InitAndClearArray(logBuffer);
	String_Format1(&log, "Failed allocating memory for: %c", place);
	ErrorHandler_Fail(log.buffer);
}

void Mem_Set(void* dst, UInt8 value, UInt32 numBytes) { memset(dst, value, numBytes); }
void Mem_Copy(void* dst, void* src, UInt32 numBytes)  { memcpy(dst, src,   numBytes); }

#if CC_BUILD_WIN
void* Mem_Alloc(UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, 0, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapAlloc(heap, HEAP_ZERO_MEMORY, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	UInt32 numBytes = numElems * elemsSize; /* TODO: avoid overflow here */
	void* ptr = HeapReAlloc(heap, 0, mem, numBytes);
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void Mem_Free(void** mem) {
	if (mem == NULL || *mem == NULL) return;
	HeapFree(heap, 0, *mem);
	*mem = NULL;
}
#elif CC_BUILD_NIX
void* Mem_Alloc(UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	void* ptr = malloc(numElems * elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_AllocCleared(UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	void* ptr = calloc(numElems, elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void* Mem_Realloc(void* mem, UInt32 numElems, UInt32 elemsSize, const UChar* place) {
	void* ptr = realloc(mem, numElems * elemsSize); /* TODO: avoid overflow here */
	if (!ptr) Platform_AllocFailed(place);
	return ptr;
}

void Mem_Free(void** mem) {
	if (mem == NULL || *mem == NULL) return;
	free(*mem);
	*mem = NULL;
}
#endif


/*########################################################################################################################*
*------------------------------------------------------Logging/Time-------------------------------------------------------*
*#########################################################################################################################*/
void Platform_Log1(const UChar* format, const void* a1) {
	Platform_Log4(format, a1, NULL, NULL, NULL);
}
void Platform_Log2(const UChar* format, const void* a1, const void* a2) {
	Platform_Log4(format, a1, a2, NULL, NULL);
}
void Platform_Log3(const UChar* format, const void* a1, const void* a2, const void* a3) {
	Platform_Log4(format, a1, a2, a3, NULL);
}

void Platform_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	UChar msgBuffer[String_BufferSize(512)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(&msg);
}

#if CC_BUILD_WIN
void Platform_Log(STRING_PURE String* message) {
	/* TODO: log to console */
	OutputDebugStringA(message->buffer);
	OutputDebugStringA("\n");
}

void Platform_LogConst(const UChar* message) {
	/* TODO: log to console */
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
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

bool stopwatch_highResolution;
LARGE_INTEGER stopwatch_freq;
void Stopwatch_Measure(struct Stopwatch* timer) {
	if (stopwatch_highResolution) {
		LARGE_INTEGER value;
		QueryPerformanceCounter(&value);
		timer->Data[0] = value.QuadPart;
	} else {
		FILETIME value;
		GetSystemTimeAsFileTime(&value);
		timer->Data[0] = (Int64)value.dwLowDateTime | ((Int64)value.dwHighDateTime << 32);
	}
}
void Stopwatch_Start(struct Stopwatch* timer) { Stopwatch_Measure(timer); }

/* TODO: check this is actually accurate */
Int32 Stopwatch_ElapsedMicroseconds(struct Stopwatch* timer) {
	Int64 start = timer->Data[0];
	Stopwatch_Measure(timer);
	Int64 end = timer->Data[0];

	if (stopwatch_highResolution) {
		return (Int32)(((end - start) * 1000 * 1000) / stopwatch_freq.QuadPart);
	} else {
		return (Int32)((end - start) / 10);
	}
}
#elif CC_BUILD_NIX
void Platform_Log(STRING_PURE String* message) { puts(message->buffer); }
void Platform_LogConst(const UChar* message) { puts(message); }

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

void Stopwatch_Measure(struct Stopwatch* timer) {
	struct timespec value;
	/* TODO: CLOCK_MONOTONIC_RAW ?? */
	clock_gettime(CLOCK_MONOTONIC, &value);
	timer->Data[0] = value.tv_sec;
	timer->Data[1] = value.tv_nsec;
}
void Stopwatch_Start(struct Stopwatch* timer) { Stopwatch_Measure(timer); }

/* TODO: check this is actually accurate */
Int32 Stopwatch_ElapsedMicroseconds(struct Stopwatch* timer) {
	Int64 startS = timer->Data[0], startNS = timer->Data[1];
	Stopwatch_Measure(timer);
	Int64 endS = timer->Data[0], endNS = timer->Data[1];

#define NS_PER_SEC 1000000000LL
	Int64 elapsedNS = ((endS - startS) * NS_PER_SEC + endNS) - startNS;
	return elapsedNS / 1000;
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------Directory/File------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
bool Directory_Exists(STRING_PURE String* path) {
	WCHAR data[512]; Platform_ConvertString(data, path);
	UInt32 attribs = GetFileAttributesW(data);
	return attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Create(STRING_PURE String* path) {
	WCHAR data[512]; Platform_ConvertString(data, path);
	BOOL success = CreateDirectoryW(data, NULL);
	return Win_Return(success);
}

bool File_Exists(STRING_PURE String* path) {
	WCHAR data[512]; Platform_ConvertString(data, path);
	UInt32 attribs = GetFileAttributesW(data);
	return attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
}

ReturnCode Directory_Enum(STRING_PURE String* path, void* obj, Directory_EnumCallback callback) {
	UChar fileBuffer[String_BufferSize(MAX_PATH + 10)];
	String file = String_InitAndClearArray(fileBuffer);
	/* Need to append \* to search for files in directory */
	String_Format1(&file, "%s\\*", path);
	WCHAR data[512]; Platform_ConvertString(data, &file);

	WIN32_FIND_DATAW entry;
	HANDLE find = FindFirstFileW(data, &entry);
	if (find == INVALID_HANDLE_VALUE) return GetLastError();

	do {
		if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		String_Clear(&file);
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

ReturnCode File_GetModifiedTime(STRING_PURE String* path, DateTime* time) {
	void* file; ReturnCode result = File_Open(&file, path);
	if (result) return result;

	FILETIME writeTime;
	if (GetFileTime(file, NULL, NULL, &writeTime)) {
		SYSTEMTIME sysTime;
		FileTimeToSystemTime(&writeTime, &sysTime);
		Platform_FromSysTime(time, &sysTime);
	} else {
		result = GetLastError();
	}

	File_Close(file);
	return result;
}

ReturnCode File_Do(void** file, STRING_PURE String* path, DWORD access, DWORD createMode) {
	WCHAR data[512]; Platform_ConvertString(data, path);
	*file = CreateFileW(data, access, FILE_SHARE_READ, NULL, createMode, 0, NULL);
	return Win_Return(*file != INVALID_HANDLE_VALUE);
}

ReturnCode File_Open(void** file, STRING_PURE String* path) {
	return File_Do(file, path, GENERIC_READ, OPEN_EXISTING);
}
ReturnCode File_Create(void** file, STRING_PURE String* path) {
	return File_Do(file, path, GENERIC_WRITE, CREATE_ALWAYS);
}
ReturnCode File_Append(void** file, STRING_PURE String* path) {
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
bool Directory_Exists(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_ConvertString(data, path);
	struct stat sb;
	return stat(data, &sb) == 0 && S_ISDIR(sb.st_mode);
}

ReturnCode Directory_Create(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_ConvertString(data, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return Nix_Return(mkdir(data, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1);
}

bool File_Exists(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_ConvertString(data, path);
	struct stat sb;
	return stat(data, &sb) == 0 && S_ISREG(sb.st_mode);
}

ReturnCode Directory_Enum(STRING_PURE String* path, void* obj, Directory_EnumCallback callback) {
	UInt8 data[1024]; Platform_ConvertString(data, path);
	DIR* dirPtr = opendir(data);
	if (!dirPtr) return errno;

	UInt8 fileBuffer[String_BufferSize(FILENAME_SIZE)];
	String file = String_InitAndClearArray(fileBuffer);
	struct dirent* entry;

	/* TODO: does this also include subdirectories */
	while (entry = readdir(dirPtr)) {
		UInt16 len = String_CalcLen(entry->d_name, UInt16_MaxValue);
		String_Clear(&file);
		String_DecodeUtf8(&file, entry->d_name, len);

		Utils_UNSAFE_GetFilename(&file);
		callback(&file, obj);
	}

	int result = errno; /* return code from readdir */
	closedir(dirPtr);
	return result;
}

ReturnCode File_GetModifiedTime(STRING_PURE String* path, DateTime* time) {
	UInt8 data[1024]; Platform_ConvertString(data, path);
	struct stat sb;
	if (stat(data, &sb) == -1) return errno;

	DateTime_FromTotalMs(time, UNIX_EPOCH + sb.st_mtime); 
	return 0;
}

ReturnCode File_Do(void** file, STRING_PURE String* path, int mode) {
	UInt8 data[1024]; Platform_ConvertString(data, path);
	*file = open(data, mode, (6 << 6) | (4 << 3) | 4); /* rw|r|r */
	return Nix_Return(*file != -1);
}

ReturnCode File_Open(void** file, STRING_PURE String* path) {
	return File_Do(file, path, O_RDONLY);
}
ReturnCode File_Create(void** file, STRING_PURE String* path) {
	return File_Do(file, path, O_WRONLY | O_CREAT | O_TRUNC);
}
ReturnCode File_Append(void** file, STRING_PURE String* path) {
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
DWORD WINAPI Thread_StartCallback(LPVOID lpParam) {
	Thread_StartFunc* func = (Thread_StartFunc*)lpParam;
	(*func)();
	return 0;
}

void* Thread_Start(Thread_StartFunc* func) {
	DWORD threadID;
	void* handle = CreateThread(NULL, 0, Thread_StartCallback, func, 0, &threadID);
	if (!handle) {
		ErrorHandler_FailWithCode(GetLastError(), "Creating thread");
	}
	return handle;
}

void Thread_Join(void* handle) {
	WaitForSingleObject((HANDLE)handle, INFINITE);
}

void Thread_FreeHandle(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Freeing thread handle");
	}
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
		ErrorHandler_FailWithCode(GetLastError(), "Creating waitable");
	}
	return handle;
}

void Waitable_Free(void* handle) {
	if (!CloseHandle((HANDLE)handle)) {
		ErrorHandler_FailWithCode(GetLastError(), "Freeing waitable");
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
void* Thread_Start(Thread_StartFunc* func) {
	if (threadIndex == Array_Elems(threadList)) ErrorHandler_Fail("Cannot allocate thread");
	pthread_t* ptr = &threadList[threadIndex];
	int result = pthread_create(ptr, NULL, Thread_StartCallback, func);

	ErrorHandler_CheckOrFail(result, "Creating thread");
	threadIndex++; return ptr;
}

void Thread_Join(void* handle) {
	int result = pthread_join(*((pthread_t*)handle), NULL);
	ErrorHandler_CheckOrFail(result, "Joining thread");
}

void Thread_FreeHandle(void* handle) {
	int result = pthread_detach(*((pthread_t*)handle));
	ErrorHandler_CheckOrFail(result, "Detaching thread");
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
#endif


/*########################################################################################################################*
*--------------------------------------------------------Font/Text--------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
int CALLBACK Font_GetNamesCallback(CONST LOGFONT* desc, CONST TEXTMETRIC* metrics, DWORD fontType, LPVOID obj) {
	UInt32 i;
	UChar nameBuffer[String_BufferSize(LF_FACESIZE)];
	String name = String_InitAndClearArray(nameBuffer);

	/* don't want international variations of font names too */
	if (desc->lfFaceName[0] == '@' || desc->lfCharSet != ANSI_CHARSET) return 1;
	
	if ((fontType & RASTER_FONTTYPE) || (fontType & TRUETYPE_FONTTYPE)) {
		for (i = 0; i < LF_FACESIZE && desc->lfFaceName[i]; i++) {
			String_Append(&name, Convert_UnicodeToCP437(desc->lfFaceName[i]));
		}
		StringsBuffer_Add((StringsBuffer*)obj, &name);
	}
	return 1;
}

void Font_GetNames(StringsBuffer* buffer) {
	EnumFontFamiliesW(hdc, NULL, Font_GetNamesCallback, buffer);
}

void Font_Make(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style) {
	desc->Size    = size; 
	desc->Style   = style;
	LOGFONTA font = { 0 };

	font.lfHeight    = -Math_CeilDiv(size * GetDeviceCaps(hdc, LOGPIXELSY), 72);
	font.lfUnderline = style == FONT_STYLE_UNDERLINE;
	font.lfWeight    = style == FONT_STYLE_BOLD ? FW_BOLD : FW_NORMAL;
	font.lfQuality   = ANTIALIASED_QUALITY; /* TODO: CLEARTYPE_QUALITY looks slightly better */

	String dstName = String_Init(font.lfFaceName, 0, LF_FACESIZE);
	String_AppendString(&dstName, fontName);
	desc->Handle = CreateFontIndirectA(&font);
	if (!desc->Handle) ErrorHandler_Fail("Creating font handle failed");
}

void Font_Free(struct FontDesc* desc) {
	if (!DeleteObject(desc->Handle)) ErrorHandler_Fail("Deleting font handle failed");
	desc->Handle = NULL;
}

/* TODO: not associate font with device so much */
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args) {
	WCHAR data[512]; Platform_ConvertString(data, &args->Text);
	HGDIOBJ oldFont = SelectObject(hdc, args->Font.Handle);
	SIZE area; GetTextExtentPointW(hdc, data, args->Text.length, &area);

	SelectObject(hdc, oldFont);
	return Size2D_Make(area.cx, area.cy);
}

HBITMAP platform_dib;
HBITMAP platform_oldBmp;
struct Bitmap* platform_bmp;
void* platform_bits;

void Platform_SetBitmap(struct Bitmap* bmp) {
	platform_bmp = bmp;
	platform_bits = NULL;

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bmp->Width;
	bmi.bmiHeader.biHeight = -bmp->Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	platform_dib = CreateDIBSection(hdc, &bmi, 0, &platform_bits, NULL, 0);
	if (!platform_dib) ErrorHandler_Fail("Failed to allocate DIB for text");
	platform_oldBmp = SelectObject(hdc, platform_dib);
}

/* TODO: check return codes and stuff */
/* TODO: make text prettier.. somehow? */
/* TODO: Do we need to / 255 instead of >> 8 ? */
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col) {
	WCHAR strUnicode[String_BufferSize(FILENAME_SIZE)];
	Platform_ConvertString(strUnicode, &args->Text);

	HGDIOBJ oldFont = (HFONT)SelectObject(hdc, (HFONT)args->Font.Handle);
	SIZE area; GetTextExtentPointW(hdc, strUnicode, args->Text.length, &area);
	TextOutW(hdc, 0, 0, strUnicode, args->Text.length);

	Int32 xx, yy;
	struct Bitmap* bmp = platform_bmp;
	for (yy = 0; yy < area.cy; yy++) {
		UInt8* src = (UInt8*)platform_bits + (yy * (bmp->Width << 2));
		UInt8* dst = (UInt8*)Bitmap_GetRow(bmp, y + yy); dst += x * BITMAP_SIZEOF_PIXEL;

		for (xx = 0; xx < area.cx; xx++) {
			UInt8 intensity = *src, invIntensity = UInt8_MaxValue - intensity;
			dst[0] = ((col.B * intensity) >> 8) + ((dst[0] * invIntensity) >> 8);
			dst[1] = ((col.G * intensity) >> 8) + ((dst[1] * invIntensity) >> 8);
			dst[2] = ((col.R * intensity) >> 8) + ((dst[2] * invIntensity) >> 8);
			//dst[3] = ((col.A * intensity) >> 8) + ((dst[3] * invIntensity) >> 8);
			dst[3] = intensity                  + ((dst[3] * invIntensity) >> 8);
			src += BITMAP_SIZEOF_PIXEL; dst += BITMAP_SIZEOF_PIXEL;
		}
	}

	SelectObject(hdc, oldFont);
	//DrawTextA(hdc, args->Text.buffer, args->Text.length,
	//	&r, DT_NOPREFIX | DT_SINGLELINE | DT_NOCLIP);
	return Size2D_Make(area.cx, area.cy);
}

void Platform_ReleaseBitmap(void) {
	/* TODO: Check return values */
	SelectObject(hdc, platform_oldBmp);
	DeleteObject(platform_dib);

	platform_oldBmp = NULL;
	platform_dib = NULL;
	platform_bmp = NULL;
}
#elif CC_BUILD_NIX
/* TODO: Implement these stubs */
void Font_Make(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style) { desc->Size = size; }
void Font_Free(struct FontDesc* desc) { }
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args) { }
void Platform_SetBitmap(struct Bitmap* bmp) { }
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col) { }
void Platform_ReleaseBitmap(void) { }
#endif


/*########################################################################################################################*
*---------------------------------------------------------Socket----------------------------------------------------------*
*#########################################################################################################################*/
void Socket_Create(SocketPtr* socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		ErrorHandler_FailWithCode(Socket__Error(), "Failed to create socket");
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

ReturnCode Socket_Connect(SocketPtr socket, STRING_PURE String* ip, Int32 port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip->buffer);
	addr.sin_port = htons((UInt16)port);

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

	if (selectMode == SOCKET_SELECT_READ) {
		selectCount = select(1, &set, NULL, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_WRITE) {
		selectCount = select(1, NULL, &set, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_ERROR) {
		selectCount = select(1, NULL, NULL, &set, &time);
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
void Http_Init(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(PROGRAM_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) ErrorHandler_FailWithCode(GetLastError(), "Failed to init WinINet");
}

ReturnCode Http_MakeRequest(struct AsyncRequest* request, void** handle) {
	String url = String_FromRawArray(request->URL);
	UChar headersBuffer[String_BufferSize(STRING_SIZE * 2)];
	String headers = String_MakeNull();

	/* https://stackoverflow.com/questions/25308488/c-wininet-custom-http-headers */
	if (request->Etag[0] || request->LastModified.Year) {
		headers = String_InitAndClearArray(headersBuffer);
		if (request->LastModified.Year > 0) {
			String_AppendConst(&headers, "If-Modified-Since: ");
			DateTime_HttpDate(&request->LastModified, &headers);
			String_AppendConst(&headers, "\r\n");
		}

		if (request->Etag[0]) {
			String etag = String_FromRawArray(request->Etag);
			String_AppendConst(&headers, "If-None-Match: ");
			String_AppendString(&headers, &etag);
			String_AppendConst(&headers, "\r\n");
		}
		String_AppendConst(&headers, "\r\n\r\n");
	}

	*handle = InternetOpenUrlA(hInternet, url.buffer, headers.buffer, headers.length,
		INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0);
	return Win_Return(*handle);
}

/* TODO: Test last modified and etag even work */
#define Http_Query(flags, result) HttpQueryInfoA(handle, flags, result, &bufferLen, NULL)
ReturnCode Http_GetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size) {
	DWORD bufferLen;

	UInt32 status;
	bufferLen = sizeof(DWORD);
	if (!Http_Query(HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status)) return GetLastError();
	request->StatusCode = status;

	bufferLen = sizeof(DWORD);
	if (!Http_Query(HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, size)) return GetLastError();

	SYSTEMTIME lastModified;
	bufferLen = sizeof(SYSTEMTIME);
	if (Http_Query(HTTP_QUERY_LAST_MODIFIED | HTTP_QUERY_FLAG_SYSTEMTIME, &lastModified)) {
		Platform_FromSysTime(&request->LastModified, &lastModified);
	}

	String etag = String_InitAndClearArray(request->Etag);
	bufferLen = etag.capacity;
	Http_Query(HTTP_QUERY_ETAG, etag.buffer);

	return 0;
}

ReturnCode Http_GetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress) {
	if (size == 0) return ERROR_NOT_SUPPORTED;
	*data = Mem_Alloc(size, sizeof(UInt8), "http get data");

	*progress = 0;
	UInt8* buffer = *data;
	UInt32 left = size, read, totalRead = 0;

	while (left > 0) {
		UInt32 toRead = left, avail = 0;
		/* only read as much data that is pending */
		if (InternetQueryDataAvailable(handle, &avail, 0, 0)) {
			toRead = min(toRead, avail);
		}

		bool success = InternetReadFile(handle, buffer, toRead, &read);
		if (!success) { Mem_Free(data); return GetLastError(); }

		if (!read) break;
		buffer += read; totalRead += read; left -= read;
		*progress = (Int32)(100.0f * totalRead / size);
	}

	*progress = 100;
	return 0;
}

ReturnCode Http_FreeRequest(void* handle) { return Win_Return(InternetCloseHandle(handle)); }
ReturnCode Http_Free(void) { return Win_Return(InternetCloseHandle(hInternet)); }
#elif CC_BUILD_NIX
void Http_Init(void) { }
ReturnCode Http_MakeRequest(struct AsyncRequest* request, void** handle) { return 1; }
ReturnCode Http_GetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size) { return 1; }
ReturnCode Http_GetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress) { return 1; }
ReturnCode Http_FreeRequest(void* handle) { return 1; }
ReturnCode Http_Free(void) { return 1; }
#endif


/*########################################################################################################################*
*----------------------------------------------------------Audio----------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
struct AudioContext {
	HWAVEOUT Handle;
	WAVEHDR Headers[AUDIO_MAX_CHUNKS];
	struct AudioFormat Format;
	Int32 NumBuffers;
};
struct AudioContext Audio_Contexts[20];

void Audio_Init(AudioHandle* handle, Int32 buffers) {
	Int32 i, j;
	for (i = 0; i < Array_Elems(Audio_Contexts); i++) {
		struct AudioContext* ctx = &Audio_Contexts[i];
		if (ctx->NumBuffers) continue;
		ctx->NumBuffers = buffers;

		*handle = i;
		for (j = 0; j < buffers; j++) {
			ctx->Headers[j].dwFlags = WHDR_DONE;
		}
		return;
	}
	ErrorHandler_Fail("No free audio contexts");
}

void Audio_Free(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	if (!ctx->Handle) return;

	ReturnCode result = waveOutClose(ctx->Handle);
	ErrorHandler_CheckOrFail(result, "Audio - closing device");
	Mem_Set(ctx, 0, sizeof(struct AudioContext));
}

struct AudioFormat* Audio_GetFormat(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	return &ctx->Format;
}

void Audio_SetFormat(AudioHandle handle, struct AudioFormat* format) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	struct AudioFormat* cur = &ctx->Format;

	/* only recreate handle if we need to */
	if (AudioFormat_Eq(cur, format)) return;
	if (ctx->Handle) {
		ReturnCode result = waveOutClose(ctx->Handle);
		ErrorHandler_CheckOrFail(result, "Audio - closing device");
	}

	WAVEFORMATEX fmt = { 0 };
	fmt.nChannels = format->Channels;
	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.wBitsPerSample = format->BitsPerSample;
	fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
	fmt.nSamplesPerSec = format->SampleRate;
	fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

	if (waveOutGetNumDevs() == 0u) ErrorHandler_Fail("No audio devices found");
	ReturnCode result = waveOutOpen(&ctx->Handle, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
	ErrorHandler_CheckOrFail(result, "Audio - opening device");
	ctx->Format = *format;
}

void Audio_PlayData(AudioHandle handle, Int32 idx, void* data, UInt32 dataSize) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];
	Mem_Set(hdr, 0, sizeof(WAVEHDR));

	hdr->lpData = data;
	hdr->dwBufferLength = dataSize;
	hdr->dwLoops = 1;

	ReturnCode result = waveOutPrepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR));
	ErrorHandler_CheckOrFail(result, "Audio - prepare header");
	result = waveOutWrite(ctx->Handle, hdr, sizeof(WAVEHDR));
	ErrorHandler_CheckOrFail(result, "Audio - write header");
}

bool Audio_IsCompleted(AudioHandle handle, Int32 idx) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	WAVEHDR* hdr = &ctx->Headers[idx];
	if (!(hdr->dwFlags & WHDR_DONE)) return false;

	if (hdr->dwFlags & WHDR_PREPARED) {
		ReturnCode result = waveOutUnprepareHeader(ctx->Handle, hdr, sizeof(WAVEHDR));
		ErrorHandler_CheckOrFail(result, "Audio - unprepare header");
	}
	return true;
}

bool Audio_IsFinished(AudioHandle handle) {
	struct AudioContext* ctx = &Audio_Contexts[handle];
	Int32 i;
	for (i = 0; i < ctx->NumBuffers; i++) {
		if (!Audio_IsCompleted(handle, i)) return false;
	}
	return true;
}
#elif CC_BUILD_NIX
#endif


/*########################################################################################################################*
*--------------------------------------------------------Platform---------------------------------------------------------*
*#########################################################################################################################*/
#if CC_BUILD_WIN
void Platform_ConvertString(void* dstPtr, STRING_PURE String* src) {
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

	stopwatch_highResolution = QueryPerformanceFrequency(&stopwatch_freq);
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
	WCHAR dirName[FILENAME_SIZE + 1] = { 0 };
	DWORD len = GetModuleFileNameW(NULL, dirName, FILENAME_SIZE);
	if (len == 0) return;

	/* get rid of filename at end of directory*/
	for (; len > 0; len--) {
		if (dirName[len] == '/' || dirName[len] == '\\') break;
		dirName[len] = '\0';
	}

	SetCurrentDirectoryW(dirName);
}

void Platform_Exit(ReturnCode code) { ExitProcess(code); }

STRING_PURE String Platform_GetCommandLineArgs(void) {
	String args = String_FromReadonly(GetCommandLineA());

	Int32 argsStart;
	if (args.buffer[0] == '"') {
		/* Handle path argument in full "path" form, which can include spaces */
		argsStart = String_IndexOf(&args, '"', 1) + 1;
	} else {
		argsStart = String_IndexOf(&args, ' ', 0) + 1;
	}

	if (argsStart == 0) argsStart = args.length;
	args = String_UNSAFE_SubstringAt(&args, argsStart);

	/* get rid of duplicate leading spaces before first arg */
	while (args.length && args.buffer[0] == ' ') {
		args = String_UNSAFE_SubstringAt(&args, 1);
	}
	return args;
}
#elif CC_BUILD_NIX
void Platform_ConvertString(void* dstPtr, STRING_PURE String* src) {
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

	/* TODO: Use Xinerama and XRandR for querying these */
	struct DisplayDevice device = { 0 };
	device.Bounds.Width   = DisplayWidth(display, screen);
	device.Bounds.Height  = DisplayHeight(display, screen);
	device.BitsPerPixel   = DefaultDepth(display, screen);
	DisplayDevice_Default = device;

	DisplayDevice_Meta[0] = display;
	DisplayDevice_Meta[1] = screen;
	DisplayDevice_Meta[2] = rootWin;
}

void Platform_Init(void) {
	Platform_InitDisplay();
	pthread_mutex_init(&event_mutex, NULL);
}

void Platform_Free(void) {
	pthread_mutex_destroy(&event_mutex);
}

void Platform_Exit(ReturnCode code) { exit(code); }

STRING_PURE String Platform_GetCommandLineArgs(void) {
	/* TODO: Implement this */
	return String_MakeNull();
}
#endif
