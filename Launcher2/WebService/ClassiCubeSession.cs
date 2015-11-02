using System;
using System.Collections.Generic;
using System.Net;
using System.Threading;

namespace Launcher2 {
	
	public sealed class ClassicubeSession : IWebTask {
		
		const string classicubeNetUri = "https://www.classicube.net/",
		loginUri = "https://www.classicube.net/acc/login",
		publicServersUri = "https://www.classicube.net/server/list",
		playUri = "https://www.classicube.net/server/play/";
		const string wrongCredentialsMessage = "Login failed";
		const string loggedInAs = @"<a href=""/acc"" class=""button"">";
		StringComparison ordinal = StringComparison.Ordinal;
		
		public List<ServerListEntry> Servers = new List<ServerListEntry>();
		
		public void LoginAsync( string user, string password ) {
			Username = user;
			Working = true;
			Exception = null;
			Status = "&eSigning in..";
			Servers = new List<ServerListEntry>();
			
			Thread thread = new Thread( LoginWorker, 256 * 1024 );
			thread.Name = "Launcher.CCLoginAsync";
			thread.Start( password );
		}
		
		void LoginWorker( object password ) {
			// Sign in to classicube.net
			try {
				Login( Username, (string)password );
			} catch( WebException ex ) {
				Finish( false, ex, "sign in" ); return;
			} catch( InvalidOperationException ex ) {
				Finish( false, null, "&eFailed to sign in: " +
				       Environment.NewLine + ex.Message ); return;
			}
			
			// Retrieve list of public servers
			Status = "&eRetrieving public servers list..";
			try {
				Servers = GetPublicServers();
			} catch( WebException ex ) {
				Servers = new List<ServerListEntry>();
				Finish( false, ex, "retrieving servers list" ); return;
			}
			Finish( true, null, "&eSigned in" );
		}
		
		void Login( string user, string password ) {
			Username = user;
			// Step 1: GET csrf token from login page.
			var swGet = System.Diagnostics.Stopwatch.StartNew();
			var getResponse = GetHtml( loginUri, classicubeNetUri );			
			string token = null;
			foreach( string line in getResponse ) {
				//Console.WriteLine( line );
				if( line.StartsWith( @"				  <input id=""csrf_token""", ordinal ) ) {
					const int tokenStart = 68;
					int tokenEnd = line.IndexOf( '"', tokenStart );
					token = line.Substring( tokenStart, tokenEnd - tokenStart );
					Log( "cc token get took " + swGet.ElapsedMilliseconds );
					swGet.Stop();
					break;
				}
			}
			
			// Step 2: POST to login page with csrf token.
			string loginData = String.Format(
				"csrf_token={0}&username={1}&password={2}",
				Uri.EscapeDataString( token ),
				Uri.EscapeDataString( user ),
				Uri.EscapeDataString( password )
			);
			
			var sw = System.Diagnostics.Stopwatch.StartNew();
			var response = PostHtml( loginUri, loginUri, loginData );
			foreach( string line in response ) {
				if( line.Contains( wrongCredentialsMessage ) ) {
					throw new InvalidOperationException( "Wrong username or password." );
				} else if( line.Contains( loggedInAs ) ) {
					Log( "cc login took " + sw.ElapsedMilliseconds );
					sw.Stop();
					return;
				}
			}
		}
		
		public ClientStartData GetConnectInfo( string hash ) {
			string uri = playUri + hash;
			var response = GetHtml( uri, classicubeNetUri );
			ClientStartData data = new ClientStartData();
			data.Username = Username;
			
			foreach( string line in response ) {
				int index = 0;
				// Look for <param name="x" value="x"> tags
				if( (index = line.IndexOf( "<param", ordinal )) > 0 ) {
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
		
		public List<ServerListEntry> GetPublicServers() {
			var sw = System.Diagnostics.Stopwatch.StartNew();
			var response = GetHtml( publicServersUri, classicubeNetUri );
			List<ServerListEntry> servers = new List<ServerListEntry>();
			int index = -1;
			
			string hash = null;
			string name = null;
			string players = null;
			string maxPlayers = null;
			
			foreach( string line in response ) {
				if( line.StartsWith( "				<strong><a href", ordinal ) ) {
					const int hashStart = 34;
					int hashEnd = line.IndexOf( '/', hashStart );
					hash = line.Substring( hashStart, hashEnd - hashStart );
					
					int nameStart = hashEnd + 3; // point to first char of name
					int nameEnd = line.IndexOf( '<', nameStart );
					name = line.Substring( nameStart, nameEnd - nameStart );
					name = WebUtility.HtmlDecode( name );
					index++;
				}
				if( index < 0 ) continue;
				
				if( line.StartsWith( @"			<td class=""players"">", ordinal ) ) {
					const int playersStart = 24;
					int playersEnd = line.IndexOf( '/', playersStart );
					players = line.Substring( playersStart, playersEnd - playersStart );
					
					int maxPlayersStart = playersEnd + 1;
					int maxPlayersEnd = line.IndexOf( ']', playersStart );
					maxPlayers = line.Substring( maxPlayersStart, maxPlayersEnd - maxPlayersStart );
					servers.Add( new ServerListEntry( hash, name, players, maxPlayers, "" ) );
				}
			}
			Log( "cc servers took " + sw.ElapsedMilliseconds );
			sw.Stop();
			return servers;
		}
	}
}