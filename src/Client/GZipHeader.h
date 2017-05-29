#ifndef CS_GZIPHEADERREADER_H
#define CS_GZIPHEADERREADER_H
#include "Typedefs.h"
#include "Stream.h"
/* Skips the GZip header in a stream.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef Int32 GZipState;
#define GZipState_Header1           0
#define GZipState_Header2           1
#define GZipState_CompressionMethod 2
#define GZipState_Flags             3
#define GZipState_LastModifiedTime  4
#define GZipState_CompressionFlags  5
#define GZipState_OperatingSystem   6
#define GZipState_HeaderChecksum    7
#define GZipState_Filename          8
#define GZipState_Comment           9
#define GZipState_Done              10

typedef struct GZipHeader {
	/* State header reader is up to.*/
	GZipState State;
	
	/* Whether header has finished being read. */
	bool Done;

	Int32 Flags;

	Int32 PartsRead;
} GZipHeader;

/* Initalises state of GZIP header. */
void GZipHeader_Init(GZipHeader* header);

/* Reads part of the GZIP header. header.Done is set to true on completion. */
void GZipHeader_Read(Stream* s, GZipHeader* header);

static bool GZipHeader_ReadAndCheckByte(Stream* s, GZipHeader* header, UInt8 expected);

static bool GZipHeader_ReadByte(Stream* s, GZipHeader* header, Int32* value);
#endif