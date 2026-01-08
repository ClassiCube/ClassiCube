#include "Core.h"

#if defined CC_BUILD_WEB
/* Implemented in web http backend without C worker thread(s) */
#elif !defined CC_BUILD_NETWORKING
#include "Http.h"
#include "Game.h"

void HttpRequest_Free(struct HttpRequest* request) { }

cc_bool Http_GetResult(int reqID, struct HttpRequest* item) {
	return false;
}

cc_bool Http_GetCurrent(int* reqID, int* progress) {
	return false;
}

int Http_AsyncGetSkin(const cc_string* skinName, cc_uint8 flags) {
	return -1;
}
int Http_AsyncGetData(const cc_string* url, cc_uint8 flags) {
	return -1;
}
int Http_AsyncGetHeaders(const cc_string* url, cc_uint8 flags) {
	return -1;
}
int Http_AsyncPostData(const cc_string* url, cc_uint8 flags, const void* data, cc_uint32 size, struct StringsBuffer* cookies) {
	return -1;
}
int Http_AsyncGetDataEx(const cc_string* url, cc_uint8 flags, const cc_string* lastModified, const cc_string* etag, struct StringsBuffer* cookies) {
	return -1;
}

int Http_CheckProgress(int reqID) { return -1; }

void Http_LogError(const char* action, const struct HttpRequest* item) { }

void Http_TryCancel(int reqID) { }

void Http_UrlEncodeUtf8(cc_string* dst, const cc_string* src) { }

void Http_ClearPending(void) { }

static void Http_NullInit(void) { }

struct IGameComponent Http_Component = {
	Http_NullInit
};
#else
#include "_HttpBase.h"

#if CC_NET_BACKEND == CC_NET_BACKEND_BUILTIN
#include "Errors.h"
#include "PackedCol.h"
#include "SSL.h"

/*########################################################################################################################*
*---------------------------------------------------------HttpUrl---------------------------------------------------------*
*#########################################################################################################################*/
/* Components of a URL */
struct HttpUrl {
	cc_bool https;      /* Whether HTTPS or just HTTP protocol */
	cc_string address;  /* Address of server (e.g. "classicube.net:8080") */
	cc_string resource; /* Path being accessed (and query string) */
	char _addressBuffer[STRING_SIZE + 8];
	char _resourceBuffer[STRING_SIZE * 4];
};

static void HttpUrl_EncodeUrl(cc_string* dst, const cc_string* src) {
	cc_uint8 data[4];
	int i, len;
	char c;

	for (i = 0; i < src->length; i++) {
		c   = src->buffer[i];
		len = Convert_CP437ToUtf8(c, data);

		/* URL path/query must not be URL encoded (it normally would be) */
		if (c == '/' || c == '?' || c == '=' || c == '&') {
			String_Append(dst, c);
		} else {
			Http_UrlEncode(dst, data, len);
		}
	}
}

static const cc_string url_rewrite_srcs[] = {
	#define URL_REMAP_FUNC(src_base, src_host, dst_base, dst_host) String_FromConst(src_host),
	#include "_HttpUrlMap.h"
};
static const cc_string url_rewrite_dsts[] = {
	#undef  URL_REMAP_FUNC
	#define URL_REMAP_FUNC(src_base, src_host, dst_base, dst_host) String_FromConst(dst_host),
	#include "_HttpUrlMap.h"
};

