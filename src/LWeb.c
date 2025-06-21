#include "LWeb.h"
#ifndef CC_DISABLE_LAUNCHER
#include "String.h"
#include "Launcher.h"
#include "Platform.h"
#include "Stream.h"
#include "Logger.h"
#include "Window.h"
#include "Options.h"
#include "PackedCol.h"
#include "Errors.h"
#include "Utils.h"
#include "Http.h"
#include "LBackend.h"

/*########################################################################################################################*
*----------------------------------------------------------JSON-----------------------------------------------------------*
*#########################################################################################################################*/
#define TOKEN_NONE  0
#define TOKEN_NUM   1
#define TOKEN_TRUE  2
#define TOKEN_FALSE 3
#define TOKEN_NULL  4
/* Consumes n characters from the JSON stream */
#define JsonContext_Consume(ctx, n) ctx->cur += n; ctx->left -= n;

static const cc_string strTrue  = String_FromConst("true");
static const cc_string strFalse = String_FromConst("false");
static const cc_string strNull  = String_FromConst("null");

static cc_bool Json_IsWhitespace(char c) {
	return c == '\r' || c == '\n' || c == '\t' || c == ' ';
}

static cc_bool Json_IsNumber(char c) {
	return c == '-' || c == '.' || (c >= '0' && c <= '9');
}

static cc_bool Json_ConsumeConstant(struct JsonContext* ctx, const cc_string* value) {
	int i;
	if (value->length > ctx->left) return false;

	for (i = 0; i < value->length; i++) {
		if (ctx->cur[i] != value->buffer[i]) return false;
	}

	JsonContext_Consume(ctx, value->length);
	return true;
}

static int Json_ConsumeToken(struct JsonContext* ctx) {
	char c;
	for (; ctx->left && Json_IsWhitespace(*ctx->cur); ) { JsonContext_Consume(ctx, 1); }
	if (!ctx->left) return TOKEN_NONE;

	c = *ctx->cur;
	if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == '"' || c == ':') {
		JsonContext_Consume(ctx, 1); return c;
	}

	/* number token forms part of value, don't consume it */
	if (Json_IsNumber(c)) return TOKEN_NUM;

	if (Json_ConsumeConstant(ctx, &strTrue))  return TOKEN_TRUE;
	if (Json_ConsumeConstant(ctx, &strFalse)) return TOKEN_FALSE;
	if (Json_ConsumeConstant(ctx, &strNull))  return TOKEN_NULL;

	/* invalid token */
	JsonContext_Consume(ctx, 1);
	ctx->failed = true;
	return TOKEN_NONE;
}

static cc_string Json_ConsumeNumber(struct JsonContext* ctx) {
	int len = 0;
	for (; ctx->left && Json_IsNumber(*ctx->cur); len++) { JsonContext_Consume(ctx, 1); }
	return String_Init(ctx->cur - len, len, len);
}

static void Json_ConsumeString(struct JsonContext* ctx, cc_string* str) {
	int codepoint, h[4];
	char c;
	str->length = 0;

	for (; ctx->left;) {
		c = *ctx->cur; JsonContext_Consume(ctx, 1);
		if (c == '"') return;
		if (c != '\\') { String_Append(str, c); continue; }

		/* form of \X */
		if (!ctx->left) break;
		c = *ctx->cur; JsonContext_Consume(ctx, 1);
		if (c == '/' || c == '\\' || c == '"') { String_Append(str, c); continue; }
		if (c == 'n') { String_Append(str, '\n'); continue; }

		/* form of \uYYYY */
		if (c != 'u' || ctx->left < 4) break;
		if (!PackedCol_Unhex(ctx->cur, h, 4)) break;

		codepoint = (h[0] << 12) | (h[1] << 8) | (h[2] << 4) | h[3];
		/* don't want control characters in names/software */
		/* TODO: Convert to CP437.. */
		if (codepoint >= 32) String_Append(str, codepoint);
		JsonContext_Consume(ctx, 4);
	}

	ctx->failed = true; str->length = 0;
}
static cc_string Json_ConsumeValue(int token, struct JsonContext* ctx);

