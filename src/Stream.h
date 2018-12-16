#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "Constants.h"
#include "Platform.h"
/* Defines an abstract way of reading and writing data in a streaming manner.
   Also provides common helper methods for reading/writing data to/from streams.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

struct Stream;
/* Represents a stream that can be written to and/or read from. */
struct Stream {
	/* Attempts to read some bytes from this stream. */
	ReturnCode (*Read)(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified);
	/* Attempts to efficiently read a single byte from this stream. (fallbacks to Read) */
	ReturnCode (*ReadU8)(struct Stream* s, uint8_t* data);
	/* Attempts to write some bytes to this stream. */
	ReturnCode (*Write)(struct Stream* s, const uint8_t* data, uint32_t count, uint32_t* modified);
	/* Attempts to quickly advance the position in this stream. (falls back to reading then discarding) */
	ReturnCode (*Skip)(struct Stream* s, uint32_t count);

	/* Attempts to seek to the given position in this stream. (may not be supported) */
	ReturnCode (*Seek)(struct Stream* s, uint32_t position);
	/* Attempts to find current position this stream. (may not be supported) */
	ReturnCode (*Position)(struct Stream* s, uint32_t* position);
	/* Attempts to find total length of this stream. (may not be supported) */
	ReturnCode (*Length)(struct Stream* s, uint32_t* length);
	/* Attempts to close this stream, freeing associated resources. */
	ReturnCode (*Close)(struct Stream* s);
	
	union {
		FileHandle File;
		void* Inflate;
		/* NOTE: These structs rely on overlapping Meta_Mem fields being the same! Don't change them */
		struct { uint8_t* Cur; uint32_t Left, Length; uint8_t* Base; } Mem;
		struct { struct Stream* Source; uint32_t Left, Length; } Portion;
		struct { uint8_t* Cur; uint32_t Left, Length; uint8_t* Base; struct Stream* Source; uint32_t End; } Buffered;
		struct { uint8_t* Cur; uint32_t Left, Last;   uint8_t* Base; struct Stream* Source; } Ogg;
		struct { struct Stream* Source; uint32_t CRC32; } CRC32;
	} Meta;
};

/* Attempts to fully read up to count bytes from the stream. */
ReturnCode Stream_Read(struct Stream* s, uint8_t* buffer, uint32_t count);
/* Attempts to fully write up to count bytes from the stream. */
ReturnCode Stream_Write(struct Stream* s, const uint8_t* buffer, uint32_t count);
/* Initalises default function pointers for a stream. (all read, write, seeks return an error) */
void Stream_Init(struct Stream* s);
/* Slow way of reading a U8 integer through stream->Read(stream, 1, tmp). */
ReturnCode Stream_DefaultReadU8(struct Stream* s, uint8_t* data);

/* Wrapper for File_Open() then Stream_FromFile() */
CC_API ReturnCode Stream_OpenFile(struct Stream* s, const String* path);
/* Wrapper for File_Create() then Stream_FromFile() */
CC_API ReturnCode Stream_CreateFile(struct Stream* s, const String* path);
/* Creates or overwrites a file, setting the contents to the given data. */
ReturnCode Stream_WriteTo(const String* path, uint8_t* data, uint32_t length);
/* Wraps a file, allowing reading from/writing to/seeking in the file. */
CC_API void Stream_FromFile(struct Stream* s, FileHandle file);

/* Wraps another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
CC_API void Stream_ReadonlyPortion(struct Stream* s, struct Stream* source, uint32_t len);
/* Wraps a block of memory, allowing reading from and seeking in the block. */
CC_API void Stream_ReadonlyMemory(struct Stream* s, void* data, uint32_t len);
/* Wraps a block of memory, allowing writing to and seeking in the block. */
CC_API void Stream_WriteonlyMemory(struct Stream* s, void* data, uint32_t len);
/* Wraps another Stream, reading through an intermediary buffer. (Useful for files, since each read call is expensive) */
CC_API void Stream_ReadonlyBuffered(struct Stream* s, struct Stream* source, void* data, uint32_t size);

/* Reads a little-endian 16 bit unsigned integer from memory. */
uint16_t Stream_GetU16_LE(const uint8_t* data);
/* Reads a big-endian 16 bit unsigned integer from memory. */
uint16_t Stream_GetU16_BE(const uint8_t* data);
/* Reads a little-endian 32 bit unsigned integer from memory. */
uint32_t Stream_GetU32_LE(const uint8_t* data);
/* Reads a big-endian 32 bit unsigned integer from memory. */
uint32_t Stream_GetU32_BE(const uint8_t* data);

/* Writes a big-endian 16 bit unsigned integer to memory. */
void Stream_SetU16_BE(uint8_t* data, uint16_t value);
/* Writes a little-endian 32 bit unsigned integer to memory. */
void Stream_SetU32_LE(uint8_t* data, uint32_t value);
/* Writes a big-endian 32 bit unsigned integer to memory. */
void Stream_SetU32_BE(uint8_t* data, uint32_t value);
/* Reads a little-endian 32 bit unsigned integer from the stream. */
ReturnCode Stream_ReadU32_LE(struct Stream* s, uint32_t* value);
/* Reads a big-endian 32 bit unsigned integer from the stream. */
ReturnCode Stream_ReadU32_BE(struct Stream* s, uint32_t* value);

/* Reads a line of UTF8 encoded character from the stream. */
/* NOTE: Reads one byte at a time. May want to use Stream_ReadonlyBuffered. */
CC_API ReturnCode Stream_ReadLine(struct Stream* s, String* text);
/* Writes a line of UTF8 encoded text to the stream. */
CC_API ReturnCode Stream_WriteLine(struct Stream* s, String* text);
#endif
