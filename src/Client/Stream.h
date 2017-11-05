#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "Typedefs.h"
#include "String.h"
#include "ErrorHandler.h"
#include "Constants.h"
/* Defines an abstract way of reading and writing data in a streaming manner.
   Also provides common helper methods for reading/writing data to/from streams.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define STREAM_SEEKFROM_BEGIN 0
#define STREAM_SEEKFROM_CURRENT 1
#define STREAM_SEEKFROM_END 2

/* Represents a stream that can be written to and/or read from. */
typedef struct Stream_ {
	/* Performs a read. Result is a ReturnCode, number of read bytes is output via pointer. */
	ReturnCode (*Read)(struct Stream_* stream, UInt8* data, UInt32 count, UInt32* modified);
	/* Performs a write. Result is a ReturnCode, number of written bytes is output via pointer. */
	ReturnCode (*Write)(struct Stream_* stream, UInt8* data, UInt32 count, UInt32* modified);
	/* Closes the stream. Result is a ReturnCode. */
	ReturnCode (*Close)(struct Stream_* stream);
	/* Moves backwards or forwards by given number of bytes from seek offset in the stream. Result is a ReturnCode. */
	ReturnCode (*Seek)(struct Stream_* stream, Int32 offset, Int32 seekType);
	/* General purpose pointer metadata for the stream. */
	void* Data;
	/* General purpose numerical metadata for the stream. */
	UInt32 Data2;
	/* Raw name buffer */
	UInt8 NameBuffer[String_BufferSize(FILENAME_SIZE)];
	/* The name of the stream. */
	String Name;
} Stream;

/* Fully reads up to count bytes or fails. */
void Stream_Read(Stream* stream, UInt8* buffer, UInt32 count);
/* Fully writes up to count bytes or fails. */
void Stream_Write(Stream* stream, UInt8* buffer, UInt32 count);
/* Attempts to read a byte (returning -1 if could not read) */
Int32 Stream_TryReadByte(Stream* stream);
/* Sets the name of the given stream. */
void Stream_SetName(Stream* stream, STRING_PURE String* name);
/* Constructs a Stream wrapping a file. */
void Stream_FromFile(Stream* stream, void* file, STRING_PURE String* name);
/* Constructs a readonly Stream wrapping another Stream, 
but only allowing reading up to 'len' bytes from the wrapped stream. */
void Stream_ReadonlyPortion(Stream* stream, Stream* underlying, UInt32 len);

/* Reads an unsigned 8 bit integer from the given stream. */
UInt8 Stream_ReadUInt8(Stream* stream);
/* Reads a nsigned 8 bit integer from the given stream. */
#define Stream_ReadInt8(stream) ((Int8)Stream_ReadUInt8(stream))
/* Reads a little endian unsigned 16 bit integer from the given stream. */
UInt16 Stream_ReadUInt16_LE(Stream* stream);
/* Reads a little endian signed 16 bit integer from the given stream. */
#define Stream_ReadInt16_LE(stream) ((Int16)Stream_ReadUInt16_LE(stream))
/* Reads a big endian unsigned 16 bit integer from the given stream. */
UInt16 Stream_ReadUInt16_BE(Stream* stream);
/* Reads a big endian signed 16 bit integer from the given stream. */
#define Stream_ReadInt16_BE(stream) ((Int16)Stream_ReadUInt16_BE(stream))
/* Reads a little endian unsigned 32 bit integer from the given stream. */
UInt32 Stream_ReadUInt32_LE(Stream* stream);
/* Reads a little endian signed 32 bit integer from the given stream. */
#define Stream_ReadInt32_LE(stream) ((Int32)Stream_ReadUInt32_LE(stream))
/* Reads a big endian unsigned 64 bit integer from the given stream. */
UInt32 Stream_ReadUInt32_BE(Stream* stream);
/* Reads a big endian signed 64 bit integer from the given stream. */
#define Stream_ReadInt32_BE(stream) ((Int32)Stream_ReadUInt32_BE(stream))
/* Reads a big endian unsigned 64 bit integer from the given stream. */
UInt64 Stream_ReadUInt64_BE(Stream* stream);
/* Reads a big endian signed 64 bit integer from the given stream. */
#define Stream_ReadInt64_BE(stream) ((Int64)Stream_ReadUInt64_BE(stream))

/* Writes an unsigned 8 bit integer to the given stream. */
void Stream_WriteUInt8(Stream* stream, UInt8 value);
/* Writes a signed 8 bit integer to the given stream. */
#define Stream_WriteInt8(stream, value) Stream_WriteUInt8(stream, (UInt8)(value))
/* Writes a little endian unsigned 16 bit integer to the given stream. */
void Stream_WriteUInt16_LE(Stream* stream, UInt16 value);
/* Writes a little endian signed 16 bit integer to the given stream. */
#define Stream_WriteInt16_LE(stream, value) Stream_WriteUInt16_LE(stream, (UInt16)(value))
/* Writes a big endian unsigned 16 bit integer to the given stream. */
void Stream_WriteUInt16_BE(Stream* stream, UInt16 value);
/* Writes a big endian signed 16 bit integer to the given stream. */
#define Stream_WriteInt16_BE(stream, value) Stream_WriteUInt16_BE(stream, (UInt16)(value))
/* Writes a little endian unsigned 32 bit integer to the given stream. */
void Stream_WriteUInt32_LE(Stream* stream, UInt32 value);
/* Writes a little endian signed 32 bit integer to the given stream. */
#define Stream_WriteInt32_LE(stream, value) Stream_WriteUInt32_LE(stream, (UInt32)(value))
/* Writes a big endian unsigned 32 bit integer to the given stream. */
void Stream_WriteUInt32_BE(Stream* stream, UInt32 value);
/* Writes a big endian signed 32 bit integer to the given stream. */
#define Stream_WriteInt32_BE(stream, value) Stream_WriteUInt32_BE(stream, (UInt32)(value))

/* Reads a line of UTF8 encoding text from the given stream. */
void Stream_ReadLine(Stream* stream, STRING_TRANSIENT String* text);
/* Writes a line of UTF8 encoded text to the given stream. */
void Stream_WriteLine(Stream* stream, STRING_TRANSIENT String* text);
#endif