using System;
using System.IO;
using System.Net;
using ClassicalSharp;
using ClassicalSharp.Network;
using OpenTK;

namespace DefaultPlugin.Network {

	public partial class ClassicNetworkProcessor : NetworkProcessor {
		
		public ClassicNetworkProcessor( Game window ) : base( window ) {
		}
		
		public string ServerName, ServerMotd;
		bool sendHeldBlock = false;
		bool useMessageTypes = false;
		bool useBlockPermissions = false;
		
		public override void Connect( IPAddress address, int port ) {
			base.Connect( address, port );
			reader = new FastNetReader( stream );
			gzippedMap = new FixedBufferStream( reader.buffer );
			WritePacket( MakeLoginPacket( Window.Username, Window.Mppass ) );
		}
		
		public override void SendChat( string text ) {
			if( !String.IsNullOrEmpty( text ) ) {
				WritePacket( MakeMessagePacket( text ) );
			}
		}
		
		public override void SendPosition( Vector3 pos, float yawDegrees, float pitchDegrees ) {
			byte payload = sendHeldBlock ? (byte)Window.Inventory.HeldBlock : (byte)0xFF;
			byte yawPacked = Utils.DegreesToPacked( yawDegrees );
			byte pitchPacked = Utils.DegreesToPacked( pitchDegrees );
			WritePacket( MakePositionPacket( pos, yawPacked, pitchPacked, payload ) );
		}
		
		public override void SendSetBlock( int x, int y, int z, byte block ) {
			WritePacket( MakeSetBlockPacket( (short)x, (short)y, (short)z, block == 0 ? (byte)0 : (byte)1, block ) );
		}
		
		void CheckForNewTerrainAtlas() {
			DownloadedItem item;
			Window.AsyncDownloader.TryGetItem( "terrain", out item );
			if( item != null && item.Bmp != null ) {
				Window.ChangeTerrainAtlas( item.Bmp );
			}
		}
		
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			
			try {
				reader.ReadPendingData();
			} catch( IOException ex ) {
				Utils.LogError( "Error while reading packets: {0}{1}", Environment.NewLine, ex );
				Window.Disconnect( "&eLost connection to the server", "Underlying connection was closed" );
				Dispose();
				return;
			}
			
			int received = reader.size;
			while( received > 0 ) {
				byte opcode = reader.buffer[0];
				if( received < packetSizes[opcode] ) break;
				ReadPacket( opcode );
				received = reader.size;
			}
			
			Player player = Window.LocalPlayer;
			if( receivedFirstPosition ) {				
				SendPosition( player.Position, player.YawDegrees, player.PitchDegrees );
			}
			CheckForNewTerrainAtlas();
			CheckForWomEnvironment();
		}
				
		void UpdateLocation( byte playerId, LocationUpdate update, bool interpolate ) {
			Player player = playerId == 0xFF ? Window.LocalPlayer : Window.NetPlayers[playerId];
			if( player != null ) {
				player.SetLocation( update, interpolate );
			}
		}
	}
}