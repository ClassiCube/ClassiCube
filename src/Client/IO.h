#ifndef CS_IO_H
#define CS_IO_H
#include "Typedefs.h"
#include "String.h"
#include "ErrorHandler.h"
#include "Compiler.h"
/* Abstracts platform specific I/O operations.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Returns whether a file with the given name exists. */
bool IO_FileExists(STRING_TRANSIENT String* path);

/* Returns whether a directory with the given name exists. */
bool IO_DirectoryExists(STRING_TRANSIENT String* path);

/* Creates a new directory. */
ReturnCode IO_DirectoryCreate(STRING_TRANSIENT String* path);


/* Opens an existing file. */
ReturnCode IO_FileOpen(void** file, STRING_TRANSIENT String* path, bool readOnly);

/* Creates or overwrites an existing file. */
ReturnCode IO_FileCreate(void** file, STRING_TRANSIENT String* path);

/* Reads a block of bytes from the given file, returning a platform-specific return code. */
ReturnCode IO_FileRead(void* file, UInt8* buffer, UInt32 count, UInt32* bytesRead);

/* Writes a block of bytes to the given file, returning a platform-specific return code. */
ReturnCode IO_FileWrite(void* file, UInt8* buffer, UInt32 count, UInt32* bytesWritten);

/* Closes the given file. */
ReturnCode IO_FileClose(void* file);
#endif