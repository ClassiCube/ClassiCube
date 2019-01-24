#include "LWeb.h"
#include "Launcher.h"
#include "Platform.h"
#include "Stream.h"
#include "Logger.h"

/*########################################################################################################################*
*----------------------------------------------------------JSON-----------------------------------------------------------*
*#########################################################################################################################*/
#define TOKEN_NONE  0
#define TOKEN_NUM   1
#define TOKEN_TRUE  2
#define TOKEN_FALSE 3
#define TOKEN_NULL  4
/* Consumes n characters from the JSON stream */
#define JsonContext_Consume(ctx, n) ctx->Cur += n; ctx->Left -= n;

const static String strTrue  = String_FromConst("true");
const static String strFalse = String_FromConst("false");
const static String strNull  = String_FromConst("null");

static bool Json_IsWhitespace(char c) {
	return c == '\r' || c == '\n' || c == '\t' || c == ' ';
}

static bool Json_IsNumber(char c) {
	return c == '-' || c == '.' || (c >= '0' && c <= '9');
}

static bool Json_ConsumeConstant(struct JsonContext* ctx, const String* value) {
	int i;
	if (value->length > ctx->Left) return false;

	for (i = 0; i < value->length; i++) {
		if (ctx->Cur[i] != value->buffer[i]) return false;
	}

	JsonContext_Consume(ctx, value->length);
	return true;
}

static int Json_ConsumeToken(struct JsonContext* ctx) {
	char c;
	for (; ctx->Left && Json_IsWhitespace(*ctx->Cur); ) { JsonContext_Consume(ctx, 1); }
	if (!ctx->Left) return TOKEN_NONE;

	c = *ctx->Cur;
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
	return TOKEN_NONE;
}

static String Json_ConsumeNumber(struct JsonContext* ctx) {
	int len = 0;
	for (; ctx->Left && Json_IsNumber(*ctx->Cur); len++) { JsonContext_Consume(ctx, 1); }
	return String_Init(ctx->Cur - len, len, len);
}

static void Json_ConsumeString(struct JsonContext* ctx, String* str) {
	int codepoint, h[4];
	char c;
	str->length = 0;

	for (; ctx->Left;) {
		c = *ctx->Cur; JsonContext_Consume(ctx, 1);
		if (c == '"') return;
		if (c != '\\') { String_Append(str, c); continue; }

		/* form of \X */
		if (!ctx->Left) break;
		c = *ctx->Cur; JsonContext_Consume(ctx, 1);
		if (c == '/' || c == '\\' || c == '"') { String_Append(str, c); continue; }

		/* form of \uYYYY */
		if (c != 'u' || ctx->Left < 4) break;

		if (!PackedCol_Unhex(ctx->Cur[0], &h[0])) break;
		if (!PackedCol_Unhex(ctx->Cur[1], &h[1])) break;
		if (!PackedCol_Unhex(ctx->Cur[2], &h[2])) break;
		if (!PackedCol_Unhex(ctx->Cur[3], &h[3])) break;

		codepoint = (h[0] << 12) | (h[1] << 8) | (h[2] << 4) | h[3];
		/* don't want control characters in names/software */
		/* TODO: Convert to CP437.. */
		if (codepoint >= 32) String_Append(str, codepoint);
		JsonContext_Consume(ctx, 4);
	}

	ctx->Failed = true; str->length = 0;
}
static String Json_ConsumeValue(int token, struct JsonContext* ctx);

static void Json_ConsumeObject(struct JsonContext* ctx) {
	char keyBuffer[STRING_SIZE];
	String value, oldKey = ctx->CurKey;
	int token;
	ctx->OnNewObject(ctx);

	while (true) {
		token = Json_ConsumeToken(ctx);
		if (token == ',') continue;
		if (token == '}') return;

		if (token != '"') { ctx->Failed = true; return; }
		String_InitArray(ctx->CurKey, keyBuffer);
		Json_ConsumeString(ctx, &ctx->CurKey);

		token = Json_ConsumeToken(ctx);
		if (token != ':') { ctx->Failed = true; return; }

		token = Json_ConsumeToken(ctx);
		if (token == TOKEN_NONE) { ctx->Failed = true; return; }

		value = Json_ConsumeValue(token, ctx);
		ctx->OnValue(ctx, &value);
		ctx->CurKey = oldKey;
	}
}

