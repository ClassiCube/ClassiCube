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
*------------------------------------------------Emscripten implementation------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WEB
#include <emscripten/emscripten.h>
#include "Errors.h"
extern void interop_DownloadAsync(const char* url, int method);
extern int interop_IsHttpsOnly(void);

cc_bool Http_DescribeError(cc_result res, cc_string* dst) { return false; }
/* web browsers do caching already, so don't need last modified/etags */
static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) { }

EMSCRIPTEN_KEEPALIVE void Http_OnUpdateProgress(int read, int total) {
	if (!total) return;
	http_curProgress = (int)(100.0f * read / total);
}

EMSCRIPTEN_KEEPALIVE void Http_OnFinishedAsync(void* data, int len, int status) {
	struct HttpRequest* req = &http_curRequest;
	req->data          = data;
	req->size          = len;
	req->statusCode    = status;
	req->contentLength = len;

	/* Usually because of denied by CORS */
	if (!status && !data) req->result = ERR_DOWNLOAD_INVALID;

	if (req->data) Platform_Log1("HTTP returned data: %i bytes", &req->size);
	Http_FinishRequest(req);
	Http_WorkerSignal();
}

static void Http_DownloadAsync(struct HttpRequest* req) {
	char urlBuffer[URL_MAX_SIZE]; cc_string url;
	char urlStr[NATIVE_STR_LEN];

	String_InitArray(url, urlBuffer);
	Http_BeginRequest(req, &url);
	Platform_EncodeUtf8(urlStr, &url);
	interop_DownloadAsync(urlStr, req->requestType);
}

static void Http_WorkerInit(void) {
	/* If this webpage is https://, browsers deny any http:// downloading */
	httpsOnly = interop_IsHttpsOnly();
}
static void Http_WorkerStart(void) { }
static void Http_WorkerStop(void)  { }

