// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Gui;
using ClassicalSharp.Entities;
using OpenTK;
#if __MonoCS__
using Ionic.Zlib;
#else
using System.IO.Compression;
#endif

namespace ClassicalSharp.Net {

	public partial class NetworkProcessor : INetworkProcessor {
		
		#region Writing

		public override void SendChat( string text, bool partial ) {
			if( String.IsNullOrEmpty( text ) ) return;
			byte payload = !ServerSupportsPartialMessages ? (byte)0xFF:
				partial ? (byte)1 : (byte)0;
			
			writer.WriteUInt8( (byte)Opcode.Message );
			writer.WriteUInt8( payload );
			writer.WriteString( text );
			SendPacket();
		}
		
		public override void SendPosition( Vector3 pos, float yaw, float pitch ) {
			byte payload = sendHeldBlock ? (byte)game.Inventory.HeldBlock : (byte)0xFF;
			MakePositionPacket( pos, yaw, pitch, payload );
			SendPacket();
		}
		
		public override void SendSetBlock( int x, int y, int z, bool place, byte block ) {
			writer.WriteUInt8( (byte)Opcode.SetBlockClient );
			writer.WriteInt16( (short)x );
			writer.WriteInt16( (short)y );
			writer.WriteInt16( (short)z );
			writer.WriteUInt8( place ? (byte)1 : (byte)0 );
			writer.WriteUInt8( block );
			SendPacket();
		}
		
		void MakeLoginPacket( string username, string verKey ) {
			writer.WriteUInt8( (byte)Opcode.Handshake );
			writer.WriteUInt8( 7 ); // protocol version
			writer.WriteString( username );
			writer.WriteString( verKey );
			byte payload = game.UseCPE ? (byte)0x42 : (byte)0x00;
			writer.WriteUInt8( payload );
		}
		
		void MakePositionPacket( Vector3 pos, float yaw, float pitch, byte payload ) {
			writer.WriteUInt8( (byte)Opcode.EntityTeleport );
			writer.WriteUInt8( payload ); // held block when using HeldBlock, otherwise just 255
			writer.WriteInt16( (short)(pos.X * 32) );
			writer.WriteInt16( (short)((int)(pos.Y * 32) + 51) );
			writer.WriteInt16( (short)(pos.Z * 32) );
			writer.WriteUInt8( (byte)Utils.DegreesToPacked( yaw, 256 ) );
			writer.WriteUInt8( (byte)Utils.DegreesToPacked( pitch, 256 ) );
		}
		
		#endregion
		
		
		#region Reading
		
		DateTime receiveStart;
		DeflateStream gzipStream;
		GZipHeaderReader gzipHeader;
		int mapSizeIndex, mapIndex;
		byte[] mapSize = new byte[4], map;
		FixedBufferStream gzippedMap;
		
		void HandleHandshake() {
			byte protocolVer = reader.ReadUInt8();
			ServerName = reader.ReadCp437String();
			ServerMotd = reader.ReadCp437String();
			receivedFirstPosition = false;
			
			game.LocalPlayer.Hacks.SetUserType( reader.ReadUInt8() );
			game.LocalPlayer.Hacks.ParseHackFlags( ServerName, ServerMotd );
			game.LocalPlayer.CheckHacksConsistency();
			game.Events.RaiseHackPermissionsChanged();
		}
		
		void HandlePing() { }
		
		void HandleLevelInit() {
			if( gzipStream != null )
				return;
			game.World.Reset();
			prevScreen = game.activeScreen;
			if( prevScreen is LoadingMapScreen )
				prevScreen = null;
			prevCursorVisible = game.CursorVisible;
			
			game.SetNewScreen( new LoadingMapScreen( game, ServerName, ServerMotd ), false );
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
			// Workaround for some servers that send LevelDataChunk before LevelInit
			// due to their async packet sending behaviour.
			if( gzipStream == null )
				HandleLevelInit();
			int usedLength = reader.ReadInt16();
			gzippedMap.Position = 0;
			gzippedMap.Offset = reader.index;
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
			
			reader.Skip( 1025 ); // also skip progress since we calculate client side
			float progress = map == null ? 0 : (float)mapIndex / map.Length;
			game.WorldEvents.RaiseMapLoading( progress );
		}
		
