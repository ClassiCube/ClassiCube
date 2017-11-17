#ifndef CC_ASYNCDOWNLOADER_H
#define CC_ASYNCDOWNLOADER_H
#include "Typedefs.h"
#include "Constants.h"
#include "DateTime.h"
#include "GameStructs.h"
#include "String.h"
/* Downloads images, texture packs, skins, etc in async manner.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define REQUEST_TYPE_BITMAP 0
#define REQUEST_TYPE_STRING 1
#define REQUEST_TYPE_DATA 2
#define REQUEST_TYPE_CONTENT_LENGTH 3

typedef struct AsyncRequest_ {
	UInt8 URL[String_BufferSize(STRING_SIZE)];
	UInt8 Identifier[String_BufferSize(STRING_SIZE)];

	DateTime TimeAdded;
	DateTime TimeDownloaded;
	UInt8 RequestType;
	void* Data;

	DateTime LastModified;   /* Time item ached at (if at all) */
	UInt8 Etag[String_BufferSize(STRING_SIZE)]; /* ETag of cached item (if any) */
} AsyncRequest;

void ASyncRequest_Free(AsyncRequest* request);

void AsyncDownloader_MakeComponent(void);
void AsyncDownloader_Init(STRING_PURE String* skinServer);
void AsyncDownloader_DownloadSkin(STRING_PURE String* identifier, STRING_PURE String* skinName);
void AsyncDownloader_Download(STRING_PURE  String* url, bool priority, UInt8 type, STRING_PURE String* identifier);
void AsyncDownloader_Download2(STRING_PURE String* url, bool priority, UInt8 type, STRING_PURE String* identifier, DateTime* lastModified, STRING_PURE String* etag);
void AsyncDownloader_Free(void);
bool AsyncDownloader_Get(STRING_PURE String* identifier, AsyncRequest* item);
#endif