static void Json_ConsumeObject(struct JsonContext* ctx) {
	char keyBuffer[STRING_SIZE];
	cc_string value, oldKey = ctx->curKey;
	int token;
	ctx->depth++;
	ctx->OnNewObject(ctx);

	while (true) {
		token = Json_ConsumeToken(ctx);
		if (token == ',') continue;
		if (token == '}') break;

		if (token != '"') { ctx->failed = true; break; }
		String_InitArray(ctx->curKey, keyBuffer);
		Json_ConsumeString(ctx, &ctx->curKey);

		token = Json_ConsumeToken(ctx);
		if (token != ':') { ctx->failed = true; break; }

		token = Json_ConsumeToken(ctx);
		if (token == TOKEN_NONE) { ctx->failed = true; break; }

		value = Json_ConsumeValue(token, ctx);
		ctx->OnValue(ctx, &value);
		ctx->curKey = oldKey;
	}
	ctx->depth--;
}

static void Json_ConsumeArray(struct JsonContext* ctx) {
	cc_string value;
	int token;
	ctx->depth++;
	ctx->OnNewArray(ctx);

	while (true) {
		token = Json_ConsumeToken(ctx);
		if (token == ',') continue;
		if (token == ']') break;

		if (token == TOKEN_NONE) { ctx->failed = true; break; }
		value = Json_ConsumeValue(token, ctx);
		ctx->OnValue(ctx, &value);
	}
	ctx->depth--;
}

static cc_string Json_ConsumeValue(int token, struct JsonContext* ctx) {
	switch (token) {
	case '{': Json_ConsumeObject(ctx); break;
	case '[': Json_ConsumeArray(ctx);  break;
	case '"': Json_ConsumeString(ctx, &ctx->_tmp); return ctx->_tmp;

	case TOKEN_NUM:   return Json_ConsumeNumber(ctx);
	case TOKEN_TRUE:  return strTrue;
	case TOKEN_FALSE: return strFalse;
	case TOKEN_NULL:  break;
	}
	return String_Empty;
}

static void Json_NullOnNew(struct JsonContext* ctx) { }
static void Json_NullOnValue(struct JsonContext* ctx, const cc_string* v) { }
void Json_Init(struct JsonContext* ctx, STRING_REF char* str, int len) {
	ctx->cur    = str;
	ctx->left   = len;
	ctx->failed = false;
	ctx->curKey = String_Empty;
	ctx->depth  = 0;

	ctx->OnNewArray  = Json_NullOnNew;
	ctx->OnNewObject = Json_NullOnNew;
	ctx->OnValue     = Json_NullOnValue;
	String_InitArray(ctx->_tmp, ctx->_tmpBuffer);
}

cc_bool Json_Parse(struct JsonContext* ctx) {
	int token;
	do {
		token = Json_ConsumeToken(ctx);
		Json_ConsumeValue(token, ctx);
	} while (token != TOKEN_NONE);

	return !ctx->failed;
}

static cc_bool Json_Handle(cc_uint8* data, cc_uint32 len, 
						JsonOnValue onVal, JsonOnNew newArr, JsonOnNew newObj) {
	struct JsonContext ctx;
	/* NOTE: classicube.net uses \u JSON for non ASCII, no need to UTF8 convert characters here */
	Json_Init(&ctx, (char*)data, len);
	
	if (onVal)  ctx.OnValue     = onVal;
	if (newArr) ctx.OnNewArray  = newArr;
	if (newObj) ctx.OnNewObject = newObj;
	return Json_Parse(&ctx);
}