/* Splits up the components of a URL */
static void HttpUrl_Parse(const cc_string* src, struct HttpUrl* url) {
	cc_string scheme, path, addr, resource;
	/* URL is of form [scheme]://[server host]:[server port]/[resource] */
	/* For simplicity, parsed as [scheme]://[server address]/[resource] */
	int i, idx = String_IndexOfConst(src, "://");

	scheme = idx == -1 ? String_Empty : String_UNSAFE_Substring(src,   0, idx);
	path   = idx == -1 ? *src         : String_UNSAFE_SubstringAt(src, idx + 3);

	url->https = String_CaselessEqualsConst(&scheme, "https");
	String_UNSAFE_Separate(&path, '/', &addr, &resource);

	String_InitArray(url->address, url->_addressBuffer);
	/* Converts e.g. "dl.dropbox.com" into "dl.dropboxusercontent.com" */
	for (i = 0; i < Array_Elems(url_rewrite_srcs); i++) 
	{
		if (!String_Equals(&addr, &url_rewrite_srcs[i])) continue;

		addr = url_rewrite_dsts[i];
		break;
	}
	String_Copy(&url->address, &addr);

	String_InitArray(url->resource, url->_resourceBuffer);
	String_Append(&url->resource, '/');
	/* Address may have unicode characters - need to percent encode them */
	HttpUrl_EncodeUrl(&url->resource, &resource);
}

static cc_result HttpUrl_ResolveRedirect(struct HttpUrl* parts, const cc_string* url) {
	/* absolute URL */
	if (String_IndexOfConst(url, "http://") == 0 || String_IndexOfConst(url, "https://") == 0) {		
		HttpUrl_Parse(url, parts);
		return 0;
	} 

	/* Root relative URL */
	if (url->buffer[0] == '/' && (url->length == 1 || url->buffer[1] != '/')) {
		parts->resource.length = 0;
		HttpUrl_EncodeUrl(&parts->resource, url);
		return 0;
	}

	/* TODO scheme relative or relative URL or invalid */
	return HTTP_ERR_RELATIVE;
}


/*########################################################################################################################*
*------------------------------------------------------HttpConnection-----------------------------------------------------*
*#########################################################################################################################*/
struct HttpConnection {
	cc_socket socket;
	void* sslCtx;
	cc_bool valid;
};

static void HttpConnection_Close(struct HttpConnection* conn) {
	if (conn->sslCtx) {
		SSL_Free(conn->sslCtx);
		conn->sslCtx = NULL;
	}

	if (conn->socket != -1) {
		Socket_Close(conn->socket);
		conn->socket = -1;
	}
	conn->valid = false;
}

static void ExtractHostPort(const struct HttpUrl* url, cc_string* host, cc_string* port) {
	/* address can have the form of either "host" or "host:port" */
	/* Slightly more complicated because IPv6 hosts can be e.g. [::1] */
	cc_string addr = url->address;
	int idx = String_IndexOf(&addr, ':');
	int endIdx;

	/* Special handling for raw IPv6 hosts, e.g. "[::1]:80" */
	if (addr.length && addr.buffer[0] == '[' && (endIdx = String_IndexOf(&addr, ']')) >= 0) {
		*host = String_UNSAFE_Substring(&addr, 1, endIdx - 1);
		addr  = String_UNSAFE_SubstringAt(&addr, endIdx + 1);

		idx = String_IndexOf(&addr, ':');
	} else if (idx == -1) {
		/* Hostname/IP only */
		*host = addr;
	} else {
		/* Hostname/IP:port */
		*host = String_UNSAFE_Substring(&addr, 0, idx);
	}

	if (idx == -1) {
		*port = String_Empty;
	} else {
		*port = String_UNSAFE_SubstringAt(&addr, idx + 1);
	}
}

