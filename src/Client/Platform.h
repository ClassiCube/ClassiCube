#ifndef CS_PLATFORM_H
#define CS_PLATFORM_H
#include "Typedefs.h"
#include "DateTime.h"
#include "String.h"
#include "ErrorHandler.h"
#include "Compiler.h"
/* Abstracts platform specific memory management, I/O, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


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
void Platform_Log(String message);

/* Gets the current time, in UTC timezone. */
DateTime Platform_CurrentUTCTime(void);

/* Gets the current time, in user's local timezone. */
DateTime Platform_CurrentLocalTime(void);



/* Returns whether a directory with the given name exists. */
bool Platform_DirectoryExists(STRING_TRANSIENT String* path);

/* Creates a new directory. */
ReturnCode Platform_DirectoryCreate(STRING_TRANSIENT String* path);

/* Returns whether a file with the given name exists. */
bool Platform_FileExists(STRING_TRANSIENT String* path);

/* Creates or overwrites an existing file. */
ReturnCode Platform_FileCreate(void** file, STRING_TRANSIENT String* path);

/* Opens an existing file. */
ReturnCode Platform_FileOpen(void** file, STRING_TRANSIENT String* path, bool readOnly);

/* Reads a block of bytes from the given file, returning a platform-specific return code. */
ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);

/* Writes a block of bytes to the given file, returning a platform-specific return code. */
ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten);

/* Closes the given file. */
ReturnCode Platform_FileClose(void* file);


/* Blocks the calling thread for given number of milliseconds. */
void Platform_ThreadSleep(UInt32 milliseconds);
#endif