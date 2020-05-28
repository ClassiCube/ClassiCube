#include "Http.h"
#include "Platform.h"
#include "Funcs.h"
#include "Logger.h"
#include "Stream.h"
#include "GameStructs.h"

#if defined CC_BUILD_WININET
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#ifdef UNICODE
#define Platform_DecodeString(dst, src, len) String_AppendUtf16(dst, (Codepoint*)(src), (len) * 2)
#else
#define Platform_DecodeString(dst, src, len) String_DecodeCP1252(dst, (cc_uint8*)(src), len)
#endif

#include <windows.h>
#include <wininet.h>
#elif defined CC_BUILD_WEB
/* Use fetch/XMLHttpRequest api for Emscripten */
#include <emscripten/fetch.h>
#endif

void HttpRequest_Free(struct HttpRequest* request) {
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
static int RequestList_Find(struct RequestList* list, const String* id, struct HttpRequest* item) {
	String reqID;
	int i;

	for (i = 0; i < list->count; i++) {
		reqID = String_FromRawArray(list->entries[i].id);
		if (!String_Equals(id, &reqID)) continue;

		*item = list->entries[i];
		return i;
	}
	return -1;
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
static void* workerWaitable;
static void* workerThread;
static void* pendingMutex;
static void* processedMutex;
static void* curRequestMutex;
static volatile cc_bool http_terminate;

static struct RequestList pendingReqs;
static struct RequestList processedReqs;
static struct HttpRequest http_curRequest;
static volatile int http_curProgress = ASYNC_PROGRESS_NOTHING;

#ifdef CC_BUILD_WEB
static void Http_DownloadNextAsync(void);
#endif

/* Adds a req to the list of pending requests, waking up worker thread if needed. */
static void Http_Add(const String* url, cc_bool priority, const String* id, cc_uint8 type, const String* lastModified, const String* etag, const void* data, cc_uint32 size, struct EntryList* cookies) {
	struct HttpRequest req = { 0 };

	String_CopyToRawArray(req.url, url);
	Platform_Log2("Adding %s (type %b)", url, &type);

	String_CopyToRawArray(req.id, id);
	req.requestType = type;
	
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
		req.timeAdded = DateTime_CurrentUTC_MS();
		if (priority) {
			RequestList_Prepend(&pendingReqs, &req);
		} else {
			RequestList_Append(&pendingReqs,  &req);
		}
	}
	Mutex_Unlock(pendingMutex);

#ifndef CC_BUILD_WEB
	Waitable_Signal(workerWaitable);
#else
	Http_DownloadNextAsync();
#endif
}

static const String urlRewrites[4] = {
	String_FromConst("http://dl.dropbox.com/"),  String_FromConst("https://dl.dropboxusercontent.com/"),
	String_FromConst("https://dl.dropbox.com/"), String_FromConst("https://dl.dropboxusercontent.com/")
};
/* Converts say dl.dropbox.com/xyZ into dl.dropboxusercontent.com/xyz */
static void Http_GetUrl(struct HttpRequest* req, String* dst) {
	String url = String_FromRawArray(req->url);
	String part;
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
static void Http_BeginRequest(struct HttpRequest* req, String* url) {
	Http_GetUrl(req, url);
	Platform_Log2("Downloading from %s (type %b)", url, &req->requestType);

	Mutex_Lock(curRequestMutex);
	{
		http_curRequest  = *req;
		http_curProgress = ASYNC_PROGRESS_MAKING_REQUEST;
	}
	Mutex_Unlock(curRequestMutex);
}

/* Adds given request to list of processed/completed requests */
static void Http_CompleteRequest(struct HttpRequest* req) {
	struct HttpRequest older;
	String id = String_FromRawArray(req->id);
	int index;
	req->timeDownloaded = DateTime_CurrentUTC_MS();

	index = RequestList_Find(&processedReqs, &id, &older);
	if (index >= 0) {
		/* very rare case - priority item was inserted, then inserted again (so put before first item), */
		/* and both items got downloaded before an external function removed them from the queue */
		if (older.timeAdded > req->timeAdded) {
			HttpRequest_Free(req);
		} else {
			/* normal case, replace older req */
			HttpRequest_Free(&older);
			processedReqs.entries[index] = *req;
		}
	} else {
		RequestList_Append(&processedReqs, req);
	}
}

/* Updates state after a completed http request */
static void Http_FinishRequest(struct HttpRequest* req) {
	if (req->data) Platform_Log1("HTTP returned data: %i bytes", &req->size);
	req->success = !req->result && req->statusCode == 200 && req->data && req->size;
	if (!req->success) HttpRequest_Free(req);

	Mutex_Lock(processedMutex);
	{
		Http_CompleteRequest(req);
	}
	Mutex_Unlock(processedMutex);

	Mutex_Lock(curRequestMutex);
	{
		http_curRequest.id[0] = '\0';
		http_curProgress = ASYNC_PROGRESS_NOTHING;
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

			HttpRequest_Free(item);
			RequestList_RemoveAt(&processedReqs, i);
		}
	}
	Mutex_Unlock(processedMutex);
}

static void Http_ParseCookie(struct HttpRequest* req, const String* value) {
	String name, data;
	int dataEnd;
	String_UNSAFE_Separate(value, '=', &name, &data);
	/* Cookie is: __cfduid=xyz; expires=abc; path=/; domain=.classicube.net; HttpOnly */
	/* However only the __cfduid=xyz part of the cookie should be stored */
	dataEnd = String_IndexOf(&data, ';');
	if (dataEnd >= 0) data.length = dataEnd;

	req->cookies->separator = '=';
	EntryList_Set(req->cookies, &name, &data);
}

/* Parses a HTTP header */
static void Http_ParseHeader(struct HttpRequest* req, const String* line) {
	static const String httpVersion = String_FromConst("HTTP");
	String name, value, parts[3];
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
static void Http_AddHeader(const char* key, const String* value);

/* Adds all the appropriate headers for a request. */
static void Http_SetRequestHeaders(struct HttpRequest* req) {
	static const String contentType = String_FromConst("application/x-www-form-urlencoded");
	String str, cookies; char cookiesBuffer[1024];
	int i;

	if (req->lastModified[0]) {
		str = String_FromRawArray(req->lastModified);
		Http_AddHeader("If-Modified-Since", &str);
	}
	if (req->etag[0]) {
		str = String_FromRawArray(req->etag);
		Http_AddHeader("If-None-Match", &str);
	}

	if (req->data) Http_AddHeader("Content-Type", &contentType);
	if (!req->cookies || !req->cookies->entries.count) return;

	String_InitArray(cookies, cookiesBuffer);
	for (i = 0; i < req->cookies->entries.count; i++) {
		if (i) String_AppendConst(&cookies, "; ");
		str = StringsBuffer_UNSAFE_Get(&req->cookies->entries, i);
		String_AppendString(&cookies, &str);
	}
	Http_AddHeader("Cookie", &cookies);
}

/*########################################################################################################################*
*------------------------------------------------Emscripten implementation------------------------------------------------*
*#########################################################################################################################*/
#if defined CC_BUILD_WEB
static void Http_SysInit(void) { }
static void Http_SysFree(void) { }
static void Http_DownloadAsync(struct HttpRequest* req);
cc_bool Http_DescribeError(cc_result res, String* dst) { return false; }
static void Http_AddHeader(const char* key, const String* value);

static void Http_DownloadNextAsync(void) {
	struct HttpRequest req;
	if (http_terminate || !pendingReqs.count) return;
	/* already working on a request currently */
	if (http_curRequest.id[0]) return;

	req = pendingReqs.entries[0];
	RequestList_RemoveAt(&pendingReqs, 0);
	Http_DownloadAsync(&req);
}

static void Http_UpdateProgress(emscripten_fetch_t* fetch) {
	if (!fetch->totalBytes) return;
	http_curProgress = (int)(100.0f * fetch->dataOffset / fetch->totalBytes);
}

static void Http_FinishedAsync(emscripten_fetch_t* fetch) {
	struct HttpRequest* req = &http_curRequest;
	req->data          = fetch->data;
	req->size          = fetch->numBytes;
	req->statusCode    = fetch->status;
	req->contentLength = fetch->totalBytes;

	/* Remove error handler to avoid potential infinite recurison */
	/*   Sometimes calling emscripten_fetch_close calls fetch->__attributes.onerror */
	/*   But attr.onerror is actually Http_FinishedAsync, so this will end up doing */
	/*     Http_FinishedAsync --> emscripten_fetch_close --> Http_FinishedAsync ..  */
	/*   .. and eventually the browser kills it from too much recursion */
	fetch->__attributes.onerror = NULL;

	/* data needs to persist beyond closing of fetch data */
	fetch->data = NULL;
	emscripten_fetch_close(fetch);

	Http_FinishRequest(&http_curRequest);
	Http_DownloadNextAsync();
}

static void Http_DownloadAsync(struct HttpRequest* req) {
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);

	char urlBuffer[URL_MAX_SIZE]; String url;
	char urlStr[NATIVE_STR_LEN];

	switch (req->requestType) {
	case REQUEST_TYPE_GET:  Mem_Copy(attr.requestMethod, "GET",  4); break;
	case REQUEST_TYPE_HEAD: Mem_Copy(attr.requestMethod, "HEAD", 5); break;
	case REQUEST_TYPE_POST: Mem_Copy(attr.requestMethod, "POST", 5); break;
	}

	if (req->requestType == REQUEST_TYPE_POST) {
		attr.requestData     = req->data;
		attr.requestDataSize = req->size;
	}

	/* Can't use this for all URLs, because cache-control isn't in allowed CORS headers */
	/* For example, if you try this with dropbox, you'll get a '404' with */
	/*   Access to XMLHttpRequest at 'https://dl.dropbox.com/s/a/a.zip' from */
	/*   origin 'http://www.classicube.net' has been blocked by CORS policy: */
	/*   Response to preflight request doesn't pass access control check: */
	/*   Redirect is not allowed for a preflight request. */
	/* printed to console. But this is still used for skins, that way when users change */
	/* their skins, the change shows up instantly. But 99% of the time we'll get a 304 */
	/* response and can avoid downloading the whole skin over and over. */
	if (req->url[0] == '/') {
		static const char* const headers[3] = { "cache-control", "max-age=0", NULL };
		attr.requestHeaders = headers;
	}

	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess  = Http_FinishedAsync;
	attr.onerror    = Http_FinishedAsync;
	attr.onprogress = Http_UpdateProgress;

	String_InitArray(url, urlBuffer);
	Http_BeginRequest(req, &url);
	Platform_ConvertString(urlStr, &url);

	/* TODO: SET requestHeaders!!! */
	emscripten_fetch(&attr, urlStr);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------Native implementation--------------------------------------------------*
*#########################################################################################################################*/
static cc_uint32 bufferSize;

/* Allocates initial data buffer to store response contents */
static void Http_BufferInit(struct HttpRequest* req) {
	http_curProgress = 0;
	bufferSize = req->contentLength ? req->contentLength : 1;
	req->data  = (cc_uint8*)Mem_Alloc(bufferSize, 1, "http get data");
	req->size  = 0;
}

/* Ensures data buffer has enough space left to append amount bytes, reallocates if not */
static void Http_BufferEnsure(struct HttpRequest* req, cc_uint32 amount) {
	cc_uint32 newSize = req->size + amount;
	if (newSize <= bufferSize) return;

	bufferSize = newSize;
	req->data  = (cc_uint8*)Mem_Realloc(req->data, newSize, 1, "http inc data");
}

/* Increases size and updates current progress */
static void Http_BufferExpanded(struct HttpRequest* req, cc_uint32 read) {
	req->size += read;
	if (req->contentLength) http_curProgress = (int)(100.0f * req->size / req->contentLength);
}

#if defined CC_BUILD_WININET
static HINTERNET hInternet;

/* caches connections to web servers */
struct HttpCacheEntry {
	HINTERNET Handle; /* Native connection handle. */
	String Address;   /* Address of server. (e.g. "classicube.net") */
	cc_uint16 Port;   /* Port server is listening on. (e.g 80) */
	cc_bool Https;    /* Whether HTTPS or just HTTP protocol. */
	char _addressBuffer[STRING_SIZE + 1];
};
#define HTTP_CACHE_ENTRIES 10
static struct HttpCacheEntry http_cache[HTTP_CACHE_ENTRIES];

/* Splits up the components of a URL */
static void HttpCache_MakeEntry(const String* url, struct HttpCacheEntry* entry, String* resource) {
	String scheme, path, addr, name, port;
	/* URL is of form [scheme]://[server name]:[server port]/[resource] */
	int idx = String_IndexOfConst(url, "://");

	scheme = idx == -1 ? String_Empty : String_UNSAFE_Substring(url,   0, idx);
	path   = idx == -1 ? *url         : String_UNSAFE_SubstringAt(url, idx + 3);
	entry->Https = String_CaselessEqualsConst(&scheme, "https");

	String_UNSAFE_Separate(&path, '/', &addr, resource);
	String_UNSAFE_Separate(&addr, ':', &name, &port);

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

cc_bool Http_DescribeError(cc_result res, String* dst) {
	TCHAR chars[600];
	res = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					 GetModuleHandle(TEXT("wininet.dll")), res, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), chars, 600, NULL);
	if (!res) return false;

	Platform_DecodeString(dst, chars, res);
	return true;
}

static void Http_SysInit(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(GAME_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) Logger_Abort2(GetLastError(), "Failed to init WinINet");
}

static HINTERNET curReq;
static void Http_AddHeader(const char* key, const String* value) {
	String tmp; char tmpBuffer[1024];
	String_InitArray(tmp, tmpBuffer);

	String_Format2(&tmp, "%c: %s\r\n", key, value);
	HttpAddRequestHeadersA(curReq, tmp.buffer, tmp.length, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
}

/* Creates and sends a HTTP requst */
static cc_result Http_StartRequest(struct HttpRequest* req, String* url, HINTERNET* handle) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	struct HttpCacheEntry entry;
	String path; char pathBuffer[URL_MAX_SIZE + 1];
	DWORD flags, bufferLen;
	cc_result res;

	HttpCache_MakeEntry(url, &entry, &path);
	Mem_Copy(pathBuffer, path.buffer, path.length);
	pathBuffer[path.length] = '\0';
	if ((res = HttpCache_Lookup(&entry))) return res;

	flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES;
	if (entry.Https) flags |= INTERNET_FLAG_SECURE;

	*handle = HttpOpenRequestA(entry.Handle, verbs[req->requestType], 
								pathBuffer, NULL, NULL, NULL, flags, 0);
	curReq  = *handle;
	if (!curReq) return GetLastError();

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
	char buffer[8192];
	String left, line;
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

static cc_result Http_SysDo(struct HttpRequest* req, String* url) {
	HINTERNET handle;
	cc_result res = Http_StartRequest(req, url, &handle);
	HttpRequest_Free(req);
	if (res) return res;

	http_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	res = Http_ProcessHeaders(req, handle);
	if (res) { InternetCloseHandle(handle); return res; }

	if (req->requestType != REQUEST_TYPE_HEAD) {
		res = Http_DownloadData(req, handle);
		if (res) { InternetCloseHandle(handle); return res; }
	}

	return InternetCloseHandle(handle) ? 0 : GetLastError();
}

static void Http_SysFree(void) {
	int i;
	for (i = 0; i < HTTP_CACHE_ENTRIES; i++) {
		if (!http_cache[i].Handle) continue;
		InternetCloseHandle(http_cache[i].Handle);
	}
	InternetCloseHandle(hInternet);
}
#elif defined CC_BUILD_CURL
#include "Errors.h"
#include <stddef.h>
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

typedef CURLcode (APIENTRY *FP_curl_global_init)(long flags);  static FP_curl_global_init _curl_global_init;
typedef void     (APIENTRY *FP_curl_global_cleanup)(void);     static FP_curl_global_cleanup _curl_global_cleanup;
typedef CURL*    (APIENTRY *FP_curl_easy_init)(void);          static FP_curl_easy_init _curl_easy_init;
typedef CURLcode (APIENTRY *FP_curl_easy_perform)(CURL *c);    static FP_curl_easy_perform _curl_easy_perform;
typedef CURLcode (APIENTRY *FP_curl_easy_setopt)(CURL *c, int opt, ...);    static FP_curl_easy_setopt _curl_easy_setopt;
typedef void     (APIENTRY *FP_curl_easy_cleanup)(CURL* c);    static FP_curl_easy_cleanup _curl_easy_cleanup;
typedef const char* (APIENTRY *FP_curl_easy_strerror)(CURLcode res);        static FP_curl_easy_strerror _curl_easy_strerror;
typedef void     (APIENTRY *FP_curl_slist_free_all)(struct curl_slist* l);  static FP_curl_slist_free_all _curl_slist_free_all;
typedef struct curl_slist* (APIENTRY *FP_curl_slist_append)(struct curl_slist* l, const char* v); static FP_curl_slist_append _curl_slist_append;
/* End of curl headers */

#if defined CC_BUILD_WIN
static const String curlLib = String_FromConst("libcurl.dll");
static const String curlAlt = String_FromConst("curl.dll");
#elif defined CC_BUILD_OSX
static const String curlLib = String_FromConst("/usr/lib/libcurl.4.dylib");
static const String curlAlt = String_FromConst("/usr/lib/libcurl.dylib");
#elif defined CC_BUILD_OPENBSD
static const String curlLib = String_FromConst("libcurl.so.25.17");
static const String curlAlt = String_FromConst("libcurl.so.25.16");
#else
static const String curlLib = String_FromConst("libcurl.so.4");
static const String curlAlt = String_FromConst("libcurl.so.3");
#endif

#define QUOTE(x) #x
#define LoadCurlFunc(sym) (_ ## sym = (FP_ ## sym)DynamicLib_Get2(lib, QUOTE(sym)))
static cc_bool LoadCurlFuncs(void) {
	void* lib = DynamicLib_Load2(&curlLib);
	if (!lib) { 
		Logger_DynamicLibWarn("loading", &curlLib);

		lib = DynamicLib_Load2(&curlAlt);
		if (!lib) { Logger_DynamicLibWarn("loading", &curlAlt); return false; }
	}
	/* Non-essential function missing in older curl versions */
	LoadCurlFunc(curl_easy_strerror);

	return
		LoadCurlFunc(curl_global_init)    && LoadCurlFunc(curl_global_cleanup) &&
		LoadCurlFunc(curl_easy_init)      && LoadCurlFunc(curl_easy_perform)   &&
		LoadCurlFunc(curl_easy_setopt)    && LoadCurlFunc(curl_easy_cleanup)   &&
		LoadCurlFunc(curl_slist_free_all) && LoadCurlFunc(curl_slist_append);
}

static CURL* curl;
static cc_bool curlSupported;

cc_bool Http_DescribeError(cc_result res, String* dst) {
	const char* err;
	
	if (!_curl_easy_strerror) return false;
	err = _curl_easy_strerror((CURLcode)res);
	if (!err) return false;

	String_AppendConst(dst, err);
	return true;
}

static void Http_SysInit(void) {
	static const String msg = String_FromConst("Failed to init libcurl. All HTTP requests will therefore fail.");
	CURLcode res;

	if (!LoadCurlFuncs()) { Logger_WarnFunc(&msg); return; }
	res = _curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) { Logger_SimpleWarn(res, "initing curl"); return; }

	curl = _curl_easy_init();
	if (!curl) { Logger_SimpleWarn(res, "initing curl_easy"); return; }
	curlSupported = true;
}

static struct curl_slist* headers_list;
static void Http_AddHeader(const char* key, const String* value) {
	String tmp; char tmpBuffer[1024];
	String_InitArray_NT(tmp, tmpBuffer);
	String_Format2(&tmp, "%c: %s", key, value);

	tmp.buffer[tmp.length] = '\0';
	headers_list = _curl_slist_append(headers_list, tmp.buffer);
}

/* Processes a HTTP header downloaded from the server */
static size_t Http_ProcessHeader(char *buffer, size_t size, size_t nitems, void* userdata) {
	struct HttpRequest* req = (struct HttpRequest*)userdata;
	String line = String_Init(buffer, nitems, nitems);

	/* line usually has \r\n at end */
	if (line.length && line.buffer[line.length - 1] == '\n') line.length--;
	if (line.length && line.buffer[line.length - 1] == '\r') line.length--;

	Http_ParseHeader(req, &line);
	return nitems;
}

/* Processes a chunk of data downloaded from the web server */
static size_t Http_ProcessData(char *buffer, size_t size, size_t nitems, void* userdata) {
	struct HttpRequest* req = (struct HttpRequest*)userdata;

	if (!bufferSize) Http_BufferInit(req);
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

static cc_result Http_SysDo(struct HttpRequest* req, String* url) {
	char urlStr[NATIVE_STR_LEN];
	void* post_data = req->data;
	CURLcode res;

	if (!curlSupported) {
		HttpRequest_Free(req);
		return ERR_NOT_SUPPORTED;
	}

	headers_list = NULL;
	Http_SetRequestHeaders(req);
	_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);

	Http_SetCurlOpts(req);
	Platform_ConvertString(urlStr, url);
	_curl_easy_setopt(curl, CURLOPT_URL, urlStr);

	if (req->requestType == REQUEST_TYPE_HEAD) {
		_curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	} else if (req->requestType == REQUEST_TYPE_POST) {
		_curl_easy_setopt(curl, CURLOPT_POST,   1L);
		_curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->size);
		_curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    req->data);

		/* per curl docs, we must persist POST data until request finishes */
		req->data = NULL;
		HttpRequest_Free(req);
	} else {
		/* Undo POST/HEAD state */
		_curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	}

	bufferSize = 0;
	http_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	res = _curl_easy_perform(curl);
	http_curProgress = 100;

	_curl_slist_free_all(headers_list);
	/* can free now that request has finished */
	Mem_Free(post_data);
	return res;
}

