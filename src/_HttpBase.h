#include "Http.h"
#include "String_.h"
#include "Platform.h"
#include "Funcs.h"
#include "Logger.h"
#include "Stream.h"
#include "Game.h"
#include "Utils.h"
#include "Options.h"

static cc_bool httpsOnly, httpOnly, httpsVerify;
static char skinServer_buffer[128];
static cc_string skinServer = String_FromArray(skinServer_buffer);

void HttpRequest_Free(struct HttpRequest* request) {
	Mem_Free(request->data);
	Mem_Free(request->error);

	request->data  = NULL;
	request->size  = 0;
	request->error = NULL;
	request->_capacity = 0;
}
#define HttpRequest_Copy(dst, src) Mem_Copy(dst, src, sizeof(struct HttpRequest))


/*########################################################################################################################*
*----------------------------------------------------Http requests list---------------------------------------------------*
*#########################################################################################################################*/
#define HTTP_DEF_ELEMS 10

struct RequestList {
	int count, capacity;
	struct HttpRequest* entries;
	struct HttpRequest defaultEntries[HTTP_DEF_ELEMS];
};

/* Expands request list buffer if there is no room for another request */
static void RequestList_EnsureSpace(struct RequestList* list) {
	if (list->count < list->capacity) return;
	Utils_Resize((void**)&list->entries, &list->capacity,
				sizeof(struct HttpRequest), HTTP_DEF_ELEMS, 10);
}

/* Adds a request to the list */
static void RequestList_Append(struct RequestList* list, struct HttpRequest* item, cc_uint8 flags) {
	int i;
	RequestList_EnsureSpace(list);

	if (flags & HTTP_FLAG_PRIORITY) {
		/* Shift all requests right one place */
		for (i = list->count; i > 0; i--) 
		{
			HttpRequest_Copy(&list->entries[i], &list->entries[i - 1]);
		}
		/* Insert new request at front/start */
		i = 0;
	} else {
		/* Insert new request at end */
		i = list->count;
	}

	HttpRequest_Copy(&list->entries[i], item);
	list->count++;
}

/* Removes the request at the given index */
static void RequestList_RemoveAt(struct RequestList* list, int i) {
	if (i < 0 || i >= list->count) Process_Abort("Tried to remove element at list end");

	for (; i < list->count - 1; i++) 
	{
		HttpRequest_Copy(&list->entries[i], &list->entries[i + 1]);
	}
	list->count--;
}

/* Finds index of request whose id matches the given id */
static int RequestList_Find(struct RequestList* list, int id) {
	int i;
	for (i = 0; i < list->count; i++) {
		if (id != list->entries[i].id) continue;
		return i;
	}
	return -1;
}

/* Tries to remove and free given request */
static void RequestList_TryFree(struct RequestList* list, int id) {
	int i = RequestList_Find(list, id);
	if (i < 0) return;

	HttpRequest_Free(&list->entries[i]);
	RequestList_RemoveAt(list, i);
}

/* Resets state to default */
static void RequestList_Init(struct RequestList* list) {
	list->capacity = HTTP_DEF_ELEMS;
	list->count    = 0;
	list->entries = list->defaultEntries;
}

/* Frees any dynamically allocated memory, then resets state to default */
static void RequestList_Free(struct RequestList* list) {
	if (list->entries != list->defaultEntries) Mem_Free(list->entries);
	RequestList_Init(list);
}


/*########################################################################################################################*
*---------------------------------------------------Common buffer code----------------------------------------------------*
*#########################################################################################################################*/
/* Ensures data buffer has enough space left to append amount bytes */
static cc_bool Http_BufferExpand(struct HttpRequest* req, cc_uint32 amount) {
	cc_uint32 newSize = req->size + amount;
	cc_uint8* ptr;
	if (newSize <= req->_capacity) return true;

	if (!req->_capacity) {
		/* Allocate initial storage */
		req->_capacity = req->contentLength ? req->contentLength : 1;
		req->_capacity = max(req->_capacity, newSize);

		ptr = (cc_uint8*)Mem_TryAlloc(req->_capacity, 1);
	} else {
		/* Reallocate if capacity reached */
		req->_capacity = newSize;
		ptr = (cc_uint8*)Mem_TryRealloc(req->data, newSize, 1);
	}

	if (!ptr) return false;
	req->data = ptr;
	return true;
}

/* Increases size and updates current progress */
static void Http_BufferExpanded(struct HttpRequest* req, cc_uint32 read) {
	req->size += read;
	if (req->contentLength) req->progress = (int)(100.0f * req->size / req->contentLength);
}


/*########################################################################################################################*
*---------------------------------------------------Common header code----------------------------------------------------*
*#########################################################################################################################*/
static void Http_ParseCookie(struct HttpRequest* req, const cc_string* value) {
	cc_string name, data;
	int dataEnd;
	String_UNSAFE_Separate(value, '=', &name, &data);
	/* Cookie is: __cfduid=xyz; expires=abc; path=/; domain=.classicube.net; HttpOnly */
	/* However only the __cfduid=xyz part of the cookie should be stored */
	dataEnd = String_IndexOf(&data, ';');
	if (dataEnd >= 0) data.length = dataEnd;

	EntryList_Set(req->cookies, &name, &data, '=');
}