/*########################################################################################################################*
*--------------------------------------------------------Web task---------------------------------------------------------*
*#########################################################################################################################*/
static char servicesBuffer[FILENAME_SIZE];
static cc_string servicesServer = String_FromArray(servicesBuffer);
static struct StringsBuffer CC_BIG_VAR ccCookies;

static void LWebTask_Reset(struct LWebTask* task) {
	task->completed = false;
	task->working   = true;
	task->success   = false;
}

void LWebTask_Tick(struct LWebTask* task, LWebTask_ErrorCallback errorCallback) {
	struct HttpRequest item;

	if (task->completed) return;
	if (!Http_GetResult(task->reqID, &item)) return;

	task->working   = false;
	task->completed = true;
	task->success   = item.success;

	if (item.success) {
		task->Handle(item.data, item.size);
	} else if (errorCallback) {
		errorCallback(&item);
	}
	HttpRequest_Free(&item);
}

void LWebTasks_Init(void) {
	Options_Get(SOPT_SERVICES, &servicesServer, SERVICES_SERVER);
}


/*########################################################################################################################*
*-------------------------------------------------------GetTokenTask------------------------------------------------------*
*#########################################################################################################################*/
/*
< GET /api/login/

> {
>	"username": null,
>	"authenticated": false,
>	"token": "f033ab37c30201f73f142449d037028d",
>	"errors": []
>}
*/
struct GetTokenTaskData GetTokenTask;

static void GetTokenTask_OnValue(struct JsonContext* ctx, const cc_string* str) {
	if (String_CaselessEqualsConst(&ctx->curKey, "token")) {
		String_Copy(&GetTokenTask.token, str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "username")) {
		String_Copy(&GetTokenTask.username, str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "errors")) {
		if (str->length) GetTokenTask.error = true;
	}
}

static void GetTokenTask_Handle(cc_uint8* data, cc_uint32 len) {
	static cc_string err_msg = String_FromConst("Error parsing get login token response JSON");

	cc_bool success = Json_Handle(data, len, GetTokenTask_OnValue, NULL, NULL);
	if (!success) Logger_WarnFunc(&err_msg);
}

void GetTokenTask_Run(void) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	static char tokenBuffer[STRING_SIZE];
	static char userBuffer[STRING_SIZE];
	if (GetTokenTask.Base.working) return;

	LWebTask_Reset(&GetTokenTask.Base);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "%s/login", &servicesServer);

	String_InitArray(GetTokenTask.token,    tokenBuffer);
	String_InitArray(GetTokenTask.username, userBuffer);
	GetTokenTask.error = false;

	GetTokenTask.Base.Handle = GetTokenTask_Handle;
	GetTokenTask.Base.reqID  = Http_AsyncGetDataEx(&url, 0, NULL, NULL, &ccCookies);
}


/*########################################################################################################################*
*--------------------------------------------------------SignInTask-------------------------------------------------------*
*#########################################################################################################################*/
/*
< POST /api/login/
< username=AndrewPH&password=examplePassW0rd&token=f033ab37c30201f73f142449d037028d

> {
> 	"username": "AndrewPH",
> 	"authenticated": true,
> 	"token": "33e75ff09dd601bbe69f351039152189",
> 	"errors": []
> }
*/
struct SignInTaskData SignInTask;

static void SignInTask_LogError(const cc_string* str) {
	static char errBuffer[128];
	cc_string err;

	if (String_CaselessEqualsConst(str, "username") || String_CaselessEqualsConst(str, "password")) {
		SignInTask.error   = "&cWrong username or password";
	} else if (String_CaselessEqualsConst(str, "verification")) {
		SignInTask.error   = "&cAccount verification required";
	} else if (String_CaselessEqualsConst(str, "login_code")) {
		SignInTask.error   = "&cLogin code required (Check your emails)";
		SignInTask.needMFA = true;
	} else if (str->length) {
		String_InitArray_NT(err, errBuffer);
		String_Format1(&err, "&c%s", str);

		errBuffer[err.length] = '\0';
		SignInTask.error = errBuffer;
	}
}

