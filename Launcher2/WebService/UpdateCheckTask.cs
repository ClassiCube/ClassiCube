// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using ClassicalSharp;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Net;
using System.Threading;
using JsonObject = System.Collections.Generic.Dictionary<string, object>;

namespace Launcher.Web {
	
	public class Build {
		public DateTime TimeBuilt;
		public string DirectXPath, OpenGLPath;
		public int DirectXSize, OpenGLSize;
		public string Version;
	}
	
	public sealed class UpdateCheckTask : IWebTask {
		
		public const string UpdatesUri = "http://cs.classicube.net/";
		public const string BuildsUri = "http://cs.classicube.net/builds.json";
		public Build LatestDev, LatestStable;
		
		public void CheckForUpdatesAsync() {
			Working = true;
			Done = false;
			Exception = null;
			LatestDev = null;
			LatestStable = null;
			
			Thread thread = new Thread(UpdateWorker, 256 * 1024);
			thread.Name = "Launcher.UpdateCheck";
			thread.Start();
		}
		
		void UpdateWorker() {
			try {
				CheckUpdates();
			} catch(WebException ex) {
				Finish(false, ex, null); return;
			} catch(Exception ex) {
				ErrorHandler2.LogError("UpdateCheckTask.CheckUpdates", ex);
				Finish(false, null, "&cUpdate check failed"); return;
			}
			Finish(true, null, null);
		}
		
		void CheckUpdates() {
			string response = GetHtmlAll(BuildsUri, UpdatesUri);
			int index = 0; bool success = true;
			JsonObject data = (JsonObject)Json.ParseValue(response, ref index, ref success);
			
			JsonObject devBuild = (JsonObject)data["latest"];
			JsonObject releaseBuilds = (JsonObject)data["releases"];
			LatestDev = MakeBuild(devBuild, false);
			Build[] stableBuilds = new Build[releaseBuilds.Count];
			
			int i = 0;
			foreach (KeyValuePair<string, object> pair in releaseBuilds)
				stableBuilds[i++] = MakeBuild((JsonObject)pair.Value, true);
			Array.Sort<Build>(stableBuilds,
			                  (a, b) => b.TimeBuilt.CompareTo(a.TimeBuilt));
			LatestStable = stableBuilds[0];
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