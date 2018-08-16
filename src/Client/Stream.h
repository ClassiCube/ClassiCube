#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "String.h"
#include "Constants.h"
/* Defines an abstract way of reading and writing data in a streaming manner.
   Also provides common helper methods for reading/writing data to/from streams.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

enum STREAM_SEEKFROM { STREAM_SEEKFROM_BEGIN, STREAM_SEEKFROM_CURRENT, STREAM_SEEKFROM_END };
struct Stream;
/* Represents a stream that can be written to and/or read from. */
struct Stream {
	ReturnCode (*Read)(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified);
	ReturnCode (*ReadU8)(struct Stream* stream, UInt8* data);
	ReturnCode (*Write)(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified);	
	ReturnCode (*Seek)(struct Stream* stream, Int32 offset, Int32 seekType);
	ReturnCode (*Position)(struct Stream* stream, UInt32* pos);
	ReturnCode (*Length)(struct Stream* stream, UInt32* length);
	ReturnCode (*Close)(struct Stream* stream);
	
	union {
		void* File;
		void* Inflate;
		/* NOTE: These structs rely on overlapping Meta_Mem fields being the same! Don't change them */
		struct { UInt8* Cur; UInt32 Left, Length; UInt8* Base; } Mem;
		struct { struct Stream* Source; UInt32 Left, Length; } Portion;
		struct { UInt8* Cur; UInt32 Left, Length; UInt8* Base; struct Stream* Source; } Buffered;
		struct { UInt8* Cur; UInt32 Left, Last;   UInt8* Base; struct Stream* Source; } Ogg;
		struct { struct Stream* Source; UInt32 CRC32; } CRC32;
	} Meta;
};

ReturnCode Stream_Read(struct Stream* stream, UInt8* buffer, UInt32 count);
ReturnCode Stream_Write(struct Stream* stream, UInt8* buffer, UInt32 count);
void Stream_Init(struct Stream* stream);
ReturnCode Stream_Skip(struct Stream* stream, UInt32 count);
ReturnCode Stream_DefaultReadU8(struct Stream* stream, UInt8* data);

void Stream_FromFile(struct Stream* stream, void* file);
/* Readonly Stream wrapping another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, UInt32 len);
void Stream_ReadonlyMemory(struct Stream* stream, void* data, UInt32 len);
void Stream_WriteonlyMemory(struct Stream* stream, void* data, UInt32 len);
void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, UInt32 size);

UInt16 Stream_GetU16_LE(UInt8* data);
UInt16 Stream_GetU16_BE(UInt8* data);
UInt32 Stream_GetU32_LE(UInt8* data);
UInt32 Stream_GetU32_BE(UInt8* data);

void Stream_SetU16_BE(UInt8* data, UInt16 value);
void Stream_SetU32_LE(UInt8* data, UInt32 value);
void Stream_SetU32_BE(UInt8* data, UInt32 value);
ReturnCode Stream_ReadU32_LE(struct Stream* stream, UInt32* value);
ReturnCode Stream_ReadU32_BE(struct Stream* stream, UInt32* value);

ReturnCode Stream_ReadUtf8Char(struct Stream* stream, UInt16* codepoint);
ReturnCode Stream_ReadLine(struct Stream* stream, STRING_TRANSIENT String* text);
Int32 Stream_WriteUtf8(UInt8* buffer, UInt16 codepoint);
ReturnCode Stream_WriteLine(struct Stream* stream, STRING_TRANSIENT String* text);
#endif
