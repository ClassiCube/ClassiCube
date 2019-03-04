#include "Http.h"
#include "Platform.h"
#include "Funcs.h"
#include "Logger.h"
#include "Stream.h"
#include "GameStructs.h"

#if defined CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define _WIN32_IE    0x0400
#define WINVER       0x0500
#define _WIN32_WINNT 0x0500
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <wininet.h>
#define HTTP_QUERY_ETAG 54 /* Missing from some old MingW32 headers */
#elif defined CC_BUILD_WEB
/* Use javascript web api for Emscripten */
#undef CC_BUILD_POSIX
#include <emscripten/fetch.h>
#elif defined CC_BUILD_POSIX
/* POSIX is mainly shared between Linux and OSX */
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
	int MaxElems, Count;
	struct HttpRequest* Entries;
	struct HttpRequest DefaultEntries[HTTP_DEF_ELEMS];
};

/* Expands request list buffer if there is no room for another request */
static void RequestList_EnsureSpace(struct RequestList* list) {
	if (list->Count < list->MaxElems) return;
	list->Entries = Utils_Resize(list->Entries, &list->MaxElems,
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
	list->MaxElems = HTTP_DEF_ELEMS;
	list->Count    = 0;
	list->Entries = list->DefaultEntries;
}

/* Frees any dynamically allocated memory, then resets state to default */
static void RequestList_Free(struct RequestList* list) {
	if (list->Entries != list->DefaultEntries) {
		Mem_Free(list->Entries);
	}
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
static void Http_Add(const String* url, bool priority, const String* id, uint8_t type, TimeMS* lastModified, const String* etag, const void* data, uint32_t size) {
	struct HttpRequest req = { 0 };
	String reqUrl, reqID, reqEtag;

	String_InitArray(reqUrl, req.URL);
	String_Copy(&reqUrl, url);
	String_InitArray(reqID, req.ID);
	String_Copy(&reqID, id);

	req.RequestType = type;
	Platform_Log2("Adding %s (type %b)", &reqUrl, &type);

	String_InitArray(reqEtag, req.Etag);
	if (lastModified) { req.LastModified = *lastModified; }
	if (etag)         { String_Copy(&reqEtag, etag); }

	if (data) {
		req.Data = Mem_Alloc(size, 1, "Http_PostData");
		Mem_Copy(req.Data, data, size);
		req.Size = size;
	}

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


/*########################################################################################################################*
*------------------------------------------------Emscripten implementation------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_WEB
static void Http_SysInit(void) { }
static void Http_SysFree(void) { }
static void Http_DownloadAsync(struct HttpRequest* req);

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
	req->Data       = fetch->data;
	req->Size       = fetch->numBytes;
	req->StatusCode = fetch->status;

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
#ifdef CC_BUILD_WIN
static HINTERNET hInternet;
/* TODO: Test last modified and etag even work */
#define FLAG_STATUS  HTTP_QUERY_STATUS_CODE    | HTTP_QUERY_FLAG_NUMBER
#define FLAG_LENGTH  HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER
#define FLAG_LASTMOD HTTP_QUERY_LAST_MODIFIED  | HTTP_QUERY_FLAG_SYSTEMTIME

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
	const static String schemeEnd = String_FromConst("://");
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

static void Http_SysInit(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(GAME_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) Logger_Abort2(GetLastError(), "Failed to init WinINet");
}

/* Adds custom HTTP headers for a req */
static void Http_MakeHeaders(String* headers, struct HttpRequest* req) {
	if (!req->Etag[0] && !req->LastModified && !req->Data) {
		headers->buffer = NULL; return;
	}

	if (req->LastModified) {
		String_AppendConst(headers, "If-Modified-Since: ");
		DateTime_HttpDate(req->LastModified, headers);
		String_AppendConst(headers, "\r\n");
	}

	if (req->Etag[0]) {
		String etag = String_FromRawArray(req->Etag);
		String_AppendConst(headers, "If-None-Match: ");
		String_AppendString(headers, &etag);
		String_AppendConst(headers, "\r\n");
	}

	if (req->Data) {
		String_AppendConst(headers, "Content-Type: application/x-www-form-urlencoded\r\n");
	}

	String_AppendConst(headers, "\r\n\r\n");
	headers->buffer[headers->length] = '\0';
}

/* Creates and sends a HTTP requst */
static ReturnCode Http_StartRequest(struct HttpRequest* req, HINTERNET* handle) {
	const static char* verbs[3] = { "GET", "HEAD", "POST" };
	struct HttpCacheEntry entry;
	DWORD flags;

	String headers; char headersBuffer[STRING_SIZE * 4];
	String path;    char pathBuffer[URL_MAX_SIZE + 1];
	String url = String_FromRawArray(req->URL);

	HttpCache_MakeEntry(&url, &entry, &path);
	Mem_Copy(pathBuffer, path.buffer, path.length);
	pathBuffer[path.length] = '\0';

	HttpCache_Lookup(&entry);
	/* https://stackoverflow.com/questions/25308488/c-wininet-custom-http-headers */
	String_InitArray(headers, headersBuffer);
	Http_MakeHeaders(&headers, req);

	flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD;
	if (entry.Https) flags |= INTERNET_FLAG_SECURE;

	*handle = HttpOpenRequestA(entry.Handle, verbs[req->RequestType], 
								pathBuffer, NULL, NULL, NULL, flags, 0);

	return *handle && HttpSendRequestA(*handle, headers.buffer, headers.length, 
										req->Data, req->Size) ? 0 : GetLastError();
}

/* Gets headers from a HTTP response */
static ReturnCode Http_ProcessHeaders(struct HttpRequest* req, HINTERNET handle) {
	DWORD len;

	len = sizeof(DWORD);
	if (!HttpQueryInfoA(handle, FLAG_STATUS, &req->StatusCode, &len, NULL)) return GetLastError();

	len = sizeof(DWORD);
	HttpQueryInfoA(handle, FLAG_LENGTH, &req->ContentLength, &len, NULL);

	SYSTEMTIME sysTime;
	len = sizeof(SYSTEMTIME);
	if (HttpQueryInfoA(handle, FLAG_LASTMOD, &sysTime, &len, NULL)) {
		struct DateTime time;
		time.Year   = sysTime.wYear;   time.Month  = sysTime.wMonth;
		time.Day    = sysTime.wDay;    time.Hour   = sysTime.wHour;
		time.Minute = sysTime.wMinute; time.Second = sysTime.wSecond;
		time.Milli  = sysTime.wMilliseconds;
		req->LastModified = DateTime_TotalMs(&time);
	}

	String etag = String_ClearedArray(req->Etag);
	len = etag.capacity;
	HttpQueryInfoA(handle, HTTP_QUERY_ETAG, etag.buffer, &len, NULL);

	return 0;
}

/* Downloads the data/contents of a HTTP response */
static ReturnCode Http_DownloadData(struct HttpRequest* req, HINTERNET handle) {
	uint8_t* buffer;
	uint32_t size, totalRead;
	uint32_t read, avail;
	bool success;
	
	http_curProgress = 0;
	size      = req->ContentLength ? req->ContentLength : 1;
	buffer    = Mem_Alloc(size, 1, "http get data");
	totalRead = 0;

	req->Data = buffer;
	req->Size = 0;

	for (;;) {
		if (!InternetQueryDataAvailable(handle, &avail, 0, 0)) break;
		if (!avail) break;

		/* expand if buffer is too small (some servers don't give content length) */
		if (totalRead + avail > size) {
			size   = totalRead + avail;
			buffer = Mem_Realloc(buffer, size, 1, "http inc data");
			req->Data = buffer;
		}

		success = InternetReadFile(handle, &buffer[totalRead], avail, &read);
		if (!success) { Mem_Free(buffer); return GetLastError(); }
		if (!read) break;

		totalRead += read;
		if (req->ContentLength) http_curProgress = (int)(100.0f * totalRead / size);
		req->Size += read;
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
#endif
#ifdef CC_BUILD_POSIX
static CURL* curl;

static void Http_SysInit(void) {
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) Logger_Abort2(res, "Failed to init curl");

	curl = curl_easy_init();
	if (!curl) Logger_Abort("Failed to init easy curl");
}

/* Updates progress of current download */
static int Http_UpdateProgress(void* ptr, double total, double received, double a, double b) {
	if (total) http_curProgress = (int)(100 * received / total);
	return 0;
}

/* Makes custom request HTTP headers, usually none. */
static struct curl_slist* Http_MakeHeaders(struct HttpRequest* req) {
	String tmp; char buffer[STRING_SIZE + 1];
	struct curl_slist* list = NULL;
	String etag;

	if (req->Etag[0]) {
		String_InitArray_NT(tmp, buffer);
		String_AppendConst(&tmp, "If-None-Match: ");

		etag = String_FromRawArray(req->Etag);
		String_AppendString(&tmp, &etag);
		tmp.buffer[tmp.length] = '\0';
		list = curl_slist_append(list, tmp.buffer);
	}

	if (req->LastModified) {
		String_InitArray_NT(tmp, buffer);
		String_AppendConst(&tmp, "Last-Modified: ");

		DateTime_HttpDate(req->LastModified, &tmp);
		tmp.buffer[tmp.length] = '\0';
		list = curl_slist_append(list, tmp.buffer);
	}
	return list;
}

/* Processes a HTTP header downloaded from the server */
static size_t Http_ProcessHeader(char *buffer, size_t size, size_t nitems, struct HttpRequest* req) {
	String tmp; char tmpBuffer[STRING_SIZE + 1];
	String line, name, value;
	time_t time;

	if (size != 1) return size * nitems; /* non byte header */
	line = String_Init(buffer, nitems, nitems);
	if (!String_UNSAFE_Separate(&line, ':', &name, &value)) return nitems;

	/* value usually has \r\n at end */
	if (value.length && value.buffer[value.length - 1] == '\n') value.length--;
	if (value.length && value.buffer[value.length - 1] == '\r') value.length--;
	if (!value.length) return nitems;

	if (String_CaselessEqualsConst(&name, "ETag")) {
		tmp = String_ClearedArray(req->Etag);
		String_AppendString(&tmp, &value);
	} else if (String_CaselessEqualsConst(&name, "Content-Length")) {
		Convert_ParseInt(&value, &req->ContentLength);
	} else if (String_CaselessEqualsConst(&name, "Last-Modified")) {
		String_InitArray_NT(tmp, tmpBuffer);
		String_AppendString(&tmp, &value);
		tmp.buffer[tmp.length] = '\0';

		time = curl_getdate(tmp.buffer, NULL);
		if (time == -1) return nitems;
		req->LastModified = (uint64_t)time * 1000 + UNIX_EPOCH;
	}
	return nitems;
}

static int curlBufferSize;
/* Processes a chunk of data downloaded from the web server */
static size_t Http_ProcessData(char *buffer, size_t size, size_t nitems, struct HttpRequest* req) {
	uint8_t* dst;

	if (!curlBufferSize) {
		curlBufferSize = req->ContentLength ? req->ContentLength : 1;
		req->Data = Mem_Alloc(curlBufferSize, 1, "http get data");
		req->Size = 0;
	}

	/* expand buffer if needed */
	if (req->Size + nitems > curlBufferSize) {
		curlBufferSize = req->Size + nitems;
		req->Data      = Mem_Realloc(req->Data, curlBufferSize, 1, "http inc data");
	}

	dst = (uint8_t*)req->Data + req->Size;
	Mem_Copy(dst, buffer, nitems);
	req->Size  += nitems;
	return nitems;
}

/* Sets general curl options for a request */
static void Http_SetCurlOpts(struct HttpRequest* req) {
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR,      "");
	curl_easy_setopt(curl, CURLOPT_USERAGENT,      GAME_APP_NAME);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, Http_UpdateProgress);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_ProcessHeader);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_ProcessData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);
}

