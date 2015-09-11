using System;
using System.IO;
using ClassicalSharp.Commands;

namespace ClassicalSharp.Singleplayer {
	
	/// <summary> Command that generates a new flatgrass map in singleplayer mode. </summary>
	public sealed class GenerateCommand : Command {
		
		public GenerateCommand() {
			Name = "Generate";
			Help = new [] {
				"&a/client generate [width height length]",
				"&bwidth: &eSpecifies X-axis/width of the new map.",
				"&bheight: &eSpecifies Y-axis/height of the new map.",
				"&blength: &eSpecifies Z-axis/length of the new map.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			int width, height, length;
			if( !reader.NextInt( out width ) || !reader.NextInt( out height ) || !reader.NextInt( out length ) ) {
				game.AddChat( "&e/client generate: &cInvalid dimensions." );
			} else {
				if( width < 16 || height < 16 || length < 16 ) {
					game.AddChat( "&e/client generate: &cDimensions too small." );
					return;
				}
				if( width > 1024 || height > 1024 || length > 1024 ) {
					game.AddChat( "&e/client generate: &cDimensions too large." );
					return;
				}
				if( !( game.Network is SinglePlayerServer ) ) {
					game.AddChat( "&e/client generate: &cThis command only works in singleplayer mode." );
					return;
				}
				SinglePlayerServer server = (SinglePlayerServer)game.Network;
				server.NewMap();
				game.chatInInputBuffer = "";
				server.MakeMap( width, height, length );
			}
		}
	}
	
	public sealed class LoadMapCommand : Command {
		
		public LoadMapCommand() {
			Name = "LoadMap";
			Help = new [] {
				"&a/client loadmap [filename]",
				"&bfilename: &eLoads a fcm map from the specified filename.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string path = reader.NextAll();
			if( String.IsNullOrEmpty( path ) ) return;
			
			try {
				using( FileStream fs = new FileStream( path, FileMode.Open, FileAccess.Read, FileShare.Read ) ) {
					int width, height, length;
					game.Map.Reset();
					
					byte[] blocks = MapFcm3.Load( fs, game, out width, out height, out length );
					game.Map.UseRawMap( blocks, width, height, length );
					game.RaiseOnNewMapLoaded();
				}
			} catch( FileNotFoundException ) {
				game.AddChat( "&e/client load: Couldn't find file \"" + path + "\"" );
			}
		}
	}
	
	public sealed class SaveMapCommand : Command {
		
		public SaveMapCommand() {
			Name = "SaveMap";
			Help = new [] {
				"&a/client savemap [filename]",
				"&bfilename: &eSpecifies name of the file to save the map as.",
			};
		}
		
		public override void Execute( CommandReader reader ) {
			string path = reader.NextAll();
			if( String.IsNullOrEmpty( path ) ) return;
			
			using( FileStream fs = new FileStream( path, FileMode.CreateNew, FileAccess.Write ) ) {
				MapFcm3.Save( fs, game );
			}
		}
	}
}
