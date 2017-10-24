#ifndef CC_ZIPARCHIVE_H
#define CC_ZIPARCHIVE_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"

typedef struct ZipEntry_ {
	Int32 CompressedDataSize;
	Int32 UncompressedDataSize;
	Int32 LocalHeaderOffset;
	UInt32 Crc32;
} ZipEntry;

#define ZIP_MAX_ENTRIES 2048
typedef struct ZipState_ {
	Stream* Input;
	/* Processes the data of a .ZIP entry */
	void (*ProcessEntry)(STRING_TRANSIENT String* path, Stream* data, ZipEntry* entry);
	/* Selects which .ZIP entries are actually processed */
	bool (*SelectEntry)(STRING_TRANSIENT String* path);
	Int32 EntriesCount;
	ZipEntry Entries[ZIP_MAX_ENTRIES];
} ZipState;

void Zip_Init(ZipState* state, Stream* input);
void Zip_Extract(ZipState* state);
#endif