static void SignInTask_OnValue(struct JsonContext* ctx, const cc_string* str) {
	if (String_CaselessEqualsConst(&ctx->curKey, "username")) {
		String_Copy(&SignInTask.username, str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "errors")) {
		SignInTask_LogError(str);
	}
}

static void SignInTask_Handle(cc_uint8* data, cc_uint32 len) {
	static cc_string err_msg = String_FromConst("Error parsing sign in response JSON");

	cc_bool success = Json_Handle(data, len, SignInTask_OnValue, NULL, NULL);
	if (success) return;
	
	SignInTask.error = "&cError parsing response";
	Logger_WarnFunc(&err_msg);
}

static void SignInTask_Append(cc_string* dst, const char* key, const cc_string* value) {
	String_AppendConst(dst, key);
	Http_UrlEncodeUtf8(dst, value);
}

void SignInTask_Run(const cc_string* user, const cc_string* pass, const cc_string* mfaCode) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	static char userBuffer[STRING_SIZE];
	cc_string args; char argsBuffer[1024];
	if (SignInTask.Base.working) return;

	LWebTask_Reset(&SignInTask.Base);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "%s/login", &servicesServer);

	String_InitArray(SignInTask.username, userBuffer);
	SignInTask.error   = NULL;
	SignInTask.needMFA = false;

	String_InitArray(args, argsBuffer);
	SignInTask_Append(&args, "username=",    user);
	SignInTask_Append(&args, "&password=",   pass);
	SignInTask_Append(&args, "&token=",      &GetTokenTask.token);
	SignInTask_Append(&args, "&login_code=", mfaCode);

	SignInTask.Base.Handle = SignInTask_Handle;
	SignInTask.Base.reqID  = Http_AsyncPostData(&url, 0, args.buffer, args.length, &ccCookies);
}


/*########################################################################################################################*
*-----------------------------------------------------FetchServerTask-----------------------------------------------------*
*#########################################################################################################################*/
/*
< GET /api/server/a709fabdf836a2a102c952442bf2dab1

> { "servers" : [
>	{"hash": "a709fabdf836a2a102c952442bf2dab1", "maxplayers": 70, "name": "Freebuild server", "players": 5, "software": "MCGalaxy", "uptime": 185447, "country_abbr": "CA"},
> ]}
*/
struct FetchServerData FetchServerTask;
static struct ServerInfo* curServer;

static void ServerInfo_Init(struct ServerInfo* info) {
	String_InitArray(info->hash, info->_hashBuffer);
	String_InitArray(info->name, info->_nameBuffer);
	String_InitArray(info->ip,   info->_ipBuffer);
	String_InitArray(info->mppass,   info->_mppassBuffer);
	String_InitArray(info->software, info->_softBuffer);

	info->players    = 0;
	info->maxPlayers = 0;
	info->uptime     = 0;
	info->featured   = false;
	info->country[0] = 't';
	info->country[1] = '1'; /* 'T1' for unrecognised country */
	info->_order     = -100000;
}

static void ServerInfo_Parse(struct JsonContext* ctx, const cc_string* val) {
	struct ServerInfo* info = curServer;
	if (String_CaselessEqualsConst(&ctx->curKey, "hash")) {
		String_Copy(&info->hash, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "name")) {
		String_Copy(&info->name, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "players")) {
		Convert_ParseInt(val, &info->players);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "maxplayers")) {
		Convert_ParseInt(val, &info->maxPlayers);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "uptime")) {
		Convert_ParseInt(val, &info->uptime);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "mppass")) {
		String_Copy(&info->mppass, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "ip")) {
		String_Copy(&info->ip, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "port")) {
		Convert_ParseInt(val, &info->port);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "software")) {
		String_Copy(&info->software, val);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "featured")) {
		Convert_ParseBool(val, &info->featured);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "country_abbr")) {
		/* Two letter country codes, see ISO 3166-1 alpha-2 */
		if (val->length < 2) return;

		/* classicube.net only works with lowercase flag urls */
		info->country[0] = val->buffer[0]; Char_MakeLower(info->country[0]);
		info->country[1] = val->buffer[1]; Char_MakeLower(info->country[1]);
	}
}

