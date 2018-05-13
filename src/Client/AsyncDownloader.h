#ifndef CC_ASYNCDOWNLOADER_H
#define CC_ASYNCDOWNLOADER_H
#include "Constants.h"
#include "Utils.h"
#include "GameStructs.h"
#include "Bitmap.h"
/* Downloads images, texture packs, skins, etc in async manner.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

enum REQUEST_TYPE {
	REQUEST_TYPE_DATA, REQUEST_TYPE_IMAGE,
	REQUEST_TYPE_STRING, REQUEST_TYPE_CONTENT_LENGTH,
};
#define ASYNC_PROGRESS_NOTHING -3
#define ASYNC_PROGRESS_MAKING_REQUEST -2
#define ASYNC_PROGRESS_FETCHING_DATA -1

typedef struct AsyncRequest_ {
	UInt8 URL[String_BufferSize(STRING_SIZE)];
	UInt8 ID[String_BufferSize(STRING_SIZE)];

	DateTime TimeAdded;
	DateTime TimeDownloaded;
	UInt16 StatusCode;

	union {
		struct { void* Ptr; UInt32 Size; } ResultData;
		Bitmap ResultBitmap;
		String ResultString;
		UInt32 ResultContentLength;
	};

	DateTime LastModified;   /* Time item cached at (if at all) */
	UInt8 Etag[String_BufferSize(STRING_SIZE)]; /* ETag of cached item (if any) */
	UInt8 RequestType;
} AsyncRequest;

void ASyncRequest_Free(AsyncRequest* request);

IGameComponent AsyncDownloader_MakeComponent(void);
void AsyncDownloader_GetSkin(STRING_PURE String* id, STRING_PURE String* skinName);
void AsyncDownloader_GetData(STRING_PURE String* url, bool priority, STRING_PURE String* id);
void AsyncDownloader_GetImage(STRING_PURE String* url, bool priority, STRING_PURE String* id);
void AsyncDownloader_GetString(STRING_PURE String* url, bool priority, STRING_PURE String* id); 
void AsyncDownloader_GetContentLength(STRING_PURE String* url, bool priority, STRING_PURE String* id);
/* TODO: Implement post */
//void AsyncDownloader_PostString(STRING_PURE String* url, bool priority, STRING_PURE String* id, STRING_PURE String* contents);
void AsyncDownloader_GetDataEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag);
void AsyncDownloader_GetImageEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag);

bool AsyncDownloader_Get(STRING_PURE String* id, AsyncRequest* item);
bool AsyncDownloader_GetCurrent(AsyncRequest* request, Int32* progress);
void AsyncDownloader_PurgeOldEntriesTask(ScheduledTask* task);
#endif