// This class was partially based on information from http://files.worldofminecraft.com/texturing/
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Network;

namespace ClassicalSharp {

	public partial class NetworkProcessor {
		
		string womEnvIdentifier = "womenv_0", womTerrainIdentifier = "womterrain_0";
		int womCounter = 0;
		
		void CheckForWomEnvironment() {
			DownloadedItem item;
			game.AsyncDownloader.TryGetItem( womEnvIdentifier, out item );
			if( item != null && item.Data != null ) {
				ParseWomConfig( (string)item.Data );
			}
			
			game.AsyncDownloader.TryGetItem( womTerrainIdentifier, out item );
			if( item != null && item.Data != null ) {
				game.ChangeTerrainAtlas( (Bitmap)item.Data );
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
						FastColour col = ParseWomColour( value, Map.DefaultCloudsColour );
						game.Map.SetCloudsColour( col );
					} else if( key == "environment.sky" ) {
						FastColour col = ParseWomColour( value, Map.DefaultSkyColour );
						game.Map.SetSkyColour( col );
					} else if( key == "environment.fog" ) {
						FastColour col = ParseWomColour( value, Map.DefaultFogColour );
						game.Map.SetFogColour( col );
					} else if( key == "environment.level" ) {
						int waterLevel = 0;
						if( Int32.TryParse( value, out waterLevel ) )
							game.Map.SetWaterLevel( waterLevel );
					} else if( key == "environment.terrain" ) {
						GetWomImageAsync( "terrain", value );
					} else if( key == "environment.edge" ) { // TODO: custom edges and sides
						//GetWomImageAsync( "edge", value );
					} else if( key == "environment.side" ) {
						//GetWomImageAsync( "side", value );
					} else if( key == "user.detail" && !useMessageTypes ) {
						game.AddChat( value, CpeMessage.Status2 );
					}
				}
			}
		}
		
		void GetWomImageAsync( string type, string id ) {
			const string hostFormat = "http://files.worldofminecraft.com/skin.php?type={0}&id={1}";
			string url = String.Format( hostFormat, type, Uri.EscapeDataString( id ) );
			string identifier = "wom" + type + "_" + womCounter;
			game.AsyncDownloader.DownloadImage( url, true, identifier );
		}
		
		void ReadWomConfigurationAsync() {
			string host = ServerMotd.Substring( ServerMotd.IndexOf( "cfg=" ) + 4 );
			string url = "http://" + host;
			url = url.Replace( "$U", game.Username );
			// NOTE: this (should, I did test this) ensure that if the user quickly changes to a
			// different world, the environment settings from the last world are not loaded in the
			// new world if the async 'get request' didn't complete before the new world was loaded.
			womCounter++;
			womEnvIdentifier = "womenv_" + womCounter;
			womTerrainIdentifier = "womterrain_" + womCounter;
			game.AsyncDownloader.DownloadPage( url, true, womEnvIdentifier );
			sendWomId = true;
		}
		
		static FastColour ParseWomColour( string value, FastColour defaultCol ) {
			int argb;
			return Int32.TryParse( value, out argb ) ? new FastColour( argb ) : defaultCol;
		}
	}
}