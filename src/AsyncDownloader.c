#include "AsyncDownloader.h"
#include "Platform.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "GameStructs.h"

void ASyncRequest_Free(struct AsyncRequest* request) {
	Mem_Free(request->ResultData);
	request->ResultData = NULL;
	request->ResultSize = 0;
}

#define ASYNC_DEF_ELEMS 10
struct AsyncRequestList {
	int MaxElems, Count;
	struct AsyncRequest* Requests;
	struct AsyncRequest DefaultRequests[ASYNC_DEF_ELEMS];
};

static void AsyncRequestList_EnsureSpace(struct AsyncRequestList* list) {
	if (list->Count < list->MaxElems) return;
	list->Requests = Utils_Resize(list->Requests, &list->MaxElems,
									sizeof(struct AsyncRequest), ASYNC_DEF_ELEMS, 10);
}

static void AsyncRequestList_Append(struct AsyncRequestList* list, struct AsyncRequest* item) {
	AsyncRequestList_EnsureSpace(list);
	list->Requests[list->Count++] = *item;
}

static void AsyncRequestList_Prepend(struct AsyncRequestList* list, struct AsyncRequest* item) {
	int i;
	AsyncRequestList_EnsureSpace(list);
	
	for (i = list->Count; i > 0; i--) {
		list->Requests[i] = list->Requests[i - 1];
	}
	list->Requests[0] = *item;
	list->Count++;
}

static void AsyncRequestList_RemoveAt(struct AsyncRequestList* list, int i) {
	if (i < 0 || i >= list->Count) ErrorHandler_Fail("Tried to remove element at list end");

	for (; i < list->Count - 1; i++) {
		list->Requests[i] = list->Requests[i + 1];
	}
	list->Count--;
}

static void AsyncRequestList_Init(struct AsyncRequestList* list) {
	list->MaxElems = ASYNC_DEF_ELEMS;
	list->Count    = 0;
	list->Requests = list->DefaultRequests;
}

static void AsyncRequestList_Free(struct AsyncRequestList* list) {
	if (list->Requests != list->DefaultRequests) {
		Mem_Free(list->Requests);
	}
	AsyncRequestList_Init(list);
}

static void* async_waitable;
static void* async_workerThread;
static void* async_pendingMutex;
static void* async_processedMutex;
static void* async_curRequestMutex;
static volatile bool async_terminate;

static struct AsyncRequestList async_pending;
static struct AsyncRequestList async_processed;
static String async_skinServer = String_FromConst("http://static.classicube.net/skins/");
static struct AsyncRequest async_curRequest;
static volatile int async_curProgress = ASYNC_PROGRESS_NOTHING;
/* TODO: Implement these */
static bool ManageCookies;
static bool KeepAlive;
/* TODO: Connection pooling */

static void AsyncDownloader_Add(const String* url, bool priority, const String* id, uint8_t type, TimeMS* lastModified, const String* etag, const String* data) {
	struct AsyncRequest req = { 0 };
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

	Mutex_Lock(async_pendingMutex);
	{	
		req.TimeAdded = DateTime_CurrentUTC_MS();
		if (priority) {
			AsyncRequestList_Prepend(&async_pending, &req);
		} else {
			AsyncRequestList_Append(&async_pending, &req);
		}
	}
	Mutex_Unlock(async_pendingMutex);
	Waitable_Signal(async_waitable);
}

void AsyncDownloader_GetSkin(const String* id, const String* skinName) {
	String url; char urlBuffer[STRING_SIZE];
	String_InitArray(url, urlBuffer);

	if (Utils_IsUrlPrefix(skinName, 0)) {
		String_Copy(&url, skinName);
	} else {
		String_AppendString(&url, &async_skinServer);
		String_AppendColorless(&url, skinName);
		String_AppendConst(&url, ".png");
	}

	AsyncDownloader_Add(&url, false, id, REQUEST_TYPE_DATA, NULL, NULL, NULL);
}

void AsyncDownloader_GetData(const String* url, bool priority, const String* id) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_DATA, NULL, NULL, NULL);
}

void AsyncDownloader_GetContentLength(const String* url, bool priority, const String* id) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_CONTENT_LENGTH, NULL, NULL, NULL);
}

void AsyncDownloader_PostString(const String* url, bool priority, const String* id, const String* contents) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_DATA, NULL, NULL, contents);
}

void AsyncDownloader_GetDataEx(const String* url, bool priority, const String* id, TimeMS* lastModified, const String* etag) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_DATA, lastModified, etag, NULL);
}

void AsyncDownloader_PurgeOldEntriesTask(struct ScheduledTask* task) {
	struct AsyncRequest* item;
	int i;

	Mutex_Lock(async_processedMutex);
	{
		TimeMS now = DateTime_CurrentUTC_MS();
		for (i = async_processed.Count - 1; i >= 0; i--) {
			item = &async_processed.Requests[i];
			if (item->TimeDownloaded + (10 * 1000) >= now) continue;

			ASyncRequest_Free(item);
			AsyncRequestList_RemoveAt(&async_processed, i);
		}
	}
	Mutex_Unlock(async_processedMutex);
}

static int AsyncRequestList_Find(const String* id, struct AsyncRequest* item) {
	String reqID;
	int i;

	for (i = 0; i < async_processed.Count; i++) {
		reqID = String_FromRawArray(async_processed.Requests[i].ID);
		if (!String_Equals(id, &reqID)) continue;

		*item = async_processed.Requests[i];
		return i;
	}
	return -1;
}

