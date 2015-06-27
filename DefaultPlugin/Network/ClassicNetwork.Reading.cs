using System;
using System.Drawing;
using ClassicalSharp;
using Ionic.Zlib;

namespace DefaultPlugin.Network {

	public partial class ClassicNetworkProcessor : NetworkProcessor {
			
		readonly int[] packetSizes = new int[] {
			131, 1, 1, 1028, 7, 9, 8, 74, 10, 7, 5, 4, 2,
			66, 65, 2, 67, 69, 3, 2, 3, 134, 196, 130, 3,
			8, 86, 2, 4, 66, 69, 2, 8, 138,
		};
		
		static string[] clientExtensions = new string[] {
			"EmoteFix", "ClickDistance", "HeldBlock", "BlockPermissions",
			"SelectionCuboid", "MessageTypes", "CustomBlocks", "EnvColors",
			"HackControl", "EnvMapAppearance", "ExtPlayerList", "ChangeModel",
			"EnvWeatherType",
		};

		FastNetReader reader;
		int cpeServerExtensionsCount;
		DateTime receiveStart;
		DeflateStream gzipStream;
		GZipHeaderReader gzipHeader;
		int mapSizeIndex;
		byte[] mapSize = new byte[4];
		byte[] map;
		int mapIndex;
		FixedBufferStream gzippedMap;
		