static void Json_ConsumeArray(struct JsonContext* ctx) {
	String value;
	int token;
	ctx->OnNewArray(ctx);

	while (true) {
		token = Json_ConsumeToken(ctx);
		if (token == ',') continue;
		if (token == ']') return;

		if (token == TOKEN_NONE) { ctx->Failed = true; return; }
		value = Json_ConsumeValue(token, ctx);
		ctx->OnValue(ctx, &value);
	}
}

static String Json_ConsumeValue(int token, struct JsonContext* ctx) {
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
static void Json_NullOnValue(struct JsonContext* ctx, const String* v) { }
void Json_Init(struct JsonContext* ctx, String* str) {
	ctx->Cur    = str->buffer;
	ctx->Left   = str->length;
	ctx->Failed = false;
	ctx->CurKey = String_Empty;

	ctx->OnNewArray  = Json_NullOnNew;
	ctx->OnNewObject = Json_NullOnNew;
	ctx->OnValue     = Json_NullOnValue;
	String_InitArray(ctx->_tmp, ctx->_tmpBuffer);
}

void Json_Parse(struct JsonContext* ctx) {
	int token;
	do {
		token = Json_ConsumeToken(ctx);
		Json_ConsumeValue(token, ctx);
	} while (token != TOKEN_NONE);
}

static void Json_Handle(uint8_t* data, uint32_t len, 
						JsonOnValue onVal, JsonOnNew newArr, JsonOnNew newObj) {
	struct JsonContext ctx;
	String str = String_Init(data, len, len);
	Json_Init(&ctx, &str);
	
	if (onVal)  ctx.OnValue     = onVal;
	if (newArr) ctx.OnNewArray  = newArr;
	if (newObj) ctx.OnNewObject = newObj;
	Json_Parse(&ctx);
}


/*########################################################################################################################*
*--------------------------------------------------------Web task---------------------------------------------------------*
*#########################################################################################################################*/
static void LWebTask_Reset(struct LWebTask* task) {
	task->Completed = false;
	task->Working   = true;
	task->Success   = false;
	task->Start     = DateTime_CurrentUTC_MS();
	task->Res       = 0;
	task->Status    = 0;
}

void LWebTask_Tick(struct LWebTask* task) {
	struct HttpRequest req;
	int delta;
	if (task->Completed) return;

	if (!Http_GetResult(&task->Identifier, &req)) return;
	delta = (int)(DateTime_CurrentUTC_MS() - task->Start);
	Platform_Log2("%s took %i", &task->Identifier, &delta);

	task->Res    = req.Result;
	task->Status = req.StatusCode;

	task->Working   = false;
	task->Completed = true;
	task->Success   = req.Success;
	if (task->Success) task->Handle(req.Data, req.Size);
	HttpRequest_Free(&req);
}


/*########################################################################################################################*
*-------------------------------------------------------GetTokenTask------------------------------------------------------*
*#########################################################################################################################*/
struct GetTokenTaskData GetTokenTask;
char tokenBuffer[STRING_SIZE];

static void GetTokenTask_OnValue(struct JsonContext* ctx, const String* str) {
	if (!String_CaselessEqualsConst(&ctx->CurKey, "token")) return;
	String_Copy(&GetTokenTask.Token, str);
}

static void GetTokenTask_Handle(uint8_t* data, uint32_t len) {
	Json_Handle(data, len, GetTokenTask_OnValue, NULL, NULL);
}

void GetTokenTask_Run(void) {
	const static String id  = String_FromConst("CC get token");
	const static String url = String_FromConst("https://www.classicube.net/api/login");
	if (GetTokenTask.Base.Working) return;

	LWebTask_Reset(&GetTokenTask.Base);
	String_InitArray(GetTokenTask.Token, tokenBuffer);

	GetTokenTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	GetTokenTask.Base.Handle     = GetTokenTask_Handle;
}


/*########################################################################################################################*
*--------------------------------------------------------SignInTask-------------------------------------------------------*
*#########################################################################################################################*/
struct SignInTaskData SignInTask;
char userBuffer[STRING_SIZE];

static void SignInTask_LogError(const String* str) {
	const static String userErr = String_FromConst("&cWrong username or password");
	const static String verErr  = String_FromConst("&cAccount verification required");
	const static String unkErr  = String_FromConst("&cUnknown error occurred");

	if (String_CaselessEqualsConst(str, "username") || String_CaselessEqualsConst(str, "password")) {
		SignInTask.Error = userErr;
	} else if (String_CaselessEqualsConst(str, "verification")) {
		SignInTask.Error = verErr;
	} else if (str->length) {
		SignInTask.Error = unkErr;
	}
}

static void SignInTask_OnValue(struct JsonContext* ctx, const String* str) {
	if (String_CaselessEqualsConst(&ctx->CurKey, "username")) {
		String_Copy(&SignInTask.Username, str);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "errors")) {
		SignInTask_LogError(str);
	}
}

