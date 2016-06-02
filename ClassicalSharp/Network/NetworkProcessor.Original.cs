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

namespace ClassicalSharp.Network {

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
			byte payload = cpe.sendHeldBlock ? (byte)game.Inventory.HeldBlock : (byte)0xFF;
			MakePositionPacket( pos, yaw, pitch, payload );
			SendPacket();
		}
		
		void SendSetBlock( int x, int y, int z, bool place, byte block ) {
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
			writer.WriteUInt8( (byte)Utils.DegreesToPacked( yaw ) );
			writer.WriteUInt8( (byte)Utils.DegreesToPacked( pitch ) );
		}
		
		#endregion
		
		
		#region Reading
		
		DateTime receiveStart;
		DeflateStream gzipStream;
		GZipHeaderReader gzipHeader;
		int mapSizeIndex, mapIndex;
		byte[] mapSize = new byte[4], map;
		FixedBufferStream gzippedMap;
		
		internal void HandleHandshake() {
			byte protocolVer = reader.ReadUInt8();
			ServerName = reader.ReadCp437String();
			ServerMotd = reader.ReadCp437String();
			receivedFirstPosition = false;
			game.Chat.SetLogName( ServerName );
			
			game.LocalPlayer.Hacks.SetUserType( reader.ReadUInt8() );
			game.LocalPlayer.Hacks.ParseHackFlags( ServerName, ServerMotd );
			game.LocalPlayer.CheckHacksConsistency();
			game.Events.RaiseHackPermissionsChanged();
		}
		
		internal void HandlePing() { }
		
		internal void HandleLevelInit() {
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
		
		internal void HandleLevelDataChunk() {
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
		
		internal void HandleLevelFinalise() {
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
			game.World.SetNewMap( map, mapWidth, mapHeight, mapLength );
			game.WorldEvents.RaiseOnNewMapLoaded();
			
			map = null;
			gzipStream.Dispose();
			if( sendWomId && !sentWomId ) {
				SendChat( "/womid WoMClient-2.0.7", false );
				sentWomId = true;
			}
			gzipStream = null;
			GC.Collect();
		}
		
		internal void HandleSetBlock() {
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
		
		byte[] needRemoveNames = new byte[256 >> 3];
		internal void HandleAddEntity() {
			byte id = reader.ReadUInt8();
			string name = reader.ReadAsciiString();
			name = Utils.RemoveEndPlus( name );
			AddEntity( id, name, name, true );
			// Also workaround for LegendCraft as it declares it supports ExtPlayerList but
			// doesn't send ExtAddPlayerName packets.
			
			AddTablistEntry( id, name, name, "Players", 0 );
			needRemoveNames[id >> 3] |= (byte)(1 << (id & 0x7));
		}
		
		internal void HandleEntityTeleport() {
			byte id = reader.ReadUInt8();
			ReadAbsoluteLocation( id, true );
		}
		
		internal void HandleRelPosAndOrientationUpdate() {
			byte id = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, true );
			UpdateLocation( id, update, true );
		}
		
		internal void HandleRelPositionUpdate() {
			byte id = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			
			LocationUpdate update = LocationUpdate.MakePos( x, y, z, true );
			UpdateLocation( id, update, true );
		}
		
		internal void HandleOrientationUpdate() {
			byte id = reader.ReadUInt8();
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			
			LocationUpdate update = LocationUpdate.MakeOri( yaw, pitch );
			UpdateLocation( id, update, true );
		}
		
		internal void HandleRemoveEntity() {
			byte id = reader.ReadUInt8();
			RemoveEntity( id );
		}
		
		internal void HandleMessage() {
			byte type = reader.ReadUInt8();
			// Original vanilla server uses player ids in message types, 255 for server messages.
			bool prepend = !cpe.useMessageTypes && type == 0xFF;
			
			if( !cpe.useMessageTypes ) type = (byte)MessageType.Normal;
			string text = reader.ReadChatString( ref type );
			if( prepend ) text = "&e" + text;
			
			if( !text.StartsWith("^detail.user", StringComparison.OrdinalIgnoreCase ) )
				game.Chat.Add( text, (MessageType)type );
		}
		
		internal void HandleKick() {
			string reason = reader.ReadCp437String();
			game.Disconnect( "&eLost connection to the server", reason );
			Dispose();
		}
		
		internal void HandleSetPermission() {
			game.LocalPlayer.Hacks.SetUserType( reader.ReadUInt8() );
		}
		
		void AddEntity( byte id, string displayName, string skinName, bool readPosition ) {
			skinName = Utils.StripColours( skinName );
			if( id != 0xFF ) {
				Player oldPlayer = game.Entities[id];
				if( oldPlayer != null ) {
					game.EntityEvents.RaiseRemoved( id );
					oldPlayer.Despawn();
				}
				game.Entities[id] = new NetPlayer( displayName, skinName, game, id );
				game.EntityEvents.RaiseAdded( id );
			} else {
				// Server is only allowed to change our own name colours.
				if( Utils.StripColours( displayName ) != game.Username )
					displayName = game.Username;
				game.LocalPlayer.DisplayName = displayName;
				game.LocalPlayer.SkinName = skinName;
				game.LocalPlayer.UpdateName();
			}
			
			string identifier = game.Entities[id].SkinIdentifier;
			game.AsyncDownloader.DownloadSkin( identifier, skinName );
			if( !readPosition ) return;
			
			ReadAbsoluteLocation( id, false );
			if( id == 0xFF ) {
				LocalPlayer p = game.LocalPlayer;
				p.Spawn = p.Position;
				p.SpawnYaw = p.HeadYawDegrees;
				p.SpawnPitch = p.PitchDegrees;
			}
		}
		
		void RemoveEntity( byte id ) {
			Player player = game.Entities[id];
			if( player == null ) return;
			
			if( id != 0xFF ) {
				game.EntityEvents.RaiseRemoved( id );
				player.Despawn();
				game.Entities[id] = null;
			}
			
			// See comment about LegendCraft in HandleAddEntity
			int mask = id >> 3, bit = 1 << (id & 0x7);
			if( (needRemoveNames[mask] & bit) == 0 ) return;
			game.EntityEvents.RaiseTabEntryRemoved( id );
			game.TabList.Entries[id] = null;
			needRemoveNames[mask] &= (byte)~bit;
		}

		void ReadAbsoluteLocation( byte id, bool interpolate ) {
			float x = reader.ReadInt16() / 32f;
			float y = ( reader.ReadInt16() - 51 ) / 32f; // We have to do this.
			if( id == 255 ) y += 22/32f;
			
			float z = reader.ReadInt16() / 32f;
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			if( id == 0xFF ) {
				receivedFirstPosition = true;
			}
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			UpdateLocation( id, update, interpolate );
		}
		
		void UpdateLocation( byte playerId, LocationUpdate update, bool interpolate ) {
			Player player = game.Entities[playerId];
			if( player != null ) {
				player.SetLocation( update, interpolate );
			}
		}
		#endregion
	}
}