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
			
			NetworkProcessor network = (NetworkProcessor)game.Server;
			network.UsingExtPlayerList = false;
			network.UsingPlayerClick = false;
			network.SupportsPartialMessages = false;
			network.SupportsFullCP437 = false;

			network.Set( Opcode.CpeEnvSetMapApperance,
			            network.HandleEnvSetMapAppearance, 69 );
			network.Set( Opcode.CpeDefineBlockExt,
			            network.HandleDefineBlockExt, 85 );
		}
		
		/// <summary> Sets fields / updates network handles based on the server 
		/// indicating it supports the given CPE extension. </summary>
		public void HandleEntry( string ext, int version, NetworkProcessor network ) {
			ServerExtensionsCount--;
			
			if( ext == "HeldBlock" ) {
				sendHeldBlock = true;
			} else if( ext == "MessageTypes" ) {
				useMessageTypes = true;
			} else if( ext == "ExtPlayerList" ) {
				network.UsingExtPlayerList = true;
			} else if( ext == "PlayerClick" ) {
				network.UsingPlayerClick = true;
			} else if( ext == "EnvMapAppearance" ) {
				envMapVer = version;
				if( version == 1 ) return;
				network.Set( Opcode.CpeEnvSetMapApperance,
				            network.HandleEnvSetMapAppearance2, 73 );
			} else if( ext == "LongerMessages" ) {
				network.SupportsPartialMessages = true;
			} else if( ext == "FullCP437" ) {
				network.SupportsFullCP437 = true;
			} else if( ext == "BlockDefinitionsExt" ) {
				blockDefsExtVer = version;
				if( version == 1 ) return;
				network.Set( Opcode.CpeDefineBlockExt,
				            network.HandleDefineBlockExt, 88 );
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