		void ReadPacket( byte opcode ) {
			reader.Remove( 1 ); // remove opcode
			
			switch( (PacketId)opcode ) {
				case PacketId.ServerIdentification:
					{
						byte protocolVer = reader.ReadUInt8();
						ServerName = reader.ReadString();
						ServerMotd = reader.ReadString();
						byte userType = reader.ReadUInt8();
						if( !useBlockPermissions ) {
							Window.CanDelete[(int)Block.Bedrock] = userType == 0x64;
						}
						Window.LocalPlayer.UserType = userType;
						receivedFirstPosition = false;
						Window.LocalPlayer.ParseHackFlags( ServerName, ServerMotd );
					} break;
					
				case PacketId.Ping:
					break;
					
				case PacketId.LevelInitialise:
					{
						Window.Map.Reset();
						Window.RaiseOnNewMap();
						Window.SelectionManager.Dispose();
						Window.SetNewScreen( new LoadingMapScreen( Window, ServerName, ServerMotd ) );
						if( ServerMotd.Contains( "cfg=" ) ) {
							DownloadWomDataAsync();
						}
						receivedFirstPosition = false;
						gzipHeader = new GZipHeaderReader();
						gzipStream = new DeflateStream( gzippedMap, CompressionMode.Decompress );
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
								mapSizeIndex += gzipStream.Read( mapSize, 0, 4 - mapSizeIndex );
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
						Window.RaiseMapLoading( progress );
					} break;
					
				case PacketId.LevelFinalise:
					{
						Window.SetNewScreen( new NormalScreen( Window ) );
						int mapWidth = reader.ReadInt16();
						int mapHeight = reader.ReadInt16();
						int mapLength = reader.ReadInt16();
						
						double loadingMs = ( DateTime.UtcNow - receiveStart ).TotalMilliseconds;
						Utils.LogDebug( "map loading took:" + loadingMs );
						Window.Map.UseRawMap( map, mapWidth, mapHeight, mapLength );
						Window.RaiseOnNewMapLoaded();
						map = null;
						gzipStream.Close();
						if( sendWomId && !sentWomId ) {
							SendChat( "/womid WoMClient-2.0.6" );
							sentWomId = true;
						}
						gzipStream = null;
						GC.Collect();
					} break;
					
				case PacketId.SetBlock:
					{
						int x = reader.ReadInt16();
						int y = reader.ReadInt16();
						int z = reader.ReadInt16();
						byte type = reader.ReadUInt8();
						Window.UpdateBlock( x, y, z, type );
					} break;
					
				case PacketId.AddEntity:
					{
						byte entityId = reader.ReadUInt8();
						string name = reader.ReadString();
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
						Player player = Window.NetPlayers[entityId];
						if( player != null ) {
							Window.RaiseEntityRemoved( entityId );
							player.Despawn();
							Window.NetPlayers[entityId] = null;
						}
					} break;
					
				case PacketId.Message:
					{
						byte messageType = reader.ReadUInt8();
						string text = reader.ReadWoMTextString( ref messageType, useMessageTypes );
						Window.AddChat( text, messageType );
					} break;
					
				case PacketId.Kick:
					{
						string reason = reader.ReadString();
						Window.Disconnect( "&eLost connection to the server", reason );
						Dispose();
					} break;
					
				case PacketId.SetPermission:
					{
						byte userType = reader.ReadUInt8();
						if( !useBlockPermissions ) {
							Window.CanDelete[(int)Block.Bedrock] = userType == 0x64;
						}
						Window.LocalPlayer.UserType = userType;
					} break;
					
				case PacketId.CpeExtInfo:
					{
						string appName = reader.ReadString();
						Utils.LogDebug( "Server identified itself as: " + appName );
						cpeServerExtensionsCount = reader.ReadInt16();
					} break;
					
				case PacketId.CpeExtEntry:
					{
						string extensionName = reader.ReadString();
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
						}
						cpeServerExtensionsCount--;
						if( cpeServerExtensionsCount == 0 ) {
							WritePacket( MakeExtInfo( Utils.AppName, clientExtensions.Length ) );
							for( int i = 0; i < clientExtensions.Length; i++ ) {
								string extName = clientExtensions[i];
								int version = extName == "ExtPlayerList" ? 2 : 1;
								WritePacket( MakeExtEntry( extName, version ) );
							}
						}
					} break;
					
				case PacketId.CpeSetClickDistance:
					{
						Window.LocalPlayer.ReachDistance = reader.ReadInt16() / 32f;
					} break;
					
				case PacketId.CpeCustomBlockSupportLevel:
					{
						byte supportLevel = reader.ReadUInt8();
						WritePacket( MakeCustomBlockSupportLevel( 1 ) );

						if( supportLevel == 1 ) {
							for( int i = (int)Block.CobblestoneSlab; i <= (int)Block.StoneBrick; i++ ) {
								Window.CanPlace[i] = true;
								Window.CanDelete[i] = true;
							}
							Window.RaiseBlockPermissionsChanged();
						} else {
							Utils.LogWarning( "Server's block support level is {0}, this client only supports level 1.", supportLevel );
							Utils.LogWarning( "You won't be able to see or use blocks from levels above level 1" );
						}
					} break;
					
				case PacketId.CpeHoldThis:
					{
						byte blockType = reader.ReadUInt8();
						bool noChanging = reader.ReadUInt8() != 0;
						Window.CanChangeHeldBlock = true;
						Window.HeldBlock = (Block)blockType;
						Window.CanChangeHeldBlock = !noChanging;
					} break;
					
				case PacketId.CpeExtAddPlayerName:
					{
						short nameId = reader.ReadInt16();
						string playerName = Utils.StripColours( reader.ReadString() );
						string listName = reader.ReadString();
						string groupName = reader.ReadString();
						byte groupRank = reader.ReadUInt8();
						if( nameId >= 0 && nameId <= 255 ) {
							CpeListInfo oldInfo = Window.CpePlayersList[nameId];
							CpeListInfo info = new CpeListInfo( (byte)nameId, playerName, listName, groupName, groupRank );
							Window.CpePlayersList[nameId] = info;
							
							if( oldInfo != null ) {
								Window.RaiseCpeListInfoChanged( (byte)nameId );
							} else {
								Window.RaiseCpeListInfoAdded( (byte)nameId );
							}
						}
					} break;
					
				case PacketId.CpeExtAddEntity:
					{
						byte entityId = reader.ReadUInt8();
						string displayName = reader.ReadString();
						string skinName = reader.ReadString();
						AddEntity( entityId, displayName, skinName, false );
					} break;
					
				case PacketId.CpeExtRemovePlayerName:
					{
						short nameId = reader.ReadInt16();
						if( nameId >= 0 && nameId <= 255 ) {
							Window.RaiseCpeListInfoRemoved( (byte)nameId );
						}
					} break;
					
				case PacketId.CpeMakeSelection:
					{
						byte selectionId = reader.ReadUInt8();
						string label = reader.ReadString();
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
						Window.SelectionManager.AddSelection( selectionId, p1, p2, col );
					} break;
					
				case PacketId.CpeRemoveSelection:
					{
						byte selectionId = reader.ReadUInt8();
						Window.SelectionManager.RemoveSelection( selectionId );
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
							Window.Map.SetSkyColour( invalid ? Map.DefaultSkyColour : col );
						} else if( variable == 1 ) { // clouds colour
							Window.Map.SetCloudsColour( invalid ? Map.DefaultCloudsColour : col );
						} else if( variable == 2 ) { // fog colour
							Window.Map.SetFogColour( invalid ? Map.DefaultFogColour : col );
						} else if( variable == 3 ) { // ambient light (shadow light)
							Window.Map.SetShadowlight( invalid ? Map.DefaultShadowlight : col );
						} else if( variable == 4 ) { // diffuse light (sun light)
							Window.Map.SetSunlight( invalid ? Map.DefaultSunlight : col );
						}
					} break;
					
