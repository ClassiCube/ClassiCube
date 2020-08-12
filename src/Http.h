#ifndef CC_HTTP_H
#define CC_HTTP_H
#include "Constants.h"
#include "Utils.h"
/* Aysnchronously performs http GET, HEAD, and POST requests.
   Typically this is used to download skins, texture packs, etc.
   Copyright 2014-2020 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
struct ScheduledTask;
#define URL_MAX_SIZE (STRING_SIZE * 2)

extern struct IGameComponent Http_Component;

enum HttpRequestType { REQUEST_TYPE_GET, REQUEST_TYPE_HEAD, REQUEST_TYPE_POST };
enum HttpProgress {
	ASYNC_PROGRESS_NOTHING        = -3,
	ASYNC_PROGRESS_MAKING_REQUEST = -2,
	ASYNC_PROGRESS_FETCHING_DATA  = -1
};

struct HttpRequest {
	char url[URL_MAX_SIZE]; /* URL data is downloaded from/uploaded to. */
	char id[URL_MAX_SIZE];  /* Unique identifier for this request. */
	cc_uint64 _timeAdded;   /* Timestamp this request was added to requests queue. */
	TimeMS timeDownloaded;  /* Time response contents were completely downloaded. */
	int statusCode;         /* HTTP status code returned in the response. */
	cc_uint32 contentLength; /* HTTP content length returned in the response. */

	cc_result result; /* 0 on success, otherwise platform-specific error. */
	cc_uint8*   data; /* Contents of the response. (i.e. result data) */
	cc_uint32   size; /* Size of the contents. */

	char lastModified[STRING_SIZE]; /* Time item cached at (if at all) */
	char etag[STRING_SIZE];         /* ETag of cached item (if any) */
	cc_uint8 requestType;           /* See the various REQUEST_TYPE_ */
	cc_bool success;                /* Whether Result is 0, status is 200, and data is not NULL */
	struct StringsBuffer* cookies;  /* Cookie list sent in requests. May be modified by the response. */
};

/* Frees data from a HTTP request. */
void HttpRequest_Free(struct HttpRequest* request);

/* Aschronously performs a http GET request to download a skin. */
/* If url is a skin, downloads from there. (if not, downloads from SKIN_SERVER/[skinName].png) */
/* ID of the request is set to skinName. */
void Http_AsyncGetSkin(const String* skinName);
/* Asynchronously performs a http GET request. (e.g. to download data) */
void Http_AsyncGetData(const String* url, cc_bool priority, const String* id);
/* Asynchronously performs a http HEAD request. (e.g. to get Content-Length header) */
void Http_AsyncGetHeaders(const String* url, cc_bool priority, const String* id);
/* Asynchronously performs a http POST request. (e.g. to submit data) */
/* NOTE: You don't have to persist data, a copy is made of it. */
void Http_AsyncPostData(const String* url, cc_bool priority, const String* id, const void* data, cc_uint32 size, struct StringsBuffer* cookies);
/* Asynchronously performs a http GET request. (e.g. to download data) */
/* Also sets the If-Modified-Since and If-None-Match headers. (if not NULL)  */
void Http_AsyncGetDataEx(const String* url, cc_bool priority, const String* id, const String* lastModified, const String* etag, struct StringsBuffer* cookies);

/* Encodes data using % or URL encoding. */
void Http_UrlEncode(String* dst, const cc_uint8* data, int len);
/* Converts characters to UTF8, then calls Http_URlEncode on them. */
void Http_UrlEncodeUtf8(String* dst, const String* src);
/* Outputs more detailed information about errors with http requests. */
cc_bool Http_DescribeError(cc_result res, String* dst);

/* Attempts to retrieve a fully completed request. */
/* NOTE: You MUST check Success for whether it completed successfully. */
/* (Data may still be non NULL even on error, e.g. on a http 404 error) */
cc_bool Http_GetResult(const String* id, struct HttpRequest* item);
/* Retrieves information about the request currently being processed. */
cc_bool Http_GetCurrent(struct HttpRequest* request, int* progress);
/* Clears the list of pending requests. */
void Http_ClearPending(void);
#endif
