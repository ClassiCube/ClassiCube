#ifndef CS_ZIPARCHIVE_H
#define CS_ZIPARCHIVE_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"

typedef struct ZipEntry_ {
	Int32 CompressedDataSize;
	Int32 UncompressedDataSize;
	Int32 LocalHeaderOffset;
	UInt32 Crc32;
} ZipEntry;

/* Processes the data of a .ZIP entry */
typedef void (*ZipEntryProcessor)(String path, Stream* data, ZipEntry* entry);
/* Selects which .ZIP entries are actually processed */
typedef bool (*ZipEntrySelector)(String path);

#define ZIP_MAX_ENTRIES 2048
typedef struct ZipState_ {
	Stream* Input;
	ZipEntryProcessor ProcessEntry;
	ZipEntrySelector SelectEntry;
	Int32 EntriesCount;
	ZipEntry Entries[ZIP_MAX_ENTRIES];
} ZipState;

void Zip_Init(ZipState* state, Stream* input);
void Zip_Extract(ZipState* state);
#endif