static void Http_WorkerSignal(void) {
	struct HttpRequest req;
	if (http_terminate || !pendingReqs.count) return;
	/* already working on a request currently */
	if (http_curRequest.id) return;

	req = pendingReqs.entries[0];
	RequestList_RemoveAt(&pendingReqs, 0);
	Http_DownloadAsync(&req);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------Worker implementation--------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_WEB
static void* workerWaitable;
static void* workerThread;

/* Allocates initial data buffer to store response contents */
static void Http_BufferInit(struct HttpRequest* req) {
	http_curProgress = 0;
	req->_capacity = req->contentLength ? req->contentLength : 1;
	req->data      = (cc_uint8*)Mem_Alloc(req->_capacity, 1, "http data");
	req->size      = 0;
}

/* Ensures data buffer has enough space left to append amount bytes, reallocates if not */
static void Http_BufferEnsure(struct HttpRequest* req, cc_uint32 amount) {
	cc_uint32 newSize = req->size + amount;
	if (newSize <= req->_capacity) return;

	req->_capacity = newSize;
	req->data      = (cc_uint8*)Mem_Realloc(req->data, newSize, 1, "http data+");
}

/* Increases size and updates current progress */
static void Http_BufferExpanded(struct HttpRequest* req, cc_uint32 read) {
	req->size += read;
	if (req->contentLength) http_curProgress = (int)(100.0f * req->size / req->contentLength);
}

#if defined CC_BUILD_CURL
/*########################################################################################################################*
*-----------------------------------------------------libcurl backend-----------------------------------------------------*
*#########################################################################################################################*/
#include "Errors.h"
#include <stddef.h>
/* === BEGIN CURL HEADERS === */
typedef void CURL;
struct curl_slist;
typedef int CURLcode;

#define CURL_GLOBAL_DEFAULT ((1<<0) | (1<<1)) /* SSL and Win32 options */
#define CURLOPT_WRITEDATA      (10000 + 1)
#define CURLOPT_URL            (10000 + 2)
#define CURLOPT_ERRORBUFFER    (10000 + 10)
#define CURLOPT_WRITEFUNCTION  (20000 + 11)
#define CURLOPT_POSTFIELDS     (10000 + 15)
#define CURLOPT_USERAGENT      (10000 + 18)
#define CURLOPT_HTTPHEADER     (10000 + 23)
#define CURLOPT_HEADERDATA     (10000 + 29)
#define CURLOPT_VERBOSE        (0     + 41)
#define CURLOPT_HEADER         (0     + 42)
#define CURLOPT_NOBODY         (0     + 44)
#define CURLOPT_POST           (0     + 47)
#define CURLOPT_FOLLOWLOCATION (0     + 52)
#define CURLOPT_POSTFIELDSIZE  (0     + 60)
#define CURLOPT_MAXREDIRS      (0     + 68)
#define CURLOPT_HEADERFUNCTION (20000 + 79)
#define CURLOPT_HTTPGET        (0     + 80)

#if defined _WIN32
#define APIENTRY __cdecl
#else
#define APIENTRY
#endif

static CURLcode (APIENTRY *_curl_global_init)(long flags);
static void     (APIENTRY *_curl_global_cleanup)(void);
static CURL*    (APIENTRY *_curl_easy_init)(void);
static CURLcode (APIENTRY *_curl_easy_perform)(CURL *c);
static CURLcode (APIENTRY *_curl_easy_setopt)(CURL *c, int opt, ...);
static void     (APIENTRY *_curl_easy_cleanup)(CURL* c);
static void     (APIENTRY *_curl_slist_free_all)(struct curl_slist* l);
static struct curl_slist* (APIENTRY *_curl_slist_append)(struct curl_slist* l, const char* v);
static const char* (APIENTRY *_curl_easy_strerror)(CURLcode res);
/* === END CURL HEADERS === */

#if defined CC_BUILD_WIN
static const cc_string curlLib = String_FromConst("libcurl.dll");
static const cc_string curlAlt = String_FromConst("curl.dll");
#elif defined CC_BUILD_DARWIN
static const cc_string curlLib = String_FromConst("/usr/lib/libcurl.4.dylib");
static const cc_string curlAlt = String_FromConst("/usr/lib/libcurl.dylib");
#elif defined CC_BUILD_BSD
static const cc_string curlLib = String_FromConst("libcurl.so");
static const cc_string curlAlt = String_FromConst("libcurl.so");
#else
static const cc_string curlLib = String_FromConst("libcurl.so.4");
static const cc_string curlAlt = String_FromConst("libcurl.so.3");
#endif

static cc_bool LoadCurlFuncs(void) {
	static const struct DynamicLibSym funcs[8] = {
		DynamicLib_Sym(curl_global_init),    DynamicLib_Sym(curl_global_cleanup),
		DynamicLib_Sym(curl_easy_init),      DynamicLib_Sym(curl_easy_perform),
		DynamicLib_Sym(curl_easy_setopt),    DynamicLib_Sym(curl_easy_cleanup),
		DynamicLib_Sym(curl_slist_free_all), DynamicLib_Sym(curl_slist_append)
	};
	/* Non-essential function missing in older curl versions */
	static const struct DynamicLibSym optFuncs[1] = { DynamicLib_Sym(curl_easy_strerror) };

	void* lib = DynamicLib_Load2(&curlLib);
	if (!lib) { 
		Logger_DynamicLibWarn("loading", &curlLib);

		lib = DynamicLib_Load2(&curlAlt);
		if (!lib) { Logger_DynamicLibWarn("loading", &curlAlt); return false; }
	}

	DynamicLib_GetAll(lib, optFuncs, Array_Elems(optFuncs));
	return DynamicLib_GetAll(lib, funcs, Array_Elems(funcs));
}

static CURL* curl;
static cc_bool curlSupported;

cc_bool Http_DescribeError(cc_result res, cc_string* dst) {
	const char* err;
	
	if (!_curl_easy_strerror) return false;
	err = _curl_easy_strerror((CURLcode)res);
	if (!err) return false;

	String_AppendConst(dst, err);
	return true;
}

static void Http_BackendInit(void) {
	static const cc_string msg = String_FromConst("Failed to init libcurl. All HTTP requests will therefore fail.");
	CURLcode res;

	if (!LoadCurlFuncs()) { Logger_WarnFunc(&msg); return; }
	res = _curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) { Logger_SimpleWarn(res, "initing curl"); return; }

	curl = _curl_easy_init();
	if (!curl) { Logger_SimpleWarn(res, "initing curl_easy"); return; }
	curlSupported = true;
}