static cc_result HttpConnection_Open(struct HttpConnection* conn, const struct HttpUrl* url) {
	cc_string host, port;
	cc_uint16 portNum;
	cc_result res;
	cc_sockaddr addrs[SOCKET_MAX_ADDRS];
	char addrBuf[32];
	cc_string addrStr;
	int i, numValidAddrs;

	ExtractHostPort(url, &host, &port);
	if (!Convert_ParseUInt16(&port, &portNum)) {
		portNum = url->https ? 443 : 80;
	}

	conn->socket = -1;
	conn->sslCtx = NULL;

	if ((res = Socket_ParseAddress(&host, portNum, addrs, &numValidAddrs))) return res;
	res = ERR_INVALID_ARGUMENT; /* in case 0 valid addresses */

	/* TODO: Connect in parallel instead of serial, but that's a lot of work */
	for (i = 0; i < numValidAddrs; i++)
	{
		res = Socket_Create(&conn->socket, &addrs[i], false);
		if (res) { HttpConnection_Close(conn); continue; }

		String_InitArray(addrStr, addrBuf);
		if (SockAddr_ToString(&addrs[i], &addrStr)) Platform_Log1("  Connecting to %s..", &addrStr);

		res = Socket_Connect(conn->socket, &addrs[i]);
		if (res) { HttpConnection_Close(conn); continue; }

		break; /* Successful connection */
	}
	if (res) return res;

	conn->valid = true;
	if (!url->https) return 0;
	return SSL_Init(conn->socket, &host, &conn->sslCtx);
}

static cc_result WriteAllToSocket(cc_socket socket, const cc_uint8* data, cc_uint32 count) {
	cc_uint32 sent;
	cc_result res;

	while (count)
	{
		if ((res = Socket_Write(socket, data, count, &sent))) return res;
		if (!sent) return ERR_END_OF_STREAM;

		data  += sent;
		count -= sent;
	}
	return 0;
}

static cc_result HttpConnection_Read(struct HttpConnection* conn, cc_uint8* data, cc_uint32 count, cc_uint32* read) {
	if (conn->sslCtx)
		return SSL_Read(conn->sslCtx, data, count, read);

	return Socket_Read(conn->socket,  data, count, read);
}

static cc_result HttpConnection_Write(struct HttpConnection* conn, const cc_uint8* data, cc_uint32 count) {
	if (conn->sslCtx) 
		return SSL_WriteAll(conn->sslCtx, data, count);

	return WriteAllToSocket(conn->socket, data, count);
}


/*########################################################################################################################*
*-----------------------------------------------------Connection Pool-----------------------------------------------------*
*#########################################################################################################################*/
static struct ConnectionPoolEntry {
	struct HttpConnection conn;
	cc_string addr;
	char addrBuffer[STRING_SIZE];
	cc_bool https;
} connection_pool[10];

static cc_result ConnectionPool_Insert(int i, struct HttpConnection** conn, const struct HttpUrl* url) {
	struct ConnectionPoolEntry* e = &connection_pool[i];
	*conn = &e->conn;

	String_InitArray(e->addr, e->addrBuffer);
	String_Copy(&e->addr, &url->address);
	e->https = url->https;
	return HttpConnection_Open(&e->conn, url);
}

static cc_result ConnectionPool_Open(struct HttpConnection** conn, const struct HttpUrl* url) {
	struct ConnectionPoolEntry* e;
	int i;

	for (i = 0; i < Array_Elems(connection_pool); i++)
	{
		e = &connection_pool[i];
		if (e->conn.valid && e->https == url->https && String_Equals(&e->addr, &url->address)) {
			*conn = &connection_pool[i].conn;
			return 0;
		}
	}

	for (i = 0; i < Array_Elems(connection_pool); i++)
	{
		e = &connection_pool[i];
		if (!e->conn.valid) return ConnectionPool_Insert(i, conn, url);
	}

	/* TODO: Should we be consistent in which entry gets evicted? */
	i = (cc_uint8)Stopwatch_Measure() % Array_Elems(connection_pool);
	HttpConnection_Close(&connection_pool[i].conn);
	return ConnectionPool_Insert(i, conn, url);
}


/*########################################################################################################################*
*--------------------------------------------------------HttpClient-------------------------------------------------------*
*#########################################################################################################################*/
enum HTTP_RESPONSE_STATE {
	HTTP_RESPONSE_STATE_INITIAL,
	HTTP_RESPONSE_STATE_HEADER,
	HTTP_RESPONSE_STATE_DATA,
	HTTP_RESPONSE_STATE_CHUNK_HEADER,
	HTTP_RESPONSE_STATE_CHUNK_END_R,
	HTTP_RESPONSE_STATE_CHUNK_END_N,
	HTTP_RESPONSE_STATE_CHUNK_TRAILERS,
	HTTP_RESPONSE_STATE_DONE
};
#define HTTP_LOCATION_MAX_LENGTH 256

