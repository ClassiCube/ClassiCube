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
	if (!total) return;
	http_curProgress = (int)(100.0f * read / total);
}

EMSCRIPTEN_KEEPALIVE void Http_OnFinishedAsync(int reqID, void* data, int len, int status) {
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
	interop_DownloadAsync(urlStr, req->requestType, req->id);
}

static void Http_WorkerSignal(void) {
	struct HttpRequest req;
	if (http_terminate || !pendingReqs.count) return;
	/* already working on a request currently */
	if (http_curRequest.id) return;

	req = pendingReqs.entries[0];
	RequestList_RemoveAt(&pendingReqs, 0);
	Http_DownloadAsync(&req);
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
