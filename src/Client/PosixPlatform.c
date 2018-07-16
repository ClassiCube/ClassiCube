#include "Platform.h"
#define CC_BUILD_X11 1
#if CC_BUILD_X11
#include "PackedCol.h"
#include "Drawer2D.h"
#include "Stream.h"
#include "ErrorHandler.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h> 
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define UNIX_EPOCH 62135596800

UChar* Platform_NewLine = "\n";
UChar Platform_DirectorySeparator = '/';
ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
ReturnCode ReturnCode_FileNotFound = ENOENT;
ReturnCode ReturnCode_NotSupported = EPERM;
extern ReturnCode ReturnCode_SocketInProgess;
ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;

static void Platform_UnicodeExpand(UInt8* dst, STRING_PURE String* src) {
	if (src->length > FILENAME_SIZE) ErrorHandler_Fail("String too long to expand");

	Int32 i;
	for (i = 0; i < src->length; i++) {
		UInt16 codepoint = Convert_CP437ToUnicode(src->buffer[i]);
		Int32 len = Stream_WriteUtf8(dst, codepoint); dst += len;
	}
	*dst = NULL;
}

void Platform_Init(void);
void Platform_Free(void);
void Platform_Exit(ReturnCode code) { exit(code); }
STRING_PURE String Platform_GetCommandLineArgs(void);

void* Platform_MemAlloc(UInt32 numElems, UInt32 elemsSize) { 
	return malloc(numElems * elemsSize); 
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

void Platform_Log(STRING_PURE String* message) { puts(message->buffer); puts("\n"); }
void Platform_LogConst(const UChar* message) { puts(message); puts("\n"); }
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
	if (dirPtr == NULL) return errno;

	struct dirent* entry;
	while (entry = readdir(dirPtr)) {
		puts(ep->d_name);
		/* TODO: does this also include subdirectories */
	}

	int result = errno; /* return code from readdir */
	closedir(dirPtr);
	return  result;
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

void Platform_ThreadSleep(UInt32 milliseconds);
typedef void Platform_ThreadFunc(void);
void* Platform_ThreadStart(Platform_ThreadFunc* func);
void Platform_ThreadJoin(void* handle);
/* Frees handle to thread - NOT THE THREAD ITSELF */
void Platform_ThreadFreeHandle(void* handle);

void* Platform_MutexCreate(void);
void Platform_MutexFree(void* handle);
void Platform_MutexLock(void* handle);
void Platform_MutexUnlock(void* handle);

void* Platform_EventCreate(void);
void Platform_EventFree(void* handle);
void Platform_EventSet(void* handle);
void Platform_EventWait(void* handle);

void Stopwatch_Start(Stopwatch* timer);
Int32 Stopwatch_ElapsedMicroseconds(Stopwatch* timer);

void Platform_FontMake(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style);
void Platform_FontFree(struct FontDesc* desc);
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args);
void Platform_SetBitmap(struct Bitmap* bmp);
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col);
void Platform_ReleaseBitmap(void);

void Platform_SocketCreate(void** socketResult) {
	*socketResult = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*socketResult == -1) {
		ErrorHandler_FailWithCode(errno, "Failed to create socket");
	}
}

ReturnCode Platform_SocketAvailable(void* socket, UInt32* available) {
	return ioctl(socket, FIONREAD, available);
}

ReturnCode Platform_SocketSetBlocking(void* socket, bool blocking) {
	Int32 blocking_raw = blocking ? 0 : -1;
	return ioctl(socket, FIONBIO, &blocking_raw);
}

ReturnCode Platform_SocketGetError(void* socket, ReturnCode* result) {
	Int32 resultSize = sizeof(ReturnCode);
	return getsockopt(socket, SOL_SOCKET, SO_ERROR, result, &resultSize);
}

ReturnCode Platform_SocketConnect(void* socket, STRING_PURE String* ip, Int32 port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip->buffer);
	addr.sin_port = htons((UInt16)port);

	ReturnCode result = connect(socket, (struct sockaddr*)(&addr), sizeof(addr));
	return result == -1 ? errno : 0;
}

ReturnCode Platform_SocketRead(void* socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 recvCount = recv(socket, buffer, count, 0);
	if (recvCount != -1) { *modified = recvCount; return 0; }
	*modified = 0; return errno;
}

ReturnCode Platform_SocketWrite(void* socket, UInt8* buffer, UInt32 count, UInt32* modified) {
	Int32 sentCount = send(socket, buffer, count, 0);
	if (sentCount != -1) { *modified = sentCount; return 0; }
	*modified = 0; return errno;
}

ReturnCode Platform_SocketClose(void* socket) {
	ReturnCode result = 0;
	ReturnCode result1 = shutdown(socket, SHUT_RDWR);
	if (result1 == -1) result = errno;

	ReturnCode result2 = closesocket(socket);
	if (result2 == -1) result = errno;
	return result;
}

ReturnCode Platform_SocketSelect(void* socket, Int32 selectMode, bool* success) {
	void* args[2]; args[0] = (void*)1; args[1] = socket;
	struct timeval time = { 0 };
	Int32 selectCount;

	if (selectMode == SOCKET_SELECT_READ) {
		selectCount = select(1, &args, NULL, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_WRITE) {
		selectCount = select(1, NULL, &args, NULL, &time);
	} else if (selectMode == SOCKET_SELECT_ERROR) {
		selectCount = select(1, NULL, NULL, &args, &time);
	} else {
		selectCount = -1;
	}

	if (selectCount != -1) { *success = args[0] != 0; return 0; }
	*success = false; return errno;
}

void Platform_HttpInit(void);
ReturnCode Platform_HttpMakeRequest(struct AsyncRequest* request, void** handle);
ReturnCode Platform_HttpGetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size);
ReturnCode Platform_HttpGetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress);
ReturnCode Platform_HttpFreeRequest(void* handle);
ReturnCode Platform_HttpFree(void);
#endif
