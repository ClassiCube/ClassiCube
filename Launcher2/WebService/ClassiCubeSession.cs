// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Net;
using System.Threading;
using JsonObject = System.Collections.Generic.Dictionary<string, object>;

namespace Launcher.Web {
	
	public sealed class ClassicubeSession : IWebTask {
		
		const string classicubeNetUri = "https://www.classicube.net/",
		loginUri  = "https://www.classicube.net/api/login/",
		listUri   = "https://www.classicube.net/api/servers",
		serverUri = "https://www.classicube.net/api/server/";
		
		public List<ServerListEntry> Servers = new List<ServerListEntry>();
		
		public void LoginAsync(string user, string password) {
			Username = user;
			Working = true;
			Done = false;
			Exception = null;
			Status = "&eSigning in..";
			Servers = new List<ServerListEntry>();
			
			Thread thread = new Thread(LoginWorker, 256 * 1024);
			thread.Name = "Launcher.CCLoginAsync";
			thread.Start(password);
		}
		
		public void FetchServersAsync() {
			Working = true;
			Done = false;
			Exception = null;
			
			Thread thread = new Thread(FetchServersWorker, 256 * 1024);
			thread.Name = "Launcher.CCFetchAsync";
			thread.Start();
		}
		
		void LoginWorker(object password) {
			// Sign in to classicube.net
			try {
				Login(Username, (string)password);
			} catch (WebException ex) {
				Finish(false, ex, "sign in"); return;
			} catch (InvalidOperationException ex) {
				Finish(false, null, "&c" + ex.Message); return;
			}
			
			FetchServersWorker();
			if (!Success) Servers = new List<ServerListEntry>();
		}
		
		void FetchServersWorker() {
			// Retrieve list of public servers
			Status = "&eRetrieving public servers list..";
			try {
				Servers = GetPublicServers();
			} catch (WebException ex) {
				Finish(false, ex, "retrieving servers list"); return;
			}
			Finish(true, null, "&eFetched list");
		}
		
		
		void Login(string user, string password) {
			Username = user;
			
			// Step 1: GET csrf token from login page.
			DateTime start = DateTime.UtcNow;
			string getResponse = Get(loginUri, classicubeNetUri);
			int index = 0; bool success = true;
			JsonObject data = (JsonObject)Json.ParseValue(getResponse, ref index, ref success);
			string token = (string)data["token"];			
			DateTime end = DateTime.UtcNow;
			Log("cc login took " + (end - start).TotalMilliseconds);
			
			// Step 2: POST to login page with csrf token.
			string loginData = String.Format(
				"username={0}&password={1}&token={2}",
				Uri.EscapeDataString(user),
				Uri.EscapeDataString(password),
				Uri.EscapeDataString(token)
			);
			
			start = DateTime.UtcNow;
			string response = Post(loginUri, loginUri, loginData);
			index = 0; success = true;
			data = (JsonObject)Json.ParseValue(response, ref index, ref success);
			end = DateTime.UtcNow;
			Log("cc login took " + (end - start).TotalMilliseconds);
			
			string err = GetLoginError(data);
			if (err != null) throw new InvalidOperationException(err);			
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
		
		public ClientStartData GetConnectInfo(string hash) {
			string uri = serverUri + hash;
			string response = Get(uri, classicubeNetUri);
			
			int index = 0; bool success = true;
			JsonObject root = (JsonObject)Json.ParseValue(response, ref index, ref success);
			List<object> list = (List<object>)root["servers"];
			
			JsonObject obj = (JsonObject)list[0];
			return new ClientStartData(Username, (string)obj["mppass"], 
			                           (string)obj["ip"], (string)obj["port"]);
		}
		
		public List<ServerListEntry> GetPublicServers() {
			DateTime start = DateTime.UtcNow;
			List<ServerListEntry> servers = new List<ServerListEntry>();
			string response = Get(listUri, classicubeNetUri);
			int index = 0; bool success = true;
			JsonObject root = (JsonObject)Json.ParseValue(response, ref index, ref success);
			List<object> list = (List<object>)root["servers"];
			
			for (int i = 0; i < list.Count; i++) {
				JsonObject obj = (JsonObject)list[i];
				servers.Add(new ServerListEntry(
					(string)obj["hash"], (string)obj["name"],
					(string)obj["players"], (string)obj["maxplayers"],
					(string)obj["uptime"], (string)obj["mppass"],
					(string)obj["ip"], (string)obj["port"],
					(string)obj["software"]));
			}
			
			DateTime end = DateTime.UtcNow;
			Log("cc servers took " + (end - start).TotalMilliseconds);
			return servers;
		}
	}
}