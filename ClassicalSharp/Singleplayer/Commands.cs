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
				game.Chat.Add( "&e/client generate: &cInvalid dimensions." );
			} else {
				if( width < 16 || height < 16 || length < 16 ) {
					game.Chat.Add( "&e/client generate: &cDimensions too small." );
					return;
				}
				if( width > 1024 || height > 1024 || length > 1024 ) {
					game.Chat.Add( "&e/client generate: &cDimensions too large." );
					return;
				}
				if( !( game.Network is SinglePlayerServer ) ) {
					game.Chat.Add( "&e/client generate: &cThis command only works in singleplayer mode." );
					return;
				}
				SinglePlayerServer server = (SinglePlayerServer)game.Network;
				server.NewMap();
				game.chatInInputBuffer = null;
				server.MakeMap( width, height, length );
			}
		}
	}
}
