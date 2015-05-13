// This class was partially based on information from http://files.worldofminecraft.com/texturing/
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Network;

namespace ClassicalSharp {

	public partial class NetworkProcessor {
		
		string womEnvIdentifier = "womenv_0", womTerrainIdentifier = "womterrain_0";
		void CheckForWomEnvironment() {
			DownloadedItem item;
			Window.AsyncDownloader.TryGetItem( womEnvIdentifier, out item );
			if( item != null && item.Page != null ) {
				ParseWomConfig( item.Page );
			}
			
			Window.AsyncDownloader.TryGetItem( womTerrainIdentifier, out item );
			if( item != null && item.Bmp != null ) {
				Window.ChangeTerrainAtlas( item.Bmp );
			}
		}
		
		void ParseWomConfig( string page ) {
			using( StringReader reader = new StringReader( page ) ) {
				string line;
				while( ( line = reader.ReadLine() ) != null ) {
					Utils.LogDebug( line );
					string[] parts = line.Split( new [] { '=' }, 2 );
					if( parts.Length < 2 ) continue;
					string key = parts[0].TrimEnd();
					string value = parts[1].TrimStart();
					
					if( key == "environment.cloud" ) {
						FastColour col = ParseWomColourString( value );
						Window.Map.SetCloudsColour( col );
					} else if( key == "environment.sky" ) {
						FastColour col = ParseWomColourString( value );
						Window.Map.SetSkyColour( col );
					} else if( key == "environment.fog" ) {
						FastColour col = ParseWomColourString( value );
						Window.Map.SetFogColour( col );
					} else if( key == "environment.level" ) {
						int waterLevel = Int32.Parse( value );
						Window.Map.SetWaterLevel( waterLevel );
					} else if( key == "environment.terrain" ) {
						GetWomImageAsync( "terrain", value );
					} else if( key == "environment.edge" ) { // TODO: custom edges and sides
						//GetWomImageAsync( "edge", value );
					} else if( key == "environment.side" ) {
						//GetWomImageAsync( "side", value );
					} else if( key == "user.detail" && !useMessageTypes ) {
						Window.AddChat( value, 2 );
					}
				}
			}
		}
		
		void GetWomImageAsync( string type, string id ) {
			const string hostFormat = "http://files.worldofminecraft.com/skin.php?type={0}&id={1}";
			string url = String.Format( hostFormat, type, Uri.EscapeDataString( id ) );
			string identifier = "wom" + type + "_" + womCounter;
			Window.AsyncDownloader.DownloadImage( url, true, identifier );
		}
		
		static FastColour ParseWomColourString( string value ) {
			int col = Int32.Parse( value );
			int r = ( col & 0xFF0000 ) >> 16;
			int g = ( col & 0x00FF00 ) >> 8;
			int b = ( col & 0x0000FF );
			return new FastColour( r, g, b );
		}
	}
}