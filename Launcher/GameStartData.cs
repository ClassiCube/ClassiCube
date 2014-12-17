using System;

namespace Launcher {

	public class GameStartData {
		
		public string Username;
		
		public string Mppass;
		
		public string Ip;
		
		public string Port;
		
		public GameStartData() {
		}
		
		public GameStartData( string user, string mppass, string ip, string port ) {
			Username = user;
			Mppass = mppass;
			Ip = ip;
			Port = port;
		}
	}
}