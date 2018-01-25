#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "Typedefs.h"
#include "Utils.h"
#include "2DStructs.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

extern UInt8* Platform_NewLine; /* Newline for text */
extern UInt8 Platform_DirectorySeparator;
extern ReturnCode ReturnCode_FileShareViolation;

void Platform_Init(void);
void Platform_Free(void);

void* Platform_MemAlloc(UInt32 numBytes);
void* Platform_MemRealloc(void* mem, UInt32 numBytes);
void Platform_MemFree(void* mem);
void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes);
void Platform_MemCpy(void* dst, void* src, UInt32 numBytes);

void Platform_Log(STRING_PURE String* message);
DateTime Platform_CurrentUTCTime(void);
DateTime Platform_CurrentLocalTime(void);

bool Platform_DirectoryExists(STRING_PURE String* path);
ReturnCode Platform_DirectoryCreate(STRING_PURE String* path);
bool Platform_FileExists(STRING_PURE String* path);

ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path);
ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path, bool readOnly);
ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);
ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten);
ReturnCode Platform_FileClose(void* file);
ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType);
UInt32 Platform_FilePosition(void* file);
UInt32 Platform_FileLength(void* file);

void Platform_ThreadSleep(UInt32 milliseconds);

struct DrawTextArgs_;
struct Bitmap_;
void Platform_MakeFont(FontDesc* desc, STRING_PURE String* fontName, UInt16 size, UInt16 style);
void Platform_FreeFont(FontDesc* desc);
Size2D Platform_MeasureText(struct DrawTextArgs_* args);
void Platform_DrawText(struct DrawTextArgs_* args, Int32 x, Int32 y);
void Platform_SetBitmap(struct Bitmap_* bmp);
void Platform_ReleaseBitmap(void);
#endif