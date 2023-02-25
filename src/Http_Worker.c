#include "Core.h"
#ifndef CC_BUILD_WEB
#include "_HttpBase.h"
static void* workerWaitable;
static void* workerThread;
static void* pendingMutex;
static struct RequestList pendingReqs;

static void* curRequestMutex;
static struct HttpRequest http_curRequest;
static volatile int http_curProgress = HTTP_PROGRESS_NOT_WORKING_ON;

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


/*########################################################################################################################*
*--------------------------------------------------Common downloader code-------------------------------------------------*
*#########################################################################################################################*/
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
	} else if (String_CaselessEqualsConst(&name, "X-Dropbox-Content-Length")) {
		/* dropbox stopped returning Content-Length header since switching to chunked transfer */
		/*  https://www.dropboxforum.com/t5/Discuss-Dropbox-Developer-API/Dropbox-media-can-t-be-access-by-azure-blob/td-p/575458 */
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

static void Http_SignalWorker(void) { Waitable_Signal(workerWaitable); }

/* Adds a req to the list of pending requests, waking up worker thread if needed */
static void HttpBackend_Add(struct HttpRequest* req, cc_uint8 flags) {
	Mutex_Lock(pendingMutex);
	{	
		RequestList_Append(&pendingReqs, req, flags);
	}
	Mutex_Unlock(pendingMutex);
	Http_SignalWorker();
}


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
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
#define CURLOPT_SSL_VERIFYPEER (0     + 64)
#define CURLOPT_MAXREDIRS      (0     + 68)
#define CURLOPT_HEADERFUNCTION (20000 + 79)
#define CURLOPT_HTTPGET        (0     + 80)
#define CURLOPT_SSL_VERIFYHOST (0     + 81)
#define CURLOPT_HTTP_VERSION   (0     + 84)

#define CURL_HTTP_VERSION_1_1   2L /* stick to HTTP 1.1 */

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
#elif defined CC_BUILD_NETBSD
static const cc_string curlLib = String_FromConst("libcurl.so");
static const cc_string curlAlt = String_FromConst("/usr/pkg/lib/libcurl.so");
#elif defined CC_BUILD_BSD
static const cc_string curlLib = String_FromConst("libcurl.so");
static const cc_string curlAlt = String_FromConst("libcurl.so");
#elif defined CC_BUILD_SERENITY
static const cc_string curlLib = String_FromConst("/usr/local/lib/libcurl.so");
static const cc_string curlAlt = String_FromConst("/usr/local/lib/libcurl.so");
#else
static const cc_string curlLib = String_FromConst("libcurl.so.4");
static const cc_string curlAlt = String_FromConst("libcurl.so.3");
#endif

static cc_bool LoadCurlFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(curl_global_init),    DynamicLib_Sym(curl_global_cleanup),
		DynamicLib_Sym(curl_easy_init),      DynamicLib_Sym(curl_easy_perform),
		DynamicLib_Sym(curl_easy_setopt),    DynamicLib_Sym(curl_easy_cleanup),
		DynamicLib_Sym(curl_slist_free_all), DynamicLib_Sym(curl_slist_append)
	};
	cc_bool success;
	void* lib;

	success = DynamicLib_LoadAll(&curlLib,     funcs, Array_Elems(funcs), &lib);
	if (!lib) { 
		success = DynamicLib_LoadAll(&curlAlt, funcs, Array_Elems(funcs), &lib);
	}

	/* Non-essential function missing in older curl versions */
	_curl_easy_strerror = DynamicLib_Get2(lib, "curl_easy_strerror");
	return success;
}

static CURL* curl;
static cc_bool curlSupported, curlVerbose;

static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst) {
	const char* err;
	
	if (!_curl_easy_strerror) return false;
	err = _curl_easy_strerror((CURLcode)res);
	if (!err) return false;

	String_AppendConst(dst, err);
	return true;
}

