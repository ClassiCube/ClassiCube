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
#define Platform_DecodeString(dst, src, len) String_DecodeCP1252(dst, (uint8_t*)(src), len)
#endif

#include <windows.h>
#include <wininet.h>
#elif defined CC_BUILD_WEB
/* Use fetch/XMLHttpRequest api for Emscripten */
#include <emscripten/fetch.h>
#elif defined CC_BUILD_CURL
#include <curl/curl.h>
#include <time.h>
#endif

void HttpRequest_Free(struct HttpRequest* request) {
	Mem_Free(request->Data);
	request->Data = NULL;
	request->Size = 0;
}

/*########################################################################################################################*
*----------------------------------------------------Http requests list---------------------------------------------------*
*#########################################################################################################################*/
#define HTTP_DEF_ELEMS 10
struct RequestList {
	int Count, Capacity;
	struct HttpRequest* Entries;
	struct HttpRequest DefaultEntries[HTTP_DEF_ELEMS];
};

/* Expands request list buffer if there is no room for another request */
static void RequestList_EnsureSpace(struct RequestList* list) {
	if (list->Count < list->Capacity) return;
	Utils_Resize((void**)&list->Entries, &list->Capacity,
				sizeof(struct HttpRequest), HTTP_DEF_ELEMS, 10);
}

/* Adds another request to end (for normal priority request) */
static void RequestList_Append(struct RequestList* list, struct HttpRequest* item) {
	RequestList_EnsureSpace(list);
	list->Entries[list->Count++] = *item;
}

/* Inserts a request at start (for high priority request) */
static void RequestList_Prepend(struct RequestList* list, struct HttpRequest* item) {
	int i;
	RequestList_EnsureSpace(list);
	
	for (i = list->Count; i > 0; i--) {
		list->Entries[i] = list->Entries[i - 1];
	}
	list->Entries[0] = *item;
	list->Count++;
}

/* Removes the request at the given index */
static void RequestList_RemoveAt(struct RequestList* list, int i) {
	if (i < 0 || i >= list->Count) Logger_Abort("Tried to remove element at list end");

	for (; i < list->Count - 1; i++) {
		list->Entries[i] = list->Entries[i + 1];
	}
	list->Count--;
}

/* Finds index of request whose id matches the given id */
static int RequestList_Find(struct RequestList* list, const String* id, struct HttpRequest* item) {
	String reqID;
	int i;

	for (i = 0; i < list->Count; i++) {
		reqID = String_FromRawArray(list->Entries[i].ID);
		if (!String_Equals(id, &reqID)) continue;

		*item = list->Entries[i];
		return i;
	}
	return -1;
}

/* Resets state to default */
static void RequestList_Init(struct RequestList* list) {
	list->Capacity = HTTP_DEF_ELEMS;
	list->Count    = 0;
	list->Entries = list->DefaultEntries;
}

