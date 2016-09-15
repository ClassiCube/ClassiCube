// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Network {

	public sealed class CPESupport : IGameComponent {
		
		public void Init( Game game ) { }
		public void Ready( Game game ) { }
		public void OnNewMap( Game game ) { }
		public void OnNewMapLoaded( Game game ) { }
		public void Dispose() { }

		internal int ServerExtensionsCount;
		internal bool sendHeldBlock, useMessageTypes;
		internal int envMapVer = 2, blockDefsExtVer = 2;
		internal bool needD3Fix;
		
		public void Reset( Game game ) {
			ServerExtensionsCount = 0;
			sendHeldBlock = false; useMessageTypes = false;
			envMapVer = 2; blockDefsExtVer = 2;
			needD3Fix = false; game.UseCPEBlocks = false;
			
			NetworkProcessor net = (NetworkProcessor)game.Server;
			net.ResetProtocols();
		}
		
		/// <summary> Sets fields / updates network handles based on the server 
		/// indicating it supports the given CPE extension. </summary>
		public void HandleEntry( string ext, int version, NetworkProcessor net ) {
			ServerExtensionsCount--;
			
			if( ext == "HeldBlock" ) {
				sendHeldBlock = true;
			} else if( ext == "MessageTypes" ) {
				useMessageTypes = true;
			} else if( ext == "ExtPlayerList" ) {
				net.UsingExtPlayerList = true;
			} else if( ext == "PlayerClick" ) {
				net.UsingPlayerClick = true;
			} else if( ext == "EnvMapAppearance" ) {
				envMapVer = version;
				if( version == 1 ) return;
				net.packetSizes[(byte)Opcode.CpeEnvSetMapApperance] = 73;
			} else if( ext == "LongerMessages" ) {
				net.SupportsPartialMessages = true;
			} else if( ext == "FullCP437" ) {
				net.SupportsFullCP437 = true;
			} else if( ext == "BlockDefinitionsExt" ) {
				blockDefsExtVer = version;
				if( version == 1 ) return;
				net.packetSizes[(byte)Opcode.CpeDefineBlockExt] = 88;
			}
		}
		
		public static string[] ClientExtensions = {
			"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
			"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
			"EnvWeatherType", "HackControl", "MessageTypes", "PlayerClick", "FullCP437",
			"LongerMessages", "BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors",
			"EnvMapAspect",
		};
	}
}