		void HandleLevelFinalise() {
			game.SetNewScreen( null );
			game.activeScreen = prevScreen;
			if( prevScreen != null && prevCursorVisible != game.CursorVisible )
				game.CursorVisible = prevCursorVisible;
			prevScreen = null;
			
			int mapWidth = reader.ReadInt16();
			int mapHeight = reader.ReadInt16();
			int mapLength = reader.ReadInt16();
			
			double loadingMs = ( DateTime.UtcNow - receiveStart ).TotalMilliseconds;
			Utils.LogDebug( "map loading took:" + loadingMs );
			game.World.SetData( map, mapWidth, mapHeight, mapLength );
			game.WorldEvents.RaiseOnNewMapLoaded();
			
			map = null;
			gzipStream.Dispose();
			if( sendWomId && !sentWomId ) {
				SendChat( "/womid WoMClient-2.0.7", false );
				sentWomId = true;
			}
			gzipStream = null;
			ServerName = null;
			ServerMotd = null;
			GC.Collect();
		}
		
		void HandleSetBlock() {
			int x = reader.ReadInt16();
			int y = reader.ReadInt16();
			int z = reader.ReadInt16();
			byte type = reader.ReadUInt8();
			
			#if DEBUG_BLOCKS
			if( game.World.IsNotLoaded )
				Utils.LogDebug( "Server tried to update a block while still sending us the map!" );
			else if( !game.World.IsValidPos( x, y, z ) )
				Utils.LogDebug( "Server tried to update a block at an invalid position!" );
			else
				game.UpdateBlock( x, y, z, type );
			#else
			if( !game.World.IsNotLoaded && game.World.IsValidPos( x, y, z ) )
				game.UpdateBlock( x, y, z, type );
			#endif
		}
		
		bool[] needRemoveNames;
		void HandleAddEntity() {
			byte entityId = reader.ReadUInt8();
			string name = reader.ReadAsciiString();
			name = Utils.RemoveEndPlus( name );
			AddEntity( entityId, name, name, true );
			
			// Workaround for LegendCraft as it declares it supports ExtPlayerList but
			// doesn't send ExtAddPlayerName packets. So we add a special case here, even
			// though it is technically against the specification.
			if( UsingExtPlayerList ) {
				AddCpeInfo( entityId, name, name, "Players", 0 );
				if( needRemoveNames == null )
					needRemoveNames = new bool[EntityList.MaxCount];
				needRemoveNames[entityId] = true;
			}
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
			RemoveEntity( entityId );
		}
		
		void HandleMessage() {
			byte messageType = reader.ReadUInt8();
			string text = reader.ReadChatString( ref messageType, useMessageTypes );
			if( !text.StartsWith("^detail.user", StringComparison.OrdinalIgnoreCase ) )
				game.Chat.Add( text, (MessageType)messageType );
		}
		
		void HandleKick() {
			string reason = reader.ReadAsciiString();
			game.Disconnect( "&eLost connection to the server", reason );
			Dispose();
		}
		
		void HandleSetPermission() {
			game.LocalPlayer.Hacks.SetUserType( reader.ReadUInt8() );
		}
		
		void AddEntity( byte entityId, string displayName, string skinName, bool readPosition ) {
			skinName = Utils.StripColours( skinName );
			if( entityId != 0xFF ) {
				Player oldPlayer = game.Players[entityId];
				if( oldPlayer != null ) {
					game.EntityEvents.RaiseEntityRemoved( entityId );
					oldPlayer.Despawn();
				}
				game.Players[entityId] = new NetPlayer( displayName, skinName, game, entityId );
				game.EntityEvents.RaiseEntityAdded( entityId );
			} else {
				// Server is only allowed to change our own name colours.
				if( Utils.StripColours( displayName ) != game.Username )
					displayName = game.Username;
				game.LocalPlayer.DisplayName = displayName;
				game.LocalPlayer.SkinName = skinName;
				game.LocalPlayer.UpdateName();
			}
			
			string identifier = game.Players[entityId].SkinIdentifier;
			game.AsyncDownloader.DownloadSkin( identifier, skinName );
			if( !readPosition ) return;
			
			ReadAbsoluteLocation( entityId, false );
			if( entityId == 0xFF ) {
				LocalPlayer p = game.LocalPlayer;
				p.Spawn = p.Position;
				p.SpawnYaw = p.HeadYawDegrees;
				p.SpawnPitch = p.PitchDegrees;
			}
		}
		
		void RemoveEntity( byte entityId ) {
			Player player = game.Players[entityId];
			if( player == null ) return;
			
			if( entityId != 0xFF ) {
				game.EntityEvents.RaiseEntityRemoved( entityId );
				player.Despawn();
				game.Players[entityId] = null;
			}
			// See comment about LegendCraft in HandleAddEntity
			if( needRemoveNames != null && needRemoveNames[entityId] ) {
				game.EntityEvents.RaiseCpeListInfoRemoved( entityId );
				game.CpePlayersList[entityId] = null;
				needRemoveNames[entityId] = false;
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
	}
}