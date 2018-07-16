#include "Platform.h"
#define CC_BUILD_X11 1
#if CC_BUILD_X11
#include "PackedCol.h"
#include "Drawer2D.h"
#include "Stream.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

UChar* Platform_NewLine = "\n";
UChar Platform_DirectorySeparator = '/';
extern ReturnCode ReturnCode_FileShareViolation = 1000000000; /* TODO: not used apparently */
extern ReturnCode ReturnCode_FileNotFound = ENOENT;
extern ReturnCode ReturnCode_NotSupported = EPERM;
extern ReturnCode ReturnCode_SocketInProgess;
extern ReturnCode ReturnCode_SocketWouldBlock = EWOULDBLOCK;

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
void Platform_Exit(ReturnCode code);
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

void Platform_Log(STRING_PURE String* message);
void Platform_LogConst(const UChar* message);
#define Platform_Log1(format, a1) Platform_Log4(format, a1, NULL, NULL, NULL)
#define Platform_Log2(format, a1, a2) Platform_Log4(format, a1, a2, NULL, NULL)
#define Platform_Log3(format, a1, a2, a3) Platform_Log4(format, a1, a2, a3, NULL)
void Platform_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4);
void Platform_CurrentUTCTime(DateTime* time);
void Platform_CurrentLocalTime(DateTime* time);

bool Platform_DirectoryExists(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	struct stat sb;
	return stat(data, &sb) == 0 && S_ISDIR(sb.st_mode);
}

ReturnCode Platform_DirectoryCreate(STRING_PURE String* path) {
	UInt8 data[1024]; Platform_UnicodeExpand(data, path);
	/* read/write/search permissions for owner and group, and with read/search permissions for others. */
	int result = mkdir(data, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	return result == 0 ? 0 : errno;
}

bool Platform_FileExists(STRING_PURE String* path);
typedef void Platform_EnumFilesCallback(STRING_PURE String* filename, void* obj);
ReturnCode Platform_EnumFiles(STRING_PURE String* path, void* obj, Platform_EnumFilesCallback callback);
ReturnCode Platform_FileGetWriteTime(STRING_PURE String* path, DateTime* time);

ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path);
ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path);
ReturnCode Platform_FileAppend(void** file, STRING_PURE String* path);
ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);
ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten);
ReturnCode Platform_FileClose(void* file);
ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType);
ReturnCode Platform_FilePosition(void* file, UInt32* position);
ReturnCode Platform_FileLength(void* file, UInt32* length);

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

void Platform_SocketCreate(void** socket);
ReturnCode Platform_SocketAvailable(void* socket, UInt32* available);
ReturnCode Platform_SocketSetBlocking(void* socket, bool blocking);
ReturnCode Platform_SocketGetError(void* socket, ReturnCode* result);

ReturnCode Platform_SocketConnect(void* socket, STRING_PURE String* ip, Int32 port);
ReturnCode Platform_SocketRead(void* socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Platform_SocketWrite(void* socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Platform_SocketClose(void* socket);
ReturnCode Platform_SocketSelect(void* socket, Int32 selectMode, bool* success);

void Platform_HttpInit(void);
ReturnCode Platform_HttpMakeRequest(struct AsyncRequest* request, void** handle);
ReturnCode Platform_HttpGetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size);
ReturnCode Platform_HttpGetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress);
ReturnCode Platform_HttpFreeRequest(void* handle);
ReturnCode Platform_HttpFree(void);
#endif
