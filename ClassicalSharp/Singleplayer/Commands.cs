using System;
using ClassicalSharp.Commands;

namespace ClassicalSharp.Singleplayer {
	
	/// <summary> Command that modifies the font size of chat in the normal gui screen. </summary>
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
				Window.AddChat( "&e/client generate: &cInvalid dimensions." );
			} else {
				if( width < 16 || height < 16 || length < 16 ) {
					Window.AddChat( "&e/client generate: &cDimensions too small." );
					return;
				}
				if( width > 1024 || height > 1024 || length > 1024 ) {
					Window.AddChat( "&e/client generate: &cDimensions too large." );
					return;
				}
				if( !( Window.Network is SinglePlayerServer ) ) {
					Window.AddChat( "&e/client generate: &cThis command only works in singleplayer mode." );
					return;
				}
				SinglePlayerServer server = (SinglePlayerServer)Window.Network;
				server.NewMap();
				Window.chatInInputBuffer = "";
				server.MakeMap( width, height, length );
			}
		}
	}
}