static void HttpBackend_Init(void) {
	static const cc_string msg = String_FromConst("Failed to init libcurl. All HTTP requests will therefore fail.");
	CURLcode res;

	if (!LoadCurlFuncs()) { Logger_WarnFunc(&msg); return; }
	res = _curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res) { Logger_SimpleWarn(res, "initing curl"); return; }

	curl = _curl_easy_init();
	if (!curl) { Logger_SimpleWarn(res, "initing curl_easy"); return; }

	curlSupported = true;
	curlVerbose = Options_GetBool("curl-verbose", false);
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
	_curl_easy_setopt(curl, CURLOPT_HTTP_VERSION,   CURL_HTTP_VERSION_1_1);

	_curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Http_ProcessHeader);
	_curl_easy_setopt(curl, CURLOPT_HEADERDATA,     req);
	_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  Http_ProcessData);
	_curl_easy_setopt(curl, CURLOPT_WRITEDATA,      req);

	if (curlVerbose) _curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	if (httpsVerify) return;
	_curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
}

static cc_result HttpBackend_Do(struct HttpRequest* req, cc_string* url) {
	char urlStr[NATIVE_STR_LEN];
	void* post_data = req->data;
	CURLcode res;
	if (!curlSupported) return ERR_NOT_SUPPORTED;

	req->meta = NULL;
	Http_SetRequestHeaders(req);
	_curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req->meta);

	Http_SetCurlOpts(req);
	String_EncodeUtf8(urlStr, url);
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

	/* must be at least CURL_ERROR_SIZE (256) in size */
	req->error = Mem_TryAllocCleared(257, 1);
	_curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, req->error);
	/* TODO stackalloc instead and then copy to dynamic array later? */
	/*  probably not worth the extra complexity though */

	req->_capacity   = 0;
	http_curProgress = HTTP_PROGRESS_FETCHING_DATA;
	res = _curl_easy_perform(curl);
	http_curProgress = 100;

	/* Free error string if it isn't needed */
	if (req->error && !req->error[0]) {
		Mem_Free(req->error);
		req->error = NULL;
	}

	_curl_slist_free_all((struct curl_slist*)req->meta);
	/* can free now that request has finished */
	Mem_Free(post_data);
	_curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, NULL);
	return res;
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
#include "Errors.h"

/* === BEGIN wininet.h === */
#define INETAPI DECLSPEC_IMPORT
typedef PVOID HINTERNET;
typedef WORD INTERNET_PORT;

#define INTERNET_OPEN_TYPE_PRECONFIG  0   // use registry configuration
#define INTERNET_OPEN_TYPE_DIRECT     1   // direct to net
#define INTERNET_SERVICE_HTTP         3

#define HTTP_ADDREQ_FLAG_ADD         0x20000000
#define HTTP_ADDREQ_FLAG_REPLACE     0x80000000

#define INTERNET_FLAG_RELOAD         0x80000000
#define INTERNET_FLAG_NO_CACHE_WRITE 0x04000000
#define INTERNET_FLAG_SECURE         0x00800000
#define INTERNET_FLAG_NO_COOKIES     0x00080000
#define INTERNET_FLAG_NO_UI          0x00000200

#define SECURITY_FLAG_IGNORE_REVOCATION      0x00000080
#define SECURITY_FLAG_IGNORE_UNKNOWN_CA      0x00000100
#define SECURITY_FLAG_IGNORE_CERT_CN_INVALID 0x00001000
#define HTTP_QUERY_RAW_HEADERS          21
#define INTERNET_OPTION_SECURITY_FLAGS  31

