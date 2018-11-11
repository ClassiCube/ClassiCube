#ifndef CC_TEXPACKS_H
#define CC_TEXPACKS_H
#include "Utils.h"
/* Extracts entries from a .zip archive stream (mostly resources for .zip texture pack)
   Caches terrain atlases and texture packs to avoid making redundant downloads. 
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;
struct AsyncRequest;

/* Minimal data needed to describe an entry in a .zip archive. */
struct ZipEntry { uint32_t CompressedSize, UncompressedSize, LocalHeaderOffset, Crc32; };

#define ZIP_MAX_ENTRIES 2048
/* Stores state for reading and processing entries in a .zip archive. */
struct ZipState {
	/* Source of the .zip archive data. Must be seekable. */
	struct Stream* Input;
	/* Callback function to process the data in a .zip archive entry. */
	/* Note that data stream may not be seekable. (entry data might be compressed) */
	void (*ProcessEntry)(const String* path, struct Stream* data, struct ZipEntry* entry);
	/* Predicate used to select which entries in a .zip archive get proessed.*/
	/* Return false to skip the entry. (this avoids seeking to the entry's data) */
	bool (*SelectEntry)(const String* path);
	/* Number of entries in the .zip archive. */
	int EntriesCount;
	/* Data for each entry in the .zip archive. */
	struct ZipEntry Entries[ZIP_MAX_ENTRIES];
};

/* Initialises .zip archive reader state. */
void Zip_Init(struct ZipState* state, struct Stream* input);
/* Reads and processes the entries in a .zip archive. */
/* Must have been initialised with Zip_Init first. */
ReturnCode Zip_Extract(struct ZipState* state);

/* Initialises cache state. (e.g. loading accepted/denied lists) */
void TextureCache_Init(void);
/* Whether the given URL is in list of accepted URLs. */
bool TextureCache_HasAccepted(const String* url);
/* Whether the given URL is in list of denied URLs. */
bool TextureCache_HasDenied(const String* url);
/* Adds the given URL to list of accepted URLs, then saves it. */
/* Accepted URLs are loaded without prompting the user. */
void TextureCache_Accept(const String* url);
/* Adds the given URL to list of denied URLs, then saves it. */
/* Denied URLs are never loaded. */
void TextureCache_Deny(const String* url);

/* Returns whether the given URL has been cached. */
bool TextureCache_Has(const String* url);
/* Attempts to get the cached data stream for the given url. */
bool TextureCache_Get(const String* url, struct Stream* stream);
/* Attempts to get the Last-Modified header cached for the given URL. */
/* If header is not found, returns last time the cached data was modified. */
void TextureCache_GetLastModified(const String* url, TimeMS* time);
/* Attempts to get the ETag header cached for the given URL. */
void TextureCache_GetETag(const String* url, String* etag);
/* Sets the cached data for the given url. */
void TextureCache_Set(const String* url, uint8_t* data, uint32_t length);
/* Sets the cached ETag header for the given url. */
void TextureCache_SetETag(const String* url, const String* etag);
/* Sets the cached Last-Modified header for the given url. */
void TextureCache_SetLastModified(const String* url, const TimeMS* lastModified);

void TexturePack_ExtractZip_File(const String* filename);
void TexturePack_ExtractDefault(void);
void TexturePack_ExtractCurrent(const String* url);
void TexturePack_Extract_Req(struct AsyncRequest* item);
#endif
