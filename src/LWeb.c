#include "LWeb.h"
#include "Platform.h"

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

static String strTrue  = String_FromConst("true");
static String strFalse = String_FromConst("false");
static String strNull  = String_FromConst("null");

static bool Json_IsWhitespace(char c) {
	return c == '\r' || c == '\n' || c == '\t' || c == ' ';
}

static bool Json_IsNumber(char c) {
	return c == '-' || c == '.' || (c >= '0' && c <= '9');
}

static bool Json_ConsumeConstant(struct JsonContext* ctx, String* value) {
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
	struct AsyncRequest req;
	int delta;
	if (task->Completed) return;

	if (!AsyncDownloader_Get(&task->Identifier, &req)) return;
	delta = (int)(DateTime_CurrentUTC_MS() - task->Start);
	Platform_Log2("%s took %i", &task->Identifier, &delta);

	task->Res    = req.Result;
	task->Status = req.StatusCode;

	task->Working   = false;
	task->Completed = true;
	task->Success   = !task->Res && req.ResultData && req.ResultSize;
	if (task->Success) task->Handle(req.ResultData, req.ResultSize);
}

static void LWebTask_DefaultBegin(struct LWebTask* task) {
	AsyncDownloader_GetData(&task->URL, false, &task->Identifier);
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
	static String id  = String_FromConst("CC get token");
	static String url = String_FromConst("https://www.classicube.net/api/login");
	if (GetTokenTask.Base.Working) return;

	LWebTask_Reset(&GetTokenTask.Base);
	String_InitArray(GetTokenTask.Token, tokenBuffer);

	GetTokenTask.Base.Identifier = id;
	AsyncDownloader_GetData(&url, false, &id);
	GetTokenTask.Base.Handle     = GetTokenTask_Handle;
}


/*########################################################################################################################*
*--------------------------------------------------------SignInTask-------------------------------------------------------*
*#########################################################################################################################*/
struct SignInTaskData SignInTask;
void SignInTask_Run(const String* user, const String* pass);


/*########################################################################################################################*
*-----------------------------------------------------FetchServerTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchServerData FetchServerTask;
void FetchServerTask_Run(const String* hash);


/*########################################################################################################################*
*-----------------------------------------------------FetchServersTask----------------------------------------------------*
*#########################################################################################################################*/
struct FetchServersData FetchServersTask;
void FetchServersTask_Run(void);


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
	static String id  = String_FromConst("CC update check");
	static String url = String_FromConst("http://cs.classicube.net/c_client/builds.json");
	if (CheckUpdateTask.Base.Working) return;

	LWebTask_Reset(&CheckUpdateTask.Base);
	CheckUpdateTask.DevTimestamp = 0;
	CheckUpdateTask.RelTimestamp = 0;
	String_InitArray(CheckUpdateTask.LatestRelease, relVersionBuffer);

	CheckUpdateTask.Base.Identifier = id;
	AsyncDownloader_GetData(&url, false, &id);
	CheckUpdateTask.Base.Handle     = CheckUpdateTask_Handle;
}


/*########################################################################################################################*
*-----------------------------------------------------FetchUpdateTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchUpdateData FetchUpdateTask;
void FetchUpdateTask_Run(bool release, bool d3d9);


/*########################################################################################################################*
*-----------------------------------------------------FetchFlagsTask-----------------------------------------------------*
*#########################################################################################################################*/
struct FetchFlagsData FetchFlagsTask;
void FetchFlagsTask_Run(void);
void FetchFlagsTask_Add(const String* name);

