using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Text;
using System.Threading;

namespace Launcher2 {

	public abstract class GameSession {
		
		public string Username;
		
		public virtual void ResetSession() {
			Username = null;
			cookies = new CookieContainer();
		}
		
		public bool Working;
		public WebException Exception;
		public string Status;
		public List<ServerListEntry> Servers = new List<ServerListEntry>();
		
		public void LoginAsync( string user, string password ) {
			Username = user;
			Working = true;
			Exception = null;
			Status = "&eSigning in..";
			Servers = new List<ServerListEntry>();
			
			Thread thread = new Thread( LoginWorker, 256 * 1024 );
			thread.Name = "Launcher.LoginAsync";
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
		
		void Finish( bool success, WebException ex, string status ) {
			if( !success ) 
				Username = null;
			Working = false;
			
			Exception = ex;
			Status = status;
		}
		
		public abstract void Login( string user, string password );
		
		public abstract GameStartData GetConnectInfo( string hash );
		
		public abstract List<ServerListEntry> GetPublicServers();
		
		CookieContainer cookies = new CookieContainer();
		
		protected HttpWebResponse MakeRequest( string uri, string referer, string data ) {
			HttpWebRequest request = (HttpWebRequest)WebRequest.Create( uri );
			request.UserAgent = Program.AppName;
			request.ReadWriteTimeout = 15 * 1000;
			request.Timeout = 15 * 1000;
			request.Referer = referer;
			request.KeepAlive = true;
			request.CookieContainer = cookies;
			
			// On my machine, these reduce minecraft server list download time from 40 seconds to 4.
			request.Proxy = null;
			request.AutomaticDecompression = DecompressionMethods.GZip | DecompressionMethods.Deflate;
			if( data != null ) {
				request.Method = "POST";
				request.ContentType = "application/x-www-form-urlencoded; charset=UTF-8;";
				byte[] encodedData = Encoding.UTF8.GetBytes( data );
				request.ContentLength = encodedData.Length;
				using( Stream stream = request.GetRequestStream() ) {
					stream.Write( encodedData, 0, encodedData.Length );
				}
			}
			return (HttpWebResponse)request.GetResponse();
		}
		
		protected IEnumerable<string> GetHtml( string uri, string referer ) {
			HttpWebResponse response = MakeRequest( uri, referer, null );
			return GetResponseLines( response );
		}

		protected IEnumerable<string> PostHtml( string uri, string referer, string data ) {
			HttpWebResponse response = MakeRequest( uri, referer, data );
			return GetResponseLines( response );
		}
		
		protected IEnumerable<string> GetResponseLines( HttpWebResponse response ) {
			using( Stream stream = response.GetResponseStream() ) {
				using( StreamReader reader = new StreamReader( stream ) ) {
					string line;
					while( (line = reader.ReadLine()) != null ) {
						yield return line;
					}
				}
			}
		}
		
		protected static void Log( string text ) {
			System.Diagnostics.Debug.WriteLine( text );
		}
	}
}