static void FetchServerTask_Handle(cc_uint8* data, cc_uint32 len) {
	curServer = &FetchServerTask.server;
	Json_Handle(data, len, ServerInfo_Parse, NULL, NULL);
}

void FetchServerTask_Run(const cc_string* hash) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	if (FetchServerTask.Base.working) return;

	LWebTask_Reset(&FetchServerTask.Base);
	ServerInfo_Init(&FetchServerTask.server);
	String_InitArray(url, urlBuffer);
	String_Format2(&url, "%s/server/%s", &servicesServer, hash);

	FetchServerTask.Base.Handle = FetchServerTask_Handle;
	FetchServerTask.Base.reqID  = Http_AsyncGetDataEx(&url, 0, NULL, NULL, &ccCookies);
}


/*########################################################################################################################*
*-----------------------------------------------------FetchServersTask----------------------------------------------------*
*#########################################################################################################################*/
/*
< GET /api/servers/

> { "servers" : [
>	{"hash": "a709fabdf836a2a102c952442bf2dab1", "maxplayers": 70, "name": "Freebuild server", "players": 5, "software": "MCGalaxy", "uptime": 185447, "country_abbr": "CA"},
>	{"hash": "23860c5e192cbaa4698408338efd61cc", "maxplayers": 30, "name": "Other server", "players": 0, software: "", "uptime": 54661, "country_abbr": "T1"}
> ]}
*/
struct FetchServersData FetchServersTask;
static void FetchServersTask_Count(struct JsonContext* ctx) {
	/* JSON is expected in this format: */
	/*  { "servers" :      (depth = 1)  */
	/*    [                (depth = 2)  */
	/*	     { server1 },  (depth = 3)  */
	/*		 { server2 },  (depth = 3)  */
	/*          ...                     */
	if (ctx->depth != 3) return;
	FetchServersTask.numServers++;
}

static void FetchServersTask_Next(struct JsonContext* ctx) {
	if (ctx->depth != 3) return;
	curServer++;
	ServerInfo_Init(curServer);
}

static void FetchServersTask_Handle(cc_uint8* data, cc_uint32 len) {
	static cc_string err_msg = String_FromConst("Error parsing servers list response JSON");

	int count;
	cc_bool success;
	Mem_Free(FetchServersTask.servers);
	Mem_Free(FetchServersTask.orders);
	Session_Save();

	FetchServersTask.numServers = 0;
	FetchServersTask.servers    = NULL;
	FetchServersTask.orders     = NULL;

	FetchServersTask.numServers = 0;
	success = Json_Handle(data, len, NULL, NULL, FetchServersTask_Count);
	count   = FetchServersTask.numServers;

	if (!success) Logger_WarnFunc(&err_msg);
	if (count <= 0) return;
	FetchServersTask.servers = (struct ServerInfo*)Mem_Alloc(count, sizeof(struct ServerInfo), "servers list");
	FetchServersTask.orders  = (cc_uint16*)Mem_Alloc(count, 2, "servers order");

	curServer = FetchServersTask.servers - 1;
	Json_Handle(data, len, ServerInfo_Parse, NULL, FetchServersTask_Next);
}

void FetchServersTask_Run(void) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	if (FetchServersTask.Base.working) return;

	LWebTask_Reset(&FetchServersTask.Base);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "%s/servers", &servicesServer);

	FetchServersTask.Base.Handle = FetchServersTask_Handle;
	FetchServersTask.Base.reqID  = Http_AsyncGetDataEx(&url, 0, NULL, NULL, &ccCookies);
}

