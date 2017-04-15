#ifndef CS_PLATFORM_H
#define CS_PLATFORM_H
#include "Typedefs.h"
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


// TODO: I/O FUNCTIONS
#endif