static void Http_ParseContentLength(struct HttpRequest* req, const cc_string* value) {
	int contentLen = 0;
	Convert_ParseInt(value, &contentLen);
	
	if (contentLen <= 0) return;
	req->contentLength = contentLen;
}

/* Parses a HTTP header */
static void Http_ParseHeader(struct HttpRequest* req, const cc_string* line) {
	static const cc_string httpVersion = String_FromConst("HTTP");
	cc_string name, value, parts[3];
	int numParts;

	/* HTTP[version] [status code] [status reason] */
	if (String_CaselessStarts(line, &httpVersion)) {
		numParts = String_UNSAFE_Split(line, ' ', parts, 3);
		if (numParts >= 2) Convert_ParseInt(&parts[1], &req->statusCode);
	}
	/* For all other headers:  name: value */
	if (!String_UNSAFE_Separate(line, ':', &name, &value)) return;

	if (String_CaselessEqualsConst(&name, "ETag")) {
		String_CopyToRawArray(req->etag, &value);
	} else if (String_CaselessEqualsConst(&name, "Content-Length")) {
		Http_ParseContentLength(req, &value);
	} else if (String_CaselessEqualsConst(&name, "X-Dropbox-Content-Length")) {
		/* dropbox stopped returning Content-Length header since switching to chunked transfer */
		/*  https://www.dropboxforum.com/t5/Discuss-Dropbox-Developer-API/Dropbox-media-can-t-be-access-by-azure-blob/td-p/575458 */
		Http_ParseContentLength(req, &value);
	} else if (String_CaselessEqualsConst(&name, "Last-Modified")) {
		String_CopyToRawArray(req->lastModified, &value);
	} else if (req->cookies && String_CaselessEqualsConst(&name, "Set-Cookie")) {
		Http_ParseCookie(req, &value);
	}
}

/* Adds a http header to the request headers. */
static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value);

/* Adds all the appropriate headers for a request. */
static void Http_SetRequestHeaders(struct HttpRequest* req) {
	static const cc_string contentType = String_FromConst("application/x-www-form-urlencoded");
	cc_string str, cookies; char cookiesBuffer[1024];
	int i;

	if (req->lastModified[0]) {
		str = String_FromRawArray(req->lastModified);
		Http_AddHeader(req, "If-Modified-Since", &str);
	}
	if (req->etag[0]) {
		str = String_FromRawArray(req->etag);
		Http_AddHeader(req, "If-None-Match", &str);
	}

	if (req->data) Http_AddHeader(req, "Content-Type", &contentType);
	if (!req->cookies || !req->cookies->count) return;

	String_InitArray(cookies, cookiesBuffer);
	for (i = 0; i < req->cookies->count; i++) {
		if (i) String_AppendConst(&cookies, "; ");
		str = StringsBuffer_UNSAFE_Get(req->cookies, i);
		String_AppendString(&cookies, &str);
	}
	Http_AddHeader(req, "Cookie", &cookies);
}

/* TODO: Rewrite to use a local variable instead */
static cc_string* Http_GetUserAgent_UNSAFE(void) {
	static char userAgentBuffer[STRING_SIZE];
	static cc_string userAgent;

	String_InitArray(userAgent, userAgentBuffer);
	String_AppendConst(&userAgent, GAME_APP_NAME);
	String_AppendConst(&userAgent, Platform_AppNameSuffix);
	return &userAgent;
}


/*########################################################################################################################*
*--------------------------------------------------Common downloader code-------------------------------------------------*
*#########################################################################################################################*/
static void* processedMutex;
static struct RequestList processedReqs;
static int nextReqID;
static void HttpBackend_Add(struct HttpRequest* req, cc_uint8 flags);

/* Adds a req to the list of pending requests, waking up worker thread if needed. */
static int Http_Add(const cc_string* url, cc_uint8 flags, cc_uint8 type, const cc_string* lastModified,
					const cc_string* etag, const void* data, cc_uint32 size, struct StringsBuffer* cookies) {
	static const cc_string https = String_FromConst("https://");
	static const cc_string http  = String_FromConst("http://");
	struct HttpRequest req = { 0 };

	String_CopyToRawArray(req.url, url);
	Platform_Log2("Adding %s (type %b)", url, &type);

	req.id = ++nextReqID;
	req.requestType = type;

	/* Change http:// to https:// if required */
	if (httpsOnly) {
		cc_string url_ = String_FromRawArray(req.url);
		if (String_CaselessStarts(&url_, &http)) String_InsertAt(&url_, 4, 's');
	}
	/* Change https:// to http:// if required */
	if (httpOnly) {
		cc_string url_ = String_FromRawArray(req.url);
		if (String_CaselessStarts(&url_, &https)) String_DeleteAt(&url_, 4);
	}
	
	if (lastModified) {
		String_CopyToRawArray(req.lastModified, lastModified);
	}
	if (etag) { 
		String_CopyToRawArray(req.etag, etag);
	}

	if (data) {
		req.data = (cc_uint8*)Mem_Alloc(size, 1, "Http_PostData");
		Mem_Copy(req.data, data, size);
		req.size = size;
	}
	req.cookies  = cookies;
	req.progress = HTTP_PROGRESS_NOT_WORKING_ON;

	HttpBackend_Add(&req, flags);
	return req.id;
}


