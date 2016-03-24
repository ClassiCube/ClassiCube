using System;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Network;
using ClassicalSharp.TexturePack;

namespace ClassicalSharp {

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
			
			Disconnected = false;
			receivedFirstPosition = false;
			lastPacket = DateTime.UtcNow;
			game.MapEvents.OnNewMap += OnNewMap;
			
			MakeLoginPacket( game.Username, game.Mppass );
			SendPacket();
			lastPacket = DateTime.UtcNow;
		}
		
		public override void Dispose() {
			game.MapEvents.OnNewMap -= OnNewMap;
			socket.Close();
			Disconnected = true;
		}	
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			if( (DateTime.UtcNow - lastPacket).TotalSeconds >= 20 )
				CheckDisconnection( delta );
			if( Disconnected ) return;
			
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
				if( opcode == 0xFF && lastOpcode == PacketId.CpeHackControl ) {
					reader.Skip( 1 );
					game.LocalPlayer.jumpVel = 0.42f; // assume default jump height
					game.LocalPlayer.serverJumpVel = game.LocalPlayer.jumpVel;
					continue;
				}
				
				if( opcode >= maxHandledPacket ) {
					ErrorHandler.LogError( "NetworkProcessor.Tick",
					                      "received an invalid opcode of " + opcode );
					reader.Skip( 1 );
					continue;
				}
				
				if( (reader.size - reader.index) < packetSizes[opcode] ) break;
				ReadPacket( opcode );
			}
			reader.RemoveProcessed();
			
			Player player = game.LocalPlayer;
			if( receivedFirstPosition ) {
				SendPosition( player.Position, player.HeadYawDegrees, player.PitchDegrees );
			}
			CheckAsyncResources();
			CheckForWomEnvironment();
		}
		
		readonly int[] packetSizes = {
			131, 1, 1, 1028, 7, 9, 8, 74, 10, 7, 5, 4, 2,
			66, 65, 2, 67, 69, 3, 2, 3, 134, 196, 130, 3,
			8, 86, 2, 4, 66, 69, 2, 8, 138, 0, 80, 2, 85,
			1282, 6,
		};
		
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
		PacketId lastOpcode;
		
		void ReadPacket( byte opcode ) {
			reader.Skip( 1 ); // remove opcode
			lastOpcode = (PacketId)opcode;
			Action handler = handlers[opcode];
			lastPacket = DateTime.UtcNow;
			
			if( handler == null )
				throw new NotImplementedException( "Unsupported packet:" + (PacketId)opcode );
			handler();
		}
		
		void SkipPacketData( PacketId opcode ) {
			reader.Skip( packetSizes[(byte)opcode] - 1 );
		}
		
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
				null, HandleCpeDefineBlock, HandleCpeRemoveBlockDefinition, HandleCpeDefineBlockExt,
				HandleBulkBlockUpdate, HandleSetTextColor,
			};
			maxHandledPacket = handlers.Length;
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