#include "AsyncDownloader.h"
#include "Platform.h"

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

typedef struct AsyncRequestList_ {
	UInt32 Count;
	AsyncRequest* Requests;
	AsyncRequest DefaultRequests[10];
} AsyncRequestList;

/*void AsyncRequestList_Append(AsyncRequestList* list, AsyncRequest* item);
void AsyncRequestList_Prepend(AsyncRequestList* list, AsyncRequest* item);
void AsyncRequestList_RemoveAt(AsyncRequestList* list, UInt32 i);

void AsyncRequestList_Init(AsyncRequestList* list) {
	list->Count = 0;
	list->Requests = list->DefaultRequests;
}

void AsyncRequestList_Free(AsyncRequestList* list) {
	if (list->Requests != list->DefaultRequests) { 
		Platform_MemFree(&list->Requests);
	}
	AsyncDownloader_Init(list);
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
volatile Int32 async_curRequestProgress = -3;
bool ManageCookies;
bool KeepAlive;

void AsyncDownloader_Init(void) {
	AsyncRequestList_Init(&async_pending);
	AsyncRequestList_Init(&async_processed);
	async_eventHandle     = Platform_EventCreate();
	async_pendingMutex    = Platform_MutexCreate();
	async_processedMutex  = Platform_MutexCreate();
	async_curRequestMutex = Platform_MutexCreate();
	async_workerThread = Platform_ThreadStart(DownloadThreadWorker);
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

	 Platform_EventFree(async_eventHandle);
	 Platform_MutexFree(async_pendingMutex);
	 Platform_MutexFree(async_processedMutex);
	 Platform_MutexFree(async_curRequestMutex);
 }

 void AsyncDownloader_GetSkin(STRING_PURE String* id, STRING_PURE String* skinName) {
	 UInt8 urlBuffer[String_BufferSize(STRING_SIZE)];
	 String url = String_InitAndClearArray(urlBuffer);

	 if (Utils_IsUrlPrefix(skinName, 0)) {
		 String_Set(&url, &skinName);
	 } else {
		 String_AppendString(&url, &async_skinServer);
		 String_AppendColorless(&url, skinName);
		 String_AppendConst(&url, ".png");
	 }

	 AddRequest(&url, false, id, REQUEST_TYPE_IMAGE, NULL, NULL, NULL);
 }

 void AsyncDownloader_GetData(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	 AddRequest(url, priority, id, REQUEST_TYPE_DATA, NULL, NULL, NULL);
 }

 void AsyncDownloader_GetImage(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	 AddRequest(url, priority, id, REQUEST_TYPE_IMAGE, NULL, NULL, NULL);
 }

 void AsyncDownloader_GetString(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	 AddRequest(url, priority, id, REQUEST_TYPE_STRING, NULL, NULL, NULL);
 }
		
 void AsyncDownloader_GetContentLength(STRING_PURE String* url, bool priority, STRING_PURE String* id) {
	 AddRequest(url, priority, id, REQUEST_TYPE_CONTENT_LENGTH, NULL, NULL, NULL);
 }

 void AsyncDownloader_PostString(STRING_PURE String* url, bool priority, STRING_PURE String* id, STRING_PURE String* contents) {
	 AddRequest(url, priority, id, REQUEST_TYPE_STRING, NULL, NULL, contents);
 }

 void AsyncDownloader_GetDataEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag) {
	 AddRequest(url, priority, id, REQUEST_TYPE_DATA, lastModified, etag, NULL);
 }

 void AsyncDownloader_GetImageEx(STRING_PURE String* url, bool priority, STRING_PURE String* id, DateTime* lastModified, STRING_PURE String* etag) {
	 AddRequest(url, priority, id, REQUEST_TYPE_IMAGE, lastModified, etag, NULL);
 }

 void AddRequest(String* url, bool priority, String* id, UInt8 type, DateTime* lastModified, String* etag, object data) {
	 Platform_MutexLock(async_pendingMutex);
	 {
		 AsyncRequest request = { 0 };
		 String reqUrl = String_FromEmptyArray(request.URL); String_Set(&reqUrl, url);
		 String reqID  = String_FromEmptyArray(request.ID);  String_Set(&reqID,  id);
		 request.RequestType = type;
		 request.LastModified = lastModified; // can be null
		 request.ETag = etag; // can be null
		 request.Data = data;

		 Platform_CurrentUTCTime(&request.TimeAdded);
		 if (priority) {
			 AsyncRequestList_Prepend(&async_pending, &request);
		 } else {
			 AsyncRequestList_Append(&async_pending, &request);
		 }
	 }
	 Platform_MutexUnlock(async_pendingMutex);
	 Platform_EventSet(async_eventHandle);
 }

 void PurgeOldEntriesTask(ScheduledTask* task) {
	 Platform_MutexLock(async_processedMutex);
	 {
		 DateTime now; Platform_CurrentUTCTime(&now);
		 Int32 i;
		 for (i = async_processed.Count - 1; i >= 0; i--) {
			 AsyncRequest* item = &async_processed.Requests[i];
			 if ((now - item.TimeDownloaded).TotalSeconds < 10) continue;

			 ASyncRequest_Free(item);
			 AsyncRequestList_RemoveAt(&async_processed, i);
		 }
	 }
	 Platform_MutexUnlock(async_processedMutex);
 }

 bool AsyncDownloader_Get(STRING_PURE String* id, AsyncRequest* item) {
	 bool success = false;

	 Platform_MutexLock(async_processedMutex);
	 {
		 Int32 i = FindRequest(id, item);
		 success = i >= 0;
		 if (success) AsyncRequestList_RemoveAt(&async_processed, i);
	 }
	 Platform_MutexUnlock(async_processedMutex);
	 return success;
 }

 void DownloadThreadWorker() {
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
			 Platform_MutexLock(async_curRequestMutex);
			 {
				 async_curRequest = request;
				 async_curRequestProgress = -2;
			 }
			 Platform_MutexUnlock(async_curRequestMutex);

			 ProcessRequest(request);

			 Platform_MutexLock(async_curRequestMutex);
			 {
				 async_curRequest.ID[0] = NULL;
				 async_curRequestProgress = -3;
			 }
			 Platform_MutexUnlock(async_curRequestMutex);
		 } else {
			 Platform_EventWait(async_eventHandle);
		 }
	 }
 }

 Int32 FindRequest(STRING_PURE String* id, AsyncRequest* item) {
	 Int32 i;
	 for (i = 0; i < async_processed.Count; i++) {
		 String requestID = String_FromRawArray(async_processed.Requests[i].ID);
		 if (!String_Equals(&requestID, id)) continue;

		 *item = async_processed.Requests[i];
		 return i;
	 }
	 return -1;
 }

 void ProcessRequest(AsyncRequest* request) {
	 String url = String_FromRawArray(request->URL);
	 Platform_Log2("Downloading from %s (type %b)", &url, &request->RequestType);
	 HttpStatusCode status = HttpStatusCode.OK;

	 try {
		 HttpWebRequest req = MakeRequest(request);
		 using (HttpWebResponse response = (HttpWebResponse)req.GetResponse()) {
			 request.ETag = response.Headers.Get("ETag");
			 if (response.Headers.Get("Last-Modified") != null) {
				 request.LastModified = response.LastModified;
			 }
			 request.Data = DownloadContent(request, response);
		 }
	 } catch (Exception ex) {
		 if (!(ex is WebException || ex is ArgumentException || ex is UriFormatException || ex is IOException)) throw;

		 if (ex is WebException) {
			 WebException webEx = (WebException)ex;
			 if (webEx.Response != null) {
				 status = ((HttpWebResponse)webEx.Response).StatusCode;
				 webEx.Response.Close();
			 }
			 request.WebEx = webEx;
		 }

		 if (status != HttpStatusCode.OK) {
			 Utils.LogDebug("Failed to download (" + (int)status + ") from: " + url);
		 } else {
			 Utils.LogDebug("Failed to download from: " + url);
		 }
	 }
	 request.TimeDownloaded = DateTime.UtcNow;

	 Platform_MutexLock(async_processedMutex);
	 {
		 AsyncRequest older;
		 String id = String_FromRawArray(request->ID);
		 Int32 index = FindRequest(&id, &older);

		 if (index >= 0) {
			 // very rare case - priority item was inserted, then inserted again (so put before first item),
			 // and both items got downloaded before an external function removed them from the queue
			 if (older.TimeAdded > request.TimeAdded) {
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
 object DownloadContent(Request request, HttpWebResponse response) {
	 if (request.Type == RequestType.Bitmap) {
		 MemoryStream data = DownloadBytes(response);
		 Bitmap bmp = Platform.ReadBmp32Bpp(drawer, data);

		 if (bmp == null) {
			 Utils.LogDebug("Failed to download from: " + request.Url);
		 }
		 return bmp;
	 } else if (request.Type == RequestType.String) {
		 MemoryStream data = DownloadBytes(response);
		 byte[] rawBuffer = data.GetBuffer();
		 return Encoding.UTF8.GetString(rawBuffer, 0, (int)data.Length);
	 } else if (request.Type == RequestType.ByteArray) {
		 MemoryStream data = DownloadBytes(response);
		 return data.ToArray();
	 } else if (request.Type == RequestType.ContentLength) {
		 return response.ContentLength;
	 }
	 return null;
 }

	HttpWebRequest MakeRequest(Request request) {
		HttpWebRequest req = (HttpWebRequest)WebRequest.Create(request.Url);
		req.AutomaticDecompression = DecompressionMethods.GZip;
		req.ReadWriteTimeout = 90 * 1000;
		req.Timeout = 90 * 1000;
		req.Proxy = null;
		req.UserAgent = Program.AppName;
		req.CookieContainer = Cookies;
		req.KeepAlive = KeepAlive;

		if (request.LastModified != DateTime.MinValue) {
			req.IfModifiedSince = request.LastModified;
		}
		if (request.ETag != null) {
			req.Headers["If-None-Match"] = request.ETag;
		}

		if (request.Data != null) {
			req.Method = "POST";
			req.ContentType = "application/x-www-form-urlencoded; charset=UTF-8;";
			byte[] encodedData = Encoding.UTF8.GetBytes((string)request.Data);
			req.ContentLength = encodedData.Length;
			using (Stream stream = req.GetRequestStream()) {
				stream.Write(encodedData, 0, encodedData.Length);
			}
		}
		return req;
	}

	static byte[] buffer = new byte[4096 * 8];
	MemoryStream DownloadBytes(HttpWebResponse response) {
		int length = (int)response.ContentLength;
		MemoryStream dst = length > 0 ? new MemoryStream(length) : new MemoryStream();
		CurrentItemProgress = length > 0 ? 0 : -1;

		using (Stream src = response.GetResponseStream()) {
			int read = 0;
			while ((read = src.Read(buffer, 0, buffer.Length)) > 0) {
				dst.Write(buffer, 0, read);
				if (length <= 0) continue;
				CurrentItemProgress = (int)(100 * (float)dst.Length / length);
			}
		}
		return dst;
	}
}*/