void FetchServersTask_ResetOrder(void) {
	int i;
	for (i = 0; i < FetchServersTask.numServers; i++) {
		FetchServersTask.orders[i] = i;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------CheckUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
/*
< GET /builds.json

> {"latest_ts": 1718187640.9587102, "release_ts": 1693265172.020421, "release_version": "1.3.6"}
*/
struct CheckUpdateData CheckUpdateTask;
static char relVersionBuffer[16];

CC_NOINLINE static cc_uint64 CheckUpdateTask_ParseTime(const cc_string* str) {
	cc_string time, fractional;
	cc_uint64 secs;
	/* timestamp is in form of "seconds.fractional" */
	/* But only need to care about the seconds here */
	String_UNSAFE_Separate(str, '.', &time, &fractional);

	Convert_ParseUInt64(&time, &secs);
	return secs;
}

static void CheckUpdateTask_OnValue(struct JsonContext* ctx, const cc_string* str) {
	if (String_CaselessEqualsConst(&ctx->curKey, "release_version")) {
		String_Copy(&CheckUpdateTask.latestRelease, str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "latest_ts")) {
		CheckUpdateTask.devTimestamp = CheckUpdateTask_ParseTime(str);
	} else if (String_CaselessEqualsConst(&ctx->curKey, "release_ts")) {
		CheckUpdateTask.relTimestamp = CheckUpdateTask_ParseTime(str);
	}
}

static void CheckUpdateTask_Handle(cc_uint8* data, cc_uint32 len) {
	static cc_string err_msg = String_FromConst("Error parsing update check response JSON");

	cc_bool success = Json_Handle(data, len, CheckUpdateTask_OnValue, NULL, NULL);
	if (!success) Logger_WarnFunc(&err_msg);
}

void CheckUpdateTask_Run(void) {
	static const cc_string url = String_FromConst(UPDATES_SERVER "/builds.json");
	if (CheckUpdateTask.Base.working) return;

	LWebTask_Reset(&CheckUpdateTask.Base);
	String_InitArray(CheckUpdateTask.latestRelease, relVersionBuffer);

	CheckUpdateTask.Base.Handle = CheckUpdateTask_Handle;
	CheckUpdateTask.Base.reqID  = Http_AsyncGetData(&url, 0);
}


/*########################################################################################################################*
*-----------------------------------------------------FetchUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchUpdateData FetchUpdateTask;
static void FetchUpdateTask_Handle(cc_uint8* data, cc_uint32 len) {
	static const cc_string path = String_FromConst(UPDATE_FILE);
	cc_result res;

	res = Stream_WriteAllTo(&path, data, len);
	if (res) { Logger_SysWarn(res, "saving update"); return; }

	res = Updater_SetNewBuildTime(FetchUpdateTask.timestamp);
	if (res) Logger_SysWarn(res, "setting update time");

	res = Updater_MarkExecutable();
	if (res) Logger_SysWarn(res, "making update executable");

#ifdef CC_BUILD_WIN
	Options_SetBool("update-dirty", true);
#endif
}

void FetchUpdateTask_Run(cc_bool release, int buildIndex) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	String_Format2(&url, UPDATES_SERVER "/%c/%c",
		release ? "release" : "latest",
		Updater_Info.builds[buildIndex].path);

	LWebTask_Reset(&FetchUpdateTask.Base);
	FetchUpdateTask.timestamp   = release ? CheckUpdateTask.relTimestamp : CheckUpdateTask.devTimestamp;
	FetchUpdateTask.Base.Handle = FetchUpdateTask_Handle;
	FetchUpdateTask.Base.reqID  = Http_AsyncGetData(&url, 0);
}


/*########################################################################################################################*
*-----------------------------------------------------FetchFlagsTask------------------------------------------------------*
*#########################################################################################################################*/
struct FetchFlagsData FetchFlagsTask;
static int flagsCount, flagsCapacity;
static struct Flag* flags;

static void FetchFlagsTask_DownloadNext(void);
static void FetchFlagsTask_Handle(cc_uint8* data, cc_uint32 len) {
	struct Flag* flag = &flags[FetchFlagsTask.count];
	LBackend_DecodeFlag(flag, data, len);
	
	FetchFlagsTask.count++;
	FetchFlagsTask_DownloadNext();
}

static void FetchFlagsTask_DownloadNext(void) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];
	String_InitArray(url, urlBuffer);

	if (FetchFlagsTask.Base.working)        return;
	if (FetchFlagsTask.count == flagsCount) return;

	LWebTask_Reset(&FetchFlagsTask.Base);
	String_Format2(&url, RESOURCE_SERVER "/img/flags/%r%r.png",
			&flags[FetchFlagsTask.count].country[0], &flags[FetchFlagsTask.count].country[1]);

	FetchFlagsTask.Base.Handle = FetchFlagsTask_Handle;
	FetchFlagsTask.Base.reqID  = Http_AsyncGetData(&url, 0);
}

