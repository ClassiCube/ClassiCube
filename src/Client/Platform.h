#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H
#include "Typedefs.h"
#include "DateTime.h"
#include "String.h"
#include "2DStructs.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Newline for text */
extern UInt8* Platform_NewLine;
extern UInt8 Platform_DirectorySeparator;
extern ReturnCode ReturnCode_FileShareViolation;

/* Initalises required state for this platform. */
void Platform_Init(void);
/* Releases the resources allocated for the required state. */
void Platform_Free(void);

/* Allocates a block of memory, returning NULL on failure. */
void* Platform_MemAlloc(UInt32 numBytes);
/* Frees a previously allocated block of memory. */
void Platform_MemFree(void* mem);
/* Sets a block of memory to the given byte value. */
void Platform_MemSet(void* dst, UInt8 value, UInt32 numBytes);
/* Copies a block of non-overlapping memory. */
void Platform_MemCpy(void* dst, void* src, UInt32 numBytes);

/* Logs a message to console (if attached). Implictly puts each entry on a newline. */
void Platform_Log(STRING_PURE String* message);
/* Gets the current time, in UTC timezone. */
DateTime Platform_CurrentUTCTime(void);
/* Gets the current time, in user's local timezone. */
DateTime Platform_CurrentLocalTime(void);

/* Returns whether a directory with the given name exists. */
bool Platform_DirectoryExists(STRING_PURE String* path);
/* Creates a new directory. */
ReturnCode Platform_DirectoryCreate(STRING_PURE String* path);
/* Returns whether a file with the given name exists. */
bool Platform_FileExists(STRING_PURE String* path);

/* Creates or overwrites an existing file. */
ReturnCode Platform_FileCreate(void** file, STRING_PURE String* path);
/* Opens an existing file. */
ReturnCode Platform_FileOpen(void** file, STRING_PURE String* path, bool readOnly);
/* Reads a block of bytes from the given file, returning a platform-specific return code. */
ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);
/* Writes a block of bytes to the given file, returning a platform-specific return code. */
ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten);
/* Closes the given file. */
ReturnCode Platform_FileClose(void* file);
/* Seeks / adjusts position within the file. */
ReturnCode Platform_FileSeek(void* file, Int32 offset, Int32 seekType);
/* Returns current position/offset within the file. */
UInt32 Platform_FilePosition(void* file);
/* Returns the length of the given file. */
UInt32 Platform_FileLength(void* file);

/* Blocks the calling thread for given number of milliseconds. */
void Platform_ThreadSleep(UInt32 milliseconds);

struct DrawTextArgs_;
struct Bitmap_;
/* Allocates handle for the given font. */
void Platform_MakeFont(FontDesc* desc, STRING_PURE String* fontName);
/* Frees handle for the given font. */
void Platform_FreeFont(FontDesc* desc);
/* Measures size of given text.*/
Size2D Platform_MeasureText(struct DrawTextArgs_* args);
/* Draws text onto the actively selected bitmap. */
void Platform_DrawText(struct DrawTextArgs_* args, Int32 x, Int32 y);
/* Sets the bitmap used for text drawing. */
void Platform_SetBitmap(struct Bitmap_* bmp);
/* Releases the bitmap that was used for text drawing.*/
void Platform_ReleaseBitmap(void);
#endif