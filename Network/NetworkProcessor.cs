using System;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Network;
using ClassicalSharp.Network.Packets;
using OpenTK;

namespace ClassicalSharp {

	public partial class NetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			Window = window;
			SetupHandlers();
		}
		
		Socket socket;
		NetworkStream stream;
		NetReader reader;
		NetWriter writer;
		public Game Window;
		public string ServerName, ServerMotd;
		public bool Disconnected;
		public bool receivedFirstPosition = false;
		
		public void Connect() {
			IPAddress address = Window.IPAddress;
			socket = new Socket( address.AddressFamily, SocketType.Stream, ProtocolType.Tcp );
			try {
				socket.Connect( address, Window.Port );
			} catch( SocketException ex ) {
				Utils.LogError( "Error while trying to connect: {0}{1}", Environment.NewLine, ex );
				Window.Disconnect( "&eUnable to reach " + Window.IPAddress + ":" + Window.Port,
				                  "Unable to establish an underlying connection" );
				Dispose();
				return;
			}
			stream = new NetworkStream( socket, true );
			reader = new NetReader( stream );
			writer = new NetWriter( stream );
			//SendPacket( new LoginStartOutbound( Window.Username ) );
			RunIoThreadAsync();
		}
		
		public void SendChat( string text ) {
			//SendPacket( new ChatOutbound( text ) );
		}
		
		public void SendPosition( Vector3 pos, byte yaw, byte pitch ) {
		}
		
		public void SendDeleteBlock( PickedPos pos ) {
			Vector3I loc = pos.BlockPos;
			byte dir = pos.Direction;
			//OutboundPacket startPacket = new PlayerDiggingOutbound( DigStatus.Start, loc, dir );
			//SendPacket( startPacket );
			//OutboundPacket finishPacket = new PlayerDiggingOutbound( DigStatus.Finish, loc, dir );
			//SendPacket( finishPacket );
		}
		
		public void SendPlaceBlock( PickedPos pos ) {
			Vector3I loc = pos.BlockPos;
			byte dir = pos.Direction;
			//Slot slot = Window.Inventory.HeldSlot;
			//OutboundPacket packet = new PlayerPlaceBlockOutbound( loc, dir, slot, pos.CrosshairPos );
			//SendPacket( packet );
		}
		
		public void SendPacket( OutboundPacket packet ) {
			writeQueue.Enqueue( packet );
		}
		
		public void Dispose() {		
			StopProcessing = true;
			if( thread != null ) {
				thread.Join();
			}
			socket.Close();
			Disconnected = true;
		}
		
		Vector3 lastPos = new Vector3( float.MaxValue, float.MaxValue, float.MaxValue );
		float lastYaw = float.MaxValue, lastPitch = float.MaxValue;
		public void Tick( double delta ) {
			if( Disconnected ) return;
			
			InboundPacket packet = null;
			while( readQueue.Dequeue( ref packet ) ) {
				packet.ReadCallback( Window );
			}
			
			Player player = Window.LocalPlayer;
			if( receivedFirstPosition ) {
				// Figure out which is the most optimal packet to send.
				float yaw = player.YawDegrees - 180f;
				bool ori = lastYaw != yaw || lastPitch != player.PitchDegrees;
				bool pos = lastPos != player.Position;
				lastPos = player.Position;
				lastYaw = yaw;
				lastPitch = player.PitchDegrees;
				if( ori && pos ) {
					//SendPacket( new PlayerPosAndLookOutbound( true, lastPos, lastYaw, lastPitch ) );
				} else if( pos ) {
					//SendPacket( new PlayerPosOutbound( true, lastPos ) );
				} else if( ori ) {
					//SendPacket( new PlayerLookOutbound( true, lastYaw, lastPitch ) );
				} else {
					//SendPacket( new PlayerOutbound( true ) );
				}
			}
		}
	}
}