#if CC_BUILD_MAXSTACK <= (48 * 1024)
	#define HTTP_HEADER_MAX_LENGTH 2048
	#define INPUT_BUFFER_LEN 4096
	#define SEND_BUFFER_LEN  4096
#else
	#define HTTP_HEADER_MAX_LENGTH 4096
	#define INPUT_BUFFER_LEN  8192
	#define SEND_BUFFER_LEN  16384
#endif

struct HttpClientState {
	enum HTTP_RESPONSE_STATE state;
	struct HttpConnection* conn;
	struct HttpRequest* req;
	cc_uint32 dataLeft; /* Number of bytes still to read from the current chunk or body */
	cc_bool chunked;    /* Whether content is being transferred using HTTP chunks */
	cc_bool autoClose;  /* TODO Whether connection should be dropped after request completed */
	cc_bool retried;    /* Whether request has been retried due to SSL context being closed/dropped */
	cc_uint8 redirects; /* Number of times current HTTP request has been redirected */
	cc_string header;   /* Current header being parsed */
	cc_string location; /* URL to redirect to */
	struct HttpUrl url; /* Current URL (may not be same as original URL after redirecting) */
	char _headerBuffer[HTTP_HEADER_MAX_LENGTH];
	char _locationBuffer[HTTP_LOCATION_MAX_LENGTH];
};

static void HttpClientState_Reset(struct HttpClientState* state) {
	state->state     = HTTP_RESPONSE_STATE_INITIAL;
	state->chunked   = false;
	state->dataLeft  = 0;
	state->autoClose = false;

	String_InitArray(state->header,   state->_headerBuffer);
	String_InitArray(state->location, state->_locationBuffer);
}

static void HttpClientState_Init(struct HttpClientState* state, struct HttpRequest* req) {
	cc_string url    = String_FromRawArray(req->url);
	state->req       = req;
	state->retried   = false;
	state->redirects = 0;

	HttpUrl_Parse(&url, &state->url);
	HttpClientState_Reset(state);
}


static void HttpClient_Serialise(struct HttpClientState* state) {
	static const char* verbs[] = { "GET", "HEAD", "POST" };

	struct HttpRequest* req = state->req;
	cc_string* buffer = (cc_string*)req->meta;
	/* TODO move to other functions */
	/* Write request message headers */
	String_Format2(buffer, "%c %s HTTP/1.1\r\n",
					verbs[req->requestType], &state->url.resource);

	Http_AddHeader(req, "Host",       &state->url.address);
	Http_AddHeader(req, "User-Agent", Http_GetUserAgent_UNSAFE());
	if (req->data) String_Format1(buffer, "Content-Length: %i\r\n", &req->size);

	Http_SetRequestHeaders(req);
	String_AppendConst(buffer, "\r\n");
	
	/* Write request message body */
	if (req->data) {
		String_AppendAll(buffer, req->data, req->size);
		HttpRequest_Free(req);
	} /* TODO post redirect handling */
}


static cc_result HttpClient_SendRequest(struct HttpClientState* state) {
	char inputBuffer[SEND_BUFFER_LEN];
	cc_string inputMsg;

	String_InitArray(inputMsg, inputBuffer);
	state->req->meta     = &inputMsg;
	state->req->progress = HTTP_PROGRESS_FETCHING_DATA;
	HttpClient_Serialise(state);

	return HttpConnection_Write(state->conn, (cc_uint8*)inputBuffer, inputMsg.length);
}


