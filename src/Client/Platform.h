#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "Utils.h"
#include "2DStructs.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct DrawTextArgs_ DrawTextArgs;
typedef struct Bitmap_ Bitmap;

extern UInt8* Platform_NewLine; /* Newline for text */
extern UInt8 Platform_DirectorySeparator;
extern ReturnCode ReturnCode_FileShareViolation;
extern ReturnCode ReturnCode_FileNotFound;

void Platform_Init(void);
void Platform_Free(void);
void Platform_Exit(ReturnCode code);

void* Platform_MemAlloc(UInt32 numElems, UInt32 elemsSize);
void* Platform_MemRealloc(void* mem, UInt32 numElems, UInt32 elemsSize);
void Platform_MemFree(void** mem);
void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes);
void Platform_MemCpy(void* dst, void* src, UInt32 numBytes);

void Platform_Log(STRING_PURE String* message);
void Platform_LogConst(const UInt8* message);
#define Platform_Log1(format, a1) Platform_Log4(format, a1, NULL, NULL, NULL)
#define Platform_Log2(format, a1, a2) Platform_Log4(format, a1, a2, NULL, NULL)
#define Platform_Log3(format, a1, a2, a3) Platform_Log4(format, a1, a2, a3, NULL)
void Platform_Log4(const UInt8* format, const void* a1, const void* a2, const void* a3, const void* a4);
void Platform_CurrentUTCTime(DateTime* time);
void Platform_CurrentLocalTime(DateTime* time);

bool Platform_DirectoryExists(STRING_PURE String* path);
ReturnCode Platform_DirectoryCreate(STRING_PURE String* path);
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
UInt32 Platform_FilePosition(void* file);
UInt32 Platform_FileLength(void* file);

void Platform_ThreadSleep(UInt32 milliseconds);
typedef void Platform_ThreadFunc(void);
void* Platform_ThreadStart(Platform_ThreadFunc* func);
/* Frees handle to thread - NOT THE THREAD ITSELF */
void Platform_ThreadFreeHandle(void* handle);

typedef Int64 Stopwatch;
void Stopwatch_Start(Stopwatch* timer);
Int32 Stopwatch_ElapsedMicroseconds(Stopwatch* timer);

void Platform_MakeFont(FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style);
void Platform_FreeFont(FontDesc* desc);
void Platform_SetBitmap(Bitmap* bmp);
void Platform_ReleaseBitmap(void);
Size2D Platform_MeasureText(DrawTextArgs* args);
void Platform_DrawText(DrawTextArgs* args, Int32 x, Int32 y);

void Platform_SocketCreate(void** socket);
ReturnCode Platform_SocketConnect(void* socket, STRING_PURE String* ip, Int32 port);
ReturnCode Platform_SocketRead(void* socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Platform_SocketWrite(void* socket, UInt8* buffer, UInt32 count, UInt32* modified);
ReturnCode Platform_SocketClose(void* socket);
ReturnCode Platform_SocketAvailable(void* socket, UInt32* available);
ReturnCode Platform_SocketSelectRead(void* socket, Int32 microseconds, bool* success);
#endif