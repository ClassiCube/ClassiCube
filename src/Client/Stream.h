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

struct Stream;
/* Represents a stream that can be written to and/or read from. */
struct Stream {
	ReturnCode (*Read)(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*Write)(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*Close)(struct Stream* stream);
	ReturnCode (*Seek)(struct Stream* stream, Int32 offset, Int32 seekType);
	ReturnCode (*Position)(struct Stream* stream, UInt32* pos);
	ReturnCode (*Length)(struct Stream* stream, UInt32* length);
	
	union {
		void* Meta_File;
		void* Meta_Inflate;
		/* NOTE: These structs rely on overlapping Meta_Mem fields being the same! Don't change them */
		struct {
			UInt8* Meta_Mem_Cur;                
			UInt32 Meta_Mem_Left, Meta_Mem_Length; 
			UInt8* Meta_Mem_Base; 
		};
		struct { 
			struct Stream* Meta_Portion_Source; 
			UInt32 Meta_MeM_Left, Meta_MeM_Length; 
		};
		struct { 
			UInt8* Meta_Buffered_Cur;          
			UInt32 Meta_MEM_Left, Meta_MEM_Length; 
			UInt8* Meta_Buffered_Base; struct Stream* Meta_Buffered_Source; 
		};
		struct { struct Stream* Meta_CRC32_Source; UInt32 Meta_CRC32; };
	};
	UChar NameBuffer[String_BufferSize(FILENAME_SIZE)];
	String Name;
};

void Stream_Read(struct Stream* stream, UInt8* buffer, UInt32 count);
void Stream_Write(struct Stream* stream, UInt8* buffer, UInt32 count);
ReturnCode Stream_TryWrite(struct Stream* stream, UInt8* buffer, UInt32 count);
Int32 Stream_TryReadByte(struct Stream* stream);
void Stream_SetName(struct Stream* stream, STRING_PURE String* name);
void Stream_Skip(struct Stream* stream, UInt32 count);
void Stream_SetDefaultOps(struct Stream* stream);

void Stream_FromFile(struct Stream* stream, void* file, STRING_PURE String* name);
/* Readonly Stream wrapping another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, UInt32 len);
void Stream_ReadonlyMemory(struct Stream* stream, void* data, UInt32 len, STRING_PURE String* name);
void Stream_WriteonlyMemory(struct Stream* stream, void* data, UInt32 len, STRING_PURE String* name);
void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, UInt32 size);


UInt16 Stream_GetU16_LE(UInt8* data);
UInt16 Stream_GetU16_BE(UInt8* data);
UInt32 Stream_GetU32_LE(UInt8* data);
UInt32 Stream_GetU32_BE(UInt8* data);

UInt8 Stream_ReadU8(struct Stream* stream);
UInt16 Stream_ReadU16_LE(struct Stream* stream);
UInt16 Stream_ReadU16_BE(struct Stream* stream);
UInt32 Stream_ReadU32_LE(struct Stream* stream);
UInt32 Stream_ReadU32_BE(struct Stream* stream);
#define Stream_ReadI16_BE(stream) ((Int16)Stream_ReadU16_BE(stream))
#define Stream_ReadI32_BE(stream) ((Int32)Stream_ReadU32_BE(stream))

void Stream_WriteU8(struct Stream* stream, UInt8 value);
void Stream_WriteU16_BE(struct Stream* stream, UInt16 value);
void Stream_WriteU32_LE(struct Stream* stream, UInt32 value);
void Stream_WriteU32_BE(struct Stream* stream, UInt32 value);
#define Stream_WriteI16_BE(stream, value) Stream_WriteU16_BE(stream, (UInt16)(value))
#define Stream_WriteI32_BE(stream, value) Stream_WriteU32_BE(stream, (UInt32)(value))

/* Reads a UTF8 encoded character from the given stream. Returns false if end of stream. */
bool Stream_ReadUtf8Char(struct Stream* stream, UInt16* codepoint);
/* Reads a line of UTF8 encoding text from the given stream. Returns false if end of stream. */
bool Stream_ReadLine(struct Stream* stream, STRING_TRANSIENT String* text);
/* Writes a UTF8 encoded character to the given buffer. (max bytes written is 3) */
Int32 Stream_WriteUtf8(UInt8* buffer, UInt16 codepoint);
/* Writes a line of UTF8 encoded text to the given stream. */
void Stream_WriteLine(struct Stream* stream, STRING_TRANSIENT String* text);
#endif