static void HttpClient_ParseHeader(struct HttpClientState* state, const cc_string* line) {
	static const cc_string HTTP_10_VERSION = String_FromConst("HTTP/1.0");
	cc_string name, value;
	/* HTTP 1.0 defaults to auto closing connection */
	if (String_CaselessStarts(line, &HTTP_10_VERSION)) state->autoClose = true;

	/* name: value */
	if (!String_UNSAFE_Separate(line, ':', &name, &value)) return;

	if (String_CaselessEqualsConst(&name, "Transfer-Encoding")) {
		state->chunked = String_CaselessEqualsConst(&value, "chunked");
	} else if (String_CaselessEqualsConst(&name, "Location")) {
		String_Copy(&state->location, &value);
	} else if (String_CaselessEqualsConst(&name, "Connection")) {
		if (String_CaselessEqualsConst(&value, "keep-alive")) state->autoClose = false;
		if (String_CaselessEqualsConst(&value, "close"))      state->autoClose = true;
	}
}

/* RFC 7230, section 3.3.3 - Message Body Length */
static cc_bool HttpClient_HasBody(struct HttpRequest* req) {
	/* HEAD responses never have a message body */
	if (req->requestType == REQUEST_TYPE_HEAD) return false;
	/* 1XX (Information) responses don't have message body */
	if (req->statusCode >= 100 && req->statusCode <= 199) return false;
	/* 204 (No Content) and 304 (Not Modified) also don't */
	if (req->statusCode == 204 || req->statusCode == 304) return false;

	return true;
}

static int HttpClient_BeginBody(struct HttpRequest* req, struct HttpClientState* state) {
	if (!HttpClient_HasBody(req))
		return HTTP_RESPONSE_STATE_DONE;
	
	if (state->chunked)
		return HTTP_RESPONSE_STATE_CHUNK_HEADER;
	if (req->contentLength)
		return HTTP_RESPONSE_STATE_DATA;

	/* Zero length response */
	return HTTP_RESPONSE_STATE_DONE;
}

/* RFC 7230, section 4.1 - Chunked Transfer Coding */
static int HttpClient_GetChunkLength(const cc_string* line) {
	int length = 0, i, part;

	for (i = 0; i < line->length; i++) 
	{
		char c = line->buffer[i];
		/* RFC 7230, section 4.1.1 - Chunk Extensions */
		if (c == ';') break;

		part = PackedCol_DeHex(c);
		if (part == -1) return -1;
		length = (length << 4) | part;
	}
	return length;
}

