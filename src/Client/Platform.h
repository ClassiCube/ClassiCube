#ifndef CS_PLATFORM_H
#define CS_PLATFORM_H
#include "Typedefs.h"
#include "String.h"
#include "ErrorHandler.h"
/* Abstracts platform specific memory management, IO, etc.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


/* Initalises required state for this platform. */
void Platform_Init();

/* Releases the resources allocated for the required state. */
void Platform_Free();


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


/* Returns whether a file with the given name exists. */
bool Platform_FileExists(String path);

/* Opens an existing file. */
ReturnCode Platform_FileOpen(void** file, String path, bool readOnly);

/* Creates or overwrites an existing file. */
ReturnCode Platform_FileCreate(void** file, String path);

/* Reads a block of bytes from the given file, returning a platform-specific return code. */
ReturnCode Platform_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);

/* Writes a block of bytes to the given file, returning a platform-specific return code. */
ReturnCode Platform_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten);

/* Closes the given file. */
ReturnCode Platform_FileClose(void* file);
#endif