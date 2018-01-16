// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using ClassicalSharp;
using ClassicalSharp.Network;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System;
using System.Net;
using JsonObject = System.Collections.Generic.Dictionary<string, object>;

namespace Launcher.Web {
	
	public abstract class WebTask {
		public LauncherWindow Game;
		public bool Completed = false, Success = false;
		public WebException WebEx;
		
		DateTime start;
		protected string identifier, uri, section;
		
		public void RunAsync(LauncherWindow game) {
			Game = game;
			Reset();
			Begin();
		}

		public void Tick() {
			if (Completed) return;
			Request req;
			if (!Game.Downloader.TryGetItem(identifier, out req)) return;
			Utils.LogDebug(identifier + " took " + (DateTime.UtcNow - start).TotalMilliseconds);
			
			WebEx = req.WebEx;
			Completed = true;
			Success = req != null && req.Data != null;
			if (Success) Handle(req);
		}
		
		protected virtual void Reset() {
			Completed = false; Success = false;
			start = DateTime.UtcNow;
		}		

		protected virtual void Begin() {
			Game.Downloader.AsyncGetString(uri, false, identifier);
		}
		
		protected abstract void Handle(Request req);
	}
	
	public sealed class GetCSRFTokenTask : WebTask {	
		public GetCSRFTokenTask() {
			identifier = "CC get login";
			uri = "https://www.classicube.net/api/login/";
		}
		public string Token;
		
		protected override void Handle(Request req) {
			int index = 0; bool success = true;
			JsonObject data = (JsonObject)Json.ParseValue((string)req.Data, ref index, ref success);
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
			int index = 0; bool success = true;
			JsonObject data = (JsonObject)Json.ParseValue((string)req.Data, ref index, ref success);
			
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
			entry.Hash       = (string)obj["hash"];
			entry.Name       = (string)obj["name"];
			entry.Players    = (string)obj["players"];
			entry.MaxPlayers = (string)obj["maxplayers"];
			entry.Uptime     = (string)obj["uptime"];
			entry.Mppass     = (string)obj["mppass"];
			entry.IPAddress  = (string)obj["ip"];
			entry.Port       = (string)obj["port"];
			entry.Software   = (string)obj["software"];
			
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
			int index = 0; bool success = true;
			JsonObject root = (JsonObject)Json.ParseValue((string)req.Data, ref index, ref success);
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
			int index = 0; bool success = true;
			JsonObject root = (JsonObject)Json.ParseValue((string)req.Data, ref index, ref success);
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
			int index = 0; bool success = true;
			JsonObject data = (JsonObject)Json.ParseValue((string)req.Data, ref index, ref success);
			JsonObject latest = (JsonObject)data["latest"], releases = (JsonObject)data["releases"];
			LatestDev = MakeBuild(latest, false);
			
			DateTime releaseTime = DateTime.MinValue;
			foreach (KeyValuePair<string, object> pair in releases) {
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
}