// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Hotkeys;
using ClassicalSharp.Map;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Net {

	public partial class NetworkProcessor : INetworkProcessor {
		
		#region Writing
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
			Player p = game.LocalPlayer;
			MakePlayerClick( (byte)button, buttonDown, p.HeadYawDegrees, p.PitchDegrees, targetId,
			                pos.BlockPos, pos.BlockFace );
			SendPacket();
		}
		
		void MakeExtInfo( string appName, int extensionsCount ) {
			writer.WriteUInt8( (byte)PacketId.CpeExtInfo );
			writer.WriteString( appName );
			writer.WriteInt16( (short)extensionsCount );
		}
		
		void MakeExtEntry( string extensionName, int extensionVersion ) {
			writer.WriteUInt8( (byte)PacketId.CpeExtEntry );
			writer.WriteString( extensionName );
			writer.WriteInt32( extensionVersion );
		}
		
		void MakeCustomBlockSupportLevel( byte version ) {
			writer.WriteUInt8( (byte)PacketId.CpeCustomBlockSupportLevel );
			writer.WriteUInt8( version );
		}
		
		void MakePlayerClick( byte button, bool buttonDown, float yaw, float pitch, byte targetEntity,
		                     Vector3I targetPos, CpeBlockFace targetFace ) {
			writer.WriteUInt8( (byte)PacketId.CpePlayerClick );
			writer.WriteUInt8( button );
			writer.WriteUInt8( buttonDown ? (byte)0 : (byte)1 );
			writer.WriteInt16( (short)Utils.DegreesToPacked( yaw, 65536 ) );
			writer.WriteInt16( (short)Utils.DegreesToPacked( pitch, 65536 ) );
			writer.WriteUInt8( targetEntity );
			writer.WriteInt16( (short)targetPos.X );
			writer.WriteInt16( (short)targetPos.Y );
			writer.WriteInt16( (short)targetPos.Z );
			writer.WriteUInt8( (byte)targetFace );
		}
		
		#endregion
		
		
		#region Reading
		
		int cpeServerExtensionsCount;
		bool sendHeldBlock, useMessageTypes;
		int envMapAppearanceVer = 2, blockDefinitionsExtVer = 2;
		static string[] clientExtensions = {
			"ClickDistance", "CustomBlocks", "HeldBlock", "EmoteFix", "TextHotKey", "ExtPlayerList",
			"EnvColors", "SelectionCuboid", "BlockPermissions", "ChangeModel", "EnvMapAppearance",
			"EnvWeatherType", "HackControl", "MessageTypes", "PlayerClick", "FullCP437",
			"LongerMessages", "BlockDefinitions", "BlockDefinitionsExt", "BulkBlockUpdate", "TextColors",
		};
		
		void HandleCpeExtInfo() {
			string appName = reader.ReadAsciiString();
			game.Chat.Add( "Server software: " + appName );
			
			// Workaround for MCGalaxy that send ExtEntry sync but ExtInfoAsync. This means
			// ExtEntry may sometimes arrive before ExtInfo, and thus we have to use += instead of =
			cpeServerExtensionsCount += reader.ReadInt16();
			SendCpeExtInfoReply();
		}
		
		void HandleCpeExtEntry() {
			string extName = reader.ReadAsciiString();
			int extVersion = reader.ReadInt32();
			Utils.LogDebug( "cpe ext: {0}, {1}", extName, extVersion );
			
			if( extName == "HeldBlock" ) {
				sendHeldBlock = true;
			} else if( extName == "MessageTypes" ) {
				useMessageTypes = true;
			} else if( extName == "ExtPlayerList" ) {
				UsingExtPlayerList = true;
			} else if( extName == "PlayerClick" ) {
				UsingPlayerClick = true;
			} else if( extName == "EnvMapAppearance" ) {
				envMapAppearanceVer = extVersion;
				if( extVersion >= 2 ) {
					handlers[(int)PacketId.CpeEnvSetMapApperance] = HandleCpeEnvSetMapAppearance2;
					packetSizes[(int)PacketId.CpeEnvSetMapApperance] += 4;
				}
			} else if( extName == "LongerMessages" ) {
				ServerSupportsPartialMessages = true;
			} else if( extName == "FullCP437" ) {
				ServerSupportsFullCP437 = true;
			} else if( extName == "BlockDefinitionsExt" ) {
				blockDefinitionsExtVer = extVersion;
				if( extVersion >= 2 )
					packetSizes[(int)PacketId.CpeDefineBlockExt] += 3;
			}
			cpeServerExtensionsCount--;
			SendCpeExtInfoReply();
		}
		
		void SendCpeExtInfoReply() {
			if( cpeServerExtensionsCount != 0 ) return;
			int count = clientExtensions.Length;
			if( !game.AllowCustomBlocks ) count -= 2;
			
			MakeExtInfo( Program.AppName, count );
			SendPacket();
			for( int i = 0; i < clientExtensions.Length; i++ ) {
				string name = clientExtensions[i];
				int ver = 1;
				if( name == "ExtPlayerList" ) ver = 2;
				if( name == "EnvMapAppearance" ) ver = envMapAppearanceVer;
				if( name == "BlockDefinitionsExt" ) ver = blockDefinitionsExtVer;
				
				if( !game.AllowCustomBlocks && name.StartsWith( "BlockDefinitions" ) )
					continue;
				MakeExtEntry( name, ver );
				SendPacket();
			}
		}
		
		void HandleCpeSetClickDistance() {
			game.LocalPlayer.ReachDistance = reader.ReadInt16() / 32f;
		}
		
		void HandleCpeCustomBlockSupportLevel() {
			byte supportLevel = reader.ReadUInt8();
			MakeCustomBlockSupportLevel( 1 );
			SendPacket();
			supportsCustomBlocks = true;

			if( supportLevel == 1 ) {
				for( int i = (int)Block.CobblestoneSlab; i <= (int)Block.StoneBrick; i++ ) {
					game.Inventory.CanPlace[i] = true;
					game.Inventory.CanDelete[i] = true;
				}
				game.Events.RaiseBlockPermissionsChanged();
			} else {
				Utils.LogDebug( "Server's block support level is {0}, this client only supports level 1.", supportLevel );
				Utils.LogDebug( "You won't be able to see or use blocks from levels above level 1" );
			}
		}
		
		void HandleCpeHoldThis() {
			byte blockType = reader.ReadUInt8();
			bool canChange = reader.ReadUInt8() == 0;
			game.Inventory.CanChangeHeldBlock = true;
			game.Inventory.HeldBlock = (Block)blockType;
			game.Inventory.CanChangeHeldBlock = canChange;
		}
		
		void HandleCpeSetTextHotkey() {
			string label = reader.ReadAsciiString();
			string action = reader.ReadAsciiString();
			int keyCode = reader.ReadInt32();
			byte keyMods = reader.ReadUInt8();
			
			if( keyCode < 0 || keyCode > 255 ) return;
			Key key = LwjglToKey.Map[keyCode];
			if( key == Key.Unknown ) return;
			
			Console.WriteLine( "CPE Hotkey added: " + key + "," + keyMods + " : " + action );
			if( action == "" ) {
				game.InputHandler.Hotkeys.RemoveHotkey( key, keyMods );
			} else if( action[action.Length - 1] == '\n' ) {
				action = action.Substring( 0, action.Length - 1 );
				game.InputHandler.Hotkeys.AddHotkey( key, keyMods, action, false );
			} else { // more input needed by user
				game.InputHandler.Hotkeys.AddHotkey( key, keyMods, action, true );
			}
		}
		
		void HandleCpeExtAddPlayerName() {
			short nameId = reader.ReadInt16();
			string playerName = Utils.StripColours( reader.ReadAsciiString() );
			playerName = Utils.RemoveEndPlus( playerName );
			string listName = reader.ReadAsciiString();
			listName = Utils.RemoveEndPlus( listName );
			string groupName = reader.ReadAsciiString();
			byte groupRank = reader.ReadUInt8();
			
			// Workaround for some servers that don't cast signed bytes to unsigned, before converting them to shorts.
			if( nameId < 0 )
				nameId += 256;
			if( nameId >= 0 && nameId <= 255 )
				AddCpeInfo( (byte)nameId, playerName, listName, groupName, groupRank );
		}
		
		void AddCpeInfo( byte nameId, string playerName, string listName, string groupName, byte groupRank ) {
			CpeListInfo oldInfo = game.CpePlayersList[nameId];
			CpeListInfo info = new CpeListInfo( (byte)nameId, playerName, listName, groupName, groupRank );
			game.CpePlayersList[nameId] = info;
			
			if( oldInfo != null ) {
				// Only redraw the CPE player list info if something changed.
				if( info.PlayerName != oldInfo.PlayerName || info.ListName != oldInfo.ListName ||
				   info.GroupName != oldInfo.GroupName || info.GroupRank != oldInfo.GroupRank ) {
					game.EntityEvents.RaiseCpeListInfoChanged( (byte)nameId );
				}
			} else {
				game.EntityEvents.RaiseCpeListInfoAdded( (byte)nameId );
			}
		}
		
		void HandleCpeExtAddEntity() {
			byte entityId = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			displayName = Utils.RemoveEndPlus( displayName );
			string skinName = reader.ReadAsciiString();
			skinName = Utils.RemoveEndPlus( skinName );
			AddEntity( entityId, displayName, skinName, false );
		}
		
		void HandleCpeExtRemovePlayerName() {
			short nameId = reader.ReadInt16();
			// Workaround for some servers that don't cast signed bytes to unsigned, before converting them to shorts.
			if( nameId < 0 )
				nameId += 256;
			
			if( nameId >= 0 && nameId <= 255 ) {
				game.EntityEvents.RaiseCpeListInfoRemoved( (byte)nameId );
				game.CpePlayersList[nameId] = null;
			}
		}
		
		void HandleCpeMakeSelection() {
			byte selectionId = reader.ReadUInt8();
			string label = reader.ReadAsciiString();
			short startX = reader.ReadInt16();
			short startY = reader.ReadInt16();
			short startZ = reader.ReadInt16();
			short endX = reader.ReadInt16();
			short endY = reader.ReadInt16();
			short endZ = reader.ReadInt16();
			
			byte r = (byte)reader.ReadInt16();
			byte g = (byte)reader.ReadInt16();
			byte b = (byte)reader.ReadInt16();
			byte a = (byte)reader.ReadInt16();
			
			Vector3I p1 = Vector3I.Min( startX, startY, startZ, endX, endY, endZ );
			Vector3I p2 = Vector3I.Max( startX, startY, startZ, endX, endY, endZ );
			FastColour col = new FastColour( r, g, b, a );
			game.SelectionManager.AddSelection( selectionId, p1, p2, col );
		}
		
		void HandleCpeRemoveSelection() {
			byte selectionId = reader.ReadUInt8();
			game.SelectionManager.RemoveSelection( selectionId );
		}
		
		void HandleCpeEnvColours() {
			byte variable = reader.ReadUInt8();
			short red = reader.ReadInt16();
			short green = reader.ReadInt16();
			short blue = reader.ReadInt16();
			bool invalid = red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255;
			FastColour col = new FastColour( red, green, blue );

			if( variable == 0 ) {
				game.World.SetSkyColour( invalid ? World.DefaultSkyColour : col );
			} else if( variable == 1 ) {
				game.World.SetCloudsColour( invalid ? World.DefaultCloudsColour : col );
			} else if( variable == 2 ) {
				game.World.SetFogColour( invalid ? World.DefaultFogColour : col );
			} else if( variable == 3 ) {
				game.World.SetShadowlight( invalid ? World.DefaultShadowlight : col );
			} else if( variable == 4 ) {
				game.World.SetSunlight( invalid ? World.DefaultSunlight : col );
			}
		}
		
		void HandleCpeSetBlockPermission() {
			byte blockId = reader.ReadUInt8();
			bool canPlace = reader.ReadUInt8() != 0;
			bool canDelete = reader.ReadUInt8() != 0;
			Inventory inv = game.Inventory;
			
			if( blockId == 0 ) {
				int count = supportsCustomBlocks ? BlockInfo.CpeBlocksCount
					: BlockInfo.OriginalBlocksCount;
				for( int i = 1; i < count; i++ ) {
					inv.CanPlace.SetNotOverridable( canPlace, i );
					inv.CanDelete.SetNotOverridable( canDelete, i );
				}
			} else {
				inv.CanPlace.SetNotOverridable( canPlace, blockId );
				inv.CanDelete.SetNotOverridable( canDelete, blockId );
			}
			game.Events.RaiseBlockPermissionsChanged();
		}
		
		void HandleCpeChangeModel() {
			byte playerId = reader.ReadUInt8();
			string modelName = reader.ReadAsciiString().ToLowerInvariant();
			Player player = game.Players[playerId];
			if( player != null ) player.SetModel( modelName );
		}
		
		void HandleCpeEnvSetMapApperance() {
			string url = reader.ReadAsciiString();
			game.World.SetSidesBlock( (Block)reader.ReadUInt8() );
			game.World.SetEdgeBlock( (Block)reader.ReadUInt8() );
			game.World.SetEdgeLevel( reader.ReadInt16() );
			if( !game.AllowServerTextures ) return;
			
			if( url == "" ) {
				ExtractDefault();
			} else if( Utils.IsUrlPrefix( url, 0 ) ) {
				RetrieveTexturePack( url );
			}
			Utils.LogDebug( "Image url: " + url );
		}
		
		void HandleCpeEnvSetMapAppearance2() {
			HandleCpeEnvSetMapApperance();
			game.World.SetCloudsLevel( reader.ReadInt16() );
			short maxViewDist = reader.ReadInt16();
			game.MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
			game.SetViewDistance( game.UserViewDistance, false );
		}
		
		void HandleCpeEnvWeatherType() {
			game.World.SetWeather( (Weather)reader.ReadUInt8() );
		}
		
		void HandleCpeHackControl() {
			LocalPlayer p = game.LocalPlayer;
			p.Hacks.CanFly = reader.ReadUInt8() != 0;
			p.Hacks.CanNoclip = reader.ReadUInt8() != 0;
			p.Hacks.CanSpeed = reader.ReadUInt8() != 0;
			p.Hacks.CanRespawn = reader.ReadUInt8() != 0;
			p.Hacks.CanUseThirdPersonCamera = reader.ReadUInt8() != 0;
			p.CheckHacksConsistency();
			
			float jumpHeight = reader.ReadInt16() / 32f;
			if( jumpHeight < 0 ) p.physics.jumpVel = 0.42f;
			else p.physics.CalculateJumpVelocity( jumpHeight );
			p.physics.serverJumpVel = p.physics.jumpVel;
			game.Events.RaiseHackPermissionsChanged();
		}
		
		void HandleCpeExtAddEntity2() {
			byte entityId = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			string skinName = reader.ReadAsciiString();
			AddEntity( entityId, displayName, skinName, true );
		}
		
		const int bulkCount = 256;
		unsafe void HandleBulkBlockUpdate() {
			int count = reader.ReadUInt8() + 1;
			if( game.World.IsNotLoaded ) {
				#if DEBUG_BLOCKS
				Utils.LogDebug( "Server tried to update a block while still sending us the map!" );
				#endif
				reader.Skip( bulkCount * (sizeof(int) + 1) );
				return;
			}
			int* indices = stackalloc int[bulkCount];
			for( int i = 0; i < count; i++ )
				indices[i] = reader.ReadInt32();
			reader.Skip( (bulkCount - count) * sizeof(int) );
			
			for( int i = 0; i < count; i++ ) {
				byte block = reader.ReadUInt8();
				Vector3I coords = game.World.GetCoords( indices[i] );
				
				if( coords.X < 0 ) {
					#if DEBUG_BLOCKS
					Utils.LogDebug( "Server tried to update a block at an invalid position!" );
					#endif
					continue;
				}
				game.UpdateBlock( coords.X, coords.Y, coords.Z, block );
			}
			reader.Skip( bulkCount - count );
		}
		
		void HandleSetTextColor() {
			FastColour col = new FastColour( reader.ReadUInt8(), reader.ReadUInt8(),
			                                reader.ReadUInt8(), reader.ReadUInt8() );
			byte code = reader.ReadUInt8();
			
			if( code <= ' ' || code > '~' ) return; // Control chars, space, extended chars cannot be used
			if( (code >= '0' && code <= '9') || (code >= 'a' && code <= 'f')
			   || (code >= 'A' && code <= 'F') ) return; // Standard chars cannot be used.
			if( code == '%' || code == '&' ) return; // colour code signifiers cannot be used
			game.Drawer2D.Colours[code] = col;
			game.Events.RaiseColourCodesChanged();
		}
	}
	#endregion
}