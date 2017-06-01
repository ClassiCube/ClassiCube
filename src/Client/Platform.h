#ifndef CS_PLATFORM_H
#define CS_PLATFORM_H
#include "Typedefs.h"
#include "DateTime.h"
#include "String.h"
#include "ErrorHandler.h"
/* Abstracts platform specific memory management, IO, etc.
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
#endif