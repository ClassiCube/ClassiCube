#include "Core.h"
#ifdef CC_BUILD_WEB
#include "_HttpBase.h"
#include <emscripten/emscripten.h>
#include "Errors.h"
extern void interop_DownloadAsync(const char* url, int method, int reqID);
extern int interop_IsHttpsOnly(void);
static struct RequestList workingReqs, queuedReqs;


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
	/* TODO: Stubbed as this isn't required at the moment */
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
cc_bool Http_DescribeError(cc_result res, cc_string* dst) { return false; }

#define HTTP_MAX_CONCURRENCY 6
static void Http_StartNextDownload(void) {
	char urlBuffer[URL_MAX_SIZE]; cc_string url;
	char urlStr[NATIVE_STR_LEN];
	struct HttpRequest* req;

	/* Avoid making too many requests at once */
	if (workingReqs.count >= HTTP_MAX_CONCURRENCY) return;
	if (!queuedReqs.count) return;
	String_InitArray(url, urlBuffer);

	req = &queuedReqs.entries[0];
	Http_GetUrl(req, &url);
	Platform_Log1("Fetching %s", &url);

	Platform_EncodeUtf8(urlStr, &url);
	interop_DownloadAsync(urlStr, req->requestType, req->id);
	RequestList_Append(&workingReqs, req, false);
	RequestList_RemoveAt(&queuedReqs, 0);
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
		/* Shouldn't ever happen, but log a warning anyways */
		Mem_Free(data); 
		Platform_Log1("Ignoring invalid request (%i)", &reqID);
	} else {
		req = &workingReqs.entries[idx];
		req->data          = data;
		req->size          = len;
		req->statusCode    = status;
		req->contentLength = len;

		/* Usually this happens when denied by CORS */
		if (!status && !data) req->result = ERR_DOWNLOAD_INVALID;

		if (req->data) Platform_Log1("HTTP returned data: %i bytes", &req->size);
		Http_FinishRequest(req);
		RequestList_RemoveAt(&workingReqs, idx);
	}
	Http_StartNextDownload();
}

/* Adds a req to the list of pending requests, waking up worker thread if needed */
static void HttpBackend_Add(struct HttpRequest* req, cc_bool priority) {
	RequestList_Append(&queuedReqs, req, priority);
	Http_StartNextDownload();
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_Init(void) {
	ScheduledTask_Add(30, Http_CleanCacheTask);
	/* If this webpage is https://, browsers deny any http:// downloading */
	httpsOnly = interop_IsHttpsOnly();

	RequestList_Init(&queuedReqs);
	RequestList_Init(&workingReqs);
	RequestList_Init(&processedReqs);
}
#endif