static void FetchFlagsTask_Ensure(void) {
	if (flagsCount < flagsCapacity) return;
	flagsCapacity = flagsCount + 10;

	if (flags) {
		flags = (struct Flag*)Mem_Realloc(flags, flagsCapacity, sizeof(struct Flag), "flags");
	} else {
		flags = (struct Flag*)Mem_Alloc(flagsCapacity,          sizeof(struct Flag), "flags");
	}
}

void FetchFlagsTask_Add(const struct ServerInfo* server) {
	int i;
	for (i = 0; i < flagsCount; i++) 
	{
		if (flags[i].country[0] != server->country[0]) continue;
		if (flags[i].country[1] != server->country[1]) continue;
		/* flag is already or will be downloaded */
		return;
	}
	FetchFlagsTask_Ensure();

	Bitmap_Init(flags[flagsCount].bmp, 0, 0, NULL);
	flags[flagsCount].country[0] = server->country[0];
	flags[flagsCount].country[1] = server->country[1];
	flags[flagsCount].meta = NULL;

	flagsCount++;
	FetchFlagsTask_DownloadNext();
}

struct Flag* Flags_Get(const struct ServerInfo* server) {
	int i;
	for (i = 0; i < FetchFlagsTask.count; i++) 
	{
		if (flags[i].country[0] != server->country[0]) continue;
		if (flags[i].country[1] != server->country[1]) continue;
		return &flags[i];
	}
	return NULL;
}

void Flags_Free(void) {
	int i;
	for (i = 0; i < FetchFlagsTask.count; i++) {
		Mem_Free(flags[i].bmp.scan0);
	}

    flagsCount = 0;
    FetchFlagsTask.count = 0;
}


/*########################################################################################################################*
*------------------------------------------------------Session cache------------------------------------------------------*
*#########################################################################################################################*/
static cc_string sessionKey = String_FromConst("session");
static cc_bool loadedSession;

#ifdef CC_BUILD_NETWORKING
void Session_Load(void) {
	cc_string session; char buffer[3072];
	if (loadedSession) return;
	loadedSession = true;
	/* Increase from max 512 to 2048 per entry */
	StringsBuffer_SetLengthBits(&ccCookies, 11);

	String_InitArray(session, buffer);
	Options_GetSecure(LOPT_SESSION, &session);
	if (!session.length) return;
	EntryList_Set(&ccCookies, &sessionKey, &session, '=');
}

void Session_Save(void) {
	cc_string session = EntryList_UNSAFE_Get(&ccCookies, &sessionKey, '=');
	if (!session.length) return;
	Options_SetSecure(LOPT_SESSION, &session);
}
#else
void Session_Load(void) { }

void Session_Save(void) { }
#endif

#endif
