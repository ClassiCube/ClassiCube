// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;

namespace ClassicalSharp.Net {

	public partial class NetworkProcessor : INetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			game = window;
			SetupHandlers();
		}
		
		public override bool IsSinglePlayer { get { return false; } }
		
		Socket socket;
		bool receivedFirstPosition;
		DateTime lastPacket;
		Screen prevScreen;
		bool prevCursorVisible, supportsCustomBlocks;
		
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
			
			NetworkStream stream = new NetworkStream( socket, true );
			reader = new NetReader( stream );
			writer = new NetWriter( stream );
			gzippedMap = new FixedBufferStream( reader.buffer );
			envMapAppearanceVer = 2;
			blockDefinitionsExtVer = 2;
			needD3Fix = false;
			
			Disconnected = false;
			receivedFirstPosition = false;
			lastPacket = DateTime.UtcNow;
			game.WorldEvents.OnNewMap += OnNewMap;
			
			MakeLoginPacket( game.Username, game.Mppass );
			SendPacket();
			lastPacket = DateTime.UtcNow;
		}
		
		public override void Dispose() {
			game.WorldEvents.OnNewMap -= OnNewMap;
			socket.Close();
			Disconnected = true;
		}
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			if( (DateTime.UtcNow - lastPacket).TotalSeconds >= 20 )
				CheckDisconnection( delta );
			if( Disconnected ) return;
			LocalPlayer player = game.LocalPlayer;
			
			try {
				reader.ReadPendingData();
			} catch( IOException ex ) {
				ErrorHandler.LogError( "reading packets", ex );
				game.Disconnect( "&eLost connection to the server", "I/O error when reading packets" );
				Dispose();
				return;
			} catch {
				throw;
			}
			
			while( (reader.size - reader.index) > 0 ) {
				byte opcode = reader.buffer[reader.index];
				// Workaround for older D3 servers which wrote one byte too many for HackControl packets.
				if( needD3Fix && lastOpcode == Opcode.CpeHackControl && (opcode == 0x00 || opcode == 0xFF) ) {
					Utils.LogDebug( "Skipping invalid HackControl byte from D3 server." );
					reader.Skip( 1 );
					player.physics.jumpVel = 0.42f; // assume default jump height
					player.physics.serverJumpVel = player.physics.jumpVel;
					continue;
				}
				
				if( opcode > maxHandledPacket ) {
					ErrorHandler.LogError( "NetworkProcessor.Tick",
					                      "received an invalid opcode of " + opcode );
					reader.Skip( 1 );
					continue;
				}
				
				if( (reader.size - reader.index) < packetSizes[opcode] ) break;
				ReadPacket( opcode );
			}
			
			reader.RemoveProcessed();
			if( receivedFirstPosition ) {
				SendPosition( player.Position, player.HeadYawDegrees, player.PitchDegrees );
			}
			CheckAsyncResources();
			CheckForWomEnvironment();
		}
		
		/// <summary> Sets the incoming packet handler for the given packet id. </summary>
		public void Set( Opcode opcode, Action handler, int packetSize ) {
			handlers[(byte)opcode] = handler;
			packetSizes[(byte)opcode] = (ushort)packetSize;
			maxHandledPacket = Math.Max( (byte)opcode, maxHandledPacket );
		}
		
		NetWriter writer;
		void SendPacket() {
			if( Disconnected ) {
				writer.index = 0;
				return;
			}
			try {
				writer.Send();
			} catch( IOException ex ) {
				// NOTE: Not immediately disconnecting, because it means we miss out on kick messages sometimes.
				//ErrorHandler.LogError( "writing packets", ex );
				//game.Disconnect( "&eLost connection to the server", "I/O Error while writing packets" );
				//Dispose();
				writer.index = 0;
			}
		}
		
		NetReader reader;
		Opcode lastOpcode;
		
		void ReadPacket( byte opcode ) {
			reader.Skip( 1 ); // remove opcode
			lastOpcode = (Opcode)opcode;
			Action handler = handlers[opcode];
			lastPacket = DateTime.UtcNow;
			
			if( handler == null )
				throw new NotImplementedException( "Unsupported packet:" + (Opcode)opcode );
			handler();
		}
		
		void SkipPacketData( Opcode opcode ) {
			reader.Skip( packetSizes[(byte)opcode] - 1 );
		}
		
		Action[] handlers = new Action[256];
		ushort[] packetSizes = new ushort[256];
		int maxHandledPacket = 0;
		
		void SetupHandlers() {
			Set( Opcode.Handshake, HandleHandshake, 131 );
			Set( Opcode.Ping, HandlePing, 1 );
			Set( Opcode.LevelInit, HandleLevelInit, 1 );
			Set( Opcode.LevelDataChunk, HandleLevelDataChunk, 1028 );
			Set( Opcode.LevelFinalise, HandleLevelFinalise, 7 );
			Set( Opcode.SetBlock, HandleSetBlock, 8 );
			
			Set( Opcode.AddEntity, HandleAddEntity, 74 );
			Set( Opcode.EntityTeleport, HandleEntityTeleport, 10 );
			Set( Opcode.RelPosAndOrientationUpdate, HandleRelPosAndOrientationUpdate, 7 );
			Set( Opcode.RelPosUpdate, HandleRelPositionUpdate, 5 );
			Set( Opcode.OrientationUpdate, HandleOrientationUpdate, 4 );
			Set( Opcode.RemoveEntity, HandleRemoveEntity, 2 );
			
			Set( Opcode.Message, HandleMessage, 66 );
			Set( Opcode.Kick, HandleKick, 65 );
			Set( Opcode.SetPermission, HandleSetPermission, 2 );
			
			Set( Opcode.CpeExtInfo, HandleCpeExtInfo, 67 );
			Set( Opcode.CpeExtEntry, HandleCpeExtEntry, 69 );
			Set( Opcode.CpeSetClickDistance, HandleCpeSetClickDistance, 3 );
			Set( Opcode.CpeCustomBlockSupportLevel, HandleCpeCustomBlockSupportLevel, 2 );
			Set( Opcode.CpeHoldThis, HandleCpeHoldThis, 3 );
			Set( Opcode.CpeSetTextHotkey, HandleCpeSetTextHotkey, 134 );
			
			Set( Opcode.CpeExtAddPlayerName, HandleCpeExtAddPlayerName, 196 );
			Set( Opcode.CpeExtAddEntity, HandleCpeExtAddEntity, 130 );
			Set( Opcode.CpeExtRemovePlayerName, HandleCpeExtRemovePlayerName, 3 );
			
			Set( Opcode.CpeEnvColours, HandleCpeEnvColours, 8 );
			Set( Opcode.CpeMakeSelection, HandleCpeMakeSelection, 86 );
			Set( Opcode.CpeRemoveSelection, HandleCpeRemoveSelection, 2 );
			Set( Opcode.CpeSetBlockPermission, HandleCpeSetBlockPermission, 4 );
			Set( Opcode.CpeChangeModel, HandleCpeChangeModel, 66 );
			Set( Opcode.CpeEnvSetMapApperance, HandleCpeEnvSetMapApperance, 69 );
			Set( Opcode.CpeEnvWeatherType, HandleCpeEnvWeatherType, 2 );
			Set( Opcode.CpeHackControl, HandleCpeHackControl, 8 );
			Set( Opcode.CpeExtAddEntity2, HandleCpeExtAddEntity2, 138 );
			
			Set( Opcode.CpeDefineBlock, HandleCpeDefineBlock, 80 );
			Set( Opcode.CpeRemoveBlockDefinition, HandleCpeRemoveBlockDefinition, 2 );
			Set( Opcode.CpeDefineBlockExt, HandleCpeDefineBlockExt, 85 );
			Set( Opcode.CpeBulkBlockUpdate, HandleBulkBlockUpdate, 1282 );
			Set( Opcode.CpeSetTextColor, HandleSetTextColor, 6 );
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			// wipe all existing entity states
			for( int i = 0; i < 256; i++ ) {
				if( game.CpePlayersList[i] != null ) {
					game.EntityEvents.RaiseCpeListInfoRemoved( (byte)i );
					game.CpePlayersList[i] = null;
				}
				RemoveEntity( (byte)i );
			}
		}
		
		double testAcc = 0;
		void CheckDisconnection( double delta ) {
			testAcc += delta;
			if( testAcc < 1 ) return;
			testAcc = 0;
			
			if( !socket.Connected || (socket.Poll( 1000, SelectMode.SelectRead ) && socket.Available == 0 ) ) {
				game.Disconnect( "&eDisconnected from the server",
				                "I/O connection timed out." );
				Dispose();
			}
		}
	}
}