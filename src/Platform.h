#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "Utils.h"
#include "2DStructs.h"
#include "PackedCol.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
struct DrawTextArgs;
struct FontDesc;
struct Bitmap;
struct AsyncRequest;

enum SOCKET_SELECT { SOCKET_SELECT_READ, SOCKET_SELECT_WRITE, SOCKET_SELECT_ERROR };
#if CC_BUILD_WIN
typedef void* SocketPtr;
#else
typedef Int32 SocketPtr;
#endif

extern UChar* Platform_NewLine; /* Newline for text */
extern UChar Directory_Separator;
extern ReturnCode ReturnCode_FileShareViolation;
extern ReturnCode ReturnCode_FileNotFound;
extern ReturnCode ReturnCode_NotSupported;
extern ReturnCode ReturnCode_SocketInProgess;
extern ReturnCode ReturnCode_SocketWouldBlock;
extern ReturnCode ReturnCode_InvalidArg;

void Platform_ConvertString(void* dstPtr, STRING_PURE String* src);
void Platform_Init(void);
void Platform_Free(void);
void Platform_SetWorkingDir(void);
void Platform_Exit(ReturnCode code);
STRING_PURE String Platform_GetCommandLineArgs(void);
ReturnCode Platform_StartShell(STRING_PURE String* args);

FUNC_ATTRIB(noinline) void* Mem_Alloc(UInt32 numElems, UInt32 elemsSize, const UChar* place);
FUNC_ATTRIB(noinline) void* Mem_AllocCleared(UInt32 numElems, UInt32 elemsSize, const UChar* place);
FUNC_ATTRIB(noinline) void* Mem_Realloc(void* mem, UInt32 numElems, UInt32 elemsSize, const UChar* place);
FUNC_ATTRIB(noinline) void  Mem_Free(void* mem);
void Mem_Set(void* dst, UInt8 value, UInt32 numBytes);
void Mem_Copy(void* dst, void* src, UInt32 numBytes);

void Platform_Log(STRING_PURE String* message);
void Platform_LogConst(const UChar* message);
void Platform_Log1(const UChar* format, const void* a1);
void Platform_Log2(const UChar* format, const void* a1, const void* a2);
void Platform_Log3(const UChar* format, const void* a1, const void* a2, const void* a3);
void Platform_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4);

void DateTime_CurrentUTC(DateTime* time);
void DateTime_CurrentLocal(DateTime* time);
struct Stopwatch { Int64 Data[2]; };
void Stopwatch_Start(struct Stopwatch* timer);
Int32 Stopwatch_ElapsedMicroseconds(struct Stopwatch* timer);

bool Directory_Exists(STRING_PURE String* path);
ReturnCode Directory_Create(STRING_PURE String* path);
typedef void Directory_EnumCallback(STRING_PURE String* filename, void* obj);
ReturnCode Directory_Enum(STRING_PURE String* path, void* obj, Directory_EnumCallback callback);
bool File_Exists(STRING_PURE String* path);
ReturnCode File_GetModifiedTime(STRING_PURE String* path, DateTime* time);

ReturnCode File_Create(void** file, STRING_PURE String* path);
ReturnCode File_Open(void** file, STRING_PURE String* path);
ReturnCode File_Append(void** file, STRING_PURE String* path);
ReturnCode File_Read(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);
ReturnCode File_Write(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWrote);
ReturnCode File_Close(void* file);
ReturnCode File_Seek(void* file, Int32 offset, Int32 seekType);
ReturnCode File_Position(void* file, UInt32* position);
ReturnCode File_Length(void* file, UInt32* length);

void Thread_Sleep(UInt32 milliseconds);
typedef void Thread_StartFunc(void);
void* Thread_Start(Thread_StartFunc* func);
void Thread_Join(void* handle);
/* Frees handle to thread - NOT THE THREAD ITSELF */
void Thread_FreeHandle(void* handle);

void* Mutex_Create(void);
void Mutex_Free(void* handle);
void Mutex_Lock(void* handle);
void Mutex_Unlock(void* handle);

void* Waitable_Create(void);
void Waitable_Free(void* handle);
void Waitable_Signal(void* handle);
void Waitable_Wait(void* handle); 
void Waitable_WaitFor(void* handle, UInt32 milliseconds);

void Font_GetNames(StringsBuffer* buffer);
void Font_Make(struct FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style);
void Font_Free(struct FontDesc* desc);
struct Size2D Platform_TextMeasure(struct DrawTextArgs* args);
void Platform_SetBitmap(struct Bitmap* bmp);
struct Size2D Platform_TextDraw(struct DrawTextArgs* args, Int32 x, Int32 y, PackedCol col);
void Platform_ReleaseBitmap(void);

void Socket_Create(SocketPtr* socket);
ReturnCode Socket_Available(SocketPtr socket, UInt32* available);
ReturnCode Socket_SetBlocking(SocketPtr socket, bool blocking);
ReturnCode Socket_GetError(SocketPtr socket, ReturnCode* result);

ReturnCode Socket_Connect(SocketPtr socket, STRING_PURE String* ip, Int32 port);
ReturnCode Socket_Read(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Socket_Write(SocketPtr socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Socket_Close(SocketPtr socket);
ReturnCode Socket_Select(SocketPtr socket, Int32 selectMode, bool* success);

void Http_Init(void);
ReturnCode Http_MakeRequest(struct AsyncRequest* request, void** handle);
ReturnCode Http_GetRequestHeaders(struct AsyncRequest* request, void* handle, UInt32* size);
ReturnCode Http_GetRequestData(struct AsyncRequest* request, void* handle, void** data, UInt32 size, volatile Int32* progress);
ReturnCode Http_FreeRequest(void* handle);
ReturnCode Http_Free(void);

#define AUDIO_MAX_CHUNKS 4
struct AudioFormat { UInt16 Channels, BitsPerSample; Int32 SampleRate; };
#define AudioFormat_Eq(a, b) ((a)->Channels == (b)->Channels && (a)->BitsPerSample == (b)->BitsPerSample && (a)->SampleRate == (b)->SampleRate)
typedef Int32 AudioHandle;

void Audio_Init(AudioHandle* handle, Int32 buffers);
void Audio_Free(AudioHandle handle);
struct AudioFormat* Audio_GetFormat(AudioHandle handle);
void Audio_SetFormat(AudioHandle handle, struct AudioFormat* format);
void Audio_SetVolume(AudioHandle handle, Real32 volume);
void Audio_PlayData(AudioHandle handle, Int32 idx, void* data, UInt32 dataSize);
bool Audio_IsCompleted(AudioHandle handle, Int32 idx);
bool Audio_IsFinished(AudioHandle handle);
#endif
