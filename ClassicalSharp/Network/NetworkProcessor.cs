using System;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;
using OpenTK;
#if __MonoCS__
using Ionic.Zlib;
#else
using System.IO.Compression;
#endif

namespace ClassicalSharp {

	public partial class NetworkProcessor : INetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			game = window;
			SetupHandlers();
		}
		
		public override bool IsSinglePlayer {
			get { return false; }
		}
		
		Socket socket;
		NetworkStream stream;
		Game game;
		bool receivedFirstPosition;
		
		public override void Connect( IPAddress address, int port ) {
			socket = new Socket( address.AddressFamily, SocketType.Stream, ProtocolType.Tcp );
			try {
				socket.Connect( address, port );
			} catch( SocketException ex ) {
				ErrorHandler.LogError( "connecting to server", ex );
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
			byte payload = sendHeldBlock ? (byte)game.Inventory.HeldBlock : (byte)0xFF;
			MakePositionPacket( pos, yaw, pitch, payload );
			SendPacket();
		}
		
		public override void SendSetBlock( int x, int y, int z, bool place, byte block ) {
			MakeSetBlockPacket( (short)x, (short)y, (short)z, place, block );
			SendPacket();
		}
		
		public override void Dispose() {
			socket.Close();
			Disconnected = true;
		}
		
		void CheckForNewTerrainAtlas() {
			DownloadedItem item;
			if( game.AsyncDownloader.TryGetItem( "terrain", out item ) ) {
				if( item.Data != null ) {
					game.ChangeTerrainAtlas( (Bitmap)item.Data );
				} else {
					TexturePackExtractor extractor = new TexturePackExtractor();
					extractor.Extract( game.defaultTexPack, game );
				}
			}
			
			if( game.AsyncDownloader.TryGetItem( "texturePack", out item ) ) {
				TexturePackExtractor extractor = new TexturePackExtractor();
				if( item.Data != null ) {
					extractor.Extract( (byte[])item.Data, game );
				} else {
					extractor.Extract( game.defaultTexPack, game );
				}
			}
		}
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			
			try {
				reader.ReadPendingData();
			} catch( IOException ex ) {
				ErrorHandler.LogError( "reading packets", ex );
				game.Disconnect( "&eLost connection to the server", "I/O error when reading packets" );
				Dispose();
				return;
			}
			
			while( reader.size > 0 ) {
				byte opcode = reader.buffer[0];
				// Fix for older D3 servers which wrote one byte too many for HackControl packets.
				if( opcode == 0xFF && lastOpcode == PacketId.CpeHackControl ) {
					reader.Remove( 1 );
					game.LocalPlayer.CalculateJumpVelocity( 1.4f ); // assume default jump height
					continue;
				}
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
			8, 86, 2, 4, 66, 69, 2, 8, 138, 0, 80, 2,
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
				ErrorHandler.LogError( "wrting packets", ex );
				game.Disconnect( "&eLost connection to the server", "I/O Error while writing packets" );
				Dispose();
			}
		}
		
		#endregion
		
		
		#region Reading
		
		FastNetReader reader;
		DateTime receiveStart;
		DeflateStream gzipStream;
		GZipHeaderReader gzipHeader;
		int mapSizeIndex, mapIndex;
		byte[] mapSize = new byte[4], map;
		FixedBufferStream gzippedMap;
		PacketId lastOpcode;
		
		void ReadPacket( byte opcode ) {
			reader.Remove( 1 ); // remove opcode
			lastOpcode = (PacketId)opcode;
			Action handler;
			
			if( opcode >= maxHandledPacket || (handler = handlers[opcode]) == null)
				throw new NotImplementedException( "Unsupported packet:" + (PacketId)opcode );
			handler();
		}
		
		void HandleHandshake() {
			byte protocolVer = reader.ReadUInt8();
			ServerName = reader.ReadAsciiString();
			ServerMotd = reader.ReadAsciiString();
			game.LocalPlayer.SetUserType( reader.ReadUInt8() );
			receivedFirstPosition = false;
			game.LocalPlayer.ParseHackFlags( ServerName, ServerMotd );
		}
		
		void HandlePing() {
		}
		
		void HandleLevelInit() {
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
				throw new InvalidOperationException( "You must compile ClassicalSharp with __MonoCS__ defined " +
				                                    "to run on Mono, due to a limitation in Mono." );
			}
			#endif
			
			mapSizeIndex = 0;
			mapIndex = 0;
			receiveStart = DateTime.UtcNow;
		}
		
		void HandleLevelDataChunk() {
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
			game.Events.RaiseMapLoading( progress );
		}
		
		void HandleLevelFinalise() {
			game.SetNewScreen( new NormalScreen( game ) );
			int mapWidth = reader.ReadInt16();
			int mapHeight = reader.ReadInt16();
			int mapLength = reader.ReadInt16();
			
			double loadingMs = ( DateTime.UtcNow - receiveStart ).TotalMilliseconds;
			Utils.LogDebug( "map loading took:" + loadingMs );
			game.Map.SetData( map, mapWidth, mapHeight, mapLength );
			game.Events.RaiseOnNewMapLoaded();
			
			map = null;
			gzipStream.Dispose();
			if( sendWomId && !sentWomId ) {
				SendChat( "/womid WoMClient-2.0.7" );
				sentWomId = true;
			}
			gzipStream = null;
			GC.Collect();
		}
		
		void HandleSetBlock() {
			int x = reader.ReadInt16();
			int y = reader.ReadInt16();
			int z = reader.ReadInt16();
			byte type = reader.ReadUInt8();
			
			if( game.Map.IsNotLoaded )
				Utils.LogDebug( "Server tried to update a block while still sending us the map!" );
			else if( !game.Map.IsValidPos( x, y, z ) )
				Utils.LogDebug( "Server tried to update a block at an invalid position!" );
			else
				game.UpdateBlock( x, y, z, type );
		}
		
		void HandleAddEntity() {
			byte entityId = reader.ReadUInt8();
			string name = reader.ReadAsciiString();
			name = Utils.RemoveEndPlus( name );
			AddEntity( entityId, name, name, true );
		}
		
		void HandleEntityTeleport() {
			byte entityId = reader.ReadUInt8();
			ReadAbsoluteLocation( entityId, true );
		}
		
		void HandleRelPosAndOrientationUpdate() {
			byte playerId = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, true );
			UpdateLocation( playerId, update, true );
		}
		
		void HandleRelPositionUpdate() {
			byte playerId = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			
			LocationUpdate update = LocationUpdate.MakePos( x, y, z, true );
			UpdateLocation( playerId, update, true );
		}
		
		void HandleOrientationUpdate() {
			byte playerId = reader.ReadUInt8();
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			
			LocationUpdate update = LocationUpdate.MakeOri( yaw, pitch );
			UpdateLocation( playerId, update, true );
		}
		
		void HandleRemoveEntity() {
			byte entityId = reader.ReadUInt8();
			Player player = game.Players[entityId];
			if( entityId != 0xFF && player != null ) {
				game.Events.RaiseEntityRemoved( entityId );
				player.Despawn();
				game.Players[entityId] = null;
			}
		}
		
		void HandleMessage() {
			byte messageType = reader.ReadUInt8();
			string text = reader.ReadChatString( ref messageType, useMessageTypes );
			game.Chat.Add( text, (CpeMessage)messageType );
		}
		
		void HandleKick() {
			string reason = reader.ReadAsciiString();
			game.Disconnect( "&eLost connection to the server", reason );
			Dispose();
		}
		
		void HandleSetPermission() {
			game.LocalPlayer.SetUserType( reader.ReadUInt8() );
		}
		
		
		void AddEntity( byte entityId, string displayName, string skinName, bool readPosition ) {
			if( entityId != 0xFF ) {
				Player oldPlayer = game.Players[entityId];
				if( oldPlayer != null ) {
					game.Events.RaiseEntityRemoved( entityId );
					oldPlayer.Despawn();
				}
				game.Players[entityId] = new NetPlayer( displayName, skinName, game );
				game.Events.RaiseEntityAdded( entityId );
				game.AsyncDownloader.DownloadSkin( skinName );
			}
			if( readPosition ) {
				ReadAbsoluteLocation( entityId, false );
				if( entityId == 0xFF ) {
					game.LocalPlayer.SpawnPoint = game.LocalPlayer.Position;
				}
			}
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
		
		Action[] handlers;
		int maxHandledPacket;
		
		void SetupHandlers() {
			handlers = new Action[] { HandleHandshake, HandlePing, HandleLevelInit,
				HandleLevelDataChunk, HandleLevelFinalise, null, HandleSetBlock,
				HandleAddEntity, HandleEntityTeleport, HandleRelPosAndOrientationUpdate,
				HandleRelPositionUpdate, HandleOrientationUpdate, HandleRemoveEntity,
				HandleMessage, HandleKick, HandleSetPermission,
				
				HandleCpeExtInfo, HandleCpeExtEntry, HandleCpeSetClickDistance,
				HandleCpeCustomBlockSupportLevel, HandleCpeHoldThis, HandleCpeSetTextHotkey,
				HandleCpeExtAddPlayerName, HandleCpeExtAddEntity, HandleCpeExtRemovePlayerName,
				HandleCpeEnvColours, HandleCpeMakeSelection, HandleCpeRemoveSelection,
				HandleCpeSetBlockPermission, HandleCpeChangeModel, HandleCpeEnvSetMapApperance,
				HandleCpeEnvWeatherType, HandleCpeHackControl, HandleCpeExtAddEntity2,
				null, HandleCpeDefineBlock, HandleCpeRemoveBlockDefinition,
			};
			maxHandledPacket = handlers.Length;
		}
	}
}