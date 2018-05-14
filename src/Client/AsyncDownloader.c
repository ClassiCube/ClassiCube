#include "AsyncDownloader.h"
#include "Platform.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Stream.h"

void ASyncRequest_Free(AsyncRequest* request) {
	switch (request->RequestType) {
	case REQUEST_TYPE_IMAGE:
		Platform_MemFree(&request->ResultBitmap.Scan0);
		break;
	case REQUEST_TYPE_DATA:
		Platform_MemFree(&request->ResultData.Ptr);
		break;
	case REQUEST_TYPE_STRING:
		Platform_MemFree(&request->ResultString.buffer);
		break;
	}
}

#define ASYNCREQUESTLIST_DEFELEMS 10
typedef struct AsyncRequestList_ {
	UInt32 MaxElems, Count;
	AsyncRequest* Requests;
	AsyncRequest DefaultRequests[ASYNCREQUESTLIST_DEFELEMS];
} AsyncRequestList;

void AsyncRequestList_EnsureSpace(AsyncRequestList* list) {
	if (list->Count < list->MaxElems) return;
	StringsBuffer_Resize(&list->Requests, &list->MaxElems, sizeof(AsyncRequest),
		ASYNCREQUESTLIST_DEFELEMS, 10);
}

void AsyncRequestList_Append(AsyncRequestList* list, AsyncRequest* item) {
	AsyncRequestList_EnsureSpace(list);
	list->Requests[list->Count++] = *item;
}

void AsyncRequestList_Prepend(AsyncRequestList* list, AsyncRequest* item) {
	AsyncRequestList_EnsureSpace(list);
	UInt32 i;
	for (i = list->Count; i > 0; i--) {
		list->Requests[i] = list->Requests[i - 1];
	}
	list->Requests[0] = *item;
	list->Count++;
}

void AsyncRequestList_RemoveAt(AsyncRequestList* list, UInt32 i) {
	if (i >= list->Count) ErrorHandler_Fail("Tried to remove element at list end");

	for (; i < list->Count - 1; i++) {
		list->Requests[i] = list->Requests[i + 1];
	}
	list->Count--;
}

void AsyncRequestList_Init(AsyncRequestList* list) {
	list->MaxElems = ASYNCREQUESTLIST_DEFELEMS;
	list->Count = 0;
	list->Requests = list->DefaultRequests;
}

void AsyncRequestList_Free(AsyncRequestList* list) {
	if (list->Requests != list->DefaultRequests) {
		Platform_MemFree(&list->Requests);
	}
	AsyncRequestList_Init(list);
}

void* async_eventHandle;
void* async_workerThread;
void* async_pendingMutex;
void* async_processedMutex;
void* async_curRequestMutex;
volatile bool async_terminate;

AsyncRequestList async_pending;
AsyncRequestList async_processed;
String async_skinServer = String_FromConst("http://static.classicube.net/skins/");
AsyncRequest async_curRequest;
volatile Int32 async_curProgress = ASYNC_PROGRESS_NOTHING;
/* TODO: Implement these */
bool ManageCookies;
bool KeepAlive;
/* TODO: Connection pooling */

void AsyncDownloader_Add(String* url, bool priority, String* id, UInt8 type, DateTime* lastModified, String* etag, String* data) {
	Platform_MutexLock(async_pendingMutex);
	{
		AsyncRequest req = { 0 };
		String reqUrl = String_FromEmptyArray(req.URL); String_Set(&reqUrl, url);
		String reqID = String_FromEmptyArray(req.ID);  String_Set(&reqID, id);
		req.RequestType = type;

		Platform_Log2("Adding %s (type %b)", &reqUrl, &type);

		if (lastModified != NULL) {
			req.LastModified = *lastModified;
		}
		if (etag != NULL) {
			String reqEtag = String_FromEmptyArray(req.Etag); String_Set(&reqEtag, etag);
		}
		//request.Data = data; TODO: Implement this. do we need to copy or expect caller to malloc it? 

		Platform_CurrentUTCTime(&req.TimeAdded);
		if (priority) {
			AsyncRequestList_Prepend(&async_pending, &req);
		} else {
			AsyncRequestList_Append(&async_pending, &req);
		}
	}
	Platform_MutexUnlock(async_pendingMutex);
	Platform_EventSet(async_eventHandle);
}

void AsyncDownloader_GetSkin(STRING_PURE String* id, STRING_PURE String* skinName) {
	UInt8 urlBuffer[String_BufferSize(STRING_SIZE)];
	String url = String_InitAndClearArray(urlBuffer);

	if (Utils_IsUrlPrefix(skinName, 0)) {
		String_Set(&url, skinName);
	} else {
		String_AppendString(&url, &async_skinServer);
		String_AppendColorless(&url, skinName);
		String_AppendConst(&url, ".png");
	}

	AsyncDownloader_Add(&url, false, id, REQUEST_TYPE_IMAGE, NULL, NULL, NULL);
}

