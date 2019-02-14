#include "Http.h"
#include "Platform.h"
#include "Funcs.h"
#include "Logger.h"
#include "Stream.h"
#include "GameStructs.h"

#ifdef CC_BUILD_WIN
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
#endif
/* POSIX is mainly shared between Linux and OSX */
#ifdef CC_BUILD_POSIX
#include <curl/curl.h>
#include <time.h>
#endif

void HttpRequest_Free(struct HttpRequest* request) {
	Mem_Free(request->Data);
	request->Data = NULL;
	request->Size = 0;
}


/*########################################################################################################################*
*-------------------------------------------------System/Native interface-------------------------------------------------*
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
static ReturnCode Http_DownloadData(struct HttpRequest* req, HINTERNET handle, volatile int* progress) {
	uint8_t* buffer;
	uint32_t size, totalRead;
	uint32_t read, avail;
	bool success;
	
	*progress = 0;
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
		if (req->ContentLength) *progress = (int)(100.0f * totalRead / size);
		req->Size += read;
	}

 	*progress = 100;
	return 0;
}

static ReturnCode Http_SysDo(struct HttpRequest* req, volatile int* progress) {
	HINTERNET handle;
	ReturnCode res = Http_StartRequest(req, &handle);
	HttpRequest_Free(req);
	if (res) return res;

	*progress = ASYNC_PROGRESS_FETCHING_DATA;
	res = Http_ProcessHeaders(req, handle);
	if (res) { InternetCloseHandle(handle); return res; }

	if (req->RequestType != REQUEST_TYPE_HEAD) {
		res = Http_DownloadData(req, handle, progress);
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
CURL* curl;

static void Http_SysInit(void) {
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) Logger_Abort2(res, "Failed to init curl");

	curl = curl_easy_init();
	if (!curl) Logger_Abort("Failed to init easy curl");
}

/* Updates progress of current download */
static int Http_UpdateProgress(int* progress, double total, double received, double a, double b) {
	if (total == 0) return 0;
	*progress = (int)(100 * received / total);
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

	/* TODO: Set progress */
	//if (req->ContentLength) *progress = (int)(100.0f * req->Size / curlBufferSize);
	return nitems;
}

/* Sets general curl options for a request */
static void Http_SetCurlOpts(struct HttpRequest* req, volatile int* progress) {
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR,      "");
	curl_easy_setopt(curl, CURLOPT_USERAGENT,      GAME_APP_NAME);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, Http_UpdateProgress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,     progress);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_ProcessHeader);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_ProcessData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);
}

