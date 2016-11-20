// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Gui.Screens;
using ClassicalSharp.Entities;
using OpenTK;
#if __MonoCS__
using Ionic.Zlib;
#else
using System.IO.Compression;
#endif

namespace ClassicalSharp.Network.Protocols {

	/// <summary> Implements the packets for the original classic. </summary>
	public sealed class ClassicProtocol : IProtocol {
		
		public ClassicProtocol( Game game ) : base( game ) { }
		
		public override void Init() {
			gzippedMap = new FixedBufferStream( net.reader.buffer );
			net.Set( Opcode.Handshake, HandleHandshake, 131 );
			net.Set( Opcode.Ping, HandlePing, 1 );
			net.Set( Opcode.LevelInit, HandleLevelInit, 1 );
			net.Set( Opcode.LevelDataChunk, HandleLevelDataChunk, 1028 );
			net.Set( Opcode.LevelFinalise, HandleLevelFinalise, 7 );
			net.Set( Opcode.SetBlock, HandleSetBlock, 8 );
			
			net.Set( Opcode.AddEntity, HandleAddEntity, 74 );
			net.Set( Opcode.EntityTeleport, HandleEntityTeleport, 10 );
			net.Set( Opcode.RelPosAndOrientationUpdate, HandleRelPosAndOrientationUpdate, 7 );
			net.Set( Opcode.RelPosUpdate, HandleRelPositionUpdate, 5 );
			net.Set( Opcode.OrientationUpdate, HandleOrientationUpdate, 4 );
			net.Set( Opcode.RemoveEntity, HandleRemoveEntity, 2 );
			
			net.Set( Opcode.Message, HandleMessage, 66 );
			net.Set( Opcode.Kick, HandleKick, 65 );
			net.Set( Opcode.SetPermission, HandleSetPermission, 2 );
		}
		
		DateTime mapReceiveStart;
		DeflateStream gzipStream;
		GZipHeaderReader gzipHeader;
		int mapSizeIndex, mapIndex;
		byte[] mapSize = new byte[4], map;
		FixedBufferStream gzippedMap;
		Screen prevScreen;
		bool prevCursorVisible;
		
		#region Read

		void HandleHandshake() {
			byte protocolVer = reader.ReadUInt8();
			net.ServerName = reader.ReadCp437String();
			net.ServerMotd = reader.ReadCp437String();
			game.Chat.SetLogName( net.ServerName );
			
			game.LocalPlayer.Hacks.SetUserType( reader.ReadUInt8() );
			game.LocalPlayer.Hacks.ParseHackFlags( net.ServerName, net.ServerMotd );
			game.LocalPlayer.CheckHacksConsistency();
			game.Events.RaiseHackPermissionsChanged();
		}
		
		void HandlePing() { }
		
		void HandleLevelInit() {
			if( gzipStream != null ) return;
			game.World.Reset();
			prevScreen = game.Gui.activeScreen;
			if( prevScreen is LoadingMapScreen )
				prevScreen = null;
			prevCursorVisible = game.CursorVisible;
			
			game.Gui.SetNewScreen( new LoadingMapScreen( game, net.ServerName, net.ServerMotd ), false );
			net.wom.CheckMotd();
			net.receivedFirstPosition = false;
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
			mapReceiveStart = DateTime.UtcNow;
			net.task.Interval = 1.0 / 60;
		}
		
		void HandleLevelDataChunk() {
			// Workaround for some servers that send LevelDataChunk before LevelInit
			// due to their async packet sending behaviour.
			if( gzipStream == null ) HandleLevelInit();
			int usedLength = reader.ReadInt16();
			gzippedMap.pos = 0;
			gzippedMap.bufferPos = reader.index;
			gzippedMap.len = usedLength;
			
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
			net.task.Interval = 1.0 / 20;
			game.Gui.SetNewScreen( null );
			game.Gui.activeScreen = prevScreen;
			if( prevScreen != null && prevCursorVisible != game.CursorVisible ) {
				game.CursorVisible = prevCursorVisible;
			}
			prevScreen = null;
			
			int mapWidth = reader.ReadInt16();
			int mapHeight = reader.ReadInt16();
			int mapLength = reader.ReadInt16();
			
			double loadingMs = (DateTime.UtcNow - mapReceiveStart).TotalMilliseconds;
			Utils.LogDebug( "map loading took: " + loadingMs );
			game.World.SetNewMap( map, mapWidth, mapHeight, mapLength );
			game.WorldEvents.RaiseOnNewMapLoaded();
			
			map = null;
			gzipStream.Dispose();
			net.wom.CheckSendWomID();
			gzipStream = null;
			GC.Collect();
		}
		
		void HandleSetBlock() {
			int x = reader.ReadInt16();
			int y = reader.ReadInt16();
			int z = reader.ReadInt16();
			byte type = reader.ReadUInt8();
			
			#if DEBUG_BLOCKS
			if( game.World.IsNotLoaded ) {
				Utils.LogDebug( "Server tried to update a block while still sending us the map!" );
			} else if( !game.World.IsValidPos( x, y, z ) ) {
				Utils.LogDebug( "Server tried to update a block at an invalid position!" );
			} else {
				game.UpdateBlock( x, y, z, type );
			}
			#else
			if( !game.World.IsNotLoaded && game.World.IsValidPos( x, y, z ) ) {
				game.UpdateBlock( x, y, z, type );
			}
			#endif
		}
		