void AsyncDownloader_GetData(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_DATA, NULL, NULL, NULL);
}

void AsyncDownloader_GetImage(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_IMAGE, NULL, NULL, NULL);
}

void AsyncDownloader_GetString(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_STRING, NULL, NULL, NULL);
}

void AsyncDownloader_GetContentLength(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_CONTENT_LENGTH, NULL, NULL, NULL);
}

void AsyncDownloader_PostString(STRING_PURE String* url, bool priority, STRING_PURE String* id, STRING_PURE String* contents) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_STRING, NULL, NULL, contents);
}

void AsyncDownloader_GetDataEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_DATA, lastModified, etag, NULL);
}

void AsyncDownloader_GetImageEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag) {
	AsyncDownloader_Add(url, priority, id, REQUEST_TYPE_IMAGE, lastModified, etag, NULL);
}

void AsyncDownloader_PurgeOldEntriesTask(ScheduledTask* task) {
	Platform_MutexLock(async_processedMutex);
	{
		DateTime now; Platform_CurrentUTCTime(&now);
		Int32 i;
		for (i = async_processed.Count - 1; i >= 0; i--) {
			AsyncRequest* item = &async_processed.Requests[i];
			if (DateTime_MsBetween(&item->TimeDownloaded, &now) <= 10 * 1000) continue;

			ASyncRequest_Free(item);
			AsyncRequestList_RemoveAt(&async_processed, i);
		}
	}
	Platform_MutexUnlock(async_processedMutex);
}

Int32 AsyncRequestList_Find(STRING_PURE String* id, AsyncRequest* item) {
	Int32 i;
	for (i = 0; i < async_processed.Count; i++) {
		String reqID = String_FromRawArray(async_processed.Requests[i].ID);
		if (!String_Equals(&reqID, id)) continue;

		*item = async_processed.Requests[i];
		return i;
	}
	return -1;
}

bool AsyncDownloader_Get(STRING_PURE String* id, AsyncRequest* item) {
	bool success = false;

	Platform_MutexLock(async_processedMutex);
	{
		Int32 i = AsyncRequestList_Find(id, item);
		success = i >= 0;
		if (success) AsyncRequestList_RemoveAt(&async_processed, i);
	}
	Platform_MutexUnlock(async_processedMutex);
	return success;
}

bool AsyncDownloader_GetCurrent(AsyncRequest* request, Int32* progress) {
	Platform_MutexLock(async_curRequestMutex);
	{
		*request   = async_curRequest;
		*progress = async_curProgress;
	}
	Platform_MutexUnlock(async_curRequestMutex);
	return request->ID[0] != NULL;
}