static BOOL      (WINAPI *_InternetCloseHandle)(HINTERNET hInternet);
static HINTERNET (WINAPI *_InternetConnectA)(HINTERNET hInternet, PCSTR serverName, INTERNET_PORT serverPort, PCSTR userName, PCSTR password, DWORD service, DWORD flags, DWORD_PTR context);
static HINTERNET (WINAPI *_InternetOpenA)(PCSTR agent, DWORD accessType, PCSTR lpszProxy, PCSTR proxyBypass, DWORD flags);
static BOOL      (WINAPI *_InternetQueryOptionA)(HINTERNET hInternet, DWORD option, PVOID buffer, DWORD* bufferLength);
static BOOL      (WINAPI *_InternetSetOptionA)(HINTERNET hInternet, DWORD option, PVOID buffer, DWORD bufferLength);
static BOOL      (WINAPI *_InternetQueryDataAvailable)(HINTERNET hFile, DWORD* numBytesAvailable, DWORD flags, DWORD_PTR context);
static BOOL      (WINAPI *_InternetReadFile)(HINTERNET hFile, PVOID buffer, DWORD numBytesToRead, DWORD* numBytesRead);
static BOOL      (WINAPI *_HttpQueryInfoA)(HINTERNET hRequest, DWORD infoLevel, PVOID buffer, DWORD* bufferLength, DWORD* index);
static BOOL      (WINAPI *_HttpAddRequestHeadersA)(HINTERNET hRequest, PCSTR headers, DWORD headersLength, DWORD modifiers);
static HINTERNET (WINAPI *_HttpOpenRequestA)(HINTERNET hConnect, PCSTR verb, PCSTR objectName, PCSTR version, PCSTR referrer, PCSTR* acceptTypes, DWORD flags, DWORD_PTR context);
static BOOL      (WINAPI *_HttpSendRequestA)(HINTERNET hRequest, PCSTR headers, DWORD headersLength, PVOID optional, DWORD optionalLength);
/* === END wininet.h === */

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
static HINTERNET hInternet;

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

	String_Append(resource, '/');
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
	conn = _InternetConnectA(hInternet, e->Address.buffer, e->Port, NULL, NULL, 
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
	_InternetCloseHandle(http_cache[i].Handle);
	return HttpCache_Insert(i, e);
}

static void* wininet_lib;
static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst) {
	return Platform_DescribeErrorExt(res, dst, wininet_lib);
}

static void HttpBackend_Init(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(InternetCloseHandle), 
		DynamicLib_Sym(InternetConnectA),   DynamicLib_Sym(InternetOpenA),       
		DynamicLib_Sym(InternetSetOptionA), DynamicLib_Sym(InternetQueryOptionA),
		DynamicLib_Sym(InternetReadFile),   DynamicLib_Sym(InternetQueryDataAvailable),
		DynamicLib_Sym(HttpOpenRequestA),   DynamicLib_Sym(HttpSendRequestA),   
		DynamicLib_Sym(HttpQueryInfoA),     DynamicLib_Sym(HttpAddRequestHeadersA)

	};
	static const cc_string wininet = String_FromConst("wininet.dll");
	DynamicLib_LoadAll(&wininet, funcs, Array_Elems(funcs), &wininet_lib);
	if (!wininet_lib) return;

	/* TODO: Should we use INTERNET_OPEN_TYPE_PRECONFIG instead? */
	hInternet = _InternetOpenA(GAME_APP_NAME, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) Logger_Abort2(GetLastError(), "Failed to init WinINet");
}