static void Http_SysFree(void) {
	if (!curlSupported) return;
	_curl_easy_cleanup(curl);
	_curl_global_cleanup();
}
#elif defined CC_BUILD_ANDROID
struct HttpRequest* java_req;

cc_bool Http_DescribeError(cc_result res, String* dst) {
	String err;
	JNIEnv* env;
	jvalue args[1];
	jobject obj;
	
	JavaGetCurrentEnv(env);
	args[0].i = res;
	obj       = JavaCallObject(env, "httpDescribeError", "(I)Ljava/lang/String;", args);
	if (!obj) return false;

	err = JavaGetString(env, obj);
	String_AppendString(dst, &err);
	(*env)->ReleaseStringUTFChars(env, obj, err.buffer);
	(*env)->DeleteLocalRef(env, obj);
	return true;
}

static void Http_AddHeader(const char* key, const String* value) {
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
	String line = JavaGetString(env, header);
	Platform_Log(&line);
	Http_ParseHeader(java_req, &line);
	(*env)->ReleaseStringUTFChars(env, header, line.buffer);
}

/* Processes a chunk of data downloaded from the web server */
static void JNICALL java_HttpAppendData(JNIEnv* env, jobject o, jbyteArray arr, jint len) {
	struct HttpRequest* req = java_req;
	if (!bufferSize) Http_BufferInit(req);

	Http_BufferEnsure(req, len);
	(*env)->GetByteArrayRegion(env, arr, 0, len, &req->data[req->size]);
	Http_BufferExpanded(req, len);
}

