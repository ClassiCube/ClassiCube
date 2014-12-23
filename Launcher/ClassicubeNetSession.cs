using System;
using System.IO;
using System.Net;
using System.Text;
using System.Web;
using Newtonsoft.Json.Linq;

namespace Launcher {
    public struct ClassicubeServer {
        public string Hash;
        public string Ip;
        public int OnlinePlayers;
        public int MaxPlayers;
        public string Mppass;
        public string Name;
        public int Port;
        public string Software;
        public long Uptime;
    }

    public class ClassicubeInteraction {
        public string Username;
        public string Password;
        public string ErrorString;
        private string _cachedpage;
        private readonly CookieContainer _netCookies;
        private DateTime _cachedtime;

        private const string LoginUrl = "http://www.classicube.net/api/login/";
        const string ServersUrl = "http://www.classicube.net/api/servers/";


        public ClassicubeInteraction(string username, string password) {
            Username = username;
            Password = password;

            _netCookies = new CookieContainer();
        }

        public bool Login() {
            var loginWorker = (HttpWebRequest)WebRequest.Create(LoginUrl);
            loginWorker.UserAgent = "ClassicubeNetSession";
            loginWorker.Method = "GET";
            loginWorker.CookieContainer = _netCookies;

            string pageData = null;

            var response = (HttpWebResponse)loginWorker.GetResponse();

            using (var responseStream = response.GetResponseStream()) {
                if (responseStream != null)
                    pageData = new StreamReader(responseStream).ReadToEnd();
            }

            if (pageData == null)
                throw new Exception("Failed to download Classicube.net page.");

            var jObj = JToken.Parse(pageData);
            string loginToken;

            if (jObj["authenticated"].Value<bool>() == false && jObj["errorcount"].Value<int>() == 0)
                loginToken = jObj["token"].Value<string>();
            else
                throw new Exception("An error occured logging you in.");

            var loginString = "username=" + HttpUtility.UrlEncode(Username) + "&password=" + HttpUtility.UrlEncode(Password) + "&token=" + HttpUtility.UrlEncode(loginToken);

            loginWorker = (HttpWebRequest)WebRequest.Create(LoginUrl);
            loginWorker.UserAgent = "ClassicubeNetSession";
            loginWorker.ContentType = "application/x-www-form-urlencoded";
            loginWorker.Method = "POST";
            loginWorker.CookieContainer = _netCookies;
            loginWorker.ContentLength = loginString.Length;

            using (var postStream = loginWorker.GetRequestStream()) {
                postStream.Write(Encoding.ASCII.GetBytes(loginString), 0, loginString.Length);
            }

            try {
                response = (HttpWebResponse)loginWorker.GetResponse();
            } catch (WebException e) {
                response = (HttpWebResponse)e.Response;
            }

            using (var responseStream = response.GetResponseStream()) {
                if (responseStream != null)
                    pageData = new StreamReader(responseStream).ReadToEnd();
            }

            if (pageData == null)
                throw new Exception("Failed to download Classicube.net page.");

            jObj = JToken.Parse(pageData);

            return jObj["errorcount"].Value<int>() <= 0;
        }

        public string GetServers() {
            if (_cachedpage != null && (DateTime.Now - _cachedtime).Minutes < 4) {
                return _cachedpage;
            }

            var serverWorker = (HttpWebRequest)WebRequest.Create(ServersUrl);
            serverWorker.Method = "GET";
            serverWorker.UserAgent = "ClassicubeNetSession";
            serverWorker.CookieContainer = _netCookies;

            string pageData = null;

            var response = (HttpWebResponse)serverWorker.GetResponse();

            using (var responseStream = response.GetResponseStream()) {
                if (responseStream != null)
                    pageData = new StreamReader(responseStream).ReadToEnd();
            }

            _cachedpage = pageData;
            _cachedtime = DateTime.Now;

            return pageData;
        }

        public ClassicubeServer[] GetAllServers() {
            var serverString = GetServers();
            var jsonObj = (JArray)(JToken.Parse(serverString)["servers"]);
            var result = new ClassicubeServer[jsonObj.Count];

            for (var i = 0; i < result.Length; i++) {
                result[i] = new ClassicubeServer {
                    Hash = jsonObj[i]["hash"].Value<string>(),
                    Ip = jsonObj[i]["ip"].Value<string>(),
                    Port = jsonObj[i]["port"].Value<int>(),
                    OnlinePlayers = jsonObj[i]["players"].Value<int>(),
                    MaxPlayers = jsonObj[i]["maxplayers"].Value<int>(),
                    Mppass = jsonObj[i]["mppass"].Value<string>(),
                    Name = jsonObj[i]["name"].Value<string>(),
                    Software = jsonObj[i]["software"].Value<string>(),
                };
            }

            return result;
        }

        public ClassicubeServer GetServerInfo(string serverName) {
            var serverString = GetServers();
            var jsonObj = (JArray)(JToken.Parse(serverString)["servers"]);
            var server = new ClassicubeServer();

            foreach (var item in jsonObj) {
                if (item["name"].Value<string>().ToLower() != serverName.ToLower())
                    continue;

                server.Hash = item["hash"].Value<string>();
                server.Ip = item["ip"].Value<string>();
                server.Mppass = item["mppass"].Value<string>();
                server.Port = item["port"].Value<int>();
                server.MaxPlayers = item["maxplayers"].Value<int>();
                server.OnlinePlayers = item["players"].Value<int>();
                server.Software = item["software"].Value<string>();
                break;
            }

            return server;
        }

        public ClassicubeServer GetServerByUrl(string url) {
            var serverString = GetServers();
            var jsonObj = (JArray)(JToken.Parse(serverString)["servers"]);
            var mySplits = url.Split(new[] { '/' }, StringSplitOptions.RemoveEmptyEntries);

            url = mySplits[mySplits.Length - 1];

            var server = new ClassicubeServer();

            foreach (var item in jsonObj) {
                if (!String.Equals(item["hash"].Value<string>(), url, StringComparison.CurrentCultureIgnoreCase))
                    continue;

                server.Hash = item["hash"].Value<string>();
                server.Ip = item["ip"].Value<string>();
                server.Mppass = item["mppass"].Value<string>();
                server.Port = item["port"].Value<int>();
                server.MaxPlayers = item["maxplayers"].Value<int>();
                server.OnlinePlayers = item["players"].Value<int>();
                server.Software = item["software"].Value<string>();
                break;
            }

            return server;
        }
    }
}
