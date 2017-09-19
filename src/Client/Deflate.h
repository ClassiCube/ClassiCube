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
#endif