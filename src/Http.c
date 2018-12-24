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

static void Http_SysInit(void) {
	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = InternetOpenA(GAME_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) Logger_Abort2(GetLastError(), "Failed to init WinINet");
}

static ReturnCode Http_SysMake(struct HttpRequest* req, HINTERNET* handle) {
	String url = String_FromRawArray(req->URL);
	char urlStr[URL_MAX_SIZE + 1];
	Mem_Copy(urlStr, url.buffer, url.length);

	urlStr[url.length] = '\0';
	char headersBuffer[STRING_SIZE * 3];
	String headers = String_FromArray(headersBuffer);

	/* https://stackoverflow.com/questions/25308488/c-wininet-custom-http-headers */
	if (req->Etag[0] || req->LastModified) {
		if (req->LastModified) {
			String_AppendConst(&headers, "If-Modified-Since: ");
			DateTime_HttpDate(req->LastModified, &headers);
			String_AppendConst(&headers, "\r\n");
		}

		if (req->Etag[0]) {
			String etag = String_FromRawArray(req->Etag);
			String_AppendConst(&headers, "If-None-Match: ");
			String_AppendString(&headers, &etag);
			String_AppendConst(&headers, "\r\n");
		}

		String_AppendConst(&headers, "\r\n\r\n");
		headers.buffer[headers.length] = '\0';
	} else { headers.buffer = NULL; }

	*handle = InternetOpenUrlA(hInternet, urlStr, headers.buffer, headers.length,
		INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD, 0);
	return *handle ? 0 : GetLastError();
}

static ReturnCode Http_SysGetHeaders(struct HttpRequest* req, HINTERNET handle) {
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

static ReturnCode Http_SysGetData(struct HttpRequest* req, HINTERNET handle, volatile int* progress) {
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
	ReturnCode res = Http_SysMake(req, &handle);
	if (res) return res;

	*progress = ASYNC_PROGRESS_FETCHING_DATA;
	res = Http_SysGetHeaders(req, handle);
	if (res) { InternetCloseHandle(handle); return res; }

	if (req->RequestType != REQUEST_TYPE_HEAD && req->StatusCode == 200) {
		res = Http_SysGetData(req, handle, progress);
		if (res) { InternetCloseHandle(handle); return res; }
	}

	return InternetCloseHandle(handle) ? 0 : GetLastError();
}

static ReturnCode Http_SysFree(void) {
	return InternetCloseHandle(hInternet) ? 0 : GetLastError();
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

static int Http_SysProgress(int* progress, double total, double received, double a, double b) {
	if (total == 0) return 0;
	*progress = (int)(100 * received / total);
	return 0;
}

static struct curl_slist* Http_SysMake(struct HttpRequest* req) {
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

static size_t Http_SysGetHeaders(char *buffer, size_t size, size_t nitems, struct HttpRequest* req) {
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
		/* TODO: Fix when ContentLength isn't RequestSize */
		req->Size = req->ContentLength;
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

static size_t Http_SysGetData(char *buffer, size_t size, size_t nitems, struct HttpRequest* req) {
	uint32_t total, left;
	uint8_t* dst;
		
	total = req->Size;
	if (!total || req->RequestType == REQUEST_TYPE_HEAD) return 0;
	if (!req->Data) req->Data = Mem_Alloc(total, 1, "http get data");

	/* reuse Result as an offset */
	left = total - req->Result;
	left = min(left, nitems);
	dst  = (uint8_t*)req->Data + req->Result;

	Mem_Copy(dst, buffer, left);
	req->Result += left;
	return nitems;
}

static ReturnCode Http_SysDo(struct HttpRequest* req, volatile int* progress) {
	struct curl_slist* list;
	String url = String_FromRawArray(req->URL);
	char urlStr[600];
	long status = 0;
	CURLcode res;

	Platform_ConvertString(urlStr, &url);
	curl_easy_reset(curl);
	list = Http_SysMake(req);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

	curl_easy_setopt(curl, CURLOPT_URL,            urlStr);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,      GAME_APP_NAME);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, Http_SysProgress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,     progress);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_SysGetHeaders);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_SysGetData);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);

	*progress = ASYNC_PROGRESS_FETCHING_DATA;
	res = curl_easy_perform(curl);
	*progress = 100;

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
	req->StatusCode = status;

	curl_slist_free_all(list);
	return res;
}

static ReturnCode Http_SysFree(void) {
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
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
bool Http_UseCookies;

/* Adds a request to the list of pending requests, waking up worker thread if needed. */
static void Http_Add(const String* url, bool priority, const String* id, uint8_t type, TimeMS* lastModified, const String* etag, const String* data) {
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
	/* request.Data = data; TODO: Implement this. do we need to copy or expect caller to malloc it?  */

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

	Http_Add(&url, false, id, REQUEST_TYPE_GET, NULL, NULL, NULL);
}

void Http_AsyncGetData(const String* url, bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, NULL, NULL, NULL);
}

void Http_AsyncGetHeaders(const String* url, bool priority, const String* id) {
	Http_Add(url, priority, id, REQUEST_TYPE_HEAD, NULL, NULL, NULL);
}

void AsyncDownloader_PostString(const String* url, bool priority, const String* id, const String* contents) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, NULL, NULL, contents);
}

void Http_AsyncGetDataEx(const String* url, bool priority, const String* id, TimeMS* lastModified, const String* etag) {
	Http_Add(url, priority, id, REQUEST_TYPE_GET, lastModified, etag, NULL);
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

static void Http_ProcessRequest(struct HttpRequest* request) {
	String url = String_FromRawArray(request->URL);
	uint64_t  beg, end;
	uint32_t  size, elapsed;
	uintptr_t addr;

	Platform_Log2("Downloading from %s (type %b)", &url, &request->RequestType);
	beg = Stopwatch_Measure();
	request->Result = Http_SysDo(request, &http_curProgress);
	end = Stopwatch_Measure();

	elapsed = Stopwatch_ElapsedMicroseconds(beg, end) / 1000;
	Platform_Log3("HTTP: return code %i (http %i), in %i ms", 
				&request->Result, &request->StatusCode, &elapsed);

	if (request->Data) {
		size = request->Size;
		addr = (uintptr_t)request->Data;
		Platform_Log2("HTTP returned data: %i bytes at %x", &size, &addr);
	}
}

static void Http_CompleteResult(struct HttpRequest* request) {
	struct HttpRequest older;
	String id = String_FromRawArray(request->ID);
	int index;
	request->TimeDownloaded = DateTime_CurrentUTC_MS();

	index = HttpRequestList_Find(&id, &older);
	if (index >= 0) {
		/* very rare case - priority item was inserted, then inserted again (so put before first item), */
		/* and both items got downloaded before an external function removed them from the queue */
		if (older.TimeAdded > request->TimeAdded) {
			HttpRequest_Free(request);
		} else {
			/* normal case, replace older request */
			HttpRequest_Free(&older);
			http_processed.Requests[index] = *request;				
		}
	} else {
		HttpRequestList_Append(&http_processed, request);
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
		/* Block until another thread submits a request to do */
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
		/* performing request doesn't need thread safety */
		Http_ProcessRequest(&request);

		Mutex_Lock(http_processedMutex);
		{
			Http_CompleteResult(&request);
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
