using System;
using ClassicalSharp.TexturePack;
using OpenTK.Input;

namespace ClassicalSharp {

	public partial class NetworkProcessor : INetworkProcessor {
		
		#region Writing
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
			Player p = game.LocalPlayer;
			MakePlayerClick( (byte)button, buttonDown, p.YawDegrees, p.PitchDegrees, targetId,
			                pos.BlockPos, pos.BlockFace );
		}
		
		private static void MakeExtInfo( string appName, int extensionsCount ) {
			WriteUInt8( (byte)PacketId.CpeExtInfo );
			WriteString( appName );
			WriteInt16( (short)extensionsCount );
		}
		
		private static void MakeExtEntry( string extensionName, int extensionVersion ) {
			WriteUInt8( (byte)PacketId.CpeExtEntry );
			WriteString( extensionName );
			WriteInt32( extensionVersion );
		}
		
		private static void MakeCustomBlockSupportLevel( byte version ) {
			WriteUInt8( (byte)PacketId.CpeCustomBlockSupportLevel );
			WriteUInt8( version );
		}
		
		private static void MakePlayerClick( byte button, bool buttonDown, float yaw, float pitch, byte targetEntity,
		                                    Vector3I targetPos, CpeBlockFace targetFace ) {
			WriteUInt8( (byte)PacketId.CpePlayerClick );
			WriteUInt8( button );
			WriteUInt8( buttonDown ? (byte)0 : (byte)1 );
			WriteInt16( (short)Utils.DegreesToPacked( yaw, 65536 ) );
			WriteInt16( (short)Utils.DegreesToPacked( pitch, 65536 ) );
			WriteUInt8( targetEntity );
			WriteInt16( (short)targetPos.X );
			WriteInt16( (short)targetPos.Y );
			WriteInt16( (short)targetPos.Z );
			WriteUInt8( (byte)targetFace );
		}
		
		#endregion
		
		
		#region Reading
		
		int cpeServerExtensionsCount;
		bool sendHeldBlock, useMessageTypes, usingTexturePack;
		static string[] clientExtensions = {
			"EmoteFix", "ClickDistance", "HeldBlock", "BlockPermissions",
			"SelectionCuboid", "MessageTypes", "CustomBlocks", "EnvColors",
			"HackControl", "EnvMapAppearance", "ExtPlayerList", "ChangeModel",
			"EnvWeatherType", "PlayerClick", // NOTE: There are no plans to support TextHotKey.
		};
		
		void HandleCpeExtInfo() {
			string appName = reader.ReadAsciiString();
			Utils.LogDebug( "Server identified itself as: " + appName );
			cpeServerExtensionsCount = reader.ReadInt16();
		}
		
		void HandleCpeExtEntry() {
			string extName = reader.ReadAsciiString();
			int extVersion = reader.ReadInt32();
			Utils.LogDebug( "cpe ext: " + extName + " , " + extVersion );
			
			if( extName == "HeldBlock" ) {
				sendHeldBlock = true;
			} else if( extName == "MessageTypes" ) {
				useMessageTypes = true;
			} else if( extName == "ExtPlayerList" ) {
				UsingExtPlayerList = true;
			} else if( extName == "PlayerClick" ) {
				UsingPlayerClick = true;
			} else if( extName == "EnvMapAppearance" && extVersion == 2 ) {
				usingTexturePack = true;
				packetSizes[(int)PacketId.CpeEnvSetMapApperance] += 4;
			}
			cpeServerExtensionsCount--;
			
			if( cpeServerExtensionsCount == 0 ) {
				MakeExtInfo( Utils.AppName, clientExtensions.Length );
				SendPacket();
				for( int i = 0; i < clientExtensions.Length; i++ ) {
					string name = clientExtensions[i];
					int version = (name == "ExtPlayerList" || name == "EnvMapApperance") ? 2 : 1;
					MakeExtEntry( name, version );
					SendPacket();
				}
			}
		}
		
		void HandleCpeSetClickDistance() {
			game.LocalPlayer.ReachDistance = reader.ReadInt16() / 32f;
		}
		
