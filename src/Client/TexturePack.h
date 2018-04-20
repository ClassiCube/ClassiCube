#ifndef CC_TEXPACKS_H
#define CC_TEXPACKS_H
#include "String.h"
/* Extracts entries from a .zip archive stream (mostly resources for .zip texture pack)
   Caches terrain atlases and texture packs to avoid making redundant downloads. 
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct Stream_ Stream;
typedef struct DateTime_ DateTime;
typedef struct Bitmap_ Bitmap;
typedef struct AsyncRequest_ AsyncRequest;

typedef struct ZipEntry_ {
	Int32 CompressedDataSize, UncompressedDataSize, LocalHeaderOffset; UInt32 Crc32;
} ZipEntry;

#define ZIP_MAX_ENTRIES 2048
typedef struct ZipState_ {
	Stream* Input;
	void (*ProcessEntry)(STRING_TRANSIENT String* path, Stream* data, ZipEntry* entry);
	bool (*SelectEntry)(STRING_PURE String* path);
	Int32 EntriesCount;
	ZipEntry Entries[ZIP_MAX_ENTRIES];
} ZipState;

void Zip_Init(ZipState* state, Stream* input);
void Zip_Extract(ZipState* state);

void TextureCache_Init(void);
bool TextureCache_HasAccepted(STRING_PURE String* url);
bool TextureCache_HasDenied(STRING_PURE String* url);
void TextureCache_Accept(STRING_PURE String* url);
void TextureCache_Deny(STRING_PURE String* url);

bool TextureCache_HasUrl(STRING_PURE String* url);
bool TextureCache_GetStream(STRING_PURE String* url, Stream* stream);
void TextureCache_GetLastModified(STRING_PURE String* url, DateTime* time);
void TextureCache_GetETag(STRING_PURE String* url, STRING_PURE String* etag);
void TextureCache_AddImage(STRING_PURE String* url, Bitmap* bmp);
void TextureCache_AddData(STRING_PURE String* url, UInt8* data, UInt32 length);
void TextureCache_AddETag(STRING_PURE String* url, STRING_PURE String* etag);
void TextureCache_AddLastModified(STRING_PURE String* url, DateTime* lastModified);

void TexturePack_ExtractZip_File(STRING_PURE String* filename);
void TexturePack_ExtractDefault(void);
void TexturePack_ExtractCurrent(STRING_PURE String* url);
void TexturePack_ExtractTerrainPng_Req(AsyncRequest* item);
void TexturePack_ExtractTexturePack_Req(AsyncRequest* item);
#endif