void AsyncDownloader_ProcessRequest(AsyncRequest* request) {
	String url = String_FromRawArray(request->URL);
	Platform_Log2("Downloading from %s (type %b)", &url, &request->RequestType);
	Stopwatch stopwatch; UInt32 elapsedMS;

	void* handle;
	ReturnCode result;
	Stopwatch_Start(&stopwatch);
	result = Platform_HttpMakeRequest(request, &handle);
	elapsedMS = Stopwatch_ElapsedMicroseconds(&stopwatch) / 1000;
	Platform_Log2("HTTP make request: ret code %i, in %i ms", &result, &elapsedMS);
	if (!ErrorHandler_Check(result)) return;

	async_curProgress = ASYNC_PROGRESS_FETCHING_DATA;
	UInt32 size = 0;
	Stopwatch_Start(&stopwatch);
	result = Platform_HttpGetRequestHeaders(request, handle, &size);
	elapsedMS = Stopwatch_ElapsedMicroseconds(&stopwatch) / 1000;
	UInt32 status = request->StatusCode;
	Platform_Log3("HTTP get headers: ret code %i (http %i), in %i ms", &result, &status, &elapsedMS);

	if (!ErrorHandler_Check(result) || request->StatusCode != 200) {
		Platform_HttpFreeRequest(handle); return;
	}

	void* data = NULL;
	if (request->RequestType != REQUEST_TYPE_CONTENT_LENGTH) {
		Stopwatch_Start(&stopwatch);
		result = Platform_HttpGetRequestData(request, handle, &data, size, &async_curProgress);
		elapsedMS = Stopwatch_ElapsedMicroseconds(&stopwatch) / 1000;
		Platform_Log3("HTTP get data: ret code %i (size %i), in %i ms", &result, &size, &elapsedMS);
	}

	Platform_HttpFreeRequest(handle);
	if (!ErrorHandler_Check(result)) return;

	UInt64 addr = (UInt64)data;
	Platform_Log2("OK I got the DATA! %i bytes at %x", &size, &addr);

	Stream memStream;
	switch (request->RequestType) {
	case REQUEST_TYPE_DATA:
		request->ResultData.Ptr = data;
		request->ResultData.Size = size;
		break;

	case REQUEST_TYPE_IMAGE:
		Stream_ReadonlyMemory(&memStream, data, size, &url);
		Bitmap_DecodePng(&request->ResultBitmap, &memStream);
		break;

	case REQUEST_TYPE_STRING:
		request->ResultString = String_Init(data, size, size);
		break;

	case REQUEST_TYPE_CONTENT_LENGTH:
		request->ResultContentLength = size;
		break;
	}

	Platform_CurrentUTCTime(&request->TimeDownloaded);
	Platform_MutexLock(async_processedMutex);
	{
		AsyncRequest older;
		String id = String_FromRawArray(request->ID);
		Int32 index = AsyncRequestList_Find(&id, &older);

		if (index >= 0) {
			/* very rare case - priority item was inserted, then inserted again (so put before first item), */
			/* and both items got downloaded before an external function removed them from the queue */
			if (DateTime_TotalMs(&older.TimeAdded) > DateTime_TotalMs(&request->TimeAdded)) {
				AsyncRequest tmp = older; older = *request; *request = tmp;
			}

			ASyncRequest_Free(&older);
			async_processed.Requests[index] = *request;
		} else {
			AsyncRequestList_Append(&async_processed, request);
		}
	}
	Platform_MutexUnlock(async_processedMutex);
}

void AsyncDownloader_WorkerFunc(void) {
	while (true) {
		AsyncRequest request;
		bool hasRequest = false;

		Platform_MutexLock(async_pendingMutex);
		{
			if (async_terminate) return;
			if (async_pending.Count > 0) {
				request = async_pending.Requests[0];
				hasRequest = true;
				AsyncRequestList_RemoveAt(&async_pending, 0);
			}
		}
		Platform_MutexUnlock(async_pendingMutex);

		if (hasRequest) {
			Platform_LogConst("Got something to do!");
			Platform_MutexLock(async_curRequestMutex);
			{
				async_curRequest = request;
				async_curProgress = ASYNC_PROGRESS_MAKING_REQUEST;
			}
			Platform_MutexUnlock(async_curRequestMutex);

			Platform_LogConst("Doing it");
			AsyncDownloader_ProcessRequest(&request);

			Platform_MutexLock(async_curRequestMutex);
			{
				async_curRequest.ID[0] = NULL;
				async_curProgress = ASYNC_PROGRESS_NOTHING;
			}
			Platform_MutexUnlock(async_curRequestMutex);
		} else {
			Platform_LogConst("Going back to sleep...");
			Platform_EventWait(async_eventHandle);
		}
	}
}


void AsyncDownloader_Init(void) {
	AsyncRequestList_Init(&async_pending);
	AsyncRequestList_Init(&async_processed);
	Platform_HttpInit();

	async_eventHandle = Platform_EventCreate();
	async_pendingMutex = Platform_MutexCreate();
	async_processedMutex = Platform_MutexCreate();
	async_curRequestMutex = Platform_MutexCreate();
	async_workerThread = Platform_ThreadStart(AsyncDownloader_WorkerFunc);
}

void AsyncDownloader_Reset(void) {
	Platform_MutexLock(async_pendingMutex);
	{
		AsyncRequestList_Free(&async_pending);
	}
	Platform_MutexUnlock(async_pendingMutex);
	Platform_EventSet(async_eventHandle);
}

void AsyncDownloader_Free(void) {
	async_terminate = true;
	AsyncDownloader_Reset();
	Platform_ThreadJoin(async_workerThread);
	Platform_ThreadFreeHandle(async_workerThread);

	AsyncRequestList_Free(&async_pending);
	AsyncRequestList_Free(&async_processed);
	Platform_HttpFree();

	Platform_EventFree(async_eventHandle);
	Platform_MutexFree(async_pendingMutex);
	Platform_MutexFree(async_processedMutex);
	Platform_MutexFree(async_curRequestMutex);
}

IGameComponent AsyncDownloader_MakeComponent(void) {
	IGameComponent comp = IGameComponent_MakeEmpty();
	comp.Init  = AsyncDownloader_Init;
	comp.Reset = AsyncDownloader_Reset;
	comp.Free  = AsyncDownloader_Free;
	return comp;
}