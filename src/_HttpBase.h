#include "Http.h"
#include "String.h"
#include "Platform.h"
#include "Funcs.h"
#include "Logger.h"
#include "Stream.h"
#include "Game.h"
#include "Utils.h"
#include "Options.h"

static cc_bool httpsOnly, httpOnly;
/* Frees data from a HTTP request. */
static void HttpRequest_Free(struct HttpRequest* request) {
	Mem_Free(request->data);
	request->data = NULL;
	request->size = 0;
}

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

/* Adds another request to end (for normal priority request) */
static void RequestList_Append(struct RequestList* list, struct HttpRequest* item) {
	RequestList_EnsureSpace(list);
	list->entries[list->count++] = *item;
}

/* Inserts a request at start (for high priority request) */
static void RequestList_Prepend(struct RequestList* list, struct HttpRequest* item) {
	int i;
	RequestList_EnsureSpace(list);
	
	for (i = list->count; i > 0; i--) {
		list->entries[i] = list->entries[i - 1];
	}
	list->entries[0] = *item;
	list->count++;
}

/* Removes the request at the given index */
static void RequestList_RemoveAt(struct RequestList* list, int i) {
	if (i < 0 || i >= list->count) Logger_Abort("Tried to remove element at list end");

	for (; i < list->count - 1; i++) {
		list->entries[i] = list->entries[i + 1];
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

	Mem_Free(list->entries[i].data);
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
*--------------------------------------------------Common downloader code-------------------------------------------------*
*#########################################################################################################################*/
static void* pendingMutex;
static void* processedMutex;
static void* curRequestMutex;
static volatile cc_bool http_terminate;

static struct RequestList pendingReqs;
static struct RequestList processedReqs;
static struct HttpRequest http_curRequest;
static volatile int http_curProgress = HTTP_PROGRESS_NOT_WORKING_ON;
static int nextReqID;

static void Http_WorkerInit(void);
static void Http_WorkerStart(void);
static void Http_WorkerSignal(void);
static void Http_WorkerStop(void);

/* Adds a req to the list of pending requests, waking up worker thread if needed. */
static int Http_Add(const cc_string* url, cc_bool priority, cc_uint8 type, const cc_string* lastModified,
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
	req.cookies = cookies;

	Mutex_Lock(pendingMutex);
	{	
		if (priority) {
			RequestList_Prepend(&pendingReqs, &req);
		} else {
			RequestList_Append(&pendingReqs,  &req);
		}
	}
	Mutex_Unlock(pendingMutex);
	Http_WorkerSignal();
	return req.id;
}

static const cc_string urlRewrites[4] = {
	String_FromConst("http://dl.dropbox.com/"),  String_FromConst("https://dl.dropboxusercontent.com/"),
	String_FromConst("https://dl.dropbox.com/"), String_FromConst("https://dl.dropboxusercontent.com/")
};
/* Converts say dl.dropbox.com/xyZ into dl.dropboxusercontent.com/xyz */
static void Http_GetUrl(struct HttpRequest* req, cc_string* dst) {
	cc_string url = String_FromRawArray(req->url);
	cc_string part;
	int i;

	for (i = 0; i < Array_Elems(urlRewrites); i += 2) {
		if (!String_CaselessStarts(&url, &urlRewrites[i])) continue;

		part = String_UNSAFE_SubstringAt(&url, urlRewrites[i].length);
		String_Format2(dst, "%s%s", &urlRewrites[i + 1], &part);
		return;
	}
	String_Copy(dst, &url);
}


/* Sets up state to begin a http request */
static void Http_BeginRequest(struct HttpRequest* req, cc_string* url) {
	Http_GetUrl(req, url);
	Platform_Log2("Fetching %s (type %b)", url, &req->requestType);

	Mutex_Lock(curRequestMutex);
	{
		http_curRequest  = *req;
		http_curProgress = HTTP_PROGRESS_MAKING_REQUEST;
	}
	Mutex_Unlock(curRequestMutex);
}

/* Updates state after a completed http request */
static void Http_FinishRequest(struct HttpRequest* req) {
	req->success = !req->result && req->statusCode == 200 && req->data && req->size;
	if (!req->success) HttpRequest_Free(req);

	Mutex_Lock(processedMutex);
	{
		req->timeDownloaded = DateTime_CurrentUTC_MS();
		RequestList_Append(&processedReqs, req);
	}
	Mutex_Unlock(processedMutex);

	Mutex_Lock(curRequestMutex);
	{
		http_curRequest.id = 0;
		http_curProgress   = HTTP_PROGRESS_NOT_WORKING_ON;
	}
	Mutex_Unlock(curRequestMutex);
}

/* Deletes cached responses that are over 10 seconds old */
static void Http_CleanCacheTask(struct ScheduledTask* task) {
	struct HttpRequest* item;
	int i;

	Mutex_Lock(processedMutex);
	{
		TimeMS now = DateTime_CurrentUTC_MS();
		for (i = processedReqs.count - 1; i >= 0; i--) {
			item = &processedReqs.entries[i];
			if (item->timeDownloaded + (10 * 1000) >= now) continue;

			Mem_Free(item->data);
			RequestList_RemoveAt(&processedReqs, i);
		}
	}
	Mutex_Unlock(processedMutex);
}

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
		Convert_ParseInt(&value, &req->contentLength);
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


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
int Http_AsyncGetSkin(const cc_string* skinName) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName)) {
		String_Copy(&url, skinName);
	} else {
		String_Format1(&url, SKINS_SERVER "/%s.png", skinName);
	}
	return Http_AsyncGetData(&url, false);
}