static ReturnCode Http_SysDo(struct HttpRequest* req, volatile int* progress) {
	String url = String_FromRawArray(req->URL);
	char urlStr[600];
	void* post_data = req->Data;
	struct curl_slist* list;
	long status = 0;
	CURLcode res;

	curl_easy_reset(curl);
	list = Http_MakeHeaders(req);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

	Http_SetCurlOpts(req, progress);
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
	*progress = ASYNC_PROGRESS_FETCHING_DATA;
	res = curl_easy_perform(curl);
	*progress = 100;

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


/*########################################################################################################################*
*-------------------------------------------------Asynchronous downloader-------------------------------------------------*
*#########################################################################################################################*/
#define HTTP_DEF_ELEMS 10
struct HttpRequestList {
	int MaxElems, Count;
	struct HttpRequest* Requests;
	struct HttpRequest DefaultRequests[HTTP_DEF_ELEMS];
};

static void HttpRequestList_EnsureSpace(struct HttpRequestList* list) {
	if (list->Count < list->MaxElems) return;
	list->Requests = Utils_Resize(list->Requests, &list->MaxElems,
									sizeof(struct HttpRequest), HTTP_DEF_ELEMS, 10);
}

static void HttpRequestList_Append(struct HttpRequestList* list, struct HttpRequest* item) {
	HttpRequestList_EnsureSpace(list);
	list->Requests[list->Count++] = *item;
}

static void HttpRequestList_Prepend(struct HttpRequestList* list, struct HttpRequest* item) {
	int i;
	HttpRequestList_EnsureSpace(list);
	
	for (i = list->Count; i > 0; i--) {
		list->Requests[i] = list->Requests[i - 1];
	}
	list->Requests[0] = *item;
	list->Count++;
}

static void HttpRequestList_RemoveAt(struct HttpRequestList* list, int i) {
	if (i < 0 || i >= list->Count) Logger_Abort("Tried to remove element at list end");

	for (; i < list->Count - 1; i++) {
		list->Requests[i] = list->Requests[i + 1];
	}
	list->Count--;
}

static void HttpRequestList_Init(struct HttpRequestList* list) {
	list->MaxElems = HTTP_DEF_ELEMS;
	list->Count    = 0;
	list->Requests = list->DefaultRequests;
}

static void HttpRequestList_Free(struct HttpRequestList* list) {
	if (list->Requests != list->DefaultRequests) {
		Mem_Free(list->Requests);
	}
	HttpRequestList_Init(list);
}

static void* http_waitable;
static void* http_workerThread;
static void* http_pendingMutex;
static void* http_processedMutex;
static void* http_curRequestMutex;
static volatile bool http_terminate;

static struct HttpRequestList http_pending;
static struct HttpRequestList http_processed;
const static String http_skinServer = String_FromConst("http://static.classicube.net/skins/");
static struct HttpRequest http_curRequest;
static volatile int http_curProgress = ASYNC_PROGRESS_NOTHING;

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

	Mutex_Lock(http_pendingMutex);
	{	
		req.TimeAdded = DateTime_CurrentUTC_MS();
		if (priority) {
			HttpRequestList_Prepend(&http_pending, &req);
		} else {
			HttpRequestList_Append(&http_pending, &req);
		}
	}
	Mutex_Unlock(http_pendingMutex);
	Waitable_Signal(http_waitable);
}

void Http_AsyncGetSkin(const String* id, const String* skinName) {
	String url; char urlBuffer[STRING_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName, 0)) {
		String_Copy(&url, skinName);
	} else {
		String_AppendString(&url, &http_skinServer);
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

	Mutex_Lock(http_processedMutex);
	{
		TimeMS now = DateTime_CurrentUTC_MS();
		for (i = http_processed.Count - 1; i >= 0; i--) {
			item = &http_processed.Requests[i];
			if (item->TimeDownloaded + (10 * 1000) >= now) continue;

			HttpRequest_Free(item);
			HttpRequestList_RemoveAt(&http_processed, i);
		}
	}
	Mutex_Unlock(http_processedMutex);
}

static int HttpRequestList_Find(const String* id, struct HttpRequest* item) {
	String reqID;
	int i;

	for (i = 0; i < http_processed.Count; i++) {
		reqID = String_FromRawArray(http_processed.Requests[i].ID);
		if (!String_Equals(id, &reqID)) continue;

		*item = http_processed.Requests[i];
		return i;
	}
	return -1;
}

bool Http_GetResult(const String* id, struct HttpRequest* item) {
	int i;
	Mutex_Lock(http_processedMutex);
	{
		i = HttpRequestList_Find(id, item);
		if (i >= 0) HttpRequestList_RemoveAt(&http_processed, i);
	}
	Mutex_Unlock(http_processedMutex);
	return i >= 0;
}

bool Http_GetCurrent(struct HttpRequest* request, int* progress) {
	Mutex_Lock(http_curRequestMutex);
	{
		*request  = http_curRequest;
		*progress = http_curProgress;
	}
	Mutex_Unlock(http_curRequestMutex);
	return request->ID[0];
}

void Http_ClearPending(void) {
	Mutex_Lock(http_pendingMutex);
	{
		HttpRequestList_Free(&http_pending);
	}
	Mutex_Unlock(http_pendingMutex);
	Waitable_Signal(http_waitable);
}

