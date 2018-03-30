#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "String.h"
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
	ReturnCode (*Read)(struct Stream_* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*Write)(struct Stream_* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*Close)(struct Stream_* stream);
	ReturnCode (*Seek)(struct Stream_* stream, Int32 offset, Int32 seekType);
	void* Data; UInt32 Data2;
	UInt8 NameBuffer[String_BufferSize(FILENAME_SIZE)];
	String Name;
} Stream;

void Stream_Read(Stream* stream, UInt8* buffer, UInt32 count);
void Stream_Write(Stream* stream, UInt8* buffer, UInt32 count);
Int32 Stream_TryReadByte(Stream* stream);
void Stream_SetName(Stream* stream, STRING_PURE String* name);
void Stream_FromFile(Stream* stream, void* file, STRING_PURE String* name);
/* Constructs a readonly Stream wrapping another Stream, 
but only allowing reading up to 'len' bytes from the wrapped stream. */
void Stream_ReadonlyPortion(Stream* stream, Stream* underlying, UInt32 len);

UInt8 Stream_ReadUInt8(Stream* stream);
#define Stream_ReadInt8(stream) ((Int8)Stream_ReadUInt8(stream))
UInt16 Stream_ReadUInt16_LE(Stream* stream);
#define Stream_ReadInt16_LE(stream) ((Int16)Stream_ReadUInt16_LE(stream))
UInt16 Stream_ReadUInt16_BE(Stream* stream);
#define Stream_ReadInt16_BE(stream) ((Int16)Stream_ReadUInt16_BE(stream))
UInt32 Stream_ReadUInt32_LE(Stream* stream);
#define Stream_ReadInt32_LE(stream) ((Int32)Stream_ReadUInt32_LE(stream))
UInt32 Stream_ReadUInt32_BE(Stream* stream);
#define Stream_ReadInt32_BE(stream) ((Int32)Stream_ReadUInt32_BE(stream))
UInt64 Stream_ReadUInt64_BE(Stream* stream);
#define Stream_ReadInt64_BE(stream) ((Int64)Stream_ReadUInt64_BE(stream))

void Stream_WriteUInt8(Stream* stream, UInt8 value);
#define Stream_WriteInt8(stream, value) Stream_WriteUInt8(stream, (UInt8)(value))
void Stream_WriteUInt16_LE(Stream* stream, UInt16 value);
#define Stream_WriteInt16_LE(stream, value) Stream_WriteUInt16_LE(stream, (UInt16)(value))
void Stream_WriteUInt16_BE(Stream* stream, UInt16 value);
#define Stream_WriteInt16_BE(stream, value) Stream_WriteUInt16_BE(stream, (UInt16)(value))
void Stream_WriteUInt32_LE(Stream* stream, UInt32 value);
#define Stream_WriteInt32_LE(stream, value) Stream_WriteUInt32_LE(stream, (UInt32)(value))
void Stream_WriteUInt32_BE(Stream* stream, UInt32 value);
#define Stream_WriteInt32_BE(stream, value) Stream_WriteUInt32_BE(stream, (UInt32)(value))

/* Reads a line of UTF8 encoding text from the given stream. Returns false if end of stream. */
bool Stream_ReadLine(Stream* stream, STRING_TRANSIENT String* text);
/* Writes a line of UTF8 encoded text to the given stream. */
void Stream_WriteLine(Stream* stream, STRING_TRANSIENT String* text);
#endif