int Http_AsyncGetData(const cc_string* url, cc_bool priority) {
	return Http_Add(url, priority, REQUEST_TYPE_GET, NULL, NULL, NULL, 0, NULL);
}
int Http_AsyncGetHeaders(const cc_string* url, cc_bool priority) {
	return Http_Add(url, priority, REQUEST_TYPE_HEAD, NULL, NULL, NULL, 0, NULL);
}
int Http_AsyncPostData(const cc_string* url, cc_bool priority, const void* data, cc_uint32 size, struct StringsBuffer* cookies) {
	return Http_Add(url, priority, REQUEST_TYPE_POST, NULL, NULL, data, size, cookies);
}
int Http_AsyncGetDataEx(const cc_string* url, cc_bool priority, const cc_string* lastModified, const cc_string* etag, struct StringsBuffer* cookies) {
	return Http_Add(url, priority, REQUEST_TYPE_GET, lastModified, etag, NULL, 0, cookies);
}

cc_bool Http_GetResult(int reqID, struct HttpRequest* item) {
	int i;
	Mutex_Lock(processedMutex);
	{
		i = RequestList_Find(&processedReqs, reqID);
		if (i >= 0) *item = processedReqs.entries[i];
		if (i >= 0) RequestList_RemoveAt(&processedReqs, i);
	}
	Mutex_Unlock(processedMutex);
	return i >= 0;
}

cc_bool Http_GetCurrent(int* reqID, int* progress) {
	Mutex_Lock(curRequestMutex);
	{
		*reqID    = http_curRequest.id;
		*progress = http_curProgress;
	}
	Mutex_Unlock(curRequestMutex);
	return *reqID != 0;
}

int Http_CheckProgress(int reqID) {
	int curReqID, progress;
	Http_GetCurrent(&curReqID, &progress);

	if (reqID != curReqID) progress = HTTP_PROGRESS_NOT_WORKING_ON;
	return progress;
}

void Http_ClearPending(void) {
	Mutex_Lock(pendingMutex);
	{
		RequestList_Free(&pendingReqs);
	}
	Mutex_Unlock(pendingMutex);
	Http_WorkerSignal();
}

void Http_TryCancel(int reqID) {
	Mutex_Lock(pendingMutex);
	{
		RequestList_TryFree(&pendingReqs, reqID);
	}
	Mutex_Unlock(pendingMutex);

	Mutex_Lock(processedMutex);
	{
		RequestList_TryFree(&processedReqs, reqID);
	}
	Mutex_Unlock(processedMutex);
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


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void OnInit(void) {
	http_terminate = false;
	httpOnly = Options_GetBool(OPT_HTTP_ONLY, false);

	Http_WorkerInit();
	ScheduledTask_Add(30, Http_CleanCacheTask);
	RequestList_Init(&pendingReqs);
	RequestList_Init(&processedReqs);

	pendingMutex    = Mutex_Create();
	processedMutex  = Mutex_Create();
	curRequestMutex = Mutex_Create();
	Http_WorkerStart();
}

static void OnFree(void) {
	http_terminate = true;
	Http_ClearPending();
	Http_WorkerStop();

	RequestList_Free(&pendingReqs);
	RequestList_Free(&processedReqs);

	Mutex_Free(pendingMutex);
	Mutex_Free(processedMutex);
	Mutex_Free(curRequestMutex);
}

struct IGameComponent Http_Component = {
	OnInit,           /* Init  */
	OnFree,           /* Free  */
	Http_ClearPending /* Reset */
};