/* https://httpwg.org/specs/rfc7230.html */
static cc_result HttpClient_Process(struct HttpClientState* state, char* buffer, int total) {
	struct HttpRequest* req = state->req;
	cc_uint32 left, avail, read;
	int offset = 0, chunkLen, ok;

	while (offset < total) {
		switch (state->state) {
		case HTTP_RESPONSE_STATE_INITIAL:
			state->state = HTTP_RESPONSE_STATE_HEADER;
			break;

		case HTTP_RESPONSE_STATE_HEADER:
		{
			for (; offset < total;) 
			{
				char c = buffer[offset++];
				if (c == '\r') continue;
				if (c != '\n') {
					/* Warn when a header would be truncated */
					if (state->header.length == HTTP_HEADER_MAX_LENGTH) 
						return HTTP_ERR_TRUNCATED;

					String_Append(&state->header, c); 
					continue; 
				}

				/* Zero length header = end of message headers */
				if (state->header.length == 0) {
					state->state = HttpClient_BeginBody(req, state);

					/* The rest of the request body is just content/data */
					if (state->state == HTTP_RESPONSE_STATE_DATA) {
						state->dataLeft = req->contentLength;
						ok = Http_BufferExpand(req, state->dataLeft);
						if (!ok) return ERR_OUT_OF_MEMORY;
					}
					break;
				}

				Http_ParseHeader(state->req, &state->header);
				HttpClient_ParseHeader(state, &state->header);
				state->header.length = 0;
			}
		}
		break;

		case HTTP_RESPONSE_STATE_DATA:
		{
			left  = total - offset;
			avail = state->dataLeft;
			read  = min(left, avail);

			/* TODO figure out why this bug happens */
			if (!req->data) Process_Abort("Http state broken, please report this");

			Mem_Copy(req->data + req->size, buffer + offset, read);
			Http_BufferExpanded(req, read); 

			state->dataLeft -= read;
			offset += read;

			if (!state->dataLeft) {
				state->state = state->chunked ? HTTP_RESPONSE_STATE_CHUNK_END_R : HTTP_RESPONSE_STATE_DONE;
			}
		}
		break;


		/* RFC 7230, section 4.1 - Chunked Transfer Coding */
		case HTTP_RESPONSE_STATE_CHUNK_HEADER:
		{
			for (; offset < total;) 
			{
				char c = buffer[offset++];
				if (c == '\r') continue;
				if (c != '\n') { String_Append(&state->header, c); continue; }

				chunkLen = HttpClient_GetChunkLength(&state->header);
				if (chunkLen < 0) return HTTP_ERR_CHUNK_SIZE;
				state->header.length = 0;

				if (chunkLen == 0) {
					state->state = HTTP_RESPONSE_STATE_CHUNK_TRAILERS;
				} else {
					state->state = HTTP_RESPONSE_STATE_DATA;

					state->dataLeft = chunkLen;
					ok = Http_BufferExpand(req, state->dataLeft);
					if (!ok) return ERR_OUT_OF_MEMORY;
				}
				break;
			}
		}
		break;

		/* Chunks are terminated by \r\n */
		case HTTP_RESPONSE_STATE_CHUNK_END_R:
			if (buffer[offset++] != '\r') return ERR_INVALID_ARGUMENT;

			state->state = HTTP_RESPONSE_STATE_CHUNK_END_N;
			break;

		case HTTP_RESPONSE_STATE_CHUNK_END_N:
			if (buffer[offset++] != '\n') return ERR_INVALID_ARGUMENT;

			state->state = HTTP_RESPONSE_STATE_CHUNK_HEADER;
			break;

		/* RFC 7230, section 4.1.2 - Chunked Trailer Part */
		case HTTP_RESPONSE_STATE_CHUNK_TRAILERS:
		{
			for (; offset < total;) 
			{
				char c = buffer[offset++];
				if (c == '\r') continue;
				if (c != '\n') { String_Append(&state->header, c); continue; }

				/* Zero length header = end of message trailers */
				if (state->header.length == 0) {
					state->state = HTTP_RESPONSE_STATE_DONE;
					break;
				}
				state->header.length = 0;
			}
		} 
		break;

		default:
			return 0;
		}
	}
	return 0;
}

static cc_result HttpClient_ParseResponse(struct HttpClientState* state) {
	struct HttpRequest* req = state->req;
	cc_uint8 buffer[INPUT_BUFFER_LEN];
	cc_uint8* dst;
	cc_uint32 total;
	cc_result res;

	for (;;) 
	{
		dst = state->dataLeft > INPUT_BUFFER_LEN ? (req->data + req->size) : buffer;
		res = HttpConnection_Read(state->conn, dst, INPUT_BUFFER_LEN, &total);
		if (res) return res;

		if (total == 0) {
			Platform_Log1("Http read unexpectedly returned 0 in state %i", &state->state);
			return state->state == HTTP_RESPONSE_STATE_INITIAL ? HTTP_ERR_NO_RESPONSE : ERR_END_OF_STREAM;
		}

		if (dst != buffer) {
			/* When there is more than INPUT_BUFFER_LEN bytes of unread data/content, */
			/*  there is no need to run the HTTP client state machine - just read directly */
			/*  into the output buffer to avoid a pointless Mem_Copy call */
			Http_BufferExpanded(req, total); 
			state->dataLeft -= total;
		} else {
			res = HttpClient_Process(state, (char*)buffer, total);
		}

		if (res) return res;
		if (state->state == HTTP_RESPONSE_STATE_DONE) return 0;
	}
}

