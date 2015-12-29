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
		
		public override bool IsSinglePlayer {
			get { return false; }
		}
		
		Socket socket;
		Game game;
		bool receivedFirstPosition;
		DateTime lastPacket;
		Screen prevScreen;
		bool prevCursorVisible;
		
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
			
			Disconnected = false;
			receivedFirstPosition = false;
			lastPacket = DateTime.UtcNow;
			game.MapEvents.OnNewMap += OnNewMap;
			
			MakeLoginPacket( game.Username, game.Mppass );
			SendPacket();
			lastPing = DateTime.UtcNow;
		}
		
		public override void Dispose() {
			game.MapEvents.OnNewMap -= OnNewMap;
			socket.Close();
			Disconnected = true;
		}
		
		void CheckForNewTerrainAtlas() {
			DownloadedItem item;
			if( game.AsyncDownloader.TryGetItem( "terrain", out item ) ) {
				if( item.Data != null ) {
					game.ChangeTerrainAtlas( (Bitmap)item.Data );
					TextureCache.AddToCache( item.Url, (Bitmap)item.Data );
				} else if( Is304Status( item.WebEx ) ) {
					Bitmap bmp = TextureCache.GetBitmapFromCache( item.Url );
					if( bmp == null ) // Should never happen, but handle anyways.
						ExtractDefault();
					else
						game.ChangeTerrainAtlas( bmp );
				} else {
					ExtractDefault();
				}
			}
			
			if( game.AsyncDownloader.TryGetItem( "texturePack", out item ) ) {
				if( item.Data != null ) {
					TexturePackExtractor extractor = new TexturePackExtractor();
					extractor.Extract( (byte[])item.Data, game );
					TextureCache.AddToCache( item.Url, (byte[])item.Data );
				} else if( Is304Status( item.WebEx ) ) {
					byte[] data = TextureCache.GetDataFromCache( item.Url );
					if( data == null ) { // Should never happen, but handle anyways.
						ExtractDefault();
					} else {
						TexturePackExtractor extractor = new TexturePackExtractor();
						extractor.Extract( data, game );
					}
				} else {
					ExtractDefault();
				}
			}
		}
		
		void ExtractDefault() {
			TexturePackExtractor extractor = new TexturePackExtractor();
			extractor.Extract( game.DefaultTexturePack, game );
		}
		
		bool Is304Status( WebException ex ) {
			if( ex == null || ex.Status != WebExceptionStatus.ProtocolError )
				return false;			
			HttpWebResponse response = (HttpWebResponse)ex.Response;
			return response.StatusCode == HttpStatusCode.NotModified;
		}
		
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			if( (DateTime.UtcNow - lastPing).TotalSeconds >= 15 ) {
				game.Disconnect( "&eDisconnected from the server",
				                "No ping packet received for over 15 seconds." );
				Dispose();
				return;
			}
			
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
				// Workaround for older D3 servers which wrote one byte too many for HackControl packets.
				if( opcode == 0xFF && lastOpcode == PacketId.CpeHackControl ) {
					reader.Remove( 1 );
					game.LocalPlayer.CalculateJumpVelocity( 1.4f ); // assume default jump height
					continue;
				}
				
				if( opcode >= maxHandledPacket ) {
					ErrorHandler.LogError( "NetworkProcessor.Tick",
					                      "received an invalid opcode of " + opcode );
					reader.Remove( 1 );
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
			8, 86, 2, 4, 66, 69, 2, 8, 138, 0, 80, 2, 85,
			1282,
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
			reader.Remove( 1 ); // remove opcode
			lastOpcode = (PacketId)opcode;
			Action handler = handlers[opcode];
			
			if( handler == null )
				throw new NotImplementedException( "Unsupported packet:" + (PacketId)opcode );
			handler();
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
				HandleBulkBlockUpdate,
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
	}
}