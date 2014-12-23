using System;
using System.Collections.Generic;

namespace Launcher {

	public class MinecraftSession : GameSession {
		
		const string MinecraftNetUri = "https://minecraft.net/",
		LoginUri = "https://minecraft.net/login",
		PublicServersUri = "https://minecraft.net/classic/list",
		PlayUri = "https://minecraft.net/classic/play/";
		const string LoggedInAs = @"<span class=""logged-in"">";
		const string WrongCredentialsMessage = "Oops, unknown username or password.";
		StringComparison _ordinal = StringComparison.Ordinal;
		
		public override void Login( string user, string password ) {
			Username = user;
			string loginData = String.Format(
				"username={0}&password={1}",
				Uri.EscapeDataString( user ),
				Uri.EscapeDataString( password )
			);
			
			var sw = System.Diagnostics.Stopwatch.StartNew();
			// Step 1: POST to login page username and password.
			var response = PostHtml( LoginUri, LoginUri, loginData );
			foreach( string line in response ) {
				if( line.Contains( WrongCredentialsMessage ) ) {
					throw new InvalidOperationException( "Wrong username or password." );
				} else if( line.Contains( LoggedInAs ) ) {
					Log( "login took " + sw.ElapsedMilliseconds );
					sw.Stop();
					return;
				}
			}
			throw new InvalidOperationException( "Failed to recognise the login response." );
		}
		
		public override GameStartData GetConnectInfo( string hash ) {
			string uri = PlayUri + hash;
			var response = GetHtml( uri, MinecraftNetUri );
			GameStartData data = new GameStartData();
			data.Username = Username;
			
			foreach( string line in response ) {
				int index = 0;
				// Look for <param name="x" value="x"> tags
				if( ( index = line.IndexOf( "<param", _ordinal ) ) > 0 ) {
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
			var response = GetHtml( PublicServersUri, MinecraftNetUri );
			List<ServerListEntry> servers = new List<ServerListEntry>();
			int index = -1;
			int mode = 0;
			
			string hash = null;
			string name = null;
			string players = null;
			string maxPlayers = null;
			string uptime = null;
			
			foreach( string line in response ) {
				if( line.StartsWith( "                                    <a href", _ordinal ) ) {
					const int hashStart = 59;
					int hashEnd = line.IndexOf( '"', hashStart );
					hash = line.Substring( hashStart, hashEnd - hashStart );
					
					int nameStart = hashEnd + 2; // point to first char of name
					int nameEnd = line.IndexOf( '<', nameStart );
					name = line.Substring( nameStart, nameEnd - nameStart );
					name = WebUtility.HtmlDecode( name );
					index++;
					mode = 0;
				}
				if( index < 0 ) continue;
				
				// NOTE: >16 checks that the line actually has a value.
				// this check is necessary, as the page does have lines with just "            <td>"
				if( line.Length > 16 && line.StartsWith( "            <td>", _ordinal ) ) {
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