static void SignInTask_Handle(uint8_t* data, uint32_t len) {
	Json_Handle(data, len, SignInTask_OnValue, NULL, NULL);
}

void SignInTask_Run(const String* user, const String* pass) {
	const static String id  = String_FromConst("CC post login");
	const static String url = String_FromConst("https://www.classicube.net/api/login");
	String tmp; char tmpBuffer[STRING_SIZE * 5];
	if (SignInTask.Base.Working) return;

	LWebTask_Reset(&SignInTask.Base);
	String_InitArray(SignInTask.Username, userBuffer);
	SignInTask.Error.length = 0;

	String_InitArray(tmp, tmpBuffer);
	/* TODO: URL ENCODE THIS.. */
	String_Format3(&tmp, "username=%s&password=%s&token=%s",
					user, pass, &GetTokenTask.Token);

	SignInTask.Base.Identifier = id;
	Http_AsyncPostData(&url, false, &id, tmp.buffer, tmp.length);
	SignInTask.Base.Handle     = SignInTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchServerTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchServerData FetchServerTask;
static struct ServerInfo* curServer;

static void ServerInfo_Init(struct ServerInfo* info) {
	String_InitArray(info->Hash, info->_hashBuffer);
	String_InitArray(info->Name, info->_nameBuffer);
	String_InitArray(info->IP,   info->_ipBuffer);

	String_InitArray(info->Mppass,   info->_mppassBuffer);
	String_InitArray(info->Software, info->_softBuffer);
	String_InitArray(info->Country,  info->_countryBuffer);

	info->Players    = 0;
	info->MaxPlayers = 0;
	info->Uptime     = 0;
	info->Featured   = false;
	info->_order     = -100000;
}

static void ServerInfo_Parse(struct JsonContext* ctx, const String* val) {
	struct ServerInfo* info = curServer;
	if (String_CaselessEqualsConst(&ctx->CurKey, "hash")) {
		String_Copy(&info->Hash, val);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "name")) {
		String_Copy(&info->Name, val);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "players")) {
		Convert_ParseInt(val, &info->Players);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "maxplayers")) {
		Convert_ParseInt(val, &info->MaxPlayers);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "uptime")) {
		Convert_ParseInt(val, &info->Uptime);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "mppass")) {
		String_Copy(&info->Mppass, val);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "ip")) {
		String_Copy(&info->IP, val);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "port")) {
		Convert_ParseInt(val, &info->Port);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "software")) {
		String_Copy(&info->Software, val);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "featured")) {
		Convert_ParseBool(val, &info->Featured);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "country_abbr")) {
		String_Copy(&info->Country, val);
	}
}

static void FetchServerTask_Handle(uint8_t* data, uint32_t len) {
	curServer = &FetchServerTask.Server;
	Json_Handle(data, len, ServerInfo_Parse, NULL, NULL);
}

