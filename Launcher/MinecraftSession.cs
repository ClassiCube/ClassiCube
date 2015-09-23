using System;
using System.Collections.Generic;

namespace Launcher {

	public class MinecraftSession : GameSession {
		
		const string minecraftNetUri = "https://minecraft.net/",
		loginUri = "https://minecraft.net/login",
		publicServersUri = "https://minecraft.net/classic/list",
		playUri = "https://minecraft.net/classic/play/";
		const string loggedInAs = @"<span class=""logged-in"">";
		const string wrongCredentialsMessage = "Oops, unknown username or password.";
		StringComparison ordinal = StringComparison.Ordinal;
		
		public override void Login( string user, string password ) {
			Username = user;
			string loginData = String.Format(
				"username={0}&password={1}",
				Uri.EscapeDataString( user ),
				Uri.EscapeDataString( password )
			);
			
			var sw = System.Diagnostics.Stopwatch.StartNew();
			// Step 1: POST to login page username and password.
			var response = PostHtml( loginUri, loginUri, loginData );
			foreach( string line in response ) {
				if( line.Contains( wrongCredentialsMessage ) ) {
					throw new InvalidOperationException( "Wrong username or password." );
				} else if( line.Contains( loggedInAs ) ) {
					Log( "login took " + sw.ElapsedMilliseconds );
					sw.Stop();
					return;
				}
			}
			throw new InvalidOperationException( "Failed to recognise the login response." );
		}
		
		public override GameStartData GetConnectInfo( string hash ) {
			string uri = playUri + hash;
			var response = GetHtml( uri, minecraftNetUri );
			GameStartData data = new GameStartData();
			data.Username = Username;
			
			foreach( string line in response ) {
				int index = 0;
				// Look for <param name="x" value="x"> tags
				if( ( index = line.IndexOf( "<param", ordinal ) ) > 0 ) {
					int nameStart = index + 13;
					int nameEnd = line.IndexOf( '"', nameStart );
					string paramName = line.Substring( nameStart, nameEnd - nameStart );
					
					// Don't read param value by default so we avoid allocating unnecessary 'value' strings.
					if( paramName == "server" ) {
						data.Ip = GetParamValue( line, nameEnd );
					} else if( paramName == "port" ) {
						data.Port = GetParamValue( line, nameEnd );
					} else if( paramName == "mppass" ) {
						data.Mppass = GetParamValue( line, nameEnd );
					}
				}
			}
			return data;
		}
		
		static string GetParamValue( string line, int nameEnd ) {
			int valueStart = nameEnd + 9;
			int valueEnd = line.IndexOf( '"', valueStart );
			return line.Substring( valueStart, valueEnd - valueStart );
		}
		
		public override List<ServerListEntry> GetPublicServers() {
			var sw = System.Diagnostics.Stopwatch.StartNew();
			var response = GetHtml( publicServersUri, minecraftNetUri );
			List<ServerListEntry> servers = new List<ServerListEntry>();
			bool foundStart = false;
			int mode = 0;
			
			string hash = null, name = null, players = null;
			string maxPlayers = null, uptime = null;
			
			foreach( string line in response ) {
				if( line.StartsWith( "                                    <a href", ordinal ) ) {
					const int hashStart = 59;
					int hashEnd = line.IndexOf( '"', hashStart );
					hash = line.Substring( hashStart, hashEnd - hashStart );
					
					int nameStart = hashEnd + 2; // point to first char of name
					int nameEnd = line.IndexOf( '<', nameStart );
					name = line.Substring( nameStart, nameEnd - nameStart );
					name = WebUtility.HtmlDecode( name );
					foundStart = true;
					mode = 0;
				}
				if( !foundStart ) continue;
				
				// NOTE: >16 checks that the line actually has a value.
				// this check is necessary, as the page does have lines with just "            <td>"
				if( line.Length > 16 && line.StartsWith( "            <td>", ordinal ) ) {
					const int valStart = 16;
					int valEnd = line.IndexOf( '<', valStart );
					string value = line.Substring( valStart, valEnd - valStart );
					
					if( mode == 0 ) {
						players = value;
					} else if( mode == 1 ) {
						maxPlayers = value;
					} else if( mode == 2 ) {
						uptime = value;
						servers.Add( new ServerListEntry( hash, name, players, maxPlayers, uptime ) );
					}
					mode++;
				}
			}
			Log( "servers matching took " + sw.ElapsedMilliseconds );
			sw.Stop();
			return servers;
		}
	}
}