#ifndef CC_ASYNCDOWNLOADER_H
#define CC_ASYNCDOWNLOADER_H
#include "Constants.h"
#include "Utils.h"
#include "Bitmap.h"
/* Downloads images, texture packs, skins, etc in async manner.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
struct ScheduledTask;

enum REQUEST_TYPE { REQUEST_TYPE_DATA, REQUEST_TYPE_CONTENT_LENGTH };
enum ASYNC_PROGRESS {
	ASYNC_PROGRESS_NOTHING = -3,
	ASYNC_PROGRESS_MAKING_REQUEST = -2,
	ASYNC_PROGRESS_FETCHING_DATA = -1,
};

struct AsyncRequest {
	char URL[STRING_SIZE];
	char ID[STRING_SIZE];

	DateTime TimeAdded;
	DateTime TimeDownloaded;
	UInt16 StatusCode;

	void* ResultData;
	UInt32 ResultSize;

	DateTime LastModified;   /* Time item cached at (if at all) */
	UInt8 Etag[STRING_SIZE]; /* ETag of cached item (if any) */
	UInt8 RequestType;
};

void ASyncRequest_Free(struct AsyncRequest* request);

void AsyncDownloader_MakeComponent(struct IGameComponent* comp);
void AsyncDownloader_GetSkin(STRING_PURE String* id, STRING_PURE String* skinName);
void AsyncDownloader_GetData(STRING_PURE String* url, bool priority, STRING_PURE String* id);
void AsyncDownloader_GetContentLength(STRING_PURE String* url, bool priority, STRING_PURE String* id);
/* TODO: Implement post */
/* void AsyncDownloader_PostString(STRING_PURE String* url, bool priority, STRING_PURE String* id, STRING_PURE String* contents); */
void AsyncDownloader_GetDataEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag);

bool AsyncDownloader_Get(STRING_PURE String* id, struct AsyncRequest* item);
bool AsyncDownloader_GetCurrent(struct AsyncRequest* request, Int32* progress);
void AsyncDownloader_PurgeOldEntriesTask(struct ScheduledTask* task);
#endif
