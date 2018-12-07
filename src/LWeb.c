#include "LWeb.h"
#include "Platform.h"

static void LWebTask_Reset(struct LWebTask* task) {
	task->Completed = false;
	task->Working   = true;
	task->Success   = false;
	task->Start = DateTime_CurrentUTC_MS();
}

static void LWebTask_StartAsync(struct LWebTask* task) {
	LWebTask_Reset(task);
	task->Begin(task);
}

void LWebTask_Tick(struct LWebTask* task) {
	struct AsyncRequest req;
	int delta;
	if (task->Completed) return;

	if (!AsyncDownloader_Get(&task->Identifier, &req)) return;
	delta = (int)(DateTime_CurrentUTC_MS() - task->Start);
	Platform_Log2("%s took %i", &task->Identifier, &delta);

	task->Completed = true;
	task->Success   = req.ResultData && req.ResultSize;
	if (task->Success) task->Handle(task, req.ResultData, req.ResultSize);
}

static void LWebTask_DefaultBegin(struct LWebTask* task) {
	AsyncDownloader_GetData(&task->URL, false, &task->Identifier);
}
/*
protected static JsonObject ParseJson(Request req) {
	JsonContext ctx = new JsonContext();
	ctx.Val = (string)req.Data;
	return (JsonObject)Json.ParseStream(ctx);
}

public sealed class GetCSRFTokenTask : WebTask {
	public GetCSRFTokenTask() {
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


public class ServerListEntry {
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

	public static ServerListEntry ParseEntry(JsonObject obj) {
		ServerListEntry entry = new ServerListEntry();
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
		ServerListEntry entry = ParseEntry(obj);
		Info = new ClientStartData(Username, entry.Mppass, entry.IPAddress, entry.Port, entry.Name);
	}
}

public sealed class FetchServersTask : WebTask {
	public FetchServersTask() {
		identifier = "CC get servers";
		uri = "https://www.classicube.net/api/servers";
	}

	public List<ServerListEntry> Servers;
	protected override void Reset() {
		base.Reset();
		Servers = new List<ServerListEntry>();
	}

	protected override void Handle(Request req) {
		JsonObject root = ParseJson(req);
		List<object> list = (List<object>)root["servers"];

		for (int i = 0; i < list.Count; i++) {
			JsonObject obj = (JsonObject)list[i];
			ServerListEntry entry = FetchServerTask.ParseEntry(obj);
			Servers.Add(entry);
		}
	}
}


public class Build {
	public DateTime TimeBuilt;
	public string DirectXPath, OpenGLPath;
	public int DirectXSize, OpenGLSize;
	public string Version;
}

public sealed class UpdateCheckTask : WebTask {
	public UpdateCheckTask() {
		identifier = "CC update check";
		uri = "http://cs.classicube.net/builds.json";
	}

	public Build LatestDev, LatestStable;

	protected override void Handle(Request req) {
		JsonObject data = ParseJson(req);
		JsonObject latest = (JsonObject)data["latest"];

		LatestDev = MakeBuild(latest, false);
		JsonObject releases = (JsonObject)data["releases"];

		DateTime releaseTime = DateTime.MinValue;
		foreach(KeyValuePair<string, object> pair in releases) {
			Build build = MakeBuild((JsonObject)pair.Value, true);
			if (build.TimeBuilt < releaseTime) continue;

			LatestStable = build;
			releaseTime = build.TimeBuilt;
		}
	}

	static readonly DateTime epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
	Build MakeBuild(JsonObject obj, bool release) {
		Build build = new Build();
		string timeKey = release ? "release_ts" : "ts";
		double rawTime = Double.Parse((string)obj[timeKey], CultureInfo.InvariantCulture);
		build.TimeBuilt = epoch.AddSeconds(rawTime).ToLocalTime();

		build.DirectXSize = Int32.Parse((string)obj["dx_size"]);
		build.DirectXPath = (string)obj["dx_file"];
		build.OpenGLSize = Int32.Parse((string)obj["ogl_size"]);
		build.OpenGLPath = (string)obj["ogl_file"];
		if (obj.ContainsKey("version"))
			build.Version = (string)obj["version"];
		return build;
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