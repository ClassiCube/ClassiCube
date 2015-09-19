using System;
using System.Drawing;
using System.IO;
#if __MonoCS__
using Ionic.Zlib;
#else
using System.IO.Compression;
#endif
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public partial class NetworkProcessor : INetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			game = window;
		}
		
		public override bool IsSinglePlayer {
			get { return false; }
		}
		
		Socket socket;
		NetworkStream stream;
		Game game;
		bool sendHeldBlock;
		bool useMessageTypes;
		bool useBlockPermissions;
		bool receivedFirstPosition;
		
		public override void Connect( IPAddress address, int port ) {
			socket = new Socket( address.AddressFamily, SocketType.Stream, ProtocolType.Tcp );
			try {
				socket.Connect( address, port );
			} catch( SocketException ex ) {
				Utils.LogError( "Error while trying to connect: {0}{1}", Environment.NewLine, ex );
				game.Disconnect( "&eUnable to reach " + address + ":" + port,
				                "Unable to establish an underlying connection" );
				Dispose();
				return;
			}
			stream = new NetworkStream( socket, true );
			reader = new FastNetReader( stream );
			gzippedMap = new FixedBufferStream( reader.buffer );
			MakeLoginPacket( game.Username, game.Mppass );
			SendPacket();
		}
		
		public override void SendChat( string text ) {
			if( !String.IsNullOrEmpty( text ) ) {
				MakeMessagePacket( text );
				SendPacket();
			}
		}
		
		public override void SendPosition( Vector3 pos, float yaw, float pitch ) {
			byte payload = sendHeldBlock ? (byte)game.HeldBlock : (byte)0xFF;
			MakePositionPacket( pos, yaw, pitch, payload );
			SendPacket();
		}
		
		public override void SendSetBlock( int x, int y, int z, bool place, byte block ) {
			MakeSetBlockPacket( (short)x, (short)y, (short)z, place, block );
			SendPacket();
		}
		
		public override void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos ) {
			Player p = game.LocalPlayer;
			MakePlayerClick( (byte)button, buttonDown, p.YawDegrees, p.PitchDegrees, targetId,
			                pos.BlockPos, pos.BlockFace );
		}
		
		public override void Dispose() {
			socket.Close();
			Disconnected = true;
		}
		
		void CheckForNewTerrainAtlas() {
			DownloadedItem item;
			game.AsyncDownloader.TryGetItem( "terrain", out item );
			if( item != null && item.Bmp != null ) {
				game.ChangeTerrainAtlas( item.Bmp );
			}
		}
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			
			try {
				reader.ReadPendingData();
			} catch( IOException ex ) {
				Utils.LogError( "Error while reading packets: {0}{1}", Environment.NewLine, ex );
				game.Disconnect( "&eLost connection to the server", "Underlying connection was closed" );
				Dispose();
				return;
			}
			
			while( reader.size > 0 ) {
				byte opcode = reader.buffer[0];
				if( reader.size < packetSizes[opcode] ) break;
				ReadPacket( opcode );
			}
			
			Player player = game.LocalPlayer;
			if( receivedFirstPosition ) {
				SendPosition( player.Position, player.YawDegrees, player.PitchDegrees );
			}
			CheckForNewTerrainAtlas();
			CheckForWomEnvironment();
		}
		
		readonly int[] packetSizes = {
			131, 1, 1, 1028, 7, 9, 8, 74, 10, 7, 5, 4, 2,
			66, 65, 2, 67, 69, 3, 2, 3, 134, 196, 130, 3,
			8, 86, 2, 4, 66, 69, 2, 8, 138, 0, 76, 78, 2,
		};
		
		static string[] clientExtensions = {
			"EmoteFix", "ClickDistance", "HeldBlock", "BlockPermissions",
			"SelectionCuboid", "MessageTypes", "CustomBlocks", "EnvColors",
			"HackControl", "EnvMapAppearance", "ExtPlayerList", "ChangeModel",
			"EnvWeatherType", "PlayerClick", // NOTE: There are no plans to support TextHotKey.
		};
		
		
		#region Writing
		
		static byte[] outBuffer = new byte[131];
		static int outIndex;
		private static void MakeLoginPacket( string username, string verKey ) {
			WriteUInt8( (byte)PacketId.Handshake );
			WriteUInt8( 7 ); // protocol version
			WriteString( username );
			WriteString( verKey );
			WriteUInt8( 0x42 );
		}
		
		private static void MakeSetBlockPacket( short x, short y, short z, bool place, byte block ) {
			WriteUInt8( (byte)PacketId.SetBlockClient );
			WriteInt16( x );
			WriteInt16( y );
			WriteInt16( z );
			WriteUInt8( place ? (byte)1 : (byte)0 );
			WriteUInt8( block );
		}
		
		private static void MakePositionPacket( Vector3 pos, float yaw, float pitch, byte payload ) {
			WriteUInt8( (byte)PacketId.EntityTeleport );
			WriteUInt8( payload ); // held block when using HeldBlock, otherwise just 255
			WriteInt16( (short)( pos.X * 32 ) );
			WriteInt16( (short)( (int)( pos.Y * 32 ) + 51 ) );
			WriteInt16( (short)( pos.Z * 32 ) );
			WriteUInt8( (byte)Utils.DegreesToPacked( yaw, 256 ) );
			WriteUInt8( (byte)Utils.DegreesToPacked( pitch, 256 ) );
		}
		
		private static void MakeMessagePacket( string text ) {
			WriteUInt8( (byte)PacketId.Message );
			WriteUInt8( 0xFF ); // unused
			WriteString( text );
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
		
		static void WriteString( string value ) {
			int count = Math.Min( value.Length, 64 );
			for( int i = 0; i < count; i++ ) {
				char c = value[i];
				outBuffer[outIndex + i] = (byte)( c >= '\u0080' ? '?' : c );
			}
			for( int i = value.Length; i < 64; i++ ) {
				outBuffer[outIndex + i] = (byte)' ';
			}
			outIndex += 64;
		}
		
		static void WriteUInt8( byte value ) {
			outBuffer[outIndex++] = value;
		}
		
		static void WriteInt16( short value ) {
			outBuffer[outIndex++] = (byte)( value >> 8 );
			outBuffer[outIndex++] = (byte)( value );
		}
		
		static void WriteInt32( int value ) {
			outBuffer[outIndex++] = (byte)( value >> 24 );
			outBuffer[outIndex++] = (byte)( value >> 16 );
			outBuffer[outIndex++] = (byte)( value >> 8 );
			outBuffer[outIndex++] = (byte)( value );
		}
		
		void SendPacket() {
			int packetLength = outIndex;
			outIndex = 0;
			if( Disconnected ) return;
			
			try {
				stream.Write( outBuffer, 0, packetLength );
			} catch( IOException ex ) {
				Utils.LogError( "Error while writing packets: {0}{1}", Environment.NewLine, ex );
				game.Disconnect( "&eLost connection to the server", "Underlying connection was closed" );
				Dispose();
			}
		}
		
		#endregion
		
		
		#region Reading
		FastNetReader reader;
		int cpeServerExtensionsCount;
		DateTime receiveStart;
		DeflateStream gzipStream;
		GZipHeaderReader gzipHeader;
		int mapSizeIndex, mapIndex;
		byte[] mapSize = new byte[4], map;
		FixedBufferStream gzippedMap;
		bool sendWomId = false, sentWomId = false;
		
		void ReadPacket( byte opcode ) {
			reader.Remove( 1 ); // remove opcode
			
			switch( (PacketId)opcode ) {
				case PacketId.Handshake:
					{
						byte protocolVer = reader.ReadUInt8();
						ServerName = reader.ReadAsciiString();
						ServerMotd = reader.ReadAsciiString();
						byte userType = reader.ReadUInt8();
						if( !useBlockPermissions ) {
							game.CanDelete[(int)Block.Bedrock] = userType == 0x64;
						}
						game.LocalPlayer.UserType = userType;
						receivedFirstPosition = false;
						game.LocalPlayer.ParseHackFlags( ServerName, ServerMotd );
					} break;
					
				case PacketId.Ping:
					break;
					
				case PacketId.LevelInitialise:
					{
						game.Map.Reset();
						game.SetNewScreen( new LoadingMapScreen( game, ServerName, ServerMotd ) );
						if( ServerMotd.Contains( "cfg=" ) ) {
							ReadWomConfigurationAsync();
						}
						receivedFirstPosition = false;
						gzipHeader = new GZipHeaderReader();
						// Workaround because built in mono stream assumes that the end of stream
						// has been reached the first time a read call returns 0. (MS.NET doesn't)
						#if __MonoCS__
						gzipStream = new DeflateStream( gzippedMap, true );
						#else
						gzipStream = new DeflateStream( gzippedMap, CompressionMode.Decompress );
						if( OpenTK.Configuration.RunningOnMono ) {
							Utils.LogWarning( "You are running on Mono, but this build does not support the Mono workaround." );
							Utils.LogWarning( "You should either download the Mono compatible build or define '__MonoCS__' when targeting Mono. " +
							                 "(The Mono compiler already defines this by default)" );
							Utils.LogWarning( "You will likely experience an 'Internal error (no progress possible) ReadInternal' exception when decompressing the map." );
						}
						#endif
						mapSizeIndex = 0;
						mapIndex = 0;
						receiveStart = DateTime.UtcNow;
					} break;
					
				case PacketId.LevelDataChunk:
					{
						int usedLength = reader.ReadInt16();
						gzippedMap.Position = 0;
						gzippedMap.SetLength( usedLength );
						
						if( gzipHeader.done || gzipHeader.ReadHeader( gzippedMap ) ) {
							if( mapSizeIndex < 4 ) {
								mapSizeIndex += gzipStream.Read( mapSize, mapSizeIndex, 4 - mapSizeIndex );
							}

							if( mapSizeIndex == 4 ) {
								if( map == null ) {
									int size = mapSize[0] << 24 | mapSize[1] << 16 | mapSize[2] << 8 | mapSize[3];
									map = new byte[size];
								}
								mapIndex += gzipStream.Read( map, mapIndex, map.Length - mapIndex );
							}
						}
						reader.Remove( 1024 );
						byte progress = reader.ReadUInt8();
						game.RaiseMapLoading( progress );
					} break;
					
				case PacketId.LevelFinalise:
					{
						game.SetNewScreen( new NormalScreen( game ) );
						int mapWidth = reader.ReadInt16();
						int mapHeight = reader.ReadInt16();
						int mapLength = reader.ReadInt16();
						
						double loadingMs = ( DateTime.UtcNow - receiveStart ).TotalMilliseconds;
						Utils.LogDebug( "map loading took:" + loadingMs );
						game.Map.UseRawMap( map, mapWidth, mapHeight, mapLength );
						game.RaiseOnNewMapLoaded();
						map = null;
						gzipStream.Close();
						if( sendWomId && !sentWomId ) {
							SendChat( "/womid WoMClient-2.0.7" );
							sentWomId = true;
						}
						gzipStream = null;
						GC.Collect( 0 );
					} break;
					
				case PacketId.SetBlock:
					{
						int x = reader.ReadInt16();
						int y = reader.ReadInt16();
						int z = reader.ReadInt16();
						byte type = reader.ReadUInt8();
						game.UpdateBlock( x, y, z, type );
					} break;
					
				case PacketId.AddEntity:
					{
						byte entityId = reader.ReadUInt8();
						string name = reader.ReadAsciiString();
						AddEntity( entityId, name, name, true );
					}  break;
					
				case PacketId.EntityTeleport:
					{
						byte entityId = reader.ReadUInt8();
						ReadAbsoluteLocation( entityId, true );
					} break;
					
				case PacketId.RelPosAndOrientationUpdate:
					ReadRelativeLocation();
					break;
					
				case PacketId.RelPosUpdate:
					ReadRelativePosition();
					break;
					
				case PacketId.OrientationUpdate:
					ReadOrientation();
					break;
					
				case PacketId.RemoveEntity:
					{
						byte entityId = reader.ReadUInt8();
						Player player = game.Players[entityId];
						if( entityId != 0xFF && player != null ) {
							game.RaiseEntityRemoved( entityId );
							player.Despawn();
							game.Players[entityId] = null;
						}
					} break;
					
				case PacketId.Message:
					{
						byte messageType = reader.ReadUInt8();
						string text = reader.ReadChatString( ref messageType, useMessageTypes );
						game.AddChat( text, (CpeMessage)messageType );
					} break;
					
				case PacketId.Kick:
					{
						string reason = reader.ReadAsciiString();
						game.Disconnect( "&eLost connection to the server", reason );
						Dispose();
					} break;
					
				case PacketId.SetPermission:
					{
						byte userType = reader.ReadUInt8();
						if( !useBlockPermissions ) {
							game.CanDelete[(int)Block.Bedrock] = userType == 0x64;
						}
						game.LocalPlayer.UserType = userType;
					} break;
					
				case PacketId.CpeExtInfo:
					{
						string appName = reader.ReadAsciiString();
						Utils.LogDebug( "Server identified itself as: " + appName );
						cpeServerExtensionsCount = reader.ReadInt16();
					} break;
					
				case PacketId.CpeExtEntry:
					{
						string extensionName = reader.ReadAsciiString();
						int extensionVersion = reader.ReadInt32();
						Utils.LogDebug( "cpe ext: " + extensionName + "," + extensionVersion );
						if( extensionName == "HeldBlock" ) {
							sendHeldBlock = true;
						} else if( extensionName == "MessageTypes" ) {
							useMessageTypes = true;
						} else if( extensionName == "ExtPlayerList" ) {
							UsingExtPlayerList = true;
						} else if( extensionName == "BlockPermissions" ) {
							useBlockPermissions = true;
						} else if( extensionName == "PlayerClick" ) {
							UsingPlayerClick = true;
						}
						cpeServerExtensionsCount--;
						if( cpeServerExtensionsCount == 0 ) {
							MakeExtInfo( Utils.AppName, clientExtensions.Length );
							SendPacket();
							for( int i = 0; i < clientExtensions.Length; i++ ) {
								string extName = clientExtensions[i];
								MakeExtEntry( extName, extName == "ExtPlayerList" ? 2 : 1 );
								SendPacket();
							}
						}
					} break;
					
				case PacketId.CpeSetClickDistance:
					{
						game.LocalPlayer.ReachDistance = reader.ReadInt16() / 32f;
					} break;
					
				case PacketId.CpeCustomBlockSupportLevel:
					{
						byte supportLevel = reader.ReadUInt8();
						MakeCustomBlockSupportLevel( 1 );
						SendPacket();

						if( supportLevel == 1 ) {
							for( int i = (int)Block.CobblestoneSlab; i <= (int)Block.StoneBrick; i++ ) {
								game.CanPlace[i] = true;
								game.CanDelete[i] = true;
							}
							game.RaiseBlockPermissionsChanged();
						} else {
							Utils.LogWarning( "Server's block support level is {0}, this client only supports level 1.", supportLevel );
							Utils.LogWarning( "You won't be able to see or use blocks from levels above level 1" );
						}
					} break;
					
				case PacketId.CpeHoldThis:
					{
						byte blockType = reader.ReadUInt8();
						bool canChange = reader.ReadUInt8() == 0;
						game.CanChangeHeldBlock = true;
						game.HeldBlock = (Block)blockType;
						game.CanChangeHeldBlock = canChange;
					} break;
					
				case PacketId.CpeExtAddPlayerName:
					{
						short nameId = reader.ReadInt16();
						string playerName = Utils.StripColours( reader.ReadAsciiString() );
						string listName = reader.ReadAsciiString();
						string groupName = reader.ReadAsciiString();
						byte groupRank = reader.ReadUInt8();
						if( nameId >= 0 && nameId <= 255 ) {
							CpeListInfo oldInfo = game.CpePlayersList[nameId];
							CpeListInfo info = new CpeListInfo( (byte)nameId, playerName, listName, groupName, groupRank );
							game.CpePlayersList[nameId] = info;
							
							if( oldInfo != null ) {
								game.RaiseCpeListInfoChanged( (byte)nameId );
							} else {
								game.RaiseCpeListInfoAdded( (byte)nameId );
							}
						}
					} break;
					
				case PacketId.CpeExtAddEntity:
					{
						byte entityId = reader.ReadUInt8();
						string displayName = reader.ReadAsciiString();
						string skinName = reader.ReadAsciiString();
						AddEntity( entityId, displayName, skinName, false );
					} break;
					
				case PacketId.CpeExtRemovePlayerName:
					{
						short nameId = reader.ReadInt16();
						if( nameId >= 0 && nameId <= 255 ) {
							game.RaiseCpeListInfoRemoved( (byte)nameId );
						}
					} break;
					
				case PacketId.CpeMakeSelection:
					{
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
					} break;
					
				case PacketId.CpeRemoveSelection:
					{
						byte selectionId = reader.ReadUInt8();
						game.SelectionManager.RemoveSelection( selectionId );
					} break;
					
				case PacketId.CpeEnvColours:
					{
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
					} break;
					
				case PacketId.CpeSetBlockPermission:
					{
						byte blockId = reader.ReadUInt8();
						bool canPlace = reader.ReadUInt8() != 0;
						bool canDelete = reader.ReadUInt8() != 0;
						if( blockId == 0 ) {
							for( int i = 1; i < game.CanPlace.Length; i++ ) {
								game.CanPlace[i] = canPlace;
								game.CanDelete[i] = canDelete;
							}
						} else {
							game.CanPlace[blockId] = canPlace;
							game.CanDelete[blockId] = canDelete;
						}
						game.RaiseBlockPermissionsChanged();
					} break;
					
				case PacketId.CpeChangeModel:
					{
						byte playerId = reader.ReadUInt8();
						string modelName = reader.ReadAsciiString().ToLowerInvariant();
						Player player = game.Players[playerId];
						if( player != null ) {
							player.SetModel( modelName );
						}
					} break;
					
				case PacketId.CpeEnvSetMapApperance:
					{
						string url = reader.ReadAsciiString();
						byte sideBlock = reader.ReadUInt8();
						byte edgeBlock = reader.ReadUInt8();
						short waterLevel = reader.ReadInt16();
						game.Map.SetWaterLevel( waterLevel );
						game.Map.SetEdgeBlock( (Block)edgeBlock );
						game.Map.SetSidesBlock( (Block)sideBlock );
						if( url == String.Empty ) {
							TexturePackExtractor extractor = new TexturePackExtractor();
							extractor.Extract( game.defaultTexPack, game );
						} else {
							game.Animations.Dispose();
							game.AsyncDownloader.DownloadImage( url, true, "terrain" );
						}
						Utils.LogDebug( "Image url: " + url );
					} break;
					
				case PacketId.CpeEnvWeatherType:
					{
						game.Map.SetWeather( (Weather)reader.ReadUInt8() );
					} break;
					
				case PacketId.CpeHackControl:
					{
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
					} break;
					
				case PacketId.CpeExtAddEntity2:
					{
						byte entityId = reader.ReadUInt8();
						string displayName = reader.ReadAsciiString();
						string skinName = reader.ReadAsciiString();
						AddEntity( entityId, displayName, skinName, true );
					} break;
					
				case PacketId.CpeDefineBlock:
				case PacketId.CpeDefineLiquid:
					{
						byte block = reader.ReadUInt8();
						BlockInfo info = game.BlockInfo;
						info.ResetBlockInfo( block );
						
						info.names[block] = reader.ReadAsciiString();
						byte solidity = reader.ReadUInt8();
						byte movementSpeed = reader.ReadUInt8();
						info.SetTop( reader.ReadUInt8(), (Block)block );
						info.SetSide( reader.ReadUInt8(), (Block)block );
						info.SetBottom( reader.ReadUInt8(), (Block)block );
						reader.ReadUInt8(); // opacity hint, but we ignore this.
						info.blocksLight[block] = reader.ReadUInt8() == 0;
						reader.ReadUInt8(); // walk sound, but we ignore this.
						
						if( opcode == (byte)PacketId.CpeDefineBlock ) {
							byte shape = reader.ReadUInt8();
							if( shape == 1 ) info.heights[block] = 1;
							else if( shape == 2 ) info.heights[block] = 0.5f;
							// TODO: upside down slab not properly supported
							else if( shape == 3 ) info.heights[block] = 0.5f;
							else if( shape == 4 ) info.isSprite[block] = true;
							
							byte blockDraw = reader.ReadUInt8();
							if( blockDraw == 0 ) info.isOpaque[block] = true;
							else if( blockDraw == 1 ) info.isTransparent[block] = true;
							else if( blockDraw == 2 ) info.isTranslucent[block] = true;
							else if( blockDraw == 3 ) info.isTranslucent[block] = true;
							
							Console.WriteLine( shape + "," + blockDraw );
						} else {
							byte fogDensity = reader.ReadUInt8();
							info.fogDensities[block] = fogDensity == 0 ? 0 : (fogDensity + 1) / 128f;
							info.fogColours[block] = new FastColour(
								reader.ReadUInt8(), reader.ReadUInt8(), reader.ReadUInt8() );
						}
						info.SetupCullingCache();
					} break;
					
				case PacketId.CpeRemoveBlockDefinition:
					{
						byte block = reader.ReadUInt8();
						game.BlockInfo.ResetBlockInfo( block );
					} break;
					
				default:
					throw new NotImplementedException( "Unsupported packet:" + (PacketId)opcode );
			}
		}
		
		void AddEntity( byte entityId, string displayName, string skinName, bool readPosition ) {
			if( entityId != 0xFF ) {
				Player oldPlayer = game.Players[entityId];
				if( oldPlayer != null ) {
					game.RaiseEntityRemoved( entityId );
					oldPlayer.Despawn();
				}
				game.Players[entityId] = new NetPlayer( displayName, skinName, game );
				game.RaiseEntityAdded( entityId );
				game.AsyncDownloader.DownloadSkin( skinName );
			}
			if( readPosition ) {
				ReadAbsoluteLocation( entityId, false );
				if( entityId == 0xFF ) {
					game.LocalPlayer.SpawnPoint = game.LocalPlayer.Position;
				}
			}
		}
		
		void ReadRelativeLocation() {
			byte playerId = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, true );
			UpdateLocation( playerId, update, true );
		}
		
		void ReadOrientation() {
			byte playerId = reader.ReadUInt8();
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			LocationUpdate update = LocationUpdate.MakeOri( yaw, pitch );
			UpdateLocation( playerId, update, true );
		}
		
		void ReadRelativePosition() {
			byte playerId = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			LocationUpdate update = LocationUpdate.MakePos( x, y, z, true );
			UpdateLocation( playerId, update, true );
		}
		
		void ReadAbsoluteLocation( byte playerId, bool interpolate ) {
			float x = reader.ReadInt16() / 32f;
			float y = ( reader.ReadInt16() - 51 ) / 32f; // We have to do this.
			if( playerId == 255 ) y += 22/32f;
			float z = reader.ReadInt16() / 32f;
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			if( playerId == 0xFF ) {
				receivedFirstPosition = true;
			}
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			UpdateLocation( playerId, update, interpolate );
		}
		
		void UpdateLocation( byte playerId, LocationUpdate update, bool interpolate ) {
			Player player = game.Players[playerId];
			if( player != null ) {
				player.SetLocation( update, interpolate );
			}
		}
		#endregion
	}
}