		void HandleAddEntity() {
			byte id = reader.ReadUInt8();
			string name = reader.ReadAsciiString();
			string skin = name;
			net.CheckName( id, ref name, ref skin );
			
			net.AddEntity( id, name, skin, true );
			// Also workaround for LegendCraft as it declares it supports ExtPlayerList but
			// doesn't send ExtAddPlayerName packets.
			net.AddTablistEntry( id, name, name, "Players", 0 );
			net.needRemoveNames[id >> 3] |= (byte)(1 << (id & 0x7));
		}
		
		void HandleEntityTeleport() {
			byte id = reader.ReadUInt8();
			ReadAbsoluteLocation( id, true );
		}
		
		void HandleRelPosAndOrientationUpdate() {
			byte id = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, true );
			net.UpdateLocation( id, update, true );
		}
		
		void HandleRelPositionUpdate() {
			byte id = reader.ReadUInt8();
			float x = reader.ReadInt8() / 32f;
			float y = reader.ReadInt8() / 32f;
			float z = reader.ReadInt8() / 32f;
			
			LocationUpdate update = LocationUpdate.MakePos( x, y, z, true );
			net.UpdateLocation( id, update, true );
		}
		
		void HandleOrientationUpdate() {
			byte id = reader.ReadUInt8();
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			
			LocationUpdate update = LocationUpdate.MakeOri( yaw, pitch );
			net.UpdateLocation( id, update, true );
		}
		
		void HandleRemoveEntity() {
			byte id = reader.ReadUInt8();
			net.RemoveEntity( id );
		}
		
		void HandleMessage() {
			byte type = reader.ReadUInt8();
			// Original vanilla server uses player ids in message types, 255 for server messages.
			bool prepend = !net.cpeData.useMessageTypes && type == 0xFF;
			
			if( !net.cpeData.useMessageTypes ) type = (byte)MessageType.Normal;
			string text = reader.ReadChatString( ref type );
			if( prepend ) text = "&e" + text;
			
			if( !text.StartsWith("^detail.user", StringComparison.OrdinalIgnoreCase ) )
				game.Chat.Add( text, (MessageType)type );
		}
		
		void HandleKick() {
			string reason = reader.ReadCp437String();
			game.Disconnect( "&eLost connection to the server", reason );
			net.Dispose();
		}
		
		void HandleSetPermission() {
			game.LocalPlayer.Hacks.SetUserType( reader.ReadUInt8() );
		}
		
		internal void ReadAbsoluteLocation( byte id, bool interpolate ) {
			float x = reader.ReadInt16() / 32f;
			float y = ( reader.ReadInt16() - 51 ) / 32f; // We have to do this.
			if( id == 255 ) y += 22/32f;
			
			float z = reader.ReadInt16() / 32f;
			float yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			float pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			if( id == 0xFF ) {
				net.receivedFirstPosition = true;
			}
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			net.UpdateLocation( id, update, interpolate );
		}
		#endregion
		
		#region Write
		
		internal void SendChat( string text, bool partial ) {
			int payload = !net.SupportsPartialMessages ? 0xFF: (partial ? 1 : 0);
			writer.WriteUInt8( (byte)Opcode.Message );
			
			writer.WriteUInt8( (byte)payload );
			writer.WriteString( text );
			net.SendPacket();
		}
		
		internal void SendPosition( Vector3 pos, float yaw, float pitch ) {
			byte payload = net.cpeData.sendHeldBlock ? game.Inventory.HeldBlock : (byte)0xFF;
			writer.WriteUInt8( (byte)Opcode.EntityTeleport );
			
			writer.WriteUInt8( payload ); // held block when using HeldBlock, otherwise just 255
			writer.WriteInt16( (short)(pos.X * 32) );
			writer.WriteInt16( (short)((int)(pos.Y * 32) + 51) );
			writer.WriteInt16( (short)(pos.Z * 32) );
			writer.WriteUInt8( (byte)Utils.DegreesToPacked( yaw ) );
			writer.WriteUInt8( (byte)Utils.DegreesToPacked( pitch ) );
			net.SendPacket();
		}
		
		internal void SendSetBlock( int x, int y, int z, bool place, byte block ) {
			writer.WriteUInt8( (byte)Opcode.SetBlockClient );
			
			writer.WriteInt16( (short)x );
			writer.WriteInt16( (short)y );
			writer.WriteInt16( (short)z );
			writer.WriteUInt8( place ? (byte)1 : (byte)0 );
			writer.WriteUInt8( block );
			net.SendPacket();
		}
		
		internal void SendLogin( string username, string verKey ) {
			byte payload = game.UseCPE ? (byte)0x42 : (byte)0x00;
			writer.WriteUInt8( (byte)Opcode.Handshake );
			
			writer.WriteUInt8( 7 ); // protocol version
			writer.WriteString( username );
			writer.WriteString( verKey );
			writer.WriteUInt8( payload );
			net.SendPacket();
		}
		
		#endregion
	}
}
