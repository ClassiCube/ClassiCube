#ifndef CC_TEXPACKS_H
#define CC_TEXPACKS_H
#include "Utils.h"
/* Extracts entries from a .zip archive stream (mostly resources for .zip texture pack)
   Caches terrain atlases and texture packs to avoid making redundant downloads. 
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;
struct AsyncRequest;

struct ZipEntry { UInt32 CompressedSize, UncompressedSize, LocalHeaderOffset, Crc32; };

#define ZIP_MAX_ENTRIES 2048
struct ZipState {
	struct Stream* Input;
	void (*ProcessEntry)(const String* path, struct Stream* data, struct ZipEntry* entry);
	bool (*SelectEntry)(const String* path);
	Int32 EntriesCount;
	struct ZipEntry Entries[ZIP_MAX_ENTRIES];
};

void Zip_Init(struct ZipState* state, struct Stream* input);
ReturnCode Zip_Extract(struct ZipState* state);

void TextureCache_Init(void);
bool TextureCache_HasAccepted(const String* url);
bool TextureCache_HasDenied(const String* url);
void TextureCache_Accept(const String* url);
void TextureCache_Deny(const String* url);

bool TextureCache_HasUrl(const String* url);
bool TextureCache_GetStream(const String* url, struct Stream* stream);
void TextureCache_GetLastModified(const String* url, TimeMS* time);
void TextureCache_GetETag(const String* url, String* etag);
void TextureCache_AddData(const String* url, UInt8* data, UInt32 length);
void TextureCache_AddETag(const String* url, const String* etag);
void TextureCache_AddLastModified(const String* url, TimeMS* lastModified);

void TexturePack_ExtractZip_File(const String* filename);
void TexturePack_ExtractDefault(void);
void TexturePack_ExtractCurrent(const String* url);
void TexturePack_Extract_Req(struct AsyncRequest* item);
#endif