static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) {
	cc_string tmp; char tmpBuffer[1024];
	String_InitArray(tmp, tmpBuffer);

	String_Format2(&tmp, "%c: %s\r\n", key, value);
	_HttpAddRequestHeadersA((HINTERNET)req->meta, tmp.buffer, tmp.length, 
							HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
}

/* Creates and sends a HTTP request */
static cc_result Http_StartRequest(struct HttpRequest* req, cc_string* url) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	struct HttpCacheEntry entry;
	cc_string path; char pathBuffer[URL_MAX_SIZE + 1];
	DWORD flags, bufferLen;
	HINTERNET handle;
	cc_result res;

	String_InitArray_NT(path, pathBuffer);
	HttpCache_MakeEntry(url, &entry, &path);
	pathBuffer[path.length] = '\0';

	if (!wininet_lib) return ERR_NOT_SUPPORTED;
	if ((res = HttpCache_Lookup(&entry))) return res;

	flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_COOKIES;
	if (entry.Https) flags |= INTERNET_FLAG_SECURE;

	handle = _HttpOpenRequestA(entry.Handle, verbs[req->requestType], pathBuffer, NULL, NULL, NULL, flags, 0);
	/* Fallback for Windows 95 which returns ERROR_INVALID_PARAMETER */
	if (!handle) {
		flags &= ~INTERNET_FLAG_NO_UI; /* INTERNET_FLAG_NO_UI unsupported on Windows 95 */
		handle = _HttpOpenRequestA(entry.Handle, verbs[req->requestType], pathBuffer, NULL, NULL, NULL, flags, 0);
		if (!handle) return GetLastError();
	}
	req->meta = handle;

	bufferLen = sizeof(flags);
	_InternetQueryOptionA(handle, INTERNET_OPTION_SECURITY_FLAGS, (void*)&bufferLen, &flags);
	/* Users have had issues in the past with revocation servers randomly being offline, */
	/*  which caused all https:// requests to fail. So just skip revocation check. */
	flags |= SECURITY_FLAG_IGNORE_REVOCATION;

	if (!httpsVerify) flags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
	_InternetSetOptionA(handle,   INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

	Http_SetRequestHeaders(req);
	return _HttpSendRequestA(handle, NULL, 0, req->data, req->size) ? 0 : GetLastError();
}

/* Gets headers from a HTTP response */
static cc_result Http_ProcessHeaders(struct HttpRequest* req, HINTERNET handle) {
	cc_string left, line;
	char buffer[8192];
	DWORD len = 8192;
	if (!_HttpQueryInfoA(handle, HTTP_QUERY_RAW_HEADERS, buffer, &len, NULL)) return GetLastError();

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
		if (!_InternetQueryDataAvailable(handle, &avail, 0, 0)) return GetLastError();
		if (!avail) break;
		Http_BufferEnsure(req, avail);

		if (!_InternetReadFile(handle, &req->data[req->size], avail, &read)) return GetLastError();
		if (!read) break;
		Http_BufferExpanded(req, read);
	}

 	http_curProgress = 100;
	return 0;
}

static cc_result HttpBackend_Do(struct HttpRequest* req, cc_string* url) {
	HINTERNET handle;
	cc_result res = Http_StartRequest(req, url);
	HttpRequest_Free(req);
	if (res) return res;

	handle = req->meta;
	http_curProgress = HTTP_PROGRESS_FETCHING_DATA;
	res = Http_ProcessHeaders(req, handle);
	if (res) { _InternetCloseHandle(handle); return res; }

	if (req->requestType != REQUEST_TYPE_HEAD) {
		res = Http_DownloadData(req, handle);
		if (res) { _InternetCloseHandle(handle); return res; }
	}

	return _InternetCloseHandle(handle) ? 0 : GetLastError();
}
#elif defined CC_BUILD_ANDROID
/*########################################################################################################################*
*-----------------------------------------------------Android backend-----------------------------------------------------*
*#########################################################################################################################*/
struct HttpRequest* java_req;
static jmethodID JAVA_httpInit, JAVA_httpSetHeader, JAVA_httpPerform, JAVA_httpSetData;
static jmethodID JAVA_httpDescribeError;