static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) {
	cc_string tmp; char tmpBuffer[1024];
	String_InitArray_NT(tmp, tmpBuffer);
	String_Format2(&tmp, "%c: %s", key, value);

	tmp.buffer[tmp.length] = '\0';
	req->meta = _curl_slist_append((struct curl_slist*)req->meta, tmp.buffer);
}

/* Processes a HTTP header downloaded from the server */
static size_t Http_ProcessHeader(char* buffer, size_t size, size_t nitems, void* userdata) {
	struct HttpRequest* req = (struct HttpRequest*)userdata;
	size_t len = nitems;
	cc_string line;
	/* line usually has \r\n at end */
	if (len && buffer[len - 1] == '\n') len--;
	if (len && buffer[len - 1] == '\r') len--;

	line = String_Init(buffer, len, len);
	Http_ParseHeader(req, &line);
	return nitems;
}

/* Processes a chunk of data downloaded from the web server */
static size_t Http_ProcessData(char *buffer, size_t size, size_t nitems, void* userdata) {
	struct HttpRequest* req = (struct HttpRequest*)userdata;

	if (!req->_capacity) Http_BufferInit(req);
	Http_BufferEnsure(req, nitems);

	Mem_Copy(&req->data[req->size], buffer, nitems);
	Http_BufferExpanded(req, nitems);
	return nitems;
}

/* Sets general curl options for a request */
static void Http_SetCurlOpts(struct HttpRequest* req) {
	_curl_easy_setopt(curl, CURLOPT_USERAGENT,      GAME_APP_NAME);
	_curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	_curl_easy_setopt(curl, CURLOPT_MAXREDIRS,      20L);

	_curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_ProcessHeader);
	_curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_ProcessData);
	_curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);
}

static cc_result Http_BackendDo(struct HttpRequest* req, cc_string* url) {
	char urlStr[NATIVE_STR_LEN];
	void* post_data = req->data;
	CURLcode res;
	if (!curlSupported) return ERR_NOT_SUPPORTED;

	req->meta = NULL;
	Http_SetRequestHeaders(req);
	_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req->meta);

	Http_SetCurlOpts(req);
	Platform_EncodeUtf8(urlStr, url);
	_curl_easy_setopt(curl, CURLOPT_URL, urlStr);

	if (req->requestType == REQUEST_TYPE_HEAD) {
		_curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	} else if (req->requestType == REQUEST_TYPE_POST) {
		_curl_easy_setopt(curl, CURLOPT_POST,   1L);
		_curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->size);
		_curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    req->data);

		/* per curl docs, we must persist POST data until request finishes */
		req->data = NULL;
		req->size = 0;
	} else {
		/* Undo POST/HEAD state */
		_curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	}

	req->_capacity   = 0;
	http_curProgress = HTTP_PROGRESS_FETCHING_DATA;
	res = _curl_easy_perform(curl);
	http_curProgress = 100;

	_curl_slist_free_all((struct curl_slist*)req->meta);
	/* can free now that request has finished */
	Mem_Free(post_data);
	return res;
}

static void Http_BackendFree(void) {
	if (!curlSupported) return;
	_curl_easy_cleanup(curl);
	_curl_global_cleanup();
}
#elif defined CC_BUILD_WININET
/*########################################################################################################################*
*-----------------------------------------------------WinINet backend-----------------------------------------------------*
*#########################################################################################################################*/
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <wininet.h>
static HINTERNET hInternet;

/* caches connections to web servers */
struct HttpCacheEntry {
	HINTERNET Handle;  /* Native connection handle. */
	cc_string Address; /* Address of server. (e.g. "classicube.net") */
	cc_uint16 Port;    /* Port server is listening on. (e.g 80) */
	cc_bool Https;     /* Whether HTTPS or just HTTP protocol. */
	char _addressBuffer[STRING_SIZE + 1];
};
#define HTTP_CACHE_ENTRIES 10
static struct HttpCacheEntry http_cache[HTTP_CACHE_ENTRIES];

/* Converts characters to UTF8, then calls Http_URlEncode on them. */
static void HttpCache_UrlEncodeUrl(cc_string* dst, const cc_string* src) {
	cc_uint8 data[4];
	int i, len;
	char c;

	for (i = 0; i < src->length; i++) {
		c   = src->buffer[i];
		len = Convert_CP437ToUtf8(c, data);

		/* URL path/query must not be URL encoded (it normally would be) */
		if (c == '/' || c == '?' || c == '=') {
			String_Append(dst, c);
		} else {
			Http_UrlEncode(dst, data, len);
		}
	}
}

