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
			try {
				using( Stream fs = File.OpenRead( Path.Combine( folder, file ) ) )
					using( StreamReader reader = new StreamReader( fs, false ) )
				{
					string line;
					while( (line = reader.ReadLine()) != null ) {
						if( line.Length == 0 && line[0] == '#' ) continue;
						acceptedUrls.Add( line );
					}
				}
				return true;
			} catch( FileNotFoundException ) {
				return true;
			} catch( IOException ex ) {
				ErrorHandler.LogError( "loading accepted urls", ex );
				return false;
			}
		}
		
		public bool Save() {
			try {
				if( !Directory.Exists( folder ) )
				   Directory.CreateDirectory( folder );
				
				using( Stream fs = File.Create( Path.Combine( folder, file ) ) )
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