static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst) {
	char buffer[NATIVE_STR_LEN];
	cc_string err;
	JNIEnv* env;
	jvalue args[1];
	jobject obj;
	
	JavaGetCurrentEnv(env);
	args[0].i = res;
	obj       = JavaSCall_Obj(env, JAVA_httpDescribeError, args);
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

	JavaSCall_Void(env, JAVA_httpSetHeader, args);
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

static const JNINativeMethod methods[] = {
	{ "httpParseHeader", "(Ljava/lang/String;)V", java_HttpParseHeader },
	{ "httpAppendData",  "([BI)V",                java_HttpAppendData }
};
static void CacheMethodRefs(JNIEnv* env) {
	JAVA_httpInit      = JavaGetSMethod(env, "httpInit",      "(Ljava/lang/String;Ljava/lang/String;)I");
	JAVA_httpSetHeader = JavaGetSMethod(env, "httpSetHeader", "(Ljava/lang/String;Ljava/lang/String;)V");
	JAVA_httpPerform   = JavaGetSMethod(env, "httpPerform",   "()I");
	JAVA_httpSetData   = JavaGetSMethod(env, "httpSetData",   "([B)I");

	JAVA_httpDescribeError = JavaGetSMethod(env, "httpDescribeError", "(I)Ljava/lang/String;");
}

static void HttpBackend_Init(void) {
	JNIEnv* env;
	JavaGetCurrentEnv(env);
	JavaRegisterNatives(env, methods);
	CacheMethodRefs(env);
}

static cc_result Http_InitReq(JNIEnv* env, struct HttpRequest* req, cc_string* url) {
	static const char* verbs[3] = { "GET", "HEAD", "POST" };
	jvalue args[2];
	jint res;

	args[0].l = JavaMakeString(env, url);
	args[1].l = JavaMakeConst(env,  verbs[req->requestType]);

	res = JavaSCall_Int(env, JAVA_httpInit, args);
	(*env)->DeleteLocalRef(env, args[0].l);
	(*env)->DeleteLocalRef(env, args[1].l);
	return res;
}

static cc_result Http_SetData(JNIEnv* env, struct HttpRequest* req) {
	jvalue args[1];
	jint res;

	args[0].l = JavaMakeBytes(env, req->data, req->size);
	res = JavaSCall_Int(env, JAVA_httpSetData, args);
	(*env)->DeleteLocalRef(env, args[0].l);
	return res;
}

static cc_result HttpBackend_Do(struct HttpRequest* req, cc_string* url) {
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
	res = JavaSCall_Int(env, JAVA_httpPerform, NULL);
	http_curProgress = 100;
	return res;
}
#elif defined CC_BUILD_CFNETWORK
/*########################################################################################################################*
*----------------------------------------------------CFNetwork backend----------------------------------------------------*
*#########################################################################################################################*/
#include "Errors.h"
#include <stddef.h>
#include <CFNetwork/CFNetwork.h>

static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst) {
    return false;
}

static void HttpBackend_Init(void) {
    
}

static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) {
    char tmp[NATIVE_STR_LEN];
    CFStringRef keyCF, valCF;
    CFHTTPMessageRef msg = (CFHTTPMessageRef)req->meta;
    String_EncodeUtf8(tmp, value);
    
    keyCF = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
    valCF = CFStringCreateWithCString(NULL, tmp, kCFStringEncodingUTF8);
    CFHTTPMessageSetHeaderFieldValue(msg, keyCF, valCF);
    CFRelease(keyCF);
    CFRelease(valCF);
}

static void Http_CheckHeader(const void* k, const void* v, void* ctx) {
    cc_string line; char lineBuffer[2048];
    char keyBuf[128]  = { 0 };
    char valBuf[1024] = { 0 };
    String_InitArray(line, lineBuffer);
    
    CFStringGetCString((CFStringRef)k, keyBuf, sizeof(keyBuf), kCFStringEncodingUTF8);
    CFStringGetCString((CFStringRef)v, valBuf, sizeof(valBuf), kCFStringEncodingUTF8);
    
    String_Format2(&line, "%c:%c", keyBuf, valBuf);
    Http_ParseHeader((struct HttpRequest*)ctx, &line);
    ctx = NULL;
}

static cc_result ParseResponseHeaders(struct HttpRequest* req, CFReadStreamRef stream) {
    CFHTTPMessageRef response = CFReadStreamCopyProperty(stream, kCFStreamPropertyHTTPResponseHeader);
    if (!response) return ERR_INVALID_ARGUMENT;
    
    CFDictionaryRef headers = CFHTTPMessageCopyAllHeaderFields(response);
    CFDictionaryApplyFunction(headers, Http_CheckHeader, req);
    req->statusCode = CFHTTPMessageGetResponseStatusCode(response);
    
    CFRelease(headers);
    CFRelease(response);
    return 0;
}