/* Frees any dynamically allocated memory, then resets state to default */
static void RequestList_Free(struct RequestList* list) {
	if (list->Entries != list->DefaultEntries) Mem_Free(list->Entries);
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
static volatile bool http_terminate;

static struct RequestList pendingReqs;
static struct RequestList processedReqs;
static struct HttpRequest http_curRequest;
static volatile int http_curProgress = ASYNC_PROGRESS_NOTHING;

#ifdef CC_BUILD_WEB
static void Http_DownloadNextAsync(void);
#endif

/* Adds a req to the list of pending requests, waking up worker thread if needed. */
static void Http_Add(const String* url, bool priority, const String* id, uint8_t type, const String* lastModified, const String* etag, const void* data, uint32_t size, struct EntryList* cookies) {
	struct HttpRequest req = { 0 };
	String str;

	String_InitArray(str, req.URL);
	String_Copy(&str, url);
	Platform_Log2("Adding %s (type %b)", url, &type);

	String_InitArray(str, req.ID);
	String_Copy(&str, id);
	req.RequestType = type;
	
	if (lastModified) {
		String_InitArray(str, req.LastModified);
		String_Copy(&str, lastModified);
	}

	if (etag) { 
		String_InitArray(str, req.Etag);
		String_Copy(&str, etag); 
	}

	if (data) {
		req.Data = (uint8_t*)Mem_Alloc(size, 1, "Http_PostData");
		Mem_Copy(req.Data, data, size);
		req.Size = size;
	}
	req.Cookies = cookies;

	Mutex_Lock(pendingMutex);
	{	
		req.TimeAdded = DateTime_CurrentUTC_MS();
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

/* Sets up state to begin a http request */
static void Http_BeginRequest(struct HttpRequest* req) {
	String url = String_FromRawArray(req->URL);
	Platform_Log2("Downloading from %s (type %b)", &url, &req->RequestType);

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
	String id = String_FromRawArray(req->ID);
	int index;
	req->TimeDownloaded = DateTime_CurrentUTC_MS();

	index = RequestList_Find(&processedReqs, &id, &older);
	if (index >= 0) {
		/* very rare case - priority item was inserted, then inserted again (so put before first item), */
		/* and both items got downloaded before an external function removed them from the queue */
		if (older.TimeAdded > req->TimeAdded) {
			HttpRequest_Free(req);
		} else {
			/* normal case, replace older req */
			HttpRequest_Free(&older);
			processedReqs.Entries[index] = *req;
		}
	} else {
		RequestList_Append(&processedReqs, req);
	}
}

/* Updates state after a completed http request */
static void Http_FinishRequest(struct HttpRequest* req) {
	if (req->Data) Platform_Log1("HTTP returned data: %i bytes", &req->Size);
	req->Success = !req->Result && req->StatusCode == 200 && req->Data && req->Size;
	if (!req->Success) HttpRequest_Free(req);

	Mutex_Lock(processedMutex);
	{
		Http_CompleteRequest(req);
	}
	Mutex_Unlock(processedMutex);

	Mutex_Lock(curRequestMutex);
	{
		http_curRequest.ID[0] = '\0';
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
		for (i = processedReqs.Count - 1; i >= 0; i--) {
			item = &processedReqs.Entries[i];
			if (item->TimeDownloaded + (10 * 1000) >= now) continue;

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

	req->Cookies->separator = '=';
	EntryList_Set(req->Cookies, &name, &data);
}

/* Parses a HTTP header */
static void Http_ParseHeader(struct HttpRequest* req, const String* line) {
	static const String httpVersion = String_FromConst("HTTP");
	String name, value, parts[3];
	int numParts;

	/* HTTP[version] [status code] [status reason] */
	if (String_CaselessStarts(line, &httpVersion)) {
		numParts = String_UNSAFE_Split(line, ' ', parts, 3);
		if (numParts >= 2) Convert_ParseInt(&parts[1], &req->StatusCode);
	}
	/* For all other headers:  name: value */
	if (!String_UNSAFE_Separate(line, ':', &name, &value)) return;

	if (String_CaselessEqualsConst(&name, "ETag")) {
		String_CopyToRawArray(req->Etag, &value);
	} else if (String_CaselessEqualsConst(&name, "Content-Length")) {
		Convert_ParseInt(&value, &req->ContentLength);
	} else if (String_CaselessEqualsConst(&name, "Last-Modified")) {
		String_CopyToRawArray(req->LastModified, &value);
	} else if (req->Cookies && String_CaselessEqualsConst(&name, "Set-Cookie")) {
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

	if (req->LastModified[0]) {
		str = String_FromRawArray(req->LastModified);
		Http_AddHeader("If-Modified-Since", &str);
	}
	if (req->Etag[0]) {
		str = String_FromRawArray(req->Etag);
		Http_AddHeader("If-None-Match", &str);
	}

	if (req->Data) Http_AddHeader("Content-Type", &contentType);
	if (!req->Cookies || !req->Cookies->entries.count) return;

	String_InitArray(cookies, cookiesBuffer);
	for (i = 0; i < req->Cookies->entries.count; i++) {
		if (i) String_AppendConst(&cookies, "; ");
		str = StringsBuffer_UNSAFE_Get(&req->Cookies->entries, i);
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
bool Http_DescribeError(ReturnCode res, String* dst) { return false; }
static void Http_AddHeader(const char* key, const String* value);

static void Http_DownloadNextAsync(void) {
	struct HttpRequest req;
	if (http_terminate || !pendingReqs.Count) return;
	/* already working on a request currently */
	if (http_curRequest.ID[0] != '\0') return;

	req = pendingReqs.Entries[0];
	RequestList_RemoveAt(&pendingReqs, 0);
	Http_DownloadAsync(&req);
}

static void Http_UpdateProgress(emscripten_fetch_t* fetch) {
	if (!fetch->totalBytes) return;
	http_curProgress = (int)(100.0f * fetch->dataOffset / fetch->totalBytes);
}

static void Http_FinishedAsync(emscripten_fetch_t* fetch) {
	struct HttpRequest* req = &http_curRequest;
	req->Data          = fetch->data;
	req->Size          = fetch->numBytes;
	req->StatusCode    = fetch->status;
	req->ContentLength = fetch->totalBytes;

	/* data needs to persist beyond closing of fetch data */
	fetch->data = NULL;
	emscripten_fetch_close(fetch);

	Http_FinishRequest(&http_curRequest);
	Http_DownloadNextAsync();
}

static void Http_DownloadAsync(struct HttpRequest* req) {
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);

	String url = String_FromRawArray(req->URL);
	char urlStr[600];
	Platform_ConvertString(urlStr, &url);

	switch (req->RequestType) {
	case REQUEST_TYPE_GET:  Mem_Copy(attr.requestMethod, "GET",  4); break;
	case REQUEST_TYPE_HEAD: Mem_Copy(attr.requestMethod, "HEAD", 5); break;
	case REQUEST_TYPE_POST: Mem_Copy(attr.requestMethod, "POST", 5); break;
	}

	if (req->RequestType == REQUEST_TYPE_POST) {
		attr.requestData     = req->Data;
		attr.requestDataSize = req->Size;
	}

	/* TODO: Why does this break */
	/*static const char* headers[3] = { "cache-control", "max-age=0", NULL }; */
	/*attr.requestHeaders = headers; */
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess  = Http_FinishedAsync;
	attr.onerror    = Http_FinishedAsync;
	attr.onprogress = Http_UpdateProgress;

	Http_BeginRequest(req);
	/* TODO: SET requestHeaders!!! */
	emscripten_fetch(&attr, urlStr);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------Native implementation--------------------------------------------------*
*#########################################################################################################################*/
static uint32_t bufferSize;

/* Allocates initial data buffer to store response contents */
static void Http_BufferInit(struct HttpRequest* req) {
	http_curProgress = 0;
	bufferSize = req->ContentLength ? req->ContentLength : 1;
	req->Data  = (uint8_t*)Mem_Alloc(bufferSize, 1, "http get data");
	req->Size  = 0;
}

/* Ensures data buffer has enough space left to append amount bytes, reallocates if not */
static void Http_BufferEnsure(struct HttpRequest* req, uint32_t amount) {
	uint32_t newSize = req->Size + amount;
	if (newSize <= bufferSize) return;

	bufferSize = newSize;
	req->Data  = (uint8_t*)Mem_Realloc(req->Data, newSize, 1, "http inc data");
}

/* Increases size and updates current progress */
static void Http_BufferExpanded(struct HttpRequest* req, uint32_t read) {
	req->Size += read;
	if (req->ContentLength) http_curProgress = (int)(100.0f * req->Size / req->ContentLength);
}

#if defined CC_BUILD_WININET
static HINTERNET hInternet;

/* caches connections to web servers */
struct HttpCacheEntry {
	HINTERNET Handle; /* Native connection handle. */
	String Address;   /* Address of server. (e.g. "classicube.net") */
	uint16_t Port;    /* Port server is listening on. (e.g 80) */
	bool Https;       /* Whether HTTPS or just HTTP protocol. */
	char _addressBuffer[STRING_SIZE + 1];
};
#define HTTP_CACHE_ENTRIES 10
static struct HttpCacheEntry http_cache[HTTP_CACHE_ENTRIES];

/* Splits up the components of a URL */
static void HttpCache_MakeEntry(const String* url, struct HttpCacheEntry* entry, String* resource) {
	static const String schemeEnd = String_FromConst("://");
	String scheme, path, addr, name, port;

	/* URL is of form [scheme]://[server name]:[server port]/[resource] */
	int idx = String_IndexOfString(url, &schemeEnd);

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
static ReturnCode HttpCache_Insert(int i, struct HttpCacheEntry* e) {
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
static ReturnCode HttpCache_Lookup(struct HttpCacheEntry* e) {
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
	i = (uint8_t)Stopwatch_Measure() % HTTP_CACHE_ENTRIES;
	InternetCloseHandle(http_cache[i].Handle);
	return HttpCache_Insert(i, e);
}

bool Http_DescribeError(ReturnCode res, String* dst) {
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
static ReturnCode Http_StartRequest(struct HttpRequest* req, HINTERNET* handle) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	struct HttpCacheEntry entry;
	DWORD flags;

	String path; char pathBuffer[URL_MAX_SIZE + 1];
	String url = String_FromRawArray(req->URL);

	HttpCache_MakeEntry(&url, &entry, &path);
	Mem_Copy(pathBuffer, path.buffer, path.length);
	pathBuffer[path.length] = '\0';
	HttpCache_Lookup(&entry);

	flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES;
	if (entry.Https) flags |= INTERNET_FLAG_SECURE;

	*handle = HttpOpenRequestA(entry.Handle, verbs[req->RequestType], 
								pathBuffer, NULL, NULL, NULL, flags, 0);
	curReq  = *handle;
	if (!curReq) return GetLastError();

	Http_SetRequestHeaders(req);
	return HttpSendRequestA(*handle, NULL, 0, req->Data, req->Size) ? 0 : GetLastError();
}

/* Gets headers from a HTTP response */
static ReturnCode Http_ProcessHeaders(struct HttpRequest* req, HINTERNET handle) {
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
static ReturnCode Http_DownloadData(struct HttpRequest* req, HINTERNET handle) {
	DWORD read, avail;
	Http_BufferInit(req);

	for (;;) {
		if (!InternetQueryDataAvailable(handle, &avail, 0, 0)) break;
		if (!avail) break;
		Http_BufferEnsure(req, avail);

		if (!InternetReadFile(handle, &req->Data[req->Size], avail, &read)) return GetLastError();
		if (!read) break;
		Http_BufferExpanded(req, read);
	}

 	http_curProgress = 100;
	return 0;
}

static ReturnCode Http_SysDo(struct HttpRequest* req) {
	HINTERNET handle;
	ReturnCode res = Http_StartRequest(req, &handle);
	HttpRequest_Free(req);
	if (res) return res;

	http_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	res = Http_ProcessHeaders(req, handle);
	if (res) { InternetCloseHandle(handle); return res; }

	if (req->RequestType != REQUEST_TYPE_HEAD) {
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
static CURL* curl;

bool Http_DescribeError(ReturnCode res, String* dst) {
	const char* err = curl_easy_strerror(res);
	if (!err) return false;

	String_AppendConst(dst, err);
	return true;
}

static void Http_SysInit(void) {
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) Logger_Abort2(res, "Failed to init curl");

	curl = curl_easy_init();
	if (!curl) Logger_Abort("Failed to init easy curl");
}

static struct curl_slist* headers_list;
static void Http_AddHeader(const char* key, const String* value) {
	String tmp; char tmpBuffer[1024];
	String_InitArray_NT(tmp, tmpBuffer);
	String_Format2(&tmp, "%c: %s", key, value);

	tmp.buffer[tmp.length] = '\0';
	headers_list = curl_slist_append(headers_list, tmp.buffer);
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

	Mem_Copy(&req->Data[req->Size], buffer, nitems);
	Http_BufferExpanded(req, nitems);
	return nitems;
}

/* Sets general curl options for a request */
static void Http_SetCurlOpts(struct HttpRequest* req) {
	curl_easy_setopt(curl, CURLOPT_USERAGENT,      GAME_APP_NAME);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_ProcessHeader);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_ProcessData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);
}

static ReturnCode Http_SysDo(struct HttpRequest* req) {
	String url = String_FromRawArray(req->URL);
	char urlStr[600];
	void* post_data = req->Data;
	CURLcode res;

	curl_easy_reset(curl);
	headers_list = NULL;
	Http_SetRequestHeaders(req);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);

	Http_SetCurlOpts(req);
	Platform_ConvertString(urlStr, &url);
	curl_easy_setopt(curl, CURLOPT_URL, urlStr);

	if (req->RequestType == REQUEST_TYPE_HEAD) {
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	} else if (req->RequestType == REQUEST_TYPE_POST) {
		curl_easy_setopt(curl, CURLOPT_POST,   1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,  req->Size);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     req->Data);

		/* per curl docs, we must persist POST data until request finishes */
		req->Data = NULL;
		HttpRequest_Free(req);
	}

	bufferSize = 0;
	http_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	res = curl_easy_perform(curl);
	http_curProgress = 100;

	curl_slist_free_all(headers_list);
	/* can free now that request has finished */
	Mem_Free(post_data);
	return res;
}

static void Http_SysFree(void) {
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}
#elif defined CC_BUILD_ANDROID
#include <android_native_app_glue.h>
struct HttpRequest* java_req;

bool Http_DescribeError(ReturnCode res, String* dst) {
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
static void JNICALL java_HttpParseHeader(JNIEnv* env, jclass c, jstring header) {
	String line = JavaGetString(env, header);
	Platform_Log(&line);
	Http_ParseHeader(java_req, &line);
	(*env)->ReleaseStringUTFChars(env, header, line.buffer);
}

/* Processes a chunk of data downloaded from the web server */
static void JNICALL java_HttpAppendData(JNIEnv* env, jclass c, jbyteArray arr, jint len) {
	struct HttpRequest* req = java_req;
	if (!bufferSize) Http_BufferInit(req);

	Http_BufferEnsure(req, len);
	(*env)->GetByteArrayRegion(env, arr, 0, len, &req->Data[req->Size]);
	Http_BufferExpanded(req, len);
}

static JNINativeMethod methods[2] = {
	{ "httpParseHeader", "(Ljava/lang/String;)V", java_HttpParseHeader },
	{ "httpAppendData",  "([BI)V",                java_HttpAppendData }
};
static void Http_SysInit(void) {
	struct android_app* app = (struct android_app*)App_Ptr;
	JNIEnv* env;
	JavaGetCurrentEnv(env);

	jobject instance = app->activity->clazz;
	jclass clazz     = (*env)->GetObjectClass(env, instance);
	(*env)->RegisterNatives(env, clazz, methods, 2);
}

static ReturnCode Http_InitReq(JNIEnv* env, struct HttpRequest* req) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	jvalue args[2];
	String url;
	jint res;

	url = String_FromRawArray(req->URL);
	args[0].l = JavaMakeString(env, &url);
	args[1].l = JavaMakeConst(env,  verbs[req->RequestType]);

	res = JavaCallInt(env, "httpInit", "(Ljava/lang/String;Ljava/lang/String;)I", args);
	(*env)->DeleteLocalRef(env, args[0].l);
	(*env)->DeleteLocalRef(env, args[1].l);
	return res;
}

static ReturnCode Http_SetData(JNIEnv* env, struct HttpRequest* req) {
	jvalue args[1];
	jint res;

	args[0].l = JavaMakeBytes(env, req->Data, req->Size);
	res = JavaCallInt(env, "httpSetData", "([B)I", args);
	(*env)->DeleteLocalRef(env, args[0].l);
	return res;
}

static ReturnCode Http_SysDo(struct HttpRequest* req) {
	static const String userAgent = String_FromConst(GAME_APP_NAME);
	JNIEnv* env;
	jint res;

	JavaGetCurrentEnv(env);
	if ((res = Http_InitReq(env, req))) return res;
	java_req = req;

	Http_SetRequestHeaders(req);
	Http_AddHeader("User-Agent", &userAgent);
	if (req->Data && (res = Http_SetData(env, req))) return res;

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
	struct HttpRequest request;
	bool hasRequest, stop;
	uint64_t beg, end;
	uint32_t elapsed;

	for (;;) {
		hasRequest = false;

		Mutex_Lock(pendingMutex);
		{
			stop = http_terminate;
			if (!stop && pendingReqs.Count) {
				request = pendingReqs.Entries[0];
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
		Http_BeginRequest(&request);

		beg = Stopwatch_Measure();
		request.Result = Http_SysDo(&request);
		end = Stopwatch_Measure();

		elapsed = Stopwatch_ElapsedMicroseconds(beg, end) / 1000;
		Platform_Log3("HTTP: return code %i (http %i), in %i ms",
					&request.Result, &request.StatusCode, &elapsed);
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
#define SKIN_SERVER "/skins/"
#else
/* Prefer static.classicube.net to avoid a pointless redirect */
#define SKIN_SERVER "http://static.classicube.net/skins/"
#endif

void Http_AsyncGetSkin(const String* skinName) {
	String url; char urlBuffer[STRING_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName, 0)) {
		String_Copy(&url, skinName);
	} else {
		String_AppendConst(&url, SKIN_SERVER);
		String_AppendColorless(&url, skinName);
		String_AppendConst(&url, ".png");
	}
	Http_AsyncGetData(&url, false, skinName);
}

void Http_AsyncGetData(const String* url, bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, NULL, NULL, NULL, 0, NULL);
}
void Http_AsyncGetHeaders(const String* url, bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_HEAD, NULL, NULL, NULL, 0, NULL);
}
void Http_AsyncPostData(const String* url, bool priority, const String* id, const void* data, uint32_t size, struct EntryList* cookies) {
	Http_Add(url, priority, id, REQUEST_TYPE_POST, NULL, NULL, data, size, cookies);
}
void Http_AsyncGetDataEx(const String* url, bool priority, const String* id, const String* lastModified, const String* etag, struct EntryList* cookies) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, lastModified, etag, NULL, 0, cookies);
}

bool Http_GetResult(const String* id, struct HttpRequest* item) {
	int i;
	Mutex_Lock(processedMutex);
	{
		i = RequestList_Find(&processedReqs, id, item);
		if (i >= 0) RequestList_RemoveAt(&processedReqs, i);
	}
	Mutex_Unlock(processedMutex);
	return i >= 0;
}

bool Http_GetCurrent(struct HttpRequest* request, int* progress) {
	Mutex_Lock(curRequestMutex);
	{
		*request  = http_curRequest;
		*progress = http_curProgress;
	}
	Mutex_Unlock(curRequestMutex);
	return request->ID[0];
}

void Http_ClearPending(void) {
	Mutex_Lock(pendingMutex);
	{
		RequestList_Free(&pendingReqs);
	}
	Mutex_Unlock(pendingMutex);
	Waitable_Signal(workerWaitable);
}

static bool Http_UrlDirect(uint8_t c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
		|| c == '-' || c == '_' || c == '.' || c == '~';
}

void Http_UrlEncode(String* dst, const uint8_t* data, int len) {
	int i;
	for (i = 0; i < len; i++) {
		uint8_t c = data[i];

		if (Http_UrlDirect(c)) {
			String_Append(dst, c);
		} else {
			String_Append(dst,  '%');
			String_AppendHex(dst, c);
		}
	}
}

void Http_UrlEncodeUtf8(String* dst, const String* src) {
	uint8_t data[4];
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