static ReturnCode Http_SysDo(struct HttpRequest* req) {
	String url = String_FromRawArray(req->URL);
	char urlStr[600];
	void* post_data = req->Data;
	struct curl_slist* list;
	long status = 0;
	CURLcode res;

	curl_easy_reset(curl);
	list = Http_MakeHeaders(req);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

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

	curlBufferSize = 0;
	http_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	res = curl_easy_perform(curl);
	http_curProgress = 100;

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	req->StatusCode = status;

	curl_slist_free_all(list);
	/* can free now that request has finished */
	Mem_Free(post_data);
	return res;
}

static void Http_SysFree(void) {
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}
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
		/* Block until another thread submits a req to do */
		if (!hasRequest) {
			Platform_LogConst("Going back to sleep...");
			Waitable_Wait(workerWaitable);
			continue;
		}
		Http_BeginRequest(&request);

		beg = Stopwatch_Measure();
		request.Result = Http_SysDo(&request);
		end = Stopwatch_Measure();

		Platform_Log3("HTTP: return code %i (http %i), in %i ms",
			&request.Result, &request.StatusCode, &elapsed);
		Http_FinishRequest(&request);
	}
}
#endif


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
const static String skinServer = String_FromConst("http://static.classicube.net/skins/");

void Http_AsyncGetSkin(const String* id, const String* skinName) {
	String url; char urlBuffer[STRING_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName, 0)) {
		String_Copy(&url, skinName);
	} else {
		String_AppendString(&url, &skinServer);
		String_AppendColorless(&url, skinName);
		String_AppendConst(&url, ".png");
	}
	Http_AsyncGetData(&url, false, id);
}

void Http_AsyncGetData(const String* url, bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, NULL, NULL, NULL, 0);
}
void Http_AsyncGetHeaders(const String* url, bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_HEAD, NULL, NULL, NULL, 0);
}
void Http_AsyncPostData(const String* url, bool priority, const String* id, const void* data, uint32_t size) {
	Http_Add(url, priority, id, REQUEST_TYPE_POST, NULL, NULL, data, size);
}
void Http_AsyncGetDataEx(const String* url, bool priority, const String* id, TimeMS* lastModified, const String* etag) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, lastModified, etag, NULL, 0);
}

void Http_PurgeOldEntriesTask(struct ScheduledTask* task) {
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
	Codepoint cp;
	int i, len;

	for (i = 0; i < src->length; i++) {
		cp  = Convert_CP437ToUnicode(src->buffer[i]);
		len = Convert_UnicodeToUtf8(cp, data);
		Http_UrlEncode(dst, data, len);
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_Init(void) {
	ScheduledTask_Add(30, Http_PurgeOldEntriesTask);
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