/* Attempts to download the server's response headers and data to the given req */
static void Http_ProcessRequest(struct HttpRequest* req) {
	String url = String_FromRawArray(req->URL);
	uint64_t  beg, end;
	uint32_t  size, elapsed;
	uintptr_t addr;

	Platform_Log2("Downloading from %s (type %b)", &url, &req->RequestType);
	beg = Stopwatch_Measure();
	req->Result = Http_SysDo(req, &http_curProgress);
	end = Stopwatch_Measure();

	elapsed = (int)Stopwatch_ElapsedMicroseconds(beg, end) / 1000;
	Platform_Log3("HTTP: return code %i (http %i), in %i ms", 
				&req->Result, &req->StatusCode, &elapsed);

	if (req->Data) {
		size = req->Size;
		addr = (uintptr_t)req->Data;
		Platform_Log2("HTTP returned data: %i bytes at %x", &size, &addr);
	}
	req->Success = !req->Result && req->StatusCode == 200 && req->Data && req->Size;
}

/* Adds given req to list of processed/completed requests */
static void Http_CompleteRequest(struct HttpRequest* req) {
	struct HttpRequest older;
	String id = String_FromRawArray(req->ID);
	int index;
	req->TimeDownloaded = DateTime_CurrentUTC_MS();

	index = HttpRequestList_Find(&id, &older);
	if (index >= 0) {
		/* very rare case - priority item was inserted, then inserted again (so put before first item), */
		/* and both items got downloaded before an external function removed them from the queue */
		if (older.TimeAdded > req->TimeAdded) {
			HttpRequest_Free(req);
		} else {
			/* normal case, replace older req */
			HttpRequest_Free(&older);
			http_processed.Requests[index] = *req;				
		}
	} else {
		HttpRequestList_Append(&http_processed, req);
	}
}

static void Http_WorkerLoop(void) {
	struct HttpRequest request;
	bool hasRequest, stop;

	for (;;) {
		hasRequest = false;

		Mutex_Lock(http_pendingMutex);
		{
			stop = http_terminate;
			if (!stop && http_pending.Count) {
				request = http_pending.Requests[0];
				hasRequest = true;
				HttpRequestList_RemoveAt(&http_pending, 0);
			}
		}
		Mutex_Unlock(http_pendingMutex);

		if (stop) return;
		/* Block until another thread submits a req to do */
		if (!hasRequest) {
			Platform_LogConst("Going back to sleep...");
			Waitable_Wait(http_waitable);
			continue;
		}

		Platform_LogConst("Got something to do!");
		Mutex_Lock(http_curRequestMutex);
		{
			http_curRequest = request;
			http_curProgress = ASYNC_PROGRESS_MAKING_REQUEST;
		}
		Mutex_Unlock(http_curRequestMutex);

		Platform_LogConst("Doing it");
		/* performing req doesn't need thread safety */
		Http_ProcessRequest(&request);

		Mutex_Lock(http_processedMutex);
		{
			Http_CompleteRequest(&request);
		}
		Mutex_Unlock(http_processedMutex);

		Mutex_Lock(http_curRequestMutex);
		{
			http_curRequest.ID[0] = '\0';
			http_curProgress = ASYNC_PROGRESS_NOTHING;
		}
		Mutex_Unlock(http_curRequestMutex);
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_Init(void) {
	ScheduledTask_Add(30, Http_PurgeOldEntriesTask);
	HttpRequestList_Init(&http_pending);
	HttpRequestList_Init(&http_processed);
	Http_SysInit();

	http_waitable = Waitable_Create();
	http_pendingMutex    = Mutex_Create();
	http_processedMutex  = Mutex_Create();
	http_curRequestMutex = Mutex_Create();
	http_workerThread = Thread_Start(Http_WorkerLoop, false);
}

static void Http_Free(void) {
	http_terminate = true;
	Http_ClearPending();
	Thread_Join(http_workerThread);

	HttpRequestList_Free(&http_pending);
	HttpRequestList_Free(&http_processed);
	Http_SysFree();

	Waitable_Free(http_waitable);
	Mutex_Free(http_pendingMutex);
	Mutex_Free(http_processedMutex);
	Mutex_Free(http_curRequestMutex);
}

struct IGameComponent Http_Component = {
	Http_Init,        /* Init  */
	Http_Free,        /* Free  */
	Http_ClearPending /* Reset */
};