static const JNINativeMethod methods[2] = {
	{ "httpParseHeader", "(Ljava/lang/String;)V", java_HttpParseHeader },
	{ "httpAppendData",  "([BI)V",                java_HttpAppendData }
};
static void Http_SysInit(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaRegisterNatives(env, methods);
}

static cc_result Http_InitReq(JNIEnv* env, struct HttpRequest* req, String* url) {
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

static cc_result Http_SysDo(struct HttpRequest* req, String* url) {
	static const String userAgent = String_FromConst(GAME_APP_NAME);
	JNIEnv* env;
	jint res;

	JavaGetCurrentEnv(env);
	if ((res = Http_InitReq(env, req, url))) return res;
	java_req = req;

	Http_SetRequestHeaders(req);
	Http_AddHeader("User-Agent", &userAgent);
	if (req->data && (res = Http_SetData(env, req))) return res;

	bufferSize = 0;
	http_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	res = JavaCallInt(env, "httpPerform", "()I", NULL);
	http_curProgress = 100;
	return res;
}

static void Http_SysFree(void) { }
#endif

#ifndef CC_BUILD_WEB
static void Http_WorkerLoop(void) {
	char urlBuffer[URL_MAX_SIZE]; String url;
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
		request.result = Http_SysDo(&request, &url);
		end = Stopwatch_Measure();

		elapsed = (int)Stopwatch_ElapsedMilliseconds(beg, end);
		Platform_Log3("HTTP: return code %i (http %i), in %i ms",
					&request.result, &request.statusCode, &elapsed);
		Http_FinishRequest(&request);
	}
}
#endif


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WEB
/* Access to XMLHttpRequest at 'http://static.classicube.net' from origin 'http://www.classicube.net' has been blocked by CORS policy: */
/* No 'Access-Control-Allow-Origin' header is present on the requested resource. */
#define SKIN_SERVER "http://classicube.s3.amazonaws.com/skin/"
#else
/* Prefer static.classicube.net to avoid a pointless redirect */
/* Skins were moved to use Amazon S3, so link directly to them */
#define SKIN_SERVER "http://classicube.s3.amazonaws.com/skin/"
#endif

