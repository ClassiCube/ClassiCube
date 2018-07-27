#include "Platform.h"
#if CC_BUILD_NIX
#include "PackedCol.h"
#include "Drawer2D.h"
#include "Stream.h"
#include "DisplayDevice.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Constants.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h> 
#include <X11/Xlib.h>

#define UNIX_EPOCH 62135596800

UChar* Platform_NewLine = "\n";
UChar Platform_DirectorySeparator = '/';
ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
ReturnCode ReturnCode_FileNotFound = ENOENT;
ReturnCode ReturnCode_NotSupported = EPERM;
ReturnCode ReturnCode_InvalidArg = EINVAL;

void Platform_UnicodeExpand(void* dstPtr, STRING_PURE String* src) {
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");
	UInt8* dst = dstPtr;

	Int32 i;
	for (i = 0; i < src->length; i++) {
		UInt16 codepoint = Convert_CP437ToUnicode(src->buffer[i]);
		Int32 len = Stream_WriteUtf8(dst, codepoint); dst += len;
	}
	*dst = NULL;
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

pthread_mutex_t event_mutex;
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

void* Platform_MemAlloc(UInt32 numElems, UInt32 elemsSize) { 
	return malloc(numElems * elemsSize); 
}

void* Platform_MemAllocCleared(UInt32 numElems, UInt32 elemsSize) {
	return calloc(numElems, elemsSize);
}

void* Platform_MemRealloc(void* mem, UInt32 numElems, UInt32 elemsSize) {
	return realloc(mem, numElems * elemsSize);
}

void Platform_MemFree(void** mem) {
	if (mem == NULL || *mem == NULL) return;
	free(*mem);
	*mem = NULL;
}

void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes) {
	memset(dst, value, numBytes);
}

void Platform_MemCpy(void* dst, void* src, UInt32 numBytes) {
	memcpy(dst, src, numBytes);
}

void Platform_Log(STRING_PURE String* message) { puts(message->buffer); }
void Platform_LogConst(const UChar* message) { puts(message); }
void Platform_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4) {
	UChar msgBuffer[String_BufferSize(512)];
	String msg = String_InitAndClearArray(msgBuffer);
	String_Format4(&msg, format, a1, a2, a3, a4);
	Platform_Log(&msg);
}

void Platform_FromSysTime(DateTime* time, struct tm* sysTime) {
	time->Year = sysTime->tm_year + 1900;
	time->Month = sysTime->tm_mon + 1;
	time->Day = sysTime->tm_mday;
	time->Hour = sysTime->tm_hour;
	time->Minute = sysTime->tm_min;
	time->Second = sysTime->tm_sec;
	time->Milli = 0;
}

void Platform_CurrentUTCTime(DateTime* time_) {
	struct timeval cur; struct tm utc_time;
	gettimeofday(&cur, NULL);
	time_->Milli = cur.tv_usec / 1000;

	gmtime_r(&cur.tv_sec, &utc_time);
	Platform_FromSysTime(time_, &utc_time);
}

void Platform_CurrentLocalTime(DateTime* time_) {
	struct timeval cur; struct tm loc_time;
	gettimeofday(&cur, NULL);
	time_->Milli = cur.tv_usec / 1000;

	localtime_r(&cur.tv_sec, &loc_time);
	Platform_FromSysTime(time_, &loc_time);
}

bool Platform_DirectoryExists(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	struct stat sb;
	return stat(data, &sb) == 0 && S_ISDIR(sb.st_mode);
}

ReturnCode Platform_DirectoryCreate(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	/* TODO: Is the default mode in all cases */
	return mkdir(data, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1 ? errno : 0;
}

bool Platform_FileExists(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	struct stat sb;
	return stat(data, &sb) == 0 && S_ISREG(sb.st_mode);
}

ReturnCode Platform_EnumFiles(STRING_PURE String* path, void* obj, Platform_EnumFilesCallback callback) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
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

ReturnCode Platform_FileGetModifiedTime(STRING_PURE String* path, DateTime* time) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	struct stat sb;
	if (stat(data, &sb) == -1) return errno;

	DateTime_FromTotalMs(time, UNIX_EPOCH + sb.st_mtime); 
	return 0;
}

ReturnCode Platform_FileDo(void** file, STRING_PURE String* path, int mode) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	*file = open(data, mode);
	return *file == -1 ? errno : 0;
}

ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path) {
	return Platform_FileDo(file, path, O_RDONLY);
}
ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path) {
	return Platform_FileDo(file, path, O_WRONLY | O_CREAT | O_TRUNC);
}
ReturnCode Platform_FileAppend(void** file, STRING_PURE String* path) {
	ReturnCode code = Platform_FileDo(file, path, O_WRONLY | O_CREAT);
	if (code != 0) return code;
	return Platform_FileSeek(*file, 0, STREAM_SEEKFROM_END);
}

ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead) {
	ssize_t bytes = read((int)file, buffer, count);
	if (bytes == -1) { *bytesRead = 0; return errno; }
	*bytesRead = bytes; return 0;
}

ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWrote) {
	ssize_t bytes = write((int)file, buffer, count);
	if (bytes == -1) { *bytesWrote = 0; return errno; }
	*bytesWrote = bytes; return 0;
}

