#ifndef CC_TEXPACKS_H
#define CC_TEXPACKS_H
#include "Utils.h"
/* Extracts entries from a .zip archive stream (mostly resources for .zip texture pack)
   Caches terrain atlases and texture packs to avoid making redundant downloads. 
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Stream;
struct Bitmap;
struct AsyncRequest;

struct ZipEntry {
	Int32 CompressedDataSize, UncompressedDataSize, LocalHeaderOffset; UInt32 Crc32;
};

#define ZIP_MAX_ENTRIES 2048
struct ZipState {
	struct Stream* Input;
	void (*ProcessEntry)(STRING_TRANSIENT String* path, struct Stream* data, struct ZipEntry* entry);
	bool (*SelectEntry)(STRING_PURE String* path);
	Int32 EntriesCount;
	struct ZipEntry Entries[ZIP_MAX_ENTRIES];
};

void Zip_Init(struct ZipState* state, struct Stream* input);
void Zip_Extract(struct ZipState* state);

void TextureCache_Init(void);
bool TextureCache_HasAccepted(STRING_PURE String* url);
bool TextureCache_HasDenied(STRING_PURE String* url);
void TextureCache_Accept(STRING_PURE String* url);
void TextureCache_Deny(STRING_PURE String* url);

bool TextureCache_HasUrl(STRING_PURE String* url);
bool TextureCache_GetStream(STRING_PURE String* url, struct Stream* stream);
void TextureCache_GetLastModified(STRING_PURE String* url, DateTime* time);
void TextureCache_GetETag(STRING_PURE String* url, STRING_PURE String* etag);
void TextureCache_AddImage(STRING_PURE String* url, struct Bitmap* bmp);
void TextureCache_AddData(STRING_PURE String* url, UInt8* data, UInt32 length);
void TextureCache_AddETag(STRING_PURE String* url, STRING_PURE String* etag);
void TextureCache_AddLastModified(STRING_PURE String* url, DateTime* lastModified);

void TexturePack_ExtractZip_File(STRING_PURE String* filename);
void TexturePack_ExtractDefault(void);
void TexturePack_ExtractCurrent(STRING_PURE String* url);
void TexturePack_ExtractTerrainPng_Req(struct AsyncRequest* item);
void TexturePack_ExtractTexturePack_Req(struct AsyncRequest* item);
#endif
