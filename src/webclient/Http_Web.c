#include "../_HttpBase.h"
#include "../Errors.h"
#include <emscripten/emscripten.h>

extern int interop_DownloadAsync(const char* url, int method, int reqID);
extern int interop_IsHttpsOnly(void);

static struct RequestList workingReqs, queuedReqs;
static cc_uint64 startTime;


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Http_GetResult(int reqID, struct HttpRequest* item) {
	int i = RequestList_Find(&processedReqs, reqID);

	if (i >= 0) *item = processedReqs.entries[i];
	if (i >= 0) RequestList_RemoveAt(&processedReqs, i);
	return i >= 0;
}

cc_bool Http_GetCurrent(int* reqID, int* progress) {
	// TODO: Stubbed as this isn't required at the moment
	*progress = 0;
	return 0;
}

int Http_CheckProgress(int reqID) {
	int idx = RequestList_Find(&workingReqs, reqID);
	if (idx == -1) return HTTP_PROGRESS_NOT_WORKING_ON;

	return workingReqs.entries[idx].progress;
}

void Http_ClearPending(void) {
	RequestList_Free(&queuedReqs);
	RequestList_Free(&workingReqs);
}

void Http_TryCancel(int reqID) {
	RequestList_TryFree(&queuedReqs,    reqID);
	RequestList_TryFree(&workingReqs,   reqID);
	RequestList_TryFree(&processedReqs, reqID);
}


/*########################################################################################################################*
*----------------------------------------------------Emscripten backend---------------------------------------------------*
*#########################################################################################################################*/
static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst) { 
	return false; 
}

static const cc_string url_rewrite_srcs[] = {
	#define URL_REMAP_FUNC(src_base, src_host, dst_base, dst_host) String_FromConst(src_base),
	#include "../_HttpUrlMap.h"
};
static const char* url_rewrite_dsts[] = {
	#undef  URL_REMAP_FUNC
	#define URL_REMAP_FUNC(src_base, src_host, dst_base, dst_host) dst_base,
	#include "../_HttpUrlMap.h"
};

/* Converts e.g. "http://dl.dropbox.com/xyZ" into "https://dl.dropboxusercontent.com/xyZ" */
static void GetFinalUrl(struct HttpRequest* req, cc_string* dst) {
	cc_string url = String_FromRawArray(req->url);
	cc_string resource;
	int i;

	for (i = 0; i < Array_Elems(url_rewrite_srcs); i++) 
	{
		if (!String_CaselessStarts(&url, &url_rewrite_srcs[i])) continue;

		resource = String_UNSAFE_SubstringAt(&url, url_rewrite_srcs[i].length);
		String_Format2(dst, "%c%s", url_rewrite_dsts[i], &resource);
		return;
	}
	String_Copy(dst, &url);
}

#define HTTP_MAX_CONCURRENCY 6
static void Http_StartNextDownload(void) {
	char urlBuffer[URL_MAX_SIZE]; cc_string url;
	char urlStr[NATIVE_STR_LEN];
	struct HttpRequest* req;
	cc_result res;

	// Avoid making too many requests at once
	if (workingReqs.count >= HTTP_MAX_CONCURRENCY) return;
	if (!queuedReqs.count) return;
	String_InitArray(url, urlBuffer);

	req = &queuedReqs.entries[0];
	GetFinalUrl(req, &url);
	Platform_Log1("Fetching %s", &url);

	String_EncodeUtf8(urlStr, &url);
	res = interop_DownloadAsync(urlStr, req->requestType, req->id);

	if (res) {
		// interop error code -> ClassiCube error code
		if (res == 1) res = ERR_INVALID_DATA_URL;
		req->result = res;
		
		// Invalid URL so move onto next request
		Http_FinishRequest(req);
		RequestList_RemoveAt(&queuedReqs, 0);
		Http_StartNextDownload();
	} else {
		RequestList_Append(&workingReqs, req, false);
		RequestList_RemoveAt(&queuedReqs, 0);
	}
}

EMSCRIPTEN_KEEPALIVE void Http_OnUpdateProgress(int reqID, int read, int total) {
	int idx = RequestList_Find(&workingReqs, reqID);
	if (idx == -1 || !total) return;

	workingReqs.entries[idx].progress = (int)(100.0f * read / total);
}

EMSCRIPTEN_KEEPALIVE void Http_OnFinishedAsync(int reqID, void* data, int len, int status) {
	struct HttpRequest* req;
	int idx = RequestList_Find(&workingReqs, reqID);

	if (idx == -1) {
		// Shouldn't ever happen, but log a warning anyways
		Mem_Free(data); 
		Platform_Log1("Ignoring invalid request (%i)", &reqID);
	} else {
		req = &workingReqs.entries[idx];
		req->data          = data;
		req->size          = len;
		req->statusCode    = status;
		req->contentLength = len;

		// Usually this happens when denied by CORS
		if (!status && !data) req->result = ERR_DOWNLOAD_INVALID;

		if (req->data) Platform_Log1("HTTP returned data: %i bytes", &req->size);
		Http_FinishRequest(req);
		RequestList_RemoveAt(&workingReqs, idx);
	}
	Http_StartNextDownload();
}

// Adds a req to the list of pending requests, waking up worker thread if needed
static void HttpBackend_Add(struct HttpRequest* req, cc_uint8 flags) {
	// Add time based query string parameter to bypass browser cache
	if (flags & HTTP_FLAG_NOCACHE) {
		cc_string url = String_FromRawArray(req->url);
		int lo = (int)(startTime), hi = (int)(startTime >> 32);
		String_Format2(&url, "?t=%i%i", &hi, &lo);
	}

	RequestList_Append(&queuedReqs, req, flags);
	Http_StartNextDownload();
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_Init(void) {
	Http_InitCommon();
	// If this webpage is https://, browsers deny any http:// downloading
	httpsOnly = interop_IsHttpsOnly();
	startTime = DateTime_CurrentUTC();

	RequestList_Init(&queuedReqs);
	RequestList_Init(&workingReqs);
	RequestList_Init(&processedReqs);
}