/* Splits up the components of a URL */
static void HttpCache_MakeEntry(const cc_string* url, struct HttpCacheEntry* entry, cc_string* resource) {
	cc_string scheme, path, addr, name, port, _resource;
	/* URL is of form [scheme]://[server name]:[server port]/[resource] */
	int idx = String_IndexOfConst(url, "://");

	scheme = idx == -1 ? String_Empty : String_UNSAFE_Substring(url,   0, idx);
	path   = idx == -1 ? *url         : String_UNSAFE_SubstringAt(url, idx + 3);
	entry->Https = String_CaselessEqualsConst(&scheme, "https");

	String_UNSAFE_Separate(&path, '/', &addr, &_resource);
	String_UNSAFE_Separate(&addr, ':', &name, &port);
	/* Address may have unicode characters - need to percent encode them */
	HttpCache_UrlEncodeUrl(resource, &_resource);

	String_InitArray_NT(entry->Address, entry->_addressBuffer);
	String_Copy(&entry->Address, &name);
	entry->Address.buffer[entry->Address.length] = '\0';

	if (!Convert_ParseUInt16(&port, &entry->Port)) {
		entry->Port = entry->Https ? 443 : 80;
	}
}

/* Inserts entry into the cache at the given index */
static cc_result HttpCache_Insert(int i, struct HttpCacheEntry* e) {
	HINTERNET conn;
	conn = InternetConnectA(hInternet, e->Address.buffer, e->Port, NULL, NULL, 
				INTERNET_SERVICE_HTTP, e->Https ? INTERNET_FLAG_SECURE : 0, 0);
	if (!conn) return GetLastError();

	e->Handle     = conn;
	http_cache[i] = *e;

	/* otherwise address buffer points to stack buffer */
	http_cache[i].Address.buffer = http_cache[i]._addressBuffer;
	return 0;
}

/* Finds or inserts the given entry into the cache */
static cc_result HttpCache_Lookup(struct HttpCacheEntry* e) {
	struct HttpCacheEntry* c;
	int i;

	for (i = 0; i < HTTP_CACHE_ENTRIES; i++) {
		c = &http_cache[i];
		if (c->Https == e->Https && String_Equals(&c->Address, &e->Address) && c->Port == e->Port) {
			e->Handle = c->Handle;
			return 0;
		}
	}

	for (i = 0; i < HTTP_CACHE_ENTRIES; i++) {
		if (http_cache[i].Handle) continue;
		return HttpCache_Insert(i, e);
	}

	/* TODO: Should we be consistent in which entry gets evicted? */
	i = (cc_uint8)Stopwatch_Measure() % HTTP_CACHE_ENTRIES;
	InternetCloseHandle(http_cache[i].Handle);
	return HttpCache_Insert(i, e);
}

cc_bool Http_DescribeError(cc_result res, cc_string* dst) {
	return Platform_DescribeErrorExt(res, dst, GetModuleHandle(TEXT("wininet.dll")));
}

static void Http_BackendInit(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(GAME_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) Logger_Abort2(GetLastError(), "Failed to init WinINet");
}