void Http_AsyncGetSkin(const String* skinName) {
	String url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName)) {
		String_Copy(&url, skinName);
	} else {
		String_Format1(&url, SKIN_SERVER "%s.png", skinName);
	}
	Http_AsyncGetData(&url, false, skinName);
}

void Http_AsyncGetData(const String* url, cc_bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, NULL, NULL, NULL, 0, NULL);
}
void Http_AsyncGetHeaders(const String* url, cc_bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_HEAD, NULL, NULL, NULL, 0, NULL);
}
void Http_AsyncPostData(const String* url, cc_bool priority, const String* id, const void* data, cc_uint32 size, struct EntryList* cookies) {
	Http_Add(url, priority, id, REQUEST_TYPE_POST, NULL, NULL, data, size, cookies);
}
void Http_AsyncGetDataEx(const String* url, cc_bool priority, const String* id, const String* lastModified, const String* etag, struct EntryList* cookies) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, lastModified, etag, NULL, 0, cookies);
}

cc_bool Http_GetResult(const String* id, struct HttpRequest* item) {
	int i;
	Mutex_Lock(processedMutex);
	{
		i = RequestList_Find(&processedReqs, id, item);
		if (i >= 0) RequestList_RemoveAt(&processedReqs, i);
	}
	Mutex_Unlock(processedMutex);
	return i >= 0;
}

