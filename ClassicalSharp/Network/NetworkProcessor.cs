// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Entities;
using ClassicalSharp.Events;
using ClassicalSharp.Gui;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;

namespace ClassicalSharp.Network {

	public partial class NetworkProcessor : INetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			game = window;
			cpe = game.AddComponent( new CPESupport() );
			SetupHandlers();
		}
		
		public override bool IsSinglePlayer { get { return false; } }
		
		Socket socket;
		bool receivedFirstPosition;
		DateTime lastPacket;
		Screen prevScreen;
		bool prevCursorVisible;
		CPESupport cpe;
		ScheduledTask task;
		
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
			
			reader = new NetReader( socket );
			writer = new NetWriter( socket );
			gzippedMap = new FixedBufferStream( reader.buffer );
			
			Disconnected = false;
			receivedFirstPosition = false;
			lastPacket = DateTime.UtcNow;
			game.WorldEvents.OnNewMap += OnNewMap;
			game.UserEvents.BlockChanged += BlockChanged;
			
			MakeLoginPacket( game.Username, game.Mppass );
			SendPacket();
			lastPacket = DateTime.UtcNow;
		}
		
		public override void Dispose() {
			game.WorldEvents.OnNewMap -= OnNewMap;
			game.UserEvents.BlockChanged -= BlockChanged;
			socket.Close();
			Disconnected = true;
		}
		
		public override void Tick( ScheduledTask task ) {
			if( Disconnected ) return;
			if( (DateTime.UtcNow - lastPacket).TotalSeconds >= 30 )
				CheckDisconnection( task.Interval );
			if( Disconnected ) return;
			
			LocalPlayer player = game.LocalPlayer;
			this.task = task;
			
			try {
				reader.ReadPendingData();
			} catch( SocketException ex ) {
				ErrorHandler.LogError( "reading packets", ex );
				game.Disconnect( "&eLost connection to the server", "I/O error when reading packets" );
				Dispose();
				return;
			}
			
			while( (reader.size - reader.index) > 0 ) {
				byte opcode = reader.buffer[reader.index];
				// Workaround for older D3 servers which wrote one byte too many for HackControl packets.
				if( cpe.needD3Fix && lastOpcode == Opcode.CpeHackControl
				   && (opcode == 0x00 || opcode == 0xFF) ) {
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
			} catch( SocketException ) {
				// NOTE: Not immediately disconnecting, as otherwise we sometimes miss out on kick messages
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
			
			Set( Opcode.CpeExtInfo, HandleExtInfo, 67 );
			Set( Opcode.CpeExtEntry, HandleExtEntry, 69 );
			Set( Opcode.CpeSetClickDistance, HandleSetClickDistance, 3 );
			Set( Opcode.CpeCustomBlockSupportLevel, HandleCustomBlockSupportLevel, 2 );
			Set( Opcode.CpeHoldThis, HandleHoldThis, 3 );
			Set( Opcode.CpeSetTextHotkey, HandleSetTextHotkey, 134 );
			
			Set( Opcode.CpeExtAddPlayerName, HandleExtAddPlayerName, 196 );
			Set( Opcode.CpeExtAddEntity, HandleExtAddEntity, 130 );
			Set( Opcode.CpeExtRemovePlayerName, HandleExtRemovePlayerName, 3 );
			
			Set( Opcode.CpeEnvColours, HandleEnvColours, 8 );
			Set( Opcode.CpeMakeSelection, HandleMakeSelection, 86 );
			Set( Opcode.CpeRemoveSelection, HandleRemoveSelection, 2 );
			Set( Opcode.CpeSetBlockPermission, HandleSetBlockPermission, 4 );
			Set( Opcode.CpeChangeModel, HandleChangeModel, 66 );
			Set( Opcode.CpeEnvSetMapApperance, HandleEnvSetMapAppearance, 69 );
			Set( Opcode.CpeEnvWeatherType, HandleEnvWeatherType, 2 );
			Set( Opcode.CpeHackControl, HandleHackControl, 8 );
			Set( Opcode.CpeExtAddEntity2, HandleExtAddEntity2, 138 );
			
			Set( Opcode.CpeDefineBlock, HandleDefineBlock, 80 );
			Set( Opcode.CpeRemoveBlockDefinition, HandleRemoveBlockDefinition, 2 );
			Set( Opcode.CpeDefineBlockExt, HandleDefineBlockExt, 85 );
			Set( Opcode.CpeBulkBlockUpdate, HandleBulkBlockUpdate, 1282 );
			Set( Opcode.CpeSetTextColor, HandleSetTextColor, 6 );
			Set( Opcode.CpeSetMapEnvUrl, HandleSetMapEnvUrl, 65 );
			Set( Opcode.CpeSetMapEnvProperty, HandleSetMapEnvProperty, 6 );
		}
		
		void BlockChanged( object sender, BlockChangedEventArgs e ) {
			Vector3I p = e.Coords;
			byte block = game.Inventory.HeldBlock;
			if( e.Block == 0 )
				SendSetBlock( p.X, p.Y, p.Z, false, block );
			else
				SendSetBlock( p.X, p.Y, p.Z, true, e.Block );
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			// wipe all existing entity states
			for( int i = 0; i < 256; i++ )
				RemoveEntity( (byte)i );
		}
		
		double testAcc = 0;
		void CheckDisconnection( double delta ) {
			testAcc += delta;
			if( testAcc < 1 ) return;
			testAcc = 0;
			
			if( !socket.Connected || ( socket.Poll( 1000, SelectMode.SelectRead ) && socket.Available == 0 ) ) {
				game.Disconnect( "&eDisconnected from the server", "Connection timed out" );
				Dispose();
			}
		}
		
		void CheckName( byte id, ref string displayName, ref string skinName ) {
			displayName = Utils.RemoveEndPlus( displayName );
			skinName = Utils.RemoveEndPlus( skinName );
			skinName = Utils.StripColours( skinName );
			
			// Server is only allowed to change our own name colours.
			if( id != 0xFF ) return;			
			if( Utils.StripColours( displayName ) != game.Username )
				displayName = game.Username;
			if( skinName == "" )
				skinName = game.Username;
		}
	}
}