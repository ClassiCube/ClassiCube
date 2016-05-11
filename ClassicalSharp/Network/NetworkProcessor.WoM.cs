// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// This class was partially based on information from http://files.worldofminecraft.com/texturing/
// NOTE: http://files.worldofminecraft.com/ has been down for quite a while, so support was removed on Oct 10, 2015
using System;
using System.Drawing;
using System.IO;
using ClassicalSharp.Map;
using ClassicalSharp.Network;

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor {
		
		string womEnvIdentifier = "womenv_0";
		int womCounter = 0;
		bool sendWomId = false, sentWomId = false;
		
		void CheckForWomEnvironment() {
			DownloadedItem item;
			game.AsyncDownloader.TryGetItem( womEnvIdentifier, out item );
			if( item != null && item.Data != null ) {
				ParseWomConfig( (string)item.Data );
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
						FastColour col = ParseWomColour( value, WorldEnv.DefaultCloudsColour );
						game.World.Env.SetCloudsColour( col );
					} else if( key == "environment.sky" ) {
						FastColour col = ParseWomColour( value, WorldEnv.DefaultSkyColour );
						game.World.Env.SetSkyColour( col );
					} else if( key == "environment.fog" ) {
						FastColour col = ParseWomColour( value, WorldEnv.DefaultFogColour );
						game.World.Env.SetFogColour( col );
					} else if( key == "environment.level" ) {
						int waterLevel = 0;
						if( Int32.TryParse( value, out waterLevel ) )
							game.World.Env.SetEdgeLevel( waterLevel );
					} else if( key == "user.detail" && !cpe.useMessageTypes ) {
						game.Chat.Add( value, MessageType.Status2 );
					}
				}
			}
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
			game.AsyncDownloader.DownloadPage( url, true, womEnvIdentifier );
			sendWomId = true;
		}
		
		static FastColour ParseWomColour( string value, FastColour defaultCol ) {
			int argb;
			return Int32.TryParse( value, out argb ) ? new FastColour( argb ) : defaultCol;
		}
	}
}