/*
protected static JsonObject ParseJson(Request req) {
	JsonContext ctx = new JsonContext();
	ctx.Val = (string)req.Data;
	return (JsonObject)Json.ParseStream(ctx);
}

public sealed class GetTokenTask : WebTask {
	public GetTokenTask() {
		identifier = "CC get login";
		uri = "https://www.classicube.net/api/login/";
	}
	public string Token;

	protected override void Handle(Request req) {
		JsonObject data = ParseJson(req);
		Token = (string)data["token"];
	}
}

public sealed class SignInTask : WebTask {
	public SignInTask() {
		identifier = "CC post login";
		uri = "https://www.classicube.net/api/login/";
	}
	public string Username, Password, Token, Error;

	protected override void Begin() {
		string data = String.Format(
			"username={0}&password={1}&token={2}",
			Uri.EscapeDataString(Username),
			Uri.EscapeDataString(Password),
			Uri.EscapeDataString(Token)
		);
		Game.Downloader.AsyncPostString(uri, false, identifier, data);
	}

	protected override void Handle(Request req) {
		JsonObject data = ParseJson(req);
		Error = GetLoginError(data);
		Username = (string)data["username"];
	}

	static string GetLoginError(JsonObject obj) {
		List<object> errors = (List<object>)obj["errors"];
		if (errors.Count == 0) return null;

		string err = (string)errors[0];
		if (err == "username" || err == "password") return "Wrong username or password";
		if (err == "verification") return "Account verification required";
		return "Unknown error occurred";
	}
}


public class ServerInfo {
	public string Hash, Name, Players, MaxPlayers, Flag;
	public string Uptime, IPAddress, Port, Mppass, Software;
	public bool Featured;
}

public sealed class FetchServerTask : WebTask {
	public FetchServerTask(string user, string hash) {
		Username = user;
		identifier = "CC get servers";
		uri = "https://www.classicube.net/api/server/" + hash;
	}

	public static ServerInfo ParseEntry(JsonObject obj) {
		ServerInfo entry = new ServerInfo();
		entry.Hash = (string)obj["hash"];
		entry.Name = (string)obj["name"];
		entry.Players = (string)obj["players"];
		entry.MaxPlayers = (string)obj["maxplayers"];
		entry.Uptime = (string)obj["uptime"];
		entry.Mppass = (string)obj["mppass"];
		entry.IPAddress = (string)obj["ip"];
		entry.Port = (string)obj["port"];
		entry.Software = (string)obj["software"];

		if (obj.ContainsKey("featured")) {
			entry.Featured = (bool)obj["featured"];
		}
		if (obj.ContainsKey("country_abbr")) {
			entry.Flag = Utils.ToLower((string)obj["country_abbr"]);
		}
		return entry;
	}

	public string Username;
	public ClientStartData Info;

	protected override void Handle(Request req) {
		JsonObject root = ParseJson(req);
		List<object> list = (List<object>)root["servers"];

		JsonObject obj = (JsonObject)list[0];
		ServerInfo entry = ParseEntry(obj);
		Info = new ClientStartData(Username, entry.Mppass, entry.IPAddress, entry.Port, entry.Name);
	}
}

public sealed class FetchServersTask : WebTask {
	public FetchServersTask() {
		identifier = "CC get servers";
		uri = "https://www.classicube.net/api/servers";
	}

	public List<ServerInfo> Servers;
	protected override void Reset() {
		base.Reset();
		Servers = new List<ServerInfo>();
	}

	protected override void Handle(Request req) {
		JsonObject root = ParseJson(req);
		List<object> list = (List<object>)root["servers"];

		for (int i = 0; i < list.Count; i++) {
			JsonObject obj = (JsonObject)list[i];
			ServerInfo entry = FetchServerTask.ParseEntry(obj);
			Servers.Add(entry);
		}
	}
}

public sealed class UpdateDownloadTask : WebTask {
	public UpdateDownloadTask(string dir) {
		identifier = "CC update download";
		uri = "http://cs.classicube.net/" + dir;
	}

	public byte[] ZipFile;

	protected override void Begin() {
		Game.Downloader.AsyncGetData(uri, false, identifier);
	}

	protected override void Handle(Request req) {
		ZipFile = (byte[])req.Data;
	}
}

public sealed class UpdateCClientTask : WebTask {
	public UpdateCClientTask(string file) {
		identifier = "CC CClient download";
		uri = "http://cs.classicube.net/c_client/latest/" + file;
	}

	public byte[] File;

	protected override void Begin() {
		Game.Downloader.AsyncGetData(uri, false, identifier);
	}

	protected override void Handle(Request req) {
		File = (byte[])req.Data;
	}
}


public sealed class FetchFlagsTask : WebTask {
	public FetchFlagsTask() {
		identifier = "CC get flag";
	}

	public bool PendingRedraw;
	public static int DownloadedCount;
	public static List<string> Flags = new List<string>();
	public static List<FastBitmap> Bitmaps = new List<FastBitmap>();

	public void AsyncGetFlag(string flag) {
		for (int i = 0; i < Flags.Count; i++) {
			if (Flags[i] == flag) return;
		}
		Flags.Add(flag);
	}

	protected override void Begin() {
		if (Flags.Count == DownloadedCount) return;
		uri = "http://static.classicube.net/img/flags/" + Flags[DownloadedCount] + ".png";
		Game.Downloader.AsyncGetImage(uri, false, identifier);
	}

	protected override void Handle(Request req) {
		Bitmap bmp = (Bitmap)req.Data;
		FastBitmap fast = new FastBitmap(bmp, true, true);
		Bitmaps.Add(fast);
		DownloadedCount++;
		PendingRedraw = true;

		Reset();
		Begin();
	}
}
}*/