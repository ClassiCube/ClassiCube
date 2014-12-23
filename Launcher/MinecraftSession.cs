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
	    private const StringComparison Ordinal = StringComparison.Ordinal;

	    public override void Login( string user, string password ) {
			Username = user;

			var loginData = String.Format(
				"username={0}&password={1}",
				Uri.EscapeDataString( user ),
				Uri.EscapeDataString( password )
			);
			
			var sw = System.Diagnostics.Stopwatch.StartNew();
			// Step 1: POST to login page username and password.
			var response = PostHtml( LoginUri, LoginUri, loginData );

			foreach( var line in response ) {
			    if( line.Contains( WrongCredentialsMessage ) ) {
					throw new InvalidOperationException( "Wrong username or password." );
				}

			    if (!line.Contains(LoggedInAs)) 
                    continue;
			    Log( "login took " + sw.ElapsedMilliseconds );
			    sw.Stop();
			    return;
			}

	        throw new InvalidOperationException( "Failed to recognise the login response." );
		}
		
		public override GameStartData GetConnectInfo( string hash ) {
			var uri = PlayUri + hash;
			var response = GetHtml( uri, MinecraftNetUri );
		    var data = new GameStartData {Username = Username};

		    foreach( var line in response ) {
				int index;
				// Look for <param name="x" value="x"> tags
		        if ((index = line.IndexOf("<param", Ordinal)) <= 0) 
                    continue;

		        var nameStart = index + 13;
		        var nameEnd = line.IndexOf( '"', nameStart );
		        var paramName = line.Substring( nameStart, nameEnd - nameStart );
					
		        // Don't read param value by default so we avoid allocating unnecessary 'value' strings.
		        switch (paramName) {
		            case "server":
		                data.Ip = GetParamValue( line, nameEnd );
		                break;
		            case "port":
		                data.Port = GetParamValue( line, nameEnd );
		                break;
		            case "mppass":
		                data.Mppass = GetParamValue( line, nameEnd );
		                break;
		        }
		    }

			return data;
		}
		
		static string GetParamValue( string line, int nameEnd ) {
			var valueStart = nameEnd + 9;
			var valueEnd = line.IndexOf( '"', valueStart );
			return line.Substring( valueStart, valueEnd - valueStart );
		}
		
		public override List<ServerListEntry> GetPublicServers() {
			var sw = System.Diagnostics.Stopwatch.StartNew();
			var response = GetHtml( PublicServersUri, MinecraftNetUri );
			var servers = new List<ServerListEntry>();
			var index = -1;
			var mode = 0;
			
			string hash = null;
			string name = null;
			string players = null;
			string maxPlayers = null;

		    foreach( var line in response ) {
				if( line.StartsWith( "                                    <a href", Ordinal ) ) {
					const int hashStart = 59;
					var hashEnd = line.IndexOf( '"', hashStart );
					hash = line.Substring( hashStart, hashEnd - hashStart );
					
					var nameStart = hashEnd + 2; // point to first char of name
					var nameEnd = line.IndexOf( '<', nameStart );
					name = line.Substring( nameStart, nameEnd - nameStart );
					name = WebUtility.HtmlDecode( name );
					index++;
					mode = 0;
				}
				if( index < 0 ) continue;
				
				// NOTE: >16 checks that the line actually has a value.
				// this check is necessary, as the page does have lines with just "            <td>"
		        if (line.Length <= 16 || !line.StartsWith("            <td>", Ordinal)) 
                    continue;

		        const int valStart = 16;
		        var valEnd = line.IndexOf( '<', valStart );
		        var value = line.Substring( valStart, valEnd - valStart );
					
		        switch (mode) {
		            case 0:
		                players = value;
		                break;
		            case 1:
		                maxPlayers = value;
		                break;
		            case 2:
		                var uptime = value;
		                servers.Add( new ServerListEntry( hash, name, players, maxPlayers, uptime ) );
		                break;
		        }

		        mode++;
		    }
			Log( "servers matching took " + sw.ElapsedMilliseconds );
			sw.Stop();
			return servers;
		}
	}
}