cc_bool Http_GetCurrent(struct HttpRequest* request, int* progress) {
	Mutex_Lock(curRequestMutex);
	{
		*request  = http_curRequest;
		*progress = http_curProgress;
	}
	Mutex_Unlock(curRequestMutex);
	return request->id[0];
}

void Http_ClearPending(void) {
	Mutex_Lock(pendingMutex);
	{
		RequestList_Free(&pendingReqs);
	}
	Mutex_Unlock(pendingMutex);
	Waitable_Signal(workerWaitable);
}

static cc_bool Http_UrlDirect(cc_uint8 c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
		|| c == '-' || c == '_' || c == '.' || c == '~';
}

void Http_UrlEncode(String* dst, const cc_uint8* data, int len) {
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

void Http_UrlEncodeUtf8(String* dst, const String* src) {
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
static void Http_Init(void) {
#ifdef CC_BUILD_ANDROID
	if (workerThread) return;
#endif
	ScheduledTask_Add(30, Http_CleanCacheTask);
	RequestList_Init(&pendingReqs);
	RequestList_Init(&processedReqs);
	Http_SysInit();

	workerWaitable  = Waitable_Create();
	pendingMutex    = Mutex_Create();
	processedMutex  = Mutex_Create();
	curRequestMutex = Mutex_Create();
#ifndef CC_BUILD_WEB
	workerThread    = Thread_Start(Http_WorkerLoop, false);
#endif
}

static void Http_Free(void) {
	http_terminate = true;
	Http_ClearPending();
#ifndef CC_BUILD_WEB
	Thread_Join(workerThread);
#endif

	RequestList_Free(&pendingReqs);
	RequestList_Free(&processedReqs);
	Http_SysFree();

	Waitable_Free(workerWaitable);
	Mutex_Free(pendingMutex);
	Mutex_Free(processedMutex);
	Mutex_Free(curRequestMutex);
}

struct IGameComponent Http_Component = {
	Http_Init,        /* Init  */
	Http_Free,        /* Free  */
	Http_ClearPending /* Reset */
};
