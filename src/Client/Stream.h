#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "String.h"
#include "Constants.h"
/* Defines an abstract way of reading and writing data in a streaming manner.
   Also provides common helper methods for reading/writing data to/from streams.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

enum STREAM_SEEKFROM {
	STREAM_SEEKFROM_BEGIN, STREAM_SEEKFROM_CURRENT, STREAM_SEEKFROM_END,
};

typedef struct Stream_ Stream;
/* Represents a stream that can be written to and/or read from. */
typedef struct Stream_ {
	ReturnCode (*Read)(Stream* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*Write)(Stream* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*Close)(Stream* stream);
	ReturnCode (*Seek)(Stream* stream, Int32 offset, Int32 seekType);
	ReturnCode (*Position)(Stream* stream, UInt32* pos);
	ReturnCode (*Length)(Stream* stream, UInt32* length);
	
	union {
		void* Meta_File;
		void* Meta_Inflate;
		/* NOTE: These structs rely on overlapping Meta_Mem fields being the same! Don't change them */
		struct { UInt8* Meta_Mem_Cur;         UInt32 Meta_Mem_Left, Meta_Mem_Length; UInt8* Meta_Mem_Base; };
		struct { Stream* Meta_Portion_Source; UInt32 Meta_MeM_Left, Meta_MeM_Length; };
		struct { UInt8* Meta_Buffered_Cur;    UInt32 Meta_MEM_Left, Meta_MEM_Length; UInt8* Meta_Buffered_Base; Stream* Meta_Buffered_Source; };
		struct { Stream* Meta_CRC32_Source;   UInt32 Meta_CRC32; };
	};
	UChar NameBuffer[String_BufferSize(FILENAME_SIZE)];
	String Name;
} Stream;

void Stream_Read(Stream* stream, UInt8* buffer, UInt32 count);
void Stream_Write(Stream* stream, UInt8* buffer, UInt32 count);
ReturnCode Stream_TryWrite(Stream* stream, UInt8* buffer, UInt32 count);
Int32 Stream_TryReadByte(Stream* stream);
void Stream_SetName(Stream* stream, STRING_PURE String* name);
ReturnCode Stream_Skip(Stream* stream, UInt32 count); 
void Stream_SetDefaultOps(Stream* stream);

void Stream_FromFile(Stream* stream, void* file, STRING_PURE String* name);
/* Readonly Stream wrapping another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
void Stream_ReadonlyPortion(Stream* stream, Stream* source, UInt32 len);
void Stream_ReadonlyMemory(Stream* stream, void* data, UInt32 len, STRING_PURE String* name);
void Stream_WriteonlyMemory(Stream* stream, void* data, UInt32 len, STRING_PURE String* name);
void Stream_ReadonlyBuffered(Stream* stream, Stream* source, void* data, UInt32 size);


UInt8 Stream_ReadU8(Stream* stream);
#define Stream_ReadI8(stream) ((Int8)Stream_ReadU8(stream))
UInt16 Stream_ReadU16_LE(Stream* stream);
#define Stream_ReadI16_LE(stream) ((Int16)Stream_ReadU16_LE(stream))
UInt16 Stream_ReadU16_BE(Stream* stream);
#define Stream_ReadI16_BE(stream) ((Int16)Stream_ReadU16_BE(stream))
UInt32 Stream_ReadU32_LE(Stream* stream);
#define Stream_ReadI32_LE(stream) ((Int32)Stream_ReadU32_LE(stream))
UInt32 Stream_ReadU32_BE(Stream* stream);
#define Stream_ReadI32_BE(stream) ((Int32)Stream_ReadU32_BE(stream))
UInt64 Stream_ReadU64_BE(Stream* stream);
#define Stream_ReadI64_BE(stream) ((Int64)Stream_ReadU64_BE(stream))

void Stream_WriteU8(Stream* stream, UInt8 value);
#define Stream_WriteI8(stream, value) Stream_WriteU8(stream, (UInt8)(value))
void Stream_WriteU16_LE(Stream* stream, UInt16 value);
#define Stream_WriteI16_LE(stream, value) Stream_WriteU16_LE(stream, (UInt16)(value))
void Stream_WriteU16_BE(Stream* stream, UInt16 value);
#define Stream_WriteI16_BE(stream, value) Stream_WriteU16_BE(stream, (UInt16)(value))
void Stream_WriteU32_LE(Stream* stream, UInt32 value);
#define Stream_WriteI32_LE(stream, value) Stream_WriteU32_LE(stream, (UInt32)(value))
void Stream_WriteU32_BE(Stream* stream, UInt32 value);
#define Stream_WriteI32_BE(stream, value) Stream_WriteU32_BE(stream, (UInt32)(value))

/* Reads a UTF8 encoded character from the given stream. Returns false if end of stream. */
bool Stream_ReadUtf8Char(Stream* stream, UInt16* codepoint);
/* Reads a line of UTF8 encoding text from the given stream. Returns false if end of stream. */
bool Stream_ReadLine(Stream* stream, STRING_TRANSIENT String* text);
/* Writes a UTF8 encoded character to the given buffer. (max bytes written is 3) */
Int32 Stream_WriteUtf8(UInt8* buffer, UInt16 codepoint);
/* Writes a line of UTF8 encoded text to the given stream. */
void Stream_WriteLine(Stream* stream, STRING_TRANSIENT String* text);
#endif
