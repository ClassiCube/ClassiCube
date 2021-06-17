#include "Core.h"
#ifdef CC_BUILD_WEB
#include "_HttpBase.h"
#include <emscripten/emscripten.h>
#include "Errors.h"
extern void interop_DownloadAsync(const char* url, int method);
extern int interop_IsHttpsOnly(void);

cc_bool Http_DescribeError(cc_result res, cc_string* dst) { return false; }
/* web browsers do caching already, so don't need last modified/etags */
static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) { }

EMSCRIPTEN_KEEPALIVE void Http_OnUpdateProgress(int read, int total) {
	if (!total) return;
	http_curProgress = (int)(100.0f * read / total);
}

EMSCRIPTEN_KEEPALIVE void Http_OnFinishedAsync(void* data, int len, int status) {
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
	interop_DownloadAsync(urlStr, req->requestType);
}

static void Http_WorkerInit(void) {
	/* If this webpage is https://, browsers deny any http:// downloading */
	httpsOnly = interop_IsHttpsOnly();
}
static void Http_WorkerStart(void) { }
static void Http_WorkerStop(void)  { }

static void Http_WorkerSignal(void) {
	struct HttpRequest req;
	if (http_terminate || !pendingReqs.count) return;
	/* already working on a request currently */
	if (http_curRequest.id) return;

	req = pendingReqs.entries[0];
	RequestList_RemoveAt(&pendingReqs, 0);
	Http_DownloadAsync(&req);
}
#endif
