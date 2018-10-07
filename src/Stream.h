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
	ReturnCode (*Read)(struct Stream* stream, uint8_t* data, UInt32 count, UInt32* modified);
	ReturnCode (*ReadU8)(struct Stream* stream, uint8_t* data);
	ReturnCode (*Write)(struct Stream* stream, uint8_t* data, UInt32 count, UInt32* modified);
	ReturnCode (*Seek)(struct Stream* stream, int offset, int seekType);
	ReturnCode (*Position)(struct Stream* stream, UInt32* pos);
	ReturnCode (*Length)(struct Stream* stream, UInt32* length);
	ReturnCode (*Close)(struct Stream* stream);
	
	union {
		void* File;
		void* Inflate;
		/* NOTE: These structs rely on overlapping Meta_Mem fields being the same! Don't change them */
		struct { uint8_t* Cur; UInt32 Left, Length; uint8_t* Base; } Mem;
		struct { struct Stream* Source; UInt32 Left, Length; } Portion;
		struct { uint8_t* Cur; UInt32 Left, Length; uint8_t* Base; struct Stream* Source; } Buffered;
		struct { uint8_t* Cur; UInt32 Left, Last;   uint8_t* Base; struct Stream* Source; } Ogg;
		struct { struct Stream* Source; UInt32 CRC32; } CRC32;
	} Meta;
};

ReturnCode Stream_Read(struct Stream* stream, uint8_t* buffer, UInt32 count);
ReturnCode Stream_Write(struct Stream* stream, uint8_t* buffer, UInt32 count);
void Stream_Init(struct Stream* stream);
ReturnCode Stream_Skip(struct Stream* stream, UInt32 count);
ReturnCode Stream_DefaultReadU8(struct Stream* stream, uint8_t* data);

NOINLINE_ void Stream_FromFile(struct Stream* stream, void* file);
/* Readonly Stream wrapping another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
NOINLINE_ void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, UInt32 len);
NOINLINE_ void Stream_ReadonlyMemory(struct Stream* stream, void* data, UInt32 len);
NOINLINE_ void Stream_WriteonlyMemory(struct Stream* stream, void* data, UInt32 len);
NOINLINE_ void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, UInt32 size);

uint16_t Stream_GetU16_LE(uint8_t* data);
uint16_t Stream_GetU16_BE(uint8_t* data);
UInt32 Stream_GetU32_LE(uint8_t* data);
UInt32 Stream_GetU32_BE(uint8_t* data);

void Stream_SetU16_BE(uint8_t* data, uint16_t value);
void Stream_SetU32_LE(uint8_t* data, UInt32 value);
void Stream_SetU32_BE(uint8_t* data, UInt32 value);
ReturnCode Stream_ReadU32_LE(struct Stream* stream, UInt32* value);
ReturnCode Stream_ReadU32_BE(struct Stream* stream, UInt32* value);

ReturnCode Stream_ReadUtf8(struct Stream* stream, Codepoint* cp);
ReturnCode Stream_ReadLine(struct Stream* stream, String* text);
int Stream_WriteUtf8(uint8_t* buffer, Codepoint cp);
ReturnCode Stream_WriteLine(struct Stream* stream, String* text);
#endif