/* Updates state after a completed http request */
static void Http_FinishRequest(struct HttpRequest* req) {
	req->success = !req->result && req->statusCode == 200 && req->data && req->size;

	if (!req->success) {
		char* error = req->error; req->error = NULL;
		HttpRequest_Free(req);
		req->error = error;
		/* TODO don't HttpRequest_Free here? */
	}

	Mutex_Lock(processedMutex);
	{
		req->timeDownloaded = Stopwatch_Measure();
		RequestList_Append(&processedReqs, req, false);
	}
	Mutex_Unlock(processedMutex);
}

/* Deletes cached responses that are over 10 seconds old */
static void Http_CleanCacheTask(struct ScheduledTask* task) {
	struct HttpRequest* item;
	int i;

	Mutex_Lock(processedMutex);
	{
		cc_uint64 now = Stopwatch_Measure();
		for (i = processedReqs.count - 1; i >= 0; i--) 
		{
			item = &processedReqs.entries[i];
			if (Stopwatch_ElapsedMS(item->timeDownloaded, now) < 10 * 1000) continue;

			Platform_Log1("Cleaning up forgotten download for %c", item->url);
			HttpRequest_Free(item);
			RequestList_RemoveAt(&processedReqs, i);
		}
	}
	Mutex_Unlock(processedMutex);
}


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
int Http_AsyncGetSkin(const cc_string* skinName, cc_uint8 flags) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName)) {
		String_Copy(&url, skinName);
	} else {
		String_Format2(&url, "%s/%s.png", &skinServer, skinName);
	}
	return Http_AsyncGetData(&url, flags);
}

int Http_AsyncGetData(const cc_string* url, cc_uint8 flags) {
	return Http_Add(url, flags, REQUEST_TYPE_GET, NULL, NULL, NULL, 0, NULL);
}
int Http_AsyncGetHeaders(const cc_string* url, cc_uint8 flags) {
	return Http_Add(url, flags, REQUEST_TYPE_HEAD, NULL, NULL, NULL, 0, NULL);
}
int Http_AsyncPostData(const cc_string* url, cc_uint8 flags, const void* data, cc_uint32 size, struct StringsBuffer* cookies) {
	return Http_Add(url, flags, REQUEST_TYPE_POST, NULL, NULL, data, size, cookies);
}
int Http_AsyncGetDataEx(const cc_string* url, cc_uint8 flags, const cc_string* lastModified, const cc_string* etag, struct StringsBuffer* cookies) {
	return Http_Add(url, flags, REQUEST_TYPE_GET, lastModified, etag, NULL, 0, cookies);
}

static cc_bool Http_UrlDirect(cc_uint8 c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
		|| c == '-' || c == '_' || c == '.' || c == '~';
}

void Http_UrlEncode(cc_string* dst, const cc_uint8* data, int len) {
	int i;
	for (i = 0; i < len; i++) {
		cc_uint8 c = data[i];

		if (Http_UrlDirect(c)) {
			String_Append(dst, c);
		} else {
			String_Append(dst,  '%');
			String_AppendHex(dst, c);
		}
	}
}

void Http_UrlEncodeUtf8(cc_string* dst, const cc_string* src) {
	cc_uint8 data[4];
	int i, len;

	for (i = 0; i < src->length; i++) {
		len = Convert_CP437ToUtf8(src->buffer[i], data);
		Http_UrlEncode(dst, data, len);
	}
}

/* Outputs more detailed information about errors with http requests */
static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst);

void Http_LogError(const char* action, const struct HttpRequest* item) {
	cc_string msg; char msgBuffer[512];
	String_InitArray(msg, msgBuffer);
	
	Logger_FormatWarn(&msg, item->result, action, HttpBackend_DescribeError);
	if (item->error && item->error[0]) {
		String_Format1(&msg, "\n  Error details: %c", item->error);
	}
	Logger_WarnFunc(&msg);
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_InitCommon(void) {
	httpOnly    = Options_GetBool(OPT_HTTP_ONLY,    false);
	httpsVerify = Options_GetBool(OPT_HTTPS_VERIFY, true);

	Options_Get(OPT_SKIN_SERVER, &skinServer, SKINS_SERVER);
	ScheduledTask_Add(30, Http_CleanCacheTask);
}
static void Http_Init(void);

struct IGameComponent Http_Component = {
	Http_Init,        /* Init  */
	Http_ClearPending,/* Free  */
	Http_ClearPending /* Reset */
};
