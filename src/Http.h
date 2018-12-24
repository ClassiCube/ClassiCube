#ifndef CC_HTTP_H
#define CC_HTTP_H
#include "Constants.h"
#include "Utils.h"
/* Aysnchronously performs http GET, HEAD, and POST requests.
   Typically this is used to download skins, texture packs, etc.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;
struct ScheduledTask;
#define URL_MAX_SIZE (STRING_SIZE * 2)

extern struct IGameComponent Http_Component;
/* TODO: Implement these */
extern bool Http_UseCookies;
/* TODO: Connection pooling */

enum HttpRequestType { REQUEST_TYPE_GET, REQUEST_TYPE_HEAD };
enum HttpProgress {
	ASYNC_PROGRESS_NOTHING        = -3,
	ASYNC_PROGRESS_MAKING_REQUEST = -2,
	ASYNC_PROGRESS_FETCHING_DATA  = -1
};

struct HttpRequest {
	char URL[URL_MAX_SIZE]; /* URL data is downloaded from/uploaded to. */
	char ID[STRING_SIZE];   /* Unique identifier for this request. */
	TimeMS TimeAdded;       /* Time this request was added to queue of requests. */
	TimeMS TimeDownloaded;  /* Time response contents were completely downloaded. */
	int StatusCode;         /* HTTP status code returned in the response. */
	uint32_t ContentLength; /* HTTP content length returned in the response. */

	ReturnCode Result; /* 0 on success, otherwise platform-specific error. */
	void*      Data;   /* Contents of the response. (i.e. result data) */
	uint32_t   Size;   /* Size of the contents. */

	TimeMS LastModified;    /* Time item cached at (if at all) */
	char Etag[STRING_SIZE]; /* ETag of cached item (if any) */
	uint8_t RequestType;    /* Whether to fetch contents or just headers. */
};

/* Frees data from a HTTP request. */
void HttpRequest_Free(struct HttpRequest* request);

/* Aschronously performs a http GET request to download a skin. */
/* If url is a skin, this is the same as Http_AsyncGetData. */
/* If not, instead downloads from http://static.classicube.net/skins/[skinName].png */
void Http_AsyncGetSkin(const String* id, const String* skinName);
/* Asynchronously performs a http GET request. (e.g. to download data) */
void Http_AsyncGetData(const String* url, bool priority, const String* id);
/* Asynchronously performs a http HEAD request. (e.g. to get Content-Length header) */
void Http_AsyncGetHeaders(const String* url, bool priority, const String* id);
/* Asynchronously performs a http POST request. (e.g. to submit data) */
/* NOTE: data and size MUST PERSIST until the request is completed. */
/* TODO: maybe we shouldn't force that.. instead mem alloc data and size */
void Http_AsyncPostData(const String* url, bool priority, const String* id, const void* data, const uint32_t size);
/* Asynchronously performs a http GET request. (e.g. to download data) */
/* Also sets the If-Modified-Since and If-None-Match headers. (if not NULL)  */
void Http_AsyncGetDataEx(const String* url, bool priority, const String* id, TimeMS* lastModified, const String* etag);

/* Attempts to retrieve a fully completed request. */
/* NOTE: You MUST also check Result/StatusCode, and check Size is > 0. */
/* (because a completed request may not have completed successfully) */
bool Http_GetResult(const String* id, struct HttpRequest* item);
/* Retrieves information about the request currently being processed. */
bool Http_GetCurrent(struct HttpRequest* request, int* progress);
/* Clears the list of pending requests. */
void Http_ClearPending(void);
void Http_PurgeOldEntriesTask(struct ScheduledTask* task);
#endif
