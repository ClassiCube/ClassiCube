using System;

namespace Launcher {

	public class ServerListEntry {
		
		public string Hash;
		
		/// <summary> Name of the server. </summary>
		public string Name;
		
		/// <summary> Current number of players on the server. </summary>
		public string Players;
		
		/// <summary> Maximum number of players that can play on the server. </summary>
		public string MaximumPlayers;
		
		/// <summary> How long the server has been 'alive'. </summary>
		public string Uptime;

	    public string Mppass;

		public ServerListEntry( string hash, string name, string players, string maxPlayers, string uptime ) {
			Hash = hash;
			Name = name;
			Players = players;
			MaximumPlayers = maxPlayers;
			Uptime = uptime;
		}
	}
}