static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) {
	cc_string tmp; char tmpBuffer[1024];
	String_InitArray(tmp, tmpBuffer);

	String_Format2(&tmp, "%c: %s\r\n", key, value);
	HttpAddRequestHeadersA((HINTERNET)req->meta, tmp.buffer, tmp.length, 
							HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
}

/* Creates and sends a HTTP request */
static cc_result Http_StartRequest(struct HttpRequest* req, cc_string* url, HINTERNET* handle) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	struct HttpCacheEntry entry;
	cc_string path; char pathBuffer[URL_MAX_SIZE + 1];
	DWORD flags, bufferLen;
	cc_result res;

	String_InitArray_NT(path, pathBuffer);
	HttpCache_MakeEntry(url, &entry, &path);
	pathBuffer[path.length] = '\0';
	if ((res = HttpCache_Lookup(&entry))) return res;

	flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES;
	if (entry.Https) flags |= INTERNET_FLAG_SECURE;

	*handle = HttpOpenRequestA(entry.Handle, verbs[req->requestType], 
								pathBuffer, NULL, NULL, NULL, flags, 0);
	req->meta = *handle;
	if (!req->meta) return GetLastError();

	/* ignore revocation stuff */
	bufferLen = sizeof(flags);
	InternetQueryOption(*handle, INTERNET_OPTION_SECURITY_FLAGS, (void*)&bufferLen, &flags);
	flags |= SECURITY_FLAG_IGNORE_REVOCATION;
	InternetSetOption(*handle, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

	Http_SetRequestHeaders(req);
	return HttpSendRequestA(*handle, NULL, 0, req->data, req->size) ? 0 : GetLastError();
}

/* Gets headers from a HTTP response */
static cc_result Http_ProcessHeaders(struct HttpRequest* req, HINTERNET handle) {
	cc_string left, line;
	char buffer[8192];
	DWORD len = 8192;
	if (!HttpQueryInfoA(handle, HTTP_QUERY_RAW_HEADERS, buffer, &len, NULL)) return GetLastError();

	left = String_Init(buffer, len, len);
	while (left.length) {
		String_UNSAFE_SplitBy(&left, '\0', &line);
		Http_ParseHeader(req, &line);
	}
	return 0;
}

/* Downloads the data/contents of a HTTP response */
static cc_result Http_DownloadData(struct HttpRequest* req, HINTERNET handle) {
	DWORD read, avail;
	Http_BufferInit(req);

	for (;;) {
		if (!InternetQueryDataAvailable(handle, &avail, 0, 0)) return GetLastError();
		if (!avail) break;
		Http_BufferEnsure(req, avail);

		if (!InternetReadFile(handle, &req->data[req->size], avail, &read)) return GetLastError();
		if (!read) break;
		Http_BufferExpanded(req, read);
	}

 	http_curProgress = 100;
	return 0;
}

static cc_result Http_BackendDo(struct HttpRequest* req, cc_string* url) {
	HINTERNET handle;
	cc_result res = Http_StartRequest(req, url, &handle);
	HttpRequest_Free(req);
	if (res) return res;

	http_curProgress = HTTP_PROGRESS_FETCHING_DATA;
	res = Http_ProcessHeaders(req, handle);
	if (res) { InternetCloseHandle(handle); return res; }

	if (req->requestType != REQUEST_TYPE_HEAD) {
		res = Http_DownloadData(req, handle);
		if (res) { InternetCloseHandle(handle); return res; }
	}

	return InternetCloseHandle(handle) ? 0 : GetLastError();
}

static void Http_BackendFree(void) {
	int i;
	for (i = 0; i < HTTP_CACHE_ENTRIES; i++) {
		if (!http_cache[i].Handle) continue;
		InternetCloseHandle(http_cache[i].Handle);
	}
	InternetCloseHandle(hInternet);
}
#elif defined CC_BUILD_ANDROID
/*########################################################################################################################*
*-----------------------------------------------------Android backend-----------------------------------------------------*
*#########################################################################################################################*/
struct HttpRequest* java_req;

cc_bool Http_DescribeError(cc_result res, cc_string* dst) {
	char buffer[NATIVE_STR_LEN];
	cc_string err;
	JNIEnv* env;
	jvalue args[1];
	jobject obj;
	
	JavaGetCurrentEnv(env);
	args[0].i = res;
	obj       = JavaCallObject(env, "httpDescribeError", "(I)Ljava/lang/String;", args);
	if (!obj) return false;

	err = JavaGetString(env, obj, buffer);
	String_AppendString(dst, &err);
	(*env)->DeleteLocalRef(env, obj);
	return true;
}

static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) {
	JNIEnv* env;
	jvalue args[2];

	JavaGetCurrentEnv(env);
	args[0].l = JavaMakeConst(env,  key);
	args[1].l = JavaMakeString(env, value);

	JavaCallVoid(env, "httpSetHeader", "(Ljava/lang/String;Ljava/lang/String;)V", args);
	(*env)->DeleteLocalRef(env, args[0].l);
	(*env)->DeleteLocalRef(env, args[1].l);
}

/* Processes a HTTP header downloaded from the server */
static void JNICALL java_HttpParseHeader(JNIEnv* env, jobject o, jstring header) {
	char buffer[NATIVE_STR_LEN];
	cc_string line = JavaGetString(env, header, buffer);
	Http_ParseHeader(java_req, &line);
}