static cc_result HttpBackend_Do(struct HttpRequest* req, cc_string* url) {
    static const cc_string userAgent = String_FromConst(GAME_APP_NAME);
    static CFStringRef verbs[] = { CFSTR("GET"), CFSTR("HEAD"), CFSTR("POST") };
    cc_bool gotHeaders = false;
    char tmp[NATIVE_STR_LEN];
    CFHTTPMessageRef request;
    CFStringRef urlCF;
    CFURLRef urlRef;
    cc_result result = 0;
    
    String_EncodeUtf8(tmp, url);
    urlCF  = CFStringCreateWithCString(NULL, tmp, kCFStringEncodingUTF8);
    urlRef = CFURLCreateWithString(NULL, urlCF, NULL);
    // TODO e.g. "http://www.example.com/skin/1 2.png" causes this to return null
    // TODO release urlCF
    if (!urlRef) return ERR_INVALID_DATA_URL;
    
    request = CFHTTPMessageCreateRequest(NULL, verbs[req->requestType], urlRef, kCFHTTPVersion1_1);
    req->meta = request;
    Http_SetRequestHeaders(req);
    Http_AddHeader(req, "User-Agent", &userAgent);
    CFRelease(urlRef);
    
    if (req->data && req->size) {
        CFDataRef body = CFDataCreate(NULL, req->data, req->size);
        CFHTTPMessageSetBody(request, body);
        CFRelease(body); /* TODO: ???? */
        
        req->data = NULL;
        req->size = 0;
        Mem_Free(req->data);
    }
    
    CFReadStreamRef stream = CFReadStreamCreateForHTTPRequest(NULL, request);
    CFReadStreamSetProperty(stream, kCFStreamPropertyHTTPShouldAutoredirect, kCFBooleanTrue);
    //CFHTTPReadStreamSetRedirectsAutomatically(stream, TRUE);
    CFReadStreamOpen(stream);
    UInt8 buf[1024];
    
    for (;;) {
        CFIndex read = CFReadStreamRead(stream, buf, sizeof(buf));
        if (read <= 0) break;
        
        // reading headers before for loop doesn't work
        if (!gotHeaders) {
            gotHeaders = true;
            if ((result = ParseResponseHeaders(req, stream))) break;
        }
        
        if (!req->_capacity) Http_BufferInit(req);
        Http_BufferEnsure(req, read);
        
        Mem_Copy(&req->data[req->size], buf, read);
        Http_BufferExpanded(req, read);
    }
    
    if (!gotHeaders)
        result = ParseResponseHeaders(req, stream);
    
    //Thread_Sleep(1000);
    CFRelease(request);
    return result;
}
#endif

static void ClearCurrentRequest(void) {
	Mutex_Lock(curRequestMutex);
	{
		http_curRequest.id = 0;
		http_curProgress   = HTTP_PROGRESS_NOT_WORKING_ON;
	}
	Mutex_Unlock(curRequestMutex);
}

static void WorkerLoop(void) {
	char urlBuffer[URL_MAX_SIZE]; cc_string url;
	struct HttpRequest request;
	cc_bool hasRequest;
	cc_uint64 beg, end;
	int elapsed;

	for (;;) {
		hasRequest = false;

		Mutex_Lock(pendingMutex);
		{
			if (pendingReqs.count) {
				request    = pendingReqs.entries[0];
				hasRequest = true;
				RequestList_RemoveAt(&pendingReqs, 0);
			}
		}
		Mutex_Unlock(pendingMutex);

		/* Block until another thread submits a request to do */
		if (!hasRequest) {
			Platform_LogConst("Going back to sleep...");
			Waitable_Wait(workerWaitable);
			continue;
		}

		String_InitArray(url, urlBuffer);
		Http_BeginRequest(&request, &url);

		beg = Stopwatch_Measure();
		request.result = HttpBackend_Do(&request, &url);
		end = Stopwatch_Measure();

		elapsed = Stopwatch_ElapsedMS(beg, end);
		Platform_Log4("HTTP: result %i (http %i) in %i ms (%i bytes)",
					&request.result, &request.statusCode, &elapsed, &request.size);

		Http_FinishRequest(&request);
		ClearCurrentRequest();
	}
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_Init(void) {
	Http_InitCommon();
	/* Http component gets initialised multiple times on Android */
	if (workerThread) return;

	HttpBackend_Init();
	workerWaitable = Waitable_Create();
	RequestList_Init(&pendingReqs);
	RequestList_Init(&processedReqs);

	pendingMutex    = Mutex_Create();
	processedMutex  = Mutex_Create();
	curRequestMutex = Mutex_Create();
	workerThread    = Thread_Create(WorkerLoop);

	Thread_Start2(workerThread, WorkerLoop);
}
#endif
