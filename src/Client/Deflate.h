#ifndef CS_DEFLATE_H
#define CS_DEFLATE_H
#include "Typedefs.h"
#include "Stream.h"
/* Decodes data compressed using DEFLATE.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct GZipHeader_ {
	UInt8 State;
	bool Done;
	UInt8 PartsRead;
	Int32 Flags;	
} GZipHeader;

/* Initalises state of GZIP header. */
void GZipHeader_Init(GZipHeader* header);
/* Reads part of the GZIP header. header.Done is set to true on completion of reading the header. */
void GZipHeader_Read(Stream* s, GZipHeader* header);


typedef struct ZLibHeader_ {
	UInt8 State;
	bool Done;
	Int32 LZ77WindowSize;
} ZLibHeader;

/* Initalises state of ZLIB header. */
void ZLibHeader_Init(ZLibHeader* header);
/* Reads part of the ZLIB header. header.Done is set to true on completion of reading the header. */
void ZLibHeader_Read(Stream* s, ZLibHeader* header);


#define DEFLATE_INPUT_MAX 1024
typedef struct DeflateState_ {
	UInt8 State;
	Stream* Source;
	bool LastBlock; /* Whether the last DEFLATE block has been encounted in the stream */

	UInt32 Bits;    /* Holds bits across byte boundaries*/
	UInt32 NumBits; /* Number of bits in Bits buffer*/

	UInt8 Input[DEFLATE_INPUT_MAX]; /* Buffer for input to DEFLATE */
	UInt32 NextIn;                  /* Index within Input of byte being read */
	UInt32 AvailIn;                 /* Number of bytes that can be read from Input */

	UInt8* Output;    /* Pointer for output data */
	UInt32 AvailOut;  /* Max number of bytes to output */

	UInt32 UncompressedLen;
} DeflateState;

void Deflate_Init(DeflateState* state, Stream* source);
void Deflate_Process(DeflateState* state);
#endif