/* Processes a chunk of data downloaded from the web server */
static void JNICALL java_HttpAppendData(JNIEnv* env, jobject o, jbyteArray arr, jint len) {
	struct HttpRequest* req = java_req;
	if (!req->_capacity) Http_BufferInit(req);

	Http_BufferEnsure(req, len);
	(*env)->GetByteArrayRegion(env, arr, 0, len, (jbyte*)(&req->data[req->size]));
	Http_BufferExpanded(req, len);
}

static const JNINativeMethod methods[2] = {
	{ "httpParseHeader", "(Ljava/lang/String;)V", java_HttpParseHeader },
	{ "httpAppendData",  "([BI)V",                java_HttpAppendData }
};
static void Http_BackendInit(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaRegisterNatives(env, methods);
}

static cc_result Http_InitReq(JNIEnv* env, struct HttpRequest* req, cc_string* url) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	jvalue args[2];
	jint res;

	args[0].l = JavaMakeString(env, url);
	args[1].l = JavaMakeConst(env,  verbs[req->requestType]);

	res = JavaCallInt(env, "httpInit", "(Ljava/lang/String;Ljava/lang/String;)I", args);
	(*env)->DeleteLocalRef(env, args[0].l);
	(*env)->DeleteLocalRef(env, args[1].l);
	return res;
}

static cc_result Http_SetData(JNIEnv* env, struct HttpRequest* req) {
	jvalue args[1];
	jint res;

	args[0].l = JavaMakeBytes(env, req->data, req->size);
	res = JavaCallInt(env, "httpSetData", "([B)I", args);
	(*env)->DeleteLocalRef(env, args[0].l);
	return res;
}

static cc_result Http_BackendDo(struct HttpRequest* req, cc_string* url) {
	static const cc_string userAgent = String_FromConst(GAME_APP_NAME);
	JNIEnv* env;
	jint res;

	JavaGetCurrentEnv(env);
	if ((res = Http_InitReq(env, req, url))) return res;
	java_req = req;

	Http_SetRequestHeaders(req);
	Http_AddHeader(req, "User-Agent", &userAgent);
	if (req->data && (res = Http_SetData(env, req))) return res;

	req->_capacity   = 0;
	http_curProgress = HTTP_PROGRESS_FETCHING_DATA;
	res = JavaCallInt(env, "httpPerform", "()I", NULL);
	http_curProgress = 100;
	return res;
}

static void Http_BackendFree(void) { }
#endif

static void WorkerLoop(void) {
	char urlBuffer[URL_MAX_SIZE]; cc_string url;
	struct HttpRequest request;
	cc_bool hasRequest, stop;
	cc_uint64 beg, end;
	int elapsed;

	for (;;) {
		hasRequest = false;

		Mutex_Lock(pendingMutex);
		{
			stop = http_terminate;
			if (!stop && pendingReqs.count) {
				request = pendingReqs.entries[0];
				hasRequest = true;
				RequestList_RemoveAt(&pendingReqs, 0);
			}
		}
		Mutex_Unlock(pendingMutex);

		if (stop) return;
		/* Block until another thread submits a request to do */
		if (!hasRequest) {
			Platform_LogConst("Going back to sleep...");
			Waitable_Wait(workerWaitable);
			continue;
		}

		String_InitArray(url, urlBuffer);
		Http_BeginRequest(&request, &url);

		beg = Stopwatch_Measure();
		request.result = Http_BackendDo(&request, &url);
		end = Stopwatch_Measure();

		elapsed = Stopwatch_ElapsedMS(beg, end);
		Platform_Log4("HTTP: result %i (http %i) in %i ms (%i bytes)",
					&request.result, &request.statusCode, &elapsed, &request.size);
		Http_FinishRequest(&request);
	}
}

static void Http_WorkerInit(void) {
	Http_BackendInit();
	workerWaitable = Waitable_Create();
}

static void Http_WorkerStart(void) {
	workerThread = Thread_Start(WorkerLoop);
}
static void Http_WorkerSignal(void) { Waitable_Signal(workerWaitable); }

static void Http_WorkerStop(void) { 
	Thread_Join(workerThread);
	Waitable_Free(workerWaitable);
	Http_BackendFree();
}
#endif


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
