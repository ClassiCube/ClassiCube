#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "String.h"
#include "Constants.h"
/* Defines an abstract way of reading and writing data in a streaming manner.
   Also provides common helper methods for reading/writing data to/from streams.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Origin points for when seeking in a stream. */
enum STREAM_SEEKFROM { STREAM_SEEKFROM_BEGIN, STREAM_SEEKFROM_CURRENT, STREAM_SEEKFROM_END };
struct Stream;
/* Represents a stream that can be written to and/or read from. */
struct Stream {
	/* Attempts to read some bytes from this stream. */
	ReturnCode (*Read)(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified);
	/* Attempts to efficiently read a single bye from this stream. */
	ReturnCode (*ReadU8)(struct Stream* stream, uint8_t* data);
	/* Attempts to write some bytes to this stream. */
	ReturnCode (*Write)(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified);
	/* Attempts to seek to a position in this stream. */
	ReturnCode (*Seek)(struct Stream* stream, int offset, int seekType);
	/* Attempts to find current position this stream. (may not be supported) */
	ReturnCode (*Position)(struct Stream* stream, uint32_t* pos);
	/* Attempts to find total length of this stream. (may not be supported) */
	ReturnCode (*Length)(struct Stream* stream, uint32_t* length);
	/* Attempts to close this stream, freeing associated resources. */
	ReturnCode (*Close)(struct Stream* stream);
	
	union {
		void* File;
		void* Inflate;
		/* NOTE: These structs rely on overlapping Meta_Mem fields being the same! Don't change them */
		struct { uint8_t* Cur; uint32_t Left, Length; uint8_t* Base; } Mem;
		struct { struct Stream* Source; uint32_t Left, Length; } Portion;
		struct { uint8_t* Cur; uint32_t Left, Length; uint8_t* Base; struct Stream* Source; } Buffered;
		struct { uint8_t* Cur; uint32_t Left, Last;   uint8_t* Base; struct Stream* Source; } Ogg;
		struct { struct Stream* Source; uint32_t CRC32; } CRC32;
	} Meta;
};

/* Attempts to fully read up to count bytes from the stream. */
ReturnCode Stream_Read(struct Stream* stream, uint8_t* buffer, uint32_t count);
/* Attempts to fully write up to count bytes from the stream. */
ReturnCode Stream_Write(struct Stream* stream, uint8_t* buffer, uint32_t count);
/* Initalises default function pointers for a stream. (all read, write, seeks return an error) */
void Stream_Init(struct Stream* stream);
/* Attempts to fully skip count bytes from the stream. (falling back to reading then discarding) */
ReturnCode Stream_Skip(struct Stream* stream, uint32_t count);
/* Slow way of reading a U8 integer through stream->Read(stream, 1, tmp). */
ReturnCode Stream_DefaultReadU8(struct Stream* stream, uint8_t* data);

/* Wraps a file, allowing reading from, writing to, and seeking in the file. */
NOINLINE_ void Stream_FromFile(struct Stream* stream, void* file);
/* Wraps another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
NOINLINE_ void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, uint32_t len);
/* Wraps a block of memory, allowing reading from and seeking in the block. */
NOINLINE_ void Stream_ReadonlyMemory(struct Stream* stream, void* data, uint32_t len);
/* Wraps a block of memory, allowing writing to and seeking in the block. */
NOINLINE_ void Stream_WriteonlyMemory(struct Stream* stream, void* data, uint32_t len);
/* Wraps another Stream, reading through an intermediary buffer. (Useful for files, since each read call is expensive) */
NOINLINE_ void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, uint32_t size);

/* Reads a little-endian 16 bit unsigned integer from memory. */
uint16_t Stream_GetU16_LE(uint8_t* data);
/* Reads a big-endian 16 bit unsigned integer from memory. */
uint16_t Stream_GetU16_BE(uint8_t* data);
/* Reads a little-endian 32 bit unsigned integer from memory. */
uint32_t Stream_GetU32_LE(uint8_t* data);
/* Reads a big-endian 32 bit unsigned integer from memory. */
uint32_t Stream_GetU32_BE(uint8_t* data);

/* Writes a big-endian 16 bit unsigned integer to memory. */
void Stream_SetU16_BE(uint8_t* data, uint16_t value);
/* Writes a little-endian 32 bit unsigned integer to memory. */
void Stream_SetU32_LE(uint8_t* data, uint32_t value);
/* Writes a big-endian 32 bit unsigned integer to memory. */
void Stream_SetU32_BE(uint8_t* data, uint32_t value);
/* Reads a little-endian 32 bit unsigned integer a stream. */
ReturnCode Stream_ReadU32_LE(struct Stream* stream, uint32_t* value);
/* Reads a big-endian 32 bit unsigned integer a stream. */
ReturnCode Stream_ReadU32_BE(struct Stream* stream, uint32_t* value);

/* Reads a UTF8 encoded character from the stream. */
ReturnCode Stream_ReadUtf8(struct Stream* stream, Codepoint* cp);
/* Reads a line of UTF8 encoded character from the stream. */
ReturnCode Stream_ReadLine(struct Stream* stream, String* text);
/* Writes a UTF8 encoded character to the stream. */
int Stream_WriteUtf8(uint8_t* buffer, Codepoint cp);
/* Writes a line of UTF8 encoded text to the stream. */
ReturnCode Stream_WriteLine(struct Stream* stream, String* text);
#endif
