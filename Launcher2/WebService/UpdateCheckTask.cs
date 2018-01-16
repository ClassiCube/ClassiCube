// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using ClassicalSharp;
using ClassicalSharp.Network;
using System;
using System.Collections.Generic;
using System.Globalization;
using JsonObject = System.Collections.Generic.Dictionary<string, object>;

namespace Launcher.Web {
	
	public class Build {
		public DateTime TimeBuilt;
		public string DirectXPath, OpenGLPath;
		public int DirectXSize, OpenGLSize;
		public string Version;
	}
	
	public sealed class UpdateCheckTask {
		
		public const string UpdatesIdentifier = "cc-update";
		public const string UpdatesUri = "http://cs.classicube.net/";
		public const string BuildsUri = "http://cs.classicube.net/builds.json";
		public Build LatestDev, LatestStable;
		public LauncherWindow Game;
		public bool Completed = false, Success = false;
		
		public void Init(LauncherWindow game) {
			Game = game;
			Completed = false;
			Success = false;
			Game.Downloader.DownloadPage(BuildsUri, false, UpdatesIdentifier);
		}
		
		public void Tick() {
			if (Completed) return;		
			Request req;
			if (!Game.Downloader.TryGetItem(UpdatesIdentifier, out req)) return;
			
			Completed = true;
			Success = req != null && req.Data != null;
			if (!Success) return;
			ProcessUpdate((string)req.Data);
		}
		
		void ProcessUpdate(string response) {
			int index = 0; bool success = true;
			JsonObject data = (JsonObject)Json.ParseValue(response, ref index, ref success);
			
			JsonObject devBuild = (JsonObject)data["latest"];
			JsonObject releaseBuilds = (JsonObject)data["releases"];
			LatestDev = MakeBuild(devBuild, false);
			
			DateTime releaseTime = DateTime.MinValue;
			foreach (KeyValuePair<string, object> pair in releaseBuilds) {
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
}