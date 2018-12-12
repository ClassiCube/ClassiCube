#ifndef CC_ASYNCDOWNLOADER_H
#define CC_ASYNCDOWNLOADER_H
#include "Constants.h"
#include "Utils.h"
/* Downloads images, texture packs, skins, etc in async manner.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
struct ScheduledTask;
extern struct IGameComponent AsyncDownloader_Component;
/* TODO: Implement these */
extern bool AsyncDownloader_Cookies;
/* TODO: Connection pooling */

enum REQUEST_TYPE { REQUEST_TYPE_DATA, REQUEST_TYPE_CONTENT_LENGTH };
enum AsyncProgress {
	ASYNC_PROGRESS_NOTHING        = -3,
	ASYNC_PROGRESS_MAKING_REQUEST = -2,
	ASYNC_PROGRESS_FETCHING_DATA  = -1
};

struct AsyncRequest {
	char URL[STRING_SIZE];  /* URL data is downloaded from/uploaded to. */
	char ID[STRING_SIZE];   /* Unique identifier for this request. */
	TimeMS TimeAdded;       /* Time this request was added to queue of requests. */
	TimeMS TimeDownloaded;  /* Time response contents were completely downloaded. */
	int StatusCode;         /* HTTP status code returned in the response. */
	uint32_t ContentLength; /* HTTP content length returned in the response. */

	ReturnCode Result; /* 0 on success, otherwise platform-specific error. */
	void*      Data;   /* Contents of the response. */
	uint32_t   Size;   /* Size of the contents. */

	TimeMS LastModified;    /* Time item cached at (if at all) */
	char Etag[STRING_SIZE]; /* ETag of cached item (if any) */
	uint8_t RequestType;    /* Whether to fetch contents or just headers. */
};

void ASyncRequest_Free(struct AsyncRequest* request);

void AsyncDownloader_GetSkin(const String* id, const String* skinName);
void AsyncDownloader_GetData(const String* url, bool priority, const String* id);
void AsyncDownloader_GetContentLength(const String* url, bool priority, const String* id);
/* TODO: Implement post */
void AsyncDownloader_UNSAFE_PostData(const String* url, bool priority, const String* id, const void* data, const uint32_t size);
void AsyncDownloader_GetDataEx(const String* url, bool priority, const String* id, TimeMS* lastModified, const String* etag);

bool AsyncDownloader_Get(const String* id, struct AsyncRequest* item);
bool AsyncDownloader_GetCurrent(struct AsyncRequest* request, int* progress);
void AsyncDownloader_PurgeOldEntriesTask(struct ScheduledTask* task);
#endif