		void HandleCpeCustomBlockSupportLevel() {
			byte supportLevel = reader.ReadUInt8();
			MakeCustomBlockSupportLevel( 1 );
			SendPacket();

			if( supportLevel == 1 ) {
				for( int i = (int)Block.CobblestoneSlab; i <= (int)Block.StoneBrick; i++ ) {
					game.Inventory.CanPlace[i] = true;
					game.Inventory.CanDelete[i] = true;
				}
				game.Events.RaiseBlockPermissionsChanged();
			} else {
				Utils.LogWarning( "Server's block support level is {0}, this client only supports level 1.", supportLevel );
				Utils.LogWarning( "You won't be able to see or use blocks from levels above level 1" );
			}
		}
		
		void HandleCpeHoldThis() {
			byte blockType = reader.ReadUInt8();
			bool canChange = reader.ReadUInt8() == 0;
			game.Inventory.CanChangeHeldBlock = true;
			game.Inventory.HeldBlock = (Block)blockType;
			game.Inventory.CanChangeHeldBlock = canChange;
		}
		
		void HandleCpeExtAddPlayerName() {
			short nameId = reader.ReadInt16();
			string playerName = Utils.StripColours( reader.ReadAsciiString() );
			string listName = reader.ReadAsciiString();
			string groupName = reader.ReadAsciiString();
			byte groupRank = reader.ReadUInt8();
			
			if( nameId >= 0 && nameId <= 255 ) {
				CpeListInfo oldInfo = game.CpePlayersList[nameId];
				CpeListInfo info = new CpeListInfo( (byte)nameId, playerName, listName, groupName, groupRank );
				game.CpePlayersList[nameId] = info;
				//Console.WriteLine( nameId + ": " + groupRank + " , " + groupName + " : " + listName );
				
				if( oldInfo != null ) {
					game.Events.RaiseCpeListInfoChanged( (byte)nameId );
				} else {
					game.Events.RaiseCpeListInfoAdded( (byte)nameId );
				}
			}
		}
		
		void HandleCpeExtAddEntity() {
			byte entityId = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			string skinName = reader.ReadAsciiString();
			AddEntity( entityId, displayName, skinName, false );
		}
		
