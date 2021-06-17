#include "Core.h"
#ifdef CC_BUILD_WEB
#include "_HttpBase.h"
#include <emscripten/emscripten.h>
#include "Errors.h"
extern void interop_DownloadAsync(const char* url, int method, int reqID);
extern int interop_IsHttpsOnly(void);


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
	int idx = RequestList_Find(&pendingReqs, reqID);
	if (idx == -1) return HTTP_PROGRESS_NOT_WORKING_ON;

	return pendingReqs.entries[idx].progress;
}

void Http_ClearPending(void) {
	RequestList_Free(&pendingReqs);
}

void Http_TryCancel(int reqID) {
	RequestList_TryFree(&pendingReqs,   reqID);
	RequestList_TryFree(&processedReqs, reqID);
}


/*########################################################################################################################*
*----------------------------------------------------Emscripten backend---------------------------------------------------*
*#########################################################################################################################*/
cc_bool Http_DescribeError(cc_result res, cc_string* dst) { return false; }

EMSCRIPTEN_KEEPALIVE void Http_OnUpdateProgress(int reqID, int read, int total) {
	int idx = RequestList_Find(&pendingReqs, reqID);
	if (idx == -1 || !total) return;

	pendingReqs.entries[idx].progress = (int)(100.0f * read / total);
}

EMSCRIPTEN_KEEPALIVE void Http_OnFinishedAsync(int reqID, void* data, int len, int status) {
	struct HttpRequest* req;
	int idx = RequestList_Find(&pendingReqs, reqID);

	/* Shouldn't ever happen, but log a warning anyways */
	if (idx == -1) {
		Mem_Free(data); Platform_Log1("Ignoring invalid request (%i)", &reqID); return;
	}

	req = &pendingReqs.entries[idx];
	req->data          = data;
	req->size          = len;
	req->statusCode    = status;
	req->contentLength = len;

	/* Usually this happens when denied by CORS */
	if (!status && !data) req->result = ERR_DOWNLOAD_INVALID;

	if (req->data) Platform_Log1("HTTP returned data: %i bytes", &req->size);
	Http_FinishRequest(req);
	RequestList_RemoveAt(&pendingReqs, idx);
}

/* Adds a req to the list of pending requests, waking up worker thread if needed */
static void Http_BackendAdd(struct HttpRequest* req, cc_bool priority) {
	char urlBuffer[URL_MAX_SIZE]; cc_string url;
	char urlStr[NATIVE_STR_LEN];
	String_InitArray(url, urlBuffer);

	RequestList_Append(&pendingReqs, req);
	Http_GetUrl(req, &url);
	Platform_Log2("Fetching %s (type %b)", &url, &req->requestType);

	Platform_EncodeUtf8(urlStr, &url);
	interop_DownloadAsync(urlStr, req->requestType, req->id);
}


/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void OnInit(void) {
	http_terminate = false;
	ScheduledTask_Add(30, Http_CleanCacheTask);
	/* If this webpage is https://, browsers deny any http:// downloading */
	httpsOnly = interop_IsHttpsOnly();

	RequestList_Init(&pendingReqs);
	RequestList_Init(&processedReqs);
}

static void OnFree(void) {
	http_terminate = true;
	Http_ClearPending();

	RequestList_Free(&pendingReqs);
	RequestList_Free(&processedReqs);
}

struct IGameComponent Http_Component = {
	OnInit,           /* Init  */
	OnFree,           /* Free  */
	Http_ClearPending /* Reset */
};
#endif
