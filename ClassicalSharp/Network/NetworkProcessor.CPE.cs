using System;
using ClassicalSharp.Hotkeys;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public partial class NetworkProcessor : INetworkProcessor {
		
		#region Writing
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
			Player p = game.LocalPlayer;
			MakePlayerClick( (byte)button, buttonDown, p.YawDegrees, p.PitchDegrees, targetId,
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
		int envMapApperanceVer = 2;
		static string[] clientExtensions = {
			"ClickDistance", "CustomBlocks", "HeldBlock",
			"EmoteFix", "TextHotKey", "ExtPlayerList",
			"EnvColors", "SelectionCuboid", "BlockPermissions",
			"ChangeModel", "EnvMapAppearance", "EnvWeatherType",
			"HackControl", "MessageTypes", "PlayerClick",
			"FullCP437", "LongerMessages",
			// proposals
			"BlockDefinitions", "BlockDefinitionsExt", "TextColors",
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
				envMapApperanceVer = extVersion;
				if( extVersion == 2 ) {
					handlers[(int)PacketId.CpeEnvSetMapApperance] = HandleCpeEnvSetMapAppearance2;
					packetSizes[(int)PacketId.CpeEnvSetMapApperance] += 4;
				}
			} else if( extName == "LongerMessages" ) {
				ServerSupportsPartialMessages = true;
			} else if( extName == "FullCP437" ) {
				ServerSupportsFullCP437 = true;
			}
			cpeServerExtensionsCount--;
			SendCpeExtInfoReply();
		}
		
		void SendCpeExtInfoReply() {
			if( cpeServerExtensionsCount != 0)
				return;
			MakeExtInfo( Program.AppName, clientExtensions.Length );
			SendPacket();
			for( int i = 0; i < clientExtensions.Length; i++ ) {
				string name = clientExtensions[i];
				int ver = name == "ExtPlayerList" ? 2 : 1;
				if( name == "EnvMapAppearance" ) ver = envMapApperanceVer;
				
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
				game.Map.SetSkyColour( invalid ? Map.DefaultSkyColour : col );
			} else if( variable == 1 ) {
				game.Map.SetCloudsColour( invalid ? Map.DefaultCloudsColour : col );
			} else if( variable == 2 ) {
				game.Map.SetFogColour( invalid ? Map.DefaultFogColour : col );
			} else if( variable == 3 ) {
				game.Map.SetShadowlight( invalid ? Map.DefaultShadowlight : col );
			} else if( variable == 4 ) {
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
			game.Map.SetEdgeLevel( reader.ReadInt16() );
			
			if( url == String.Empty ) {
				TexturePackExtractor extractor = new TexturePackExtractor();
				extractor.Extract( game.DefaultTexturePack, game );
			} else if( Utils.IsUrlPrefix( url ) ) {
				if( !game.AcceptedUrls.HasUrl( url ) && !game.DeniedUrls.HasUrl( url ) ) {
					game.AsyncDownloader.RetrieveContentLength( url, true, "CL_" + url );
					game.ShowWarning( new WarningScreen(
						game, "CL_" + url, "Do you want to download the server's terrain image?",
						DownloadTexturePack, null, WarningScreenTick,
						"The terrain image is located at:", url,
						"Terrain image size: Determining..." ) );
				} else {
					DownloadTexturePack( url );
				}
			}
			Utils.LogDebug( "Image url: " + url );
		}
		
		void HandleCpeEnvSetMapAppearance2() {
			HandleCpeEnvSetMapApperance();
			game.Map.SetCloudsLevel( reader.ReadInt16() );
			short maxViewDist = reader.ReadInt16();
			game.MaxViewDistance = maxViewDist <= 0 ? 32768 : maxViewDist;
			game.SetViewDistance( game.UserViewDistance, false );
		}
		
		void DownloadTexturePack( WarningScreen screen ) {
			DownloadTexturePack( ((string)screen.Metadata).Substring( 3 ) );
		}
		
		void DownloadTexturePack( string url ) {
			if( game.DeniedUrls.HasUrl( url ) ) return;			
			game.Animations.Dispose();
			DateTime lastModified = TextureCache.GetLastModifiedFromCache( url );

			if( url.Contains( ".zip" ) )
				game.AsyncDownloader.DownloadData( url, true, "texturePack", lastModified );
			else
				game.AsyncDownloader.DownloadImage( url, true, "terrain", lastModified );
		}
		
		void WarningScreenTick( WarningScreen screen ) {
			string identifier = (string)screen.Metadata;
			DownloadedItem item;
			if( !game.AsyncDownloader.TryGetItem( identifier, out item ) || item.Data == null ) return;
			
			long contentLength = (long)item.Data;
			if( contentLength <= 0 ) return;
			string url = identifier.Substring( 3 );
			
			float contentLengthMB = (contentLength / 1024f / 1024f );
			screen.SetText( "Do you want to download the server's terrain image?",
			               "The terrain image is located at:", url,
			               "Terrain image size: " + contentLengthMB.ToString( "F3" ) + " MB" );
		}
		
		void HandleCpeEnvWeatherType() {
			game.Map.SetWeather( (Weather)reader.ReadUInt8() );
		}
		
		void HandleCpeHackControl() {
			LocalPlayer p = game.LocalPlayer;
			p.CanFly = reader.ReadUInt8() != 0;
			p.CanNoclip = reader.ReadUInt8() != 0;
			p.CanSpeed = reader.ReadUInt8() != 0;
			p.CanRespawn = reader.ReadUInt8() != 0;
			p.CanUseThirdPersonCamera = reader.ReadUInt8() != 0;
			p.CheckHacksConsistency();
			
			float jumpHeight = reader.ReadInt16() / 32f;
			if( jumpHeight < 0 ) jumpHeight = 1.4f;
			p.CalculateJumpVelocity( jumpHeight );
			p.serverJumpVel = p.jumpVel;
			game.Events.RaiseHackPermissionsChanged();
		}
		
		void HandleCpeExtAddEntity2() {
			byte entityId = reader.ReadUInt8();
			string displayName = reader.ReadAsciiString();
			string skinName = reader.ReadAsciiString();
			AddEntity( entityId, displayName, skinName, true );
		}
		
		void HandleCpeDefineBlock() {
			byte block = HandleCpeDefineBlockCommonStart();
			BlockInfo info = game.BlockInfo;
			byte shape = reader.ReadUInt8();
			if( shape == 0 ) {
				info.IsSprite[block] = true;
				info.IsOpaque[block] = false;
				info.IsTransparent[block] = true;
			} else if( shape <= 16 ) {
				info.MaxBB[block].Y = shape / 16f;
			}
			
			HandleCpeDefineBlockCommonEnd( block );
			// Update sprite BoundingBox if necessary
			if( info.IsSprite[block] ) {
				using( FastBitmap fastBmp =
				      new FastBitmap( game.TerrainAtlas.AtlasBitmap, true ) ) {
					info.RecalculateBB( block, fastBmp );
				}
			}
		}
		
		void HandleCpeRemoveBlockDefinition() {
			game.BlockInfo.ResetBlockInfo( reader.ReadUInt8(), true );
			game.BlockInfo.InitLightOffsets();
		}
		
		void HandleCpeDefineBlockExt() {
			byte block = HandleCpeDefineBlockCommonStart();
			BlockInfo info = game.BlockInfo;
			Vector3 min, max;
			
			min.X = reader.ReadUInt8() / 16f; Utils.Clamp( ref min.X, 0, 15/16f );
			min.Y = reader.ReadUInt8() / 16f; Utils.Clamp( ref min.Y, 0, 15/16f );
			min.Z = reader.ReadUInt8() / 16f; Utils.Clamp( ref min.Z, 0, 15/16f );
			max.X = reader.ReadUInt8() / 16f; Utils.Clamp( ref max.X, 1/16f, 1 );
			max.Y = reader.ReadUInt8() / 16f; Utils.Clamp( ref max.Y, 1/16f, 1 );
			max.Z = reader.ReadUInt8() / 16f; Utils.Clamp( ref max.Z, 1/16f, 1 );
			
			info.MinBB[block] = min;
			info.MaxBB[block] = max;
			HandleCpeDefineBlockCommonEnd( block );
		}
		
		byte HandleCpeDefineBlockCommonStart() {
			byte block = reader.ReadUInt8();
			BlockInfo info = game.BlockInfo;
			info.ResetBlockInfo( block, false );
			
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
			byte sound = reader.ReadUInt8();
			if( sound < breakSnds.Length ) {
				info.StepSounds[block] = stepSnds[sound];
				info.DigSounds[block] = breakSnds[sound];
			}
			info.FullBright[block] = reader.ReadUInt8() != 0;
			return block;
		}
		
		void HandleCpeDefineBlockCommonEnd( byte block ) {
			BlockInfo info = game.BlockInfo;
			byte blockDraw = reader.ReadUInt8();
			if( blockDraw == 1 ) {
				info.IsTransparent[block] = true;
			} else if( blockDraw == 2 ) {
				info.IsTransparent[block] = true;
				info.CullWithNeighbours[block] = false;
			} else if( blockDraw == 3 ) {
				info.IsTranslucent[block] = true;
			} else if( blockDraw == 4 ) {
				info.IsTransparent[block] = true;
				info.IsAir[block] = true;
			}
			if( info.IsOpaque[block] )
				info.IsOpaque[block] = blockDraw == 0;
			
			byte fogDensity = reader.ReadUInt8();
			info.FogDensity[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128f;
			info.FogColour[block] = new FastColour(
				reader.ReadUInt8(), reader.ReadUInt8(), reader.ReadUInt8() );
			info.SetupCullingCache( block );
			info.InitLightOffsets();
		}
		
		const int bulkCount = 256;
		unsafe void HandleBulkBlockUpdate() {
			int count = reader.ReadUInt8() + 1;
			if( game.Map.IsNotLoaded ) {
				Utils.LogDebug( "Server tried to update a block while still sending us the map!" );
				reader.Remove( bulkCount * (sizeof(int) + 1) );
				return;
			}
			
			int* indices = stackalloc int[bulkCount];
			for( int i = 0; i < count; i++ )
				indices[i] = ReadInt32( reader.buffer, i * 4 );
			
			for( int i = 0; i < count; i++ ) {
				byte block = reader.buffer[i + bulkCount * sizeof(int)];
				Vector3I coords = game.Map.GetCoords( indices[i] );
				
				if( !game.Map.IsValidPos( coords ) ) {
					Utils.LogDebug( "Server tried to update a block at an invalid position!" );
					continue;
				}
				game.UpdateBlock( coords.X, coords.Y, coords.Z, block );
			}
			reader.Remove( bulkCount * (sizeof(int) + 1) );
		}
		
		static int ReadInt32( byte[] buffer, int offset ) {
			return buffer[offset + 0] << 24 | buffer[offset + 1] << 16
				| buffer[offset + 2] << 8 | buffer[offset + 3];
		}
		
		void HandleSetTextColor() {
			FastColour col = new FastColour( reader.ReadUInt8(), reader.ReadUInt8(), 
			                                reader.ReadUInt8(), reader.ReadUInt8() );
			byte code = reader.ReadUInt8();
			
			if( code <= ' ' || code > '~' ) return; // Control chars, space, extended chars cannot be used
			if( (code >= '0' && code <= '9') || (code >= 'a' && code <= 'f') 
			   || (code >= 'A' && code <= 'F') ) return; // Standard chars cannot be used.
			game.Drawer2D.Colours[code] = col;
		}
		
		static SoundType[] stepSnds, breakSnds;
		static NetworkProcessor() {
			stepSnds = new SoundType[10];
			breakSnds = new SoundType[10];
			stepSnds[0] = SoundType.None; breakSnds[0] = SoundType.None;
			stepSnds[1] = SoundType.Wood; breakSnds[1] = SoundType.Wood;
			stepSnds[2] = SoundType.Gravel; breakSnds[2] = SoundType.Gravel;
			stepSnds[3] = SoundType.Grass; breakSnds[3] = SoundType.Grass;
			stepSnds[4] = SoundType.Stone; breakSnds[4] = SoundType.Stone;
			// TODO: metal sound type, just use stone for now.
			stepSnds[5] = SoundType.Stone; breakSnds[5] = SoundType.Stone;
			stepSnds[6] = SoundType.Stone; breakSnds[6] = SoundType.Glass;
			stepSnds[7] = SoundType.Cloth; breakSnds[7] = SoundType.Cloth;
			stepSnds[8] = SoundType.Sand; breakSnds[8] = SoundType.Sand;
			stepSnds[9] = SoundType.Snow; breakSnds[9] = SoundType.Snow;
		}
	}
	#endregion
}