#ifndef CC_STREAM_H
#define CC_STREAM_H
#include "Constants.h"
#include "Platform.h"
CC_BEGIN_HEADER

/* Defines an abstract way of reading and writing data in a streaming manner.
   Also provides common helper methods for reading/writing data to/from streams.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

struct Stream;
/* Represents a stream that can be written to and/or read from. */
struct Stream {
	/* Attempts to read some bytes from this stream. */
	cc_result (*Read)(struct Stream* s, cc_uint8* data, cc_uint32 count, cc_uint32* modified);
	/* Attempts to efficiently read a single byte from this stream. (falls back to Read) */
	cc_result (*ReadU8)(struct Stream* s, cc_uint8* data);
	/* Attempts to write some bytes to this stream. */
	cc_result (*Write)(struct Stream* s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified);
	/* Attempts to quickly advance the position in this stream. (falls back to reading then discarding) */
	cc_result (*Skip)(struct Stream* s, cc_uint32 count);

	/* Attempts to seek to the given position in this stream. (may not be supported) */
	cc_result (*Seek)(struct Stream* s, cc_uint32 position);
	/* Attempts to find current position this stream. (may not be supported) */
	cc_result (*Position)(struct Stream* s, cc_uint32* position);
	/* Attempts to find total length of this stream. (may not be supported) */
	cc_result (*Length)(struct Stream* s, cc_uint32* length);
	/* Attempts to close this stream, freeing associated resources. */
	cc_result (*Close)(struct Stream* s);
	
	union {
		cc_file file;
		void* inflate;
		struct { cc_uint8* cur; cc_uint32 left, length; cc_uint8* base; } mem;
		struct { struct Stream* source; cc_uint32 left, length; } portion;
		struct { cc_uint8* cur; cc_uint32 left, length; cc_uint8* base; struct Stream* source; cc_uint32 end; } buffered;
		struct { struct Stream* source; cc_uint32 crc32; } crc32;
	} meta;
};

/* Attempts to fully read up to count bytes from the stream. */
CC_API  cc_result Stream_Read(      struct Stream* s, cc_uint8* buffer, cc_uint32 count);
typedef cc_result (*FP_Stream_Read)(struct Stream* s, cc_uint8* buffer, cc_uint32 count);
/* Attempts to fully write up to count bytes from the stream. */
CC_API  cc_result Stream_Write(      struct Stream* s, const cc_uint8* buffer, cc_uint32 count);
typedef cc_result (*FP_Stream_Write)(struct Stream* s, const cc_uint8* buffer, cc_uint32 count);
/* Initialises default function pointers for a stream. (all read, write, seeks return an error) */
void Stream_Init(struct Stream* s);

/* Wrapper for File_Open() then Stream_FromFile() */
CC_API  cc_result Stream_OpenFile(      struct Stream* s, const cc_string* path);
typedef cc_result (*FP_Stream_OpenFile)(struct Stream* s, const cc_string* path);
/* Wrapper for File_Create() then Stream_FromFile() */
CC_API  cc_result Stream_CreateFile(      struct Stream* s, const cc_string* path);
typedef cc_result (*FP_Stream_CreateFile)(struct Stream* s, const cc_string* path);
/* Wrapper for File_OpenOrCreate, then File_Seek(END), then Stream_FromFile() */
cc_result Stream_AppendFile(struct Stream* s, const cc_string* path);
/* Creates or overwrites a file, setting the contents to the given data. */
cc_result Stream_WriteAllTo(const cc_string* path, const cc_uint8* data, cc_uint32 length);
/* Wraps a file, allowing reading from/writing to/seeking in the file. */
CC_API void Stream_FromFile(struct Stream* s, cc_file file);

/* Wraps another Stream, only allows reading up to 'len' bytes from the wrapped stream. */
CC_API void Stream_ReadonlyPortion(struct Stream* s, struct Stream* source, cc_uint32 len);
/* Wraps a block of memory, allowing reading from and seeking in the block. */
CC_API void Stream_ReadonlyMemory(struct Stream* s, void* data, cc_uint32 len);
/* Wraps another Stream, reading through an intermediary buffer. (Useful for files, since each read call is expensive) */
CC_API void Stream_ReadonlyBuffered(struct Stream* s, struct Stream* source, void* data, cc_uint32 size);

/* Wraps another Stream, calculating a running CRC32 as data is written. */
/* To get the final CRC32, xor it with 0xFFFFFFFFUL */
void Stream_WriteonlyCrc32(struct Stream* s, struct Stream* source);

/* Reads a little-endian 16 bit unsigned integer from memory. */
cc_uint16 Stream_GetU16_LE(const cc_uint8* data);
/* Reads a big-endian 16 bit unsigned integer from memory. */
cc_uint16 Stream_GetU16_BE(const cc_uint8* data);
/* Reads a little-endian 32 bit unsigned integer from memory. */
cc_uint32 Stream_GetU32_LE(const cc_uint8* data);
/* Reads a big-endian 32 bit unsigned integer from memory. */
cc_uint32 Stream_GetU32_BE(const cc_uint8* data);

/* Writes a little-endian 16 bit unsigned integer to memory. */
void Stream_SetU16_LE(cc_uint8* data, cc_uint16 value);
/* Writes a big-endian 16 bit unsigned integer to memory. */
void Stream_SetU16_BE(cc_uint8* data, cc_uint16 value);
/* Writes a little-endian 32 bit unsigned integer to memory. */
void Stream_SetU32_LE(cc_uint8* data, cc_uint32 value);
/* Writes a big-endian 32 bit unsigned integer to memory. */
void Stream_SetU32_BE(cc_uint8* data, cc_uint32 value);
/* Reads a little-endian 32 bit unsigned integer from the stream. */
cc_result Stream_ReadU32_LE(struct Stream* s, cc_uint32* value);
/* Reads a big-endian 32 bit unsigned integer from the stream. */
cc_result Stream_ReadU32_BE(struct Stream* s, cc_uint32* value);

/* Reads a line of UTF8 encoded character from the stream. */
/* NOTE: Reads one byte at a time. May want to use Stream_ReadonlyBuffered. */
CC_API cc_result Stream_ReadLine(struct Stream* s, cc_string* text);
/* Writes a line of UTF8 encoded text to the stream. */
CC_API cc_result Stream_WriteLine(struct Stream* s, cc_string* text);

CC_END_HEADER
#endif