bool AsyncDownloader_Get(const String* id, struct AsyncRequest* item) {
	int i;
	Mutex_Lock(async_processedMutex);
	{
		i = AsyncRequestList_Find(id, item);
		if (i >= 0) AsyncRequestList_RemoveAt(&async_processed, i);
	}
	Mutex_Unlock(async_processedMutex);
	return i >= 0;
}

bool AsyncDownloader_GetCurrent(struct AsyncRequest* request, int* progress) {
	Mutex_Lock(async_curRequestMutex);
	{
		*request  = async_curRequest;
		*progress = async_curProgress;
	}
	Mutex_Unlock(async_curRequestMutex);
	return request->ID[0];
}

static void AsyncDownloader_ProcessRequest(struct AsyncRequest* request) {
	String url = String_FromRawArray(request->URL);
	uint64_t  beg, end;
	uint32_t  size, elapsed;
	uintptr_t addr;

	Platform_Log2("Downloading from %s (type %b)", &url, &request->RequestType);
	beg = Stopwatch_Measure();
	request->Result = Http_Do(request, &async_curProgress);
	end = Stopwatch_Measure();

	elapsed = Stopwatch_ElapsedMicroseconds(beg, end) / 1000;
	Platform_Log3("HTTP: return code %i (http %i), in %i ms", 
				&request->Result, &request->StatusCode, &elapsed);

	if (request->ResultData) {
		size = request->ResultSize;
		addr = (uintptr_t)request->ResultData;
		Platform_Log2("HTTP returned data: %i bytes at %x", &size, &addr);
	}
}

static void AsyncDownloader_CompleteResult(struct AsyncRequest* request) {
	struct AsyncRequest older;
	String id = String_FromRawArray(request->ID);
	int index;
	request->TimeDownloaded = DateTime_CurrentUTC_MS();

	index = AsyncRequestList_Find(&id, &older);
	if (index >= 0) {
		/* very rare case - priority item was inserted, then inserted again (so put before first item), */
		/* and both items got downloaded before an external function removed them from the queue */
		if (older.TimeAdded > request->TimeAdded) {
			ASyncRequest_Free(request);
		} else {
			/* normal case, replace older request */
			ASyncRequest_Free(&older);
			async_processed.Requests[index] = *request;				
		}
	} else {
		AsyncRequestList_Append(&async_processed, request);
	}
}

static void AsyncDownloader_WorkerFunc(void) {
	struct AsyncRequest request;
	bool hasRequest, stop;

	for (;;) {
		hasRequest = false;

		Mutex_Lock(async_pendingMutex);
		{
			stop = async_terminate;
			if (!stop && async_pending.Count) {
				request = async_pending.Requests[0];
				hasRequest = true;
				AsyncRequestList_RemoveAt(&async_pending, 0);
			}
		}
		Mutex_Unlock(async_pendingMutex);

		if (stop) return;
		/* Block until another thread submits a request to do */
		if (!hasRequest) {
			Platform_LogConst("Going back to sleep...");
			Waitable_Wait(async_waitable);
			continue;
		}

		Platform_LogConst("Got something to do!");
		Mutex_Lock(async_curRequestMutex);
		{
			async_curRequest = request;
			async_curProgress = ASYNC_PROGRESS_MAKING_REQUEST;
		}
		Mutex_Unlock(async_curRequestMutex);

		Platform_LogConst("Doing it");
		/* performing request doesn't need thread safety */
		AsyncDownloader_ProcessRequest(&request);

		Mutex_Lock(async_processedMutex);
		{
			AsyncDownloader_CompleteResult(&request);
		}
		Mutex_Unlock(async_processedMutex);

		Mutex_Lock(async_curRequestMutex);
		{
			async_curRequest.ID[0] = '\0';
			async_curProgress = ASYNC_PROGRESS_NOTHING;
		}
		Mutex_Unlock(async_curRequestMutex);
	}
}


static void AsyncDownloader_Init(void) {
	AsyncRequestList_Init(&async_pending);
	AsyncRequestList_Init(&async_processed);
	Http_Init();

	async_waitable = Waitable_Create();
	async_pendingMutex    = Mutex_Create();
	async_processedMutex  = Mutex_Create();
	async_curRequestMutex = Mutex_Create();
	async_workerThread = Thread_Start(AsyncDownloader_WorkerFunc, false);
}

static void AsyncDownloader_Reset(void) {
	Mutex_Lock(async_pendingMutex);
	{
		AsyncRequestList_Free(&async_pending);
	}
	Mutex_Unlock(async_pendingMutex);
	Waitable_Signal(async_waitable);
}

static void AsyncDownloader_Free(void) {
	async_terminate = true;
	AsyncDownloader_Reset();
	Thread_Join(async_workerThread);

	AsyncRequestList_Free(&async_pending);
	AsyncRequestList_Free(&async_processed);
	Http_Free();

	Waitable_Free(async_waitable);
	Mutex_Free(async_pendingMutex);
	Mutex_Free(async_processedMutex);
	Mutex_Free(async_curRequestMutex);
}

void AsyncDownloader_MakeComponent(struct IGameComponent* comp) {
	comp->Init  = AsyncDownloader_Init;
	comp->Reset = AsyncDownloader_Reset;
	comp->Free  = AsyncDownloader_Free;
}