void FetchServerTask_Run(const String* hash) {
	const static String id  = String_FromConst("CC fetch server");
	String url; char urlBuffer[STRING_SIZE];
	if (FetchServerTask.Base.Working) return;

	LWebTask_Reset(&FetchServerTask.Base);
	ServerInfo_Init(&FetchServerTask.Server);
	String_InitArray(url, urlBuffer);
	String_Format1(&url, "https://www.classicube.net/api/server/%s", hash);

	FetchServerTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	FetchServerTask.Base.Handle  = FetchServerTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchServersTask----------------------------------------------------*
*#########################################################################################################################*/
struct FetchServersData FetchServersTask;
static void FetchServersTask_Count(struct JsonContext* ctx) {
	FetchServersTask.NumServers++;
}

static void FetchServersTask_Next(struct JsonContext* ctx) {
	curServer++;
	if (curServer < FetchServersTask.Servers) return;
	ServerInfo_Init(curServer);
}

static void FetchServersTask_Handle(uint8_t* data, uint32_t len) {
	Mem_Free(FetchServersTask.Servers);
	Mem_Free(FetchServersTask.Orders);

	FetchServersTask.NumServers = 0;
	FetchServersTask.Servers    = NULL;
	FetchServersTask.Orders     = NULL;

	/* -1 because servers is surrounded by a { */
	FetchServersTask.NumServers = -1;
	Json_Handle(data, len, NULL, NULL, FetchServersTask_Count);

	if (FetchServersTask.NumServers <= 0) return;
	FetchServersTask.Servers = Mem_Alloc(FetchServersTask.NumServers, sizeof(struct ServerInfo), "servers list");
	FetchServersTask.Orders  = Mem_Alloc(FetchServersTask.NumServers, 2, "servers order");

	/* -2 because servers is surrounded by a { */
	curServer = FetchServersTask.Servers - 2;
	Json_Handle(data, len, ServerInfo_Parse, NULL, FetchServersTask_Next);
}

void FetchServersTask_Run(void) {
	const static String id  = String_FromConst("CC fetch servers");
	const static String url = String_FromConst("https://www.classicube.net/api/servers");
	if (FetchServersTask.Base.Working) return;
	LWebTask_Reset(&FetchServersTask.Base);

	FetchServersTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	FetchServersTask.Base.Handle = FetchServersTask_Handle;
}

void FetchServersTask_ResetOrder(void) {
	int i;
	for (i = 0; i < FetchServersTask.NumServers; i++) {
		FetchServersTask.Orders[i] = i;
	}
}


/*########################################################################################################################*
*-----------------------------------------------------CheckUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
struct CheckUpdateData CheckUpdateTask;
static char relVersionBuffer[16];

CC_NOINLINE static TimeMS CheckUpdateTask_ParseTime(const String* str) {
	String time, fractional;
	TimeMS ms;
	/* timestamp is in form of "seconds.fractional" */
	/* But only need to care about the seconds here */
	String_UNSAFE_Separate(str, '.', &time, &fractional);

	Convert_ParseUInt64(&time, &ms);
	if (!ms) return 0;
	return ms * 1000 + UNIX_EPOCH;
}

static void CheckUpdateTask_OnValue(struct JsonContext* ctx, const String* str) {
	if (String_CaselessEqualsConst(&ctx->CurKey, "release_version")) {
		String_Copy(&CheckUpdateTask.LatestRelease, str);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "latest_ts")) {
		CheckUpdateTask.DevTimestamp = CheckUpdateTask_ParseTime(str);
	} else if (String_CaselessEqualsConst(&ctx->CurKey, "release_ts")) {
		CheckUpdateTask.RelTimestamp = CheckUpdateTask_ParseTime(str);
	}
}

static void CheckUpdateTask_Handle(uint8_t* data, uint32_t len) {
	Json_Handle(data, len, CheckUpdateTask_OnValue, NULL, NULL);
}