				case PacketId.CpeSetBlockPermission:
					{
						byte blockId = reader.ReadUInt8();
						bool canPlace = reader.ReadUInt8() != 0;
						bool canDelete = reader.ReadUInt8() != 0;
						if( blockId == 0 ) {
							for( int i = 1; i < Window.CanPlace.Length; i++ ) {
								Window.CanPlace[i] = canPlace;
								Window.CanDelete[i] = canDelete;
							}
						} else {
							Window.CanPlace[blockId] = canPlace;
							Window.CanDelete[blockId] = canDelete;
						}
						Window.RaiseBlockPermissionsChanged();
					} break;
					
				case PacketId.CpeChangeModel:
					{
						byte playerId = reader.ReadUInt8();
						string modelName = reader.ReadString().ToLowerInvariant();
						Player player = playerId == 0xFF ? Window.LocalPlayer : Window.NetPlayers[playerId];
						if( player != null ) {
							player.SetModel( modelName );
						}
					} break;
					
				case PacketId.CpeEnvSetMapApperance:
					{
						string url = reader.ReadString();
						byte sideBlock = reader.ReadUInt8();
						byte edgeBlock = reader.ReadUInt8();
						short waterLevel = reader.ReadInt16();
						Window.Map.SetWaterLevel( waterLevel );
						Window.Map.SetEdgeBlock( (Block)edgeBlock );
						Window.Map.SetSidesBlock( (Block)sideBlock );
						if( url == String.Empty ) {
							Bitmap bmp = new Bitmap( "terrain.png" );
							Window.ChangeTerrainAtlas( bmp );
						} else {
							Window.AsyncDownloader.DownloadImage( url, true, "terrain" );
						}
						Utils.LogDebug( "Image url: " + url );
					} break;
					
				case PacketId.CpeEnvWeatherType:
					{
						Window.Map.SetWeather( (Weather)reader.ReadUInt8() );
					} break;
					
				case PacketId.CpeHackControl:
					{
						Window.LocalPlayer.CanFly = reader.ReadUInt8() != 0;
						Window.LocalPlayer.CanNoclip = reader.ReadUInt8() != 0;
						Window.LocalPlayer.CanSpeed = reader.ReadUInt8() != 0;
						Window.LocalPlayer.CanRespawn = reader.ReadUInt8() != 0;
						Window.CanUseThirdPersonCamera = reader.ReadUInt8() != 0;
						if( !Window.CanUseThirdPersonCamera ) {
							Window.SetCamera( false );
						}
						float jumpHeight = reader.ReadInt16() / 32f;
						if( jumpHeight < 0 ) jumpHeight = 1.4f;
						Window.LocalPlayer.CalculateJumpVelocity( jumpHeight );
					} break;
					
				case PacketId.CpeExtAddEntity2:
					{
						byte entityId = reader.ReadUInt8();
						string displayName = reader.ReadString();
						string skinName = reader.ReadString();
						AddEntity( entityId, displayName, skinName, true );
					} break;
					
				default:
					throw new NotImplementedException( "Unsupported packet:" + (PacketId)opcode );
			}
		}
		
		void AddEntity( byte entityId, string displayName, string skinName, bool readPosition ) {
			if( entityId != 0xFF ) {
				Player oldPlayer = Window.NetPlayers[entityId];
				if( oldPlayer != null ) {
					Window.RaiseEntityRemoved( entityId );
					oldPlayer.Despawn();
				}
				Window.NetPlayers[entityId] = new NetPlayer( entityId, displayName, skinName, Window );
				Window.RaiseEntityAdded( entityId );
				Window.AsyncDownloader.DownloadSkin( skinName );
			}
			if( readPosition ) {
				ReadAbsoluteLocation( entityId, false );
				if( entityId == 0xFF ) {
					Window.LocalPlayer.SpawnPoint = Window.LocalPlayer.Position;
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
			float z = reader.ReadInt16() / 32f;
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			if( playerId == 0xFF ) {
				receivedFirstPosition = true;
			}
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			UpdateLocation( playerId, update, interpolate );
		}
	}
}