		void HandleCpeExtRemovePlayerName() {
			short nameId = reader.ReadInt16();
			if( nameId >= 0 && nameId <= 255 ) {
				game.Events.RaiseCpeListInfoRemoved( (byte)nameId );
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

			if( variable == 0 ) { // sky colour
				game.Map.SetSkyColour( invalid ? Map.DefaultSkyColour : col );
			} else if( variable == 1 ) { // clouds colour
				game.Map.SetCloudsColour( invalid ? Map.DefaultCloudsColour : col );
			} else if( variable == 2 ) { // fog colour
				game.Map.SetFogColour( invalid ? Map.DefaultFogColour : col );
			} else if( variable == 3 ) { // ambient light (shadow light)
				game.Map.SetShadowlight( invalid ? Map.DefaultShadowlight : col );
			} else if( variable == 4 ) { // diffuse light (sun light)
				game.Map.SetSunlight( invalid ? Map.DefaultSunlight : col );
			}
		}
		
		void HandleCpeSetBlockPermission() {
			byte blockId = reader.ReadUInt8();
			bool canPlace = reader.ReadUInt8() != 0;
			bool canDelete = reader.ReadUInt8() != 0;
			Inventory inv = game.Inventory;
			
			if( blockId == 0 ) {
				for( int i = 1; i < BlockInfo.CpeBlocksCount; i++ ) {
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
			if( player != null ) {
				player.SetModel( modelName );
			}
		}
		
		void HandleCpeEnvSetMapApperance() {
			string url = reader.ReadAsciiString();
			game.Map.SetSidesBlock( (Block)reader.ReadUInt8() );
			game.Map.SetEdgeBlock( (Block)reader.ReadUInt8() );
			game.Map.SetWaterLevel( reader.ReadInt16() );
			if( usingTexturePack ) {
				game.Map.SetCloudsLevel( reader.ReadInt16() );
				short maxViewDist = reader.ReadInt16(); // TODO: what to do with this?
			}
			
			if( url == String.Empty ) {
				TexturePackExtractor extractor = new TexturePackExtractor();
				extractor.Extract( game.defaultTexPack, game );
			} else {
				game.Animations.Dispose();
				if( usingTexturePack )
					game.AsyncDownloader.DownloadData( url, true, "texturePack" );
				else
					game.AsyncDownloader.DownloadImage( url, true, "terrain" );
				
			}
			Utils.LogDebug( "Image url: " + url );
		}
		
		void HandleCpeEnvWeatherType() {
			game.Map.SetWeather( (Weather)reader.ReadUInt8() );
		}
		
		void HandleCpeHackControl() {
			game.LocalPlayer.CanFly = reader.ReadUInt8() != 0;
			game.LocalPlayer.CanNoclip = reader.ReadUInt8() != 0;
			game.LocalPlayer.CanSpeed = reader.ReadUInt8() != 0;
			game.LocalPlayer.CanRespawn = reader.ReadUInt8() != 0;
			game.CanUseThirdPersonCamera = reader.ReadUInt8() != 0;
			
			if( !game.CanUseThirdPersonCamera ) {
				game.SetCamera( false );
			}
			float jumpHeight = reader.ReadInt16() / 32f;
			if( jumpHeight < 0 ) jumpHeight = 1.4f;
			game.LocalPlayer.CalculateJumpVelocity( jumpHeight );
		}
		
		void HandleCpeExtAddEntity2() {
			byte entityId = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			string skinName = reader.ReadAsciiString();
			AddEntity( entityId, displayName, skinName, true );
		}
		
		void HandleCpeDefineBlock() {
			byte block = reader.ReadUInt8();
			BlockInfo info = game.BlockInfo;
			info.ResetBlockInfo( block );
			
			info.Name[block] = reader.ReadAsciiString();
			info.CollideType[block] = (BlockCollideType)reader.ReadUInt8();
			if( info.CollideType[block] != BlockCollideType.Solid ) {
				info.IsTransparent[block] = true;
				info.IsOpaque[block] = false;
			}
			
			info.SpeedMultiplier[block] = (float)Math.Pow( 2, (reader.ReadUInt8() - 128) / 64f );
			info.SetTop( reader.ReadUInt8(), (Block)block );
			info.SetSide( reader.ReadUInt8(), (Block)block );
			info.SetBottom( reader.ReadUInt8(), (Block)block );
			info.BlocksLight[block] = reader.ReadUInt8() == 0;
			reader.ReadUInt8(); // walk sound, but we ignore this.
			info.FullBright[block] = reader.ReadUInt8() != 0;
			
			byte shape = reader.ReadUInt8();
			if( shape == 2 ) {
				info.Height[block] = 0.5f;
			} else if( shape == 3 ) { // TODO: upside down slab not properly supported
				info.Height[block] = 0.5f;
			} else if( shape == 4 ) {
				info.IsSprite[block] = true;
			}
			if( info.IsOpaque[block] )
				info.IsOpaque[block] = shape == 0;
			if( shape != 1 )
				info.IsTransparent[block] = true;
			
			byte blockDraw = reader.ReadUInt8();
			if( blockDraw == 1 ) {
				info.IsTransparent[block] = true;
			} else if( blockDraw == 2 ) {
				info.IsTransparent[block] = true;
				info.CullWithNeighbours[block] = false;
			} else if( blockDraw == 3 ) {
				info.IsTranslucent[block] = true;
			}
			if( info.IsOpaque[block] )
				info.IsOpaque[block] = blockDraw == 0;
			
			byte fogDensity = reader.ReadUInt8();
			info.FogDensity[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128f;
			info.FogColour[block] = new FastColour(
				reader.ReadUInt8(), reader.ReadUInt8(), reader.ReadUInt8() );
			info.SetupCullingCache();
		}
		
		void HandleCpeRemoveBlockDefinition() {
			game.BlockInfo.ResetBlockInfo( reader.ReadUInt8() );
		}
	}
	#endregion
}