static cc_bool HttpClient_IsRedirect(struct HttpRequest* req) {
	return req->statusCode >= 300 && req->statusCode <= 399 && req->statusCode != 304;
}

static cc_result HttpClient_HandleRedirect(struct HttpClientState* state) {
	cc_result res = HttpUrl_ResolveRedirect(&state->url, &state->location);
	if (res) return res;

	HttpRequest_Free(state->req);
	Platform_Log1("  Redirecting to: %s", &state->location);
	state->req->contentLength = 0;
	return 0;
}


/*########################################################################################################################*
*-----------------------------------------------Http backend implementation-----------------------------------------------*
*#########################################################################################################################*/
static void HttpBackend_Init(void) {
	SSLBackend_Init(httpsVerify);
}

static void Http_AddHeader(struct HttpRequest* req, const char* key, const cc_string* value) {
	String_Format2((cc_string*)req->meta, "%c:%s\r\n", key, value);
}

static cc_result HttpBackend_PerformRequest(struct HttpClientState* state) {
	cc_result res;

	res = ConnectionPool_Open(&state->conn, &state->url);
	if (res) { HttpConnection_Close(state->conn); return res; }

	res = HttpClient_SendRequest(state);
	if (res) { HttpConnection_Close(state->conn); return res; }

	res = HttpClient_ParseResponse(state);
	if (res) HttpConnection_Close(state->conn);

	return res;
}
static const char* verbs[] = { "GET", "HEAD", "POST" };

static cc_result HttpBackend_Do(struct HttpRequest* req) {
	struct HttpClientState state;
	cc_result res;
	HttpClientState_Init(&state, req);

	for (;;) {
		Platform_Log4("Fetching %c%s%s (%c)", state.url.https ? "https://" : "http://", 
					&state.url.address, &state.url.resource, verbs[req->requestType]);

		res = HttpBackend_PerformRequest(&state);
		/* TODO: Can we handle this while preserving the TCP connection */
		if (res == SSL_ERR_CONTEXT_DEAD && !state.retried) {
			Platform_LogConst("Resetting connection due to SSL context being dropped..");
			res = HttpBackend_PerformRequest(&state);
			state.retried = true;
		}
		if (res == HTTP_ERR_NO_RESPONSE && !state.retried) {
			Platform_LogConst("Resetting connection due to empty response..");
			res = HttpBackend_PerformRequest(&state);
			state.retried = true;
		}
		if (res == ReturnCode_SocketDropped && !state.retried) {
			Platform_LogConst("Resetting connection due to being dropped..");
			res = HttpBackend_PerformRequest(&state);
			state.retried = true;
		}

		if (res || !HttpClient_IsRedirect(req)) break;
		if (state.redirects >= 20) return HTTP_ERR_REDIRECTS;

		/* TODO FOLLOW LOCATION PROPERLY */
		state.redirects++;
		res = HttpClient_HandleRedirect(&state);
		if (res) break;
		HttpClientState_Reset(&state);
	}
	return res;
}

static cc_bool HttpBackend_DescribeError(cc_result res, cc_string* dst) {
	return SSLBackend_DescribeError(res, dst);
}
#endif


static void* workerWaitable;
static void* workerThread;

static void* pendingMutex;
static struct RequestList pendingReqs;

static void* curRequestMutex;
static struct HttpRequest http_curRequest;


