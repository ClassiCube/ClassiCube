using System;

namespace Launcher2 {

	public class ClientStartData {
		
		public string Username;
		
		public string RealUsername;
		
		public string Mppass;
		
		public string Ip;
		
		public string Port;
		
		public ClientStartData() {
		}
		
		public ClientStartData( string user, string mppass, string ip, string port ) {
			Username = user;
			RealUsername = user;
			Mppass = mppass;
			Ip = ip;
			Port = port;
		}
	}
}