using System;
using System.Net;
using System.Threading;

namespace Launcher2 {
	
	public sealed class UpdateCheckTask : IWebTask {
		
		public const string UpdatesUri = "http://cs.classicube.net/";
		StringComparison ordinal = StringComparison.Ordinal;
		
		public string LatestStableDate, LatestStableSize;
		public string LatestDevDate, LatestDevSize;
		
		public void CheckForUpdatesAsync() {
			Working = true;
			Exception = null;
			LatestStableDate = null; LatestStableSize = null;
			LatestDevDate = null; LatestDevSize = null;
			
			Thread thread = new Thread( UpdateWorker, 256 * 1024 );
			thread.Name = "Launcher.UpdateCheck";
			thread.Start();
		}
		
		void UpdateWorker() {
			try {
				CheckUpdates();
			} catch( WebException ex ) {
				Finish( false, ex, null ); return;
			}
			Finish( true, null, null );
		}
		
		void CheckUpdates() {
			var response = GetHtml( UpdatesUri, UpdatesUri );
			foreach( string line in response ) {
				Console.WriteLine( line );
				if( line.StartsWith( "latest.", ordinal ) ) {
					int index = 0;
					string buildName = ReadToken( line, ref index );
					string date = ReadToken( line, ref index );
					string time = ReadToken( line, ref index );
					string buildSize = ReadToken( line, ref index );
					
					
					if( line.StartsWith( "latest.zip", ordinal ) ) {
						LatestDevDate = date + " " + time;
						LatestDevSize = buildSize;
					} else if( line.StartsWith( "latest.Release.zip", ordinal ) ) {
						LatestStableDate = date + " " + time;
						LatestStableSize = buildSize;
					}
				}
			}
		}
		
		string ReadToken( string value, ref int index ) {
			int start = index;
			int wordEnd = -1;
			for( ; index < value.Length; index++ ) {
				if( value[index] == ' ' ) {
					// found end of this word
					if( wordEnd == -1 )
						wordEnd = index;
				} else {
					// found start of next word
					if( wordEnd != -1 )
						break;
				}
			}
			
			if( wordEnd == -1 ) 
				wordEnd = value.Length;
			return value.Substring( start, wordEnd - start );
		}
	}
}