/*########################################################################################################################*
*----------------------------------------------------Http public api------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Http_GetResult(int reqID, struct HttpRequest* item) {
	int i;
	Mutex_Lock(processedMutex);
	{
		i = RequestList_Find(&processedReqs, reqID);
		if (i >= 0) HttpRequest_Copy(item, &processedReqs.entries[i]);
		if (i >= 0) RequestList_RemoveAt(&processedReqs, i);
	}
	Mutex_Unlock(processedMutex);
	return i >= 0;
}

cc_bool Http_GetCurrent(int* reqID, int* progress) {
	Mutex_Lock(curRequestMutex);
	{
		*reqID    = http_curRequest.id;
		*progress = http_curRequest.progress;
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


/*########################################################################################################################*
*-----------------------------------------------------Http worker---------------------------------------------------------*
*#########################################################################################################################*/
/* Sets up state to begin a http request */
static void SetCurrentRequest(struct HttpRequest* req) {
	Mutex_Lock(curRequestMutex);
	{
		HttpRequest_Copy(&http_curRequest, req);
		http_curRequest.progress = HTTP_PROGRESS_MAKING_REQUEST;
	}
	Mutex_Unlock(curRequestMutex);
}

static void PerformRequest(struct HttpRequest* req) {
	cc_uint64 beg, end;
	int elapsed;

	beg = Stopwatch_Measure();
	req->result = HttpBackend_Do(req);
	end = Stopwatch_Measure();

	elapsed = Stopwatch_ElapsedMS(beg, end);
	Platform_Log4("HTTP: result %e (http %i) in %i ms (%i bytes)",
		&req->result, &req->statusCode, &elapsed, &req->size);

	Http_FinishRequest(req);
}

static void ClearCurrentRequest(void) {
	Mutex_Lock(curRequestMutex);
	{
		http_curRequest.id       = 0;
		http_curRequest.progress = HTTP_PROGRESS_NOT_WORKING_ON;
	}
	Mutex_Unlock(curRequestMutex);
}

static void DoRequest(struct HttpRequest* request) {
	SetCurrentRequest(request);
	PerformRequest(&http_curRequest);
	ClearCurrentRequest();
}

static void WorkerLoop(void) {
	struct HttpRequest request;
	cc_bool hasRequest;

	for (;;) {
		hasRequest = false;

		Mutex_Lock(pendingMutex);
		{
			if (pendingReqs.count) {
				HttpRequest_Copy(&request, &pendingReqs.entries[0]);
				hasRequest = true;
				RequestList_RemoveAt(&pendingReqs, 0);
			}
		}
		Mutex_Unlock(pendingMutex);

		if (hasRequest) {
			DoRequest(&request);
		} else {
			/* Block until another thread submits a request to do */
			Platform_LogConst("Download queue empty, going back to sleep...");
			Waitable_Wait(workerWaitable);
		}
	}
}

/* Adds a req to the list of pending requests, waking up worker thread if needed */
static void HttpBackend_Add(struct HttpRequest* req, cc_uint8 flags) {
#if defined CC_BUILD_PSP || defined CC_BUILD_NDS
	/* TODO why doesn't threading work properly on PSP */
	DoRequest(req);
#else
	Mutex_Lock(pendingMutex);
	{
		RequestList_Append(&pendingReqs, req, flags);
	}
	Mutex_Unlock(pendingMutex);
	Waitable_Signal(workerWaitable);
#endif
}

/*########################################################################################################################*
*-----------------------------------------------------Http component------------------------------------------------------*
*#########################################################################################################################*/
static void Http_Init(void) {
	Http_InitCommon();
	http_curRequest.progress = HTTP_PROGRESS_NOT_WORKING_ON;
	/* Http component gets initialised multiple times on Android */
	if (workerThread) return;

	HttpBackend_Init();
	RequestList_Init(&pendingReqs);
	RequestList_Init(&processedReqs);

	workerWaitable  = Waitable_Create("HTTP wakeup");
	pendingMutex    = Mutex_Create("HTTP pending");
	processedMutex  = Mutex_Create("HTTP processed");
	curRequestMutex = Mutex_Create("HTTP current");
	
	Thread_Run(&workerThread, WorkerLoop, 128 * 1024, "HTTP");
}
#endif