ReturnCode Platform_FileClose(void* file) {
	return close((int)file) == -1 ? errno : 0;
}

ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType) {
	int mode = -1;
	switch (seekType) {
	case STREAM_SEEKFROM_BEGIN:   mode = SEEK_SET; break;
	case STREAM_SEEKFROM_CURRENT: mode = SEEK_CUR; break;
	case STREAM_SEEKFROM_END:     mode = SEEK_END; break;
	}

	return lseek((int)file, offset, mode) == -1 ? errno : 0;
}

ReturnCode Platform_FilePosition(void* file, UInt32* position) {
	off_t pos = lseek((int)file, 0, SEEK_CUR);
	if (pos == -1) { *position = -1; return errno; }
	*position = pos; return 0;
}

ReturnCode Platform_FileLength(void* file, UInt32* length) {
	struct stat st;
	if (fstat((int)file, &st) == -1) { *length = -1; return errno; }
	*length = st.st_size; return 0;
}

void Platform_ThreadSleep(UInt32 milliseconds) {
	usleep(milliseconds * 1000);
}

void* Platform_ThreadStartCallback(void* lpParam) {
	Platform_ThreadFunc* func = (Platform_ThreadFunc*)lpParam;
	(*func)();
	return NULL;
}

pthread_t threadList[3]; Int32 threadIndex;
void* Platform_ThreadStart(Platform_ThreadFunc* func) {
	if (threadIndex == Array_Elems(threadList)) ErrorHandler_Fail("Cannot allocate thread");
	pthread_t* ptr = &threadList[threadIndex];
	int result = pthread_create(ptr, NULL, Platform_ThreadStartCallback, func);

	ErrorHandler_CheckOrFail(result, "Creating thread");
	threadIndex++; return ptr;
}

void Platform_ThreadJoin(void* handle) {
	int result = pthread_join(*((pthread_t*)handle), NULL);
	ErrorHandler_CheckOrFail(result, "Joining thread");
}

void Platform_ThreadFreeHandle(void* handle) {
	int result = pthread_detach(*((pthread_t*)handle));
	ErrorHandler_CheckOrFail(result, "Detaching thread");
}

pthread_mutex_t mutexList[3]; Int32 mutexIndex;
void* Platform_MutexCreate(void) {
	if (mutexIndex == Array_Elems(mutexList)) ErrorHandler_Fail("Cannot allocate mutex");
	pthread_mutex_t* ptr = &mutexList[mutexIndex];
	int result = pthread_mutex_init(ptr, NULL);

	ErrorHandler_CheckOrFail(result, "Creating mutex");
	mutexIndex++; return ptr;
}

void Platform_MutexFree(void* handle) {
	int result = pthread_mutex_destroy((pthread_mutex_t*)handle);
	ErrorHandler_CheckOrFail(result, "Destroying mutex");
}

void Platform_MutexLock(void* handle) {
	int result = pthread_mutex_lock((pthread_mutex_t*)handle);
	ErrorHandler_CheckOrFail(result, "Locking mutex");
}

void Platform_MutexUnlock(void* handle) {
	int result = pthread_mutex_unlock((pthread_mutex_t*)handle);
	ErrorHandler_CheckOrFail(result, "Unlocking mutex");
}

pthread_cond_t condList[2]; Int32 condIndex;
void* Platform_EventCreate(void) {
	if (condIndex == Array_Elems(condList)) ErrorHandler_Fail("Cannot allocate event");
	pthread_cond_t* ptr = &condList[condIndex];
	int result = pthread_cond_init(ptr, NULL);

	ErrorHandler_CheckOrFail(result, "Creating event");
	condIndex++; return ptr;
}

void Platform_EventFree(void* handle) {
	int result = pthread_cond_destroy((pthread_cond_t*)handle);
	ErrorHandler_CheckOrFail(result, "Destroying event");
}

void Platform_EventSignal(void* handle) {
	int result = pthread_cond_signal((pthread_cond_t*)handle);
	ErrorHandler_CheckOrFail(result, "Signalling event");
}

void Platform_EventWait(void* handle) {
	int result = pthread_cond_wait((pthread_cond_t*)handle, &event_mutex);
	ErrorHandler_CheckOrFail(result, "Waiting event");
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

/* TODO: Implement these stubs */
void Platform_FontMake(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style) { desc->Size = size; }
void Platform_FontFree(struct FontDesc* desc) { }
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args) { }
void Platform_SetBitmap(struct Bitmap* bmp) { }
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col) { }
void Platform_ReleaseBitmap(void) { }

/* TODO: Implement these stubs */
void Platform_HttpInit(void) { }
ReturnCode Platform_HttpMakeRequest(struct AsyncRequest* request, void** handle) { return 1; }
ReturnCode Platform_HttpGetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size) { return 1; }
ReturnCode Platform_HttpGetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress) { return 1; }
ReturnCode Platform_HttpFreeRequest(void* handle) { return 1; }
ReturnCode Platform_HttpFree(void) { return 1; }
#endif
