#ifndef CC_ZIPARCHIVE_H
#define CC_ZIPARCHIVE_H
#include "Typedefs.h"
#include "String.h"
/* Extracts entries from a .zip archive stream.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct ZipEntry_ {
	Int32 CompressedDataSize, UncompressedDataSize, LocalHeaderOffset;
	UInt32 Crc32;
} ZipEntry;
typedef struct Stream_ Stream; /* Forward declaration */

#define ZIP_MAX_ENTRIES 2048
typedef struct ZipState_ {
	Stream* Input;
	void (*ProcessEntry)(STRING_TRANSIENT String* path, Stream* data, ZipEntry* entry);
	bool (*SelectEntry)(STRING_TRANSIENT String* path);
	Int32 EntriesCount;
	ZipEntry Entries[ZIP_MAX_ENTRIES];
} ZipState;

void Zip_Init(ZipState* state, Stream* input);
void Zip_Extract(ZipState* state);
#endif