void CheckUpdateTask_Run(void) {
	const static String id  = String_FromConst("CC update check");
	const static String url = String_FromConst("http://cs.classicube.net/c_client/builds.json");
	if (CheckUpdateTask.Base.Working) return;

	LWebTask_Reset(&CheckUpdateTask.Base);
	CheckUpdateTask.DevTimestamp = 0;
	CheckUpdateTask.RelTimestamp = 0;
	String_InitArray(CheckUpdateTask.LatestRelease, relVersionBuffer);

	CheckUpdateTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	CheckUpdateTask.Base.Handle     = CheckUpdateTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchUpdateData FetchUpdateTask;
static void FetchUpdateTask_Handle(uint8_t* data, uint32_t len) {
	const static String path = String_FromConst("ClassiCube.update");
	ReturnCode res;

	res = Stream_WriteAllTo(&path, data, len);
	if (res) { Logger_Warn(res, "saving update"); return; }

	res = File_SetModifiedTime(&path, FetchUpdateTask.Timestamp);
	if (res) Logger_Warn(res, "setting update time");

	res = Platform_MarkExecutable(&path);
	if (res) Logger_Warn(res, "making update executable");
}

void FetchUpdateTask_Run(bool release, bool d3d9) {
#if defined CC_BUILD_WIN
#if _M_IX86
	const char* exe_d3d9 = "ClassiCube.64.exe";
	const char* exe_ogl  = "ClassiCube.64-opengl.exe";
#elif _M_X64
	const char* exe_d3d9 = "ClassiCube.exe";
	const char* exe_ogl  = "ClassiCube.opengl.exe";
#endif
#elif defined CC_BUILD_LINUX
#if __i386__
	const char* exe_d3d9 = "ClassiCube.32";
	const char* exe_ogl  = "ClassiCube.32";
#elif __x86_64__
	const char* exe_d3d9 = "ClassiCube";
	const char* exe_ogl  = "ClassiCube";
#endif
#elif defined CC_BUILD_OSX
	const char* exe_d3d9 = "ClassiCube.osx";
	const char* exe_ogl  = "ClassiCube.osx";
#else
#warn "Unsupported platform, launcher updating will not work"
	const char* exe_d3d9 = "ClassiCube.unknown";
	const char* exe_ogl  = "ClassiCube.unknown";
#endif

	const static String id = String_FromConst("CC update fetch");
	String url; char urlBuffer[STRING_SIZE];
	String_InitArray(url, urlBuffer);

	String_Format2(&url, "http://cs.classicube.net/c_client/%c/%c",
		release ? "release" : "latest",
		d3d9    ? exe_d3d9  : exe_ogl);
	if (FetchUpdateTask.Base.Working) return;

	LWebTask_Reset(&FetchUpdateTask.Base);
	FetchUpdateTask.Timestamp = release ? CheckUpdateTask.RelTimestamp : CheckUpdateTask.DevTimestamp;

	FetchUpdateTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	FetchUpdateTask.Base.Handle = FetchUpdateTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchFlagsTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchFlagsData FetchFlagsTask;
static int flagsCount, flagsBufferCount;
struct FlagData {
	Bitmap  Bmp;
	String Name;
	char _nameBuffer[8];
};
static struct FlagData* flags;

static void FetchFlagsTask_DownloadNext(void);
static void FetchFlagsTask_Handle(uint8_t* data, uint32_t len) {
	struct Stream s;
	ReturnCode res;

	Stream_ReadonlyMemory(&s, data, len);
	res = Png_Decode(&flags[FetchFlagsTask.Count].Bmp, &s);
	if (res) Logger_Warn(res, "decoding flag");

	FetchFlagsTask.Count++;
	FetchFlagsTask_DownloadNext();
}

static void FetchFlagsTask_DownloadNext(void) {
	const static String id = String_FromConst("CC get flag");
	String url; char urlBuffer[STRING_SIZE];
	String_InitArray(url, urlBuffer);

	if (FetchFlagsTask.Base.Working)        return;
	if (FetchFlagsTask.Count == flagsCount) return;

	LWebTask_Reset(&FetchFlagsTask.Base);
	String_Format1(&url, "http://static.classicube.net/img/flags/%s.png", &flags[FetchFlagsTask.Count].Name);

	FetchFlagsTask.Base.Identifier = id;
	Http_AsyncGetData(&url, false, &id);
	FetchFlagsTask.Base.Handle = FetchFlagsTask_Handle;
}

static void FetchFlagsTask_Ensure(void) {
	if (flagsCount < flagsBufferCount) return;
	flagsBufferCount = flagsCount + 10;

	if (flags) {
		flags = Mem_Realloc(flags, flagsBufferCount, sizeof(struct FlagData), "flags");
	} else {
		flags = Mem_Alloc(flagsBufferCount, sizeof(struct FlagData), "flags");
	}
}

void FetchFlagsTask_Add(const String* name) {
	char c;
	int i;

	for (i = 0; i < flagsCount; i++) {
		if (String_CaselessEquals(name, &flags[i].Name)) return;
	}
	FetchFlagsTask_Ensure();
	
	Bitmap_Init(flags[flagsCount].Bmp, 0, 0, NULL);
	String_InitArray(flags[flagsCount].Name, flags[flagsCount]._nameBuffer);

	/* classicube.net only works with lowercase flag urls */
	for (i = 0; i < name->length; i++) {
		c = name->buffer[i];
		Char_MakeLower(c);
		String_Append(&flags[flagsCount].Name, c);
	}

	flagsCount++;
	FetchFlagsTask_DownloadNext();
}

Bitmap* Flags_Get(const String* name) {
	int i;
	for (i = 0; i < FetchFlagsTask.Count; i++) {
		if (!String_CaselessEquals(name, &flags[i].Name)) continue;
		return &flags[i].Bmp;
	}
	return NULL;
}

void Flags_Free(void) {
	int i;
	for (i = 0; i < FetchFlagsTask.Count; i++) {
		Mem_Free(flags[i].Bmp.Scan0);
	}
}
