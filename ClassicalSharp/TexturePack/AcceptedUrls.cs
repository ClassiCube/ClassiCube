using System;
using System.Collections.Generic;
using System.IO;

namespace ClassicalSharp {
	
	public sealed class AcceptedUrls {
		
		List<string> acceptedUrls = new List<string>();	
		const string folder = "texturecache", file = "acceptedurls.txt";
		
		public void AddAccepted( string url ) {
			acceptedUrls.Add( url );
			Save();
		}
		
		public bool HasAccepted( string url ) {
			return acceptedUrls.Contains( url );
		}
		
		public bool Load() {
			string path = Path.Combine( Program.AppDirectory, folder );
			path = Path.Combine( path, file );
			if( !File.Exists( path ) )
				return true;
			
			try {
				using( Stream fs = File.OpenRead( path ) )
					using( StreamReader reader = new StreamReader( fs, false ) )
				{
					string line;
					while( (line = reader.ReadLine()) != null ) {
						if( line.Length == 0 && line[0] == '#' ) continue;
						acceptedUrls.Add( line );
					}
				}
				return true;
			} catch( IOException ex ) {
				ErrorHandler.LogError( "loading accepted urls", ex );
				return false;
			}
		}
		
		public bool Save() {
			try {
				string path = Path.Combine( Program.AppDirectory, folder );
				if( !Directory.Exists( path ) )
				   Directory.CreateDirectory( path );
				
				using( Stream fs = File.Create( Path.Combine( path, file ) ) )
					using( StreamWriter writer = new StreamWriter( fs ) )
				{
					foreach( string value in acceptedUrls )
						writer.WriteLine( value );
				}
				return true;
			} catch( IOException ex ) {
				ErrorHandler.LogError( "saving accepted urls", ex );
				return false;
			}
		}
	}
}
