using System;
using System.Net;
using System.Net.Sockets;
using ClassicalSharp;
using ClassicalSharp.Network;
using OpenTK;

namespace PluginBeta173.Network {

	public partial class Beta173NetworkProcessor : NetworkProcessor {
		
		public Beta173NetworkProcessor( Game window ) : base( window ) {
			Window = window;
			SetupHandlers();
		}
		
		NetReader reader;
		NetWriter writer;
		public string ServerName, ServerMotd;
		public bool receivedFirstPosition = false;
		
		public void Connect() {
			IPAddress address = Window.IPAddress;
			socket = new Socket( address.AddressFamily, SocketType.Stream, ProtocolType.Tcp );
			try {
				socket.Connect( address, Window.Port );
			} catch( SocketException ex ) {
				Utils.LogError( "Error while trying to connect: {0}{1}", Environment.NewLine, ex );
				Window.Disconnect( "§eUnable to reach " + Window.IPAddress + ":" + Window.Port,
				                  "Unable to establish an underlying connection" );
				Dispose();
				return;
			}
			stream = new NetworkStream( socket, true );
			reader = new NetReader( stream );
			writer = new NetWriter( stream );
			SendPacket( new HandshakeOutbound( Window.Username ) );
			RunIoThreadAsync();
		}
		
		public override void SendChat( string text ) {
			SendPacket( new ChatOutbound( text ) );
		}
		
		public override void SendPosition( Vector3 pos, byte yaw, byte pitch ) {
		}
		
		public override void SendSetBlock( int x, int y, int z, byte block ) {
			throw new NotImplementedException();
		}
		
		public void SendDigBlock( PickedPos pos, DigStatus status ) {
			//Vector3I loc = pos.BlockPos;
			//byte dir = pos.Direction;
			//OutboundPacket startPacket = new PlayerDiggingOutbound( status, loc, dir );
			//SendPacket( startPacket );
		}
		
		public void SendPlaceBlock( PickedPos pos ) {
			//Vector3I loc = pos.BlockPos;
			//byte dir = pos.Direction;
			//Slot slot = Window.Inventory.HeldSlot;
			//if( !slot.IsEmpty ) {
			//	OutboundPacket packet = new PlayerPlaceBlockOutbound( loc, dir, slot );
			//	SendPacket( packet );
			//}
		}
		
		public void SendPacket( OutboundPacket packet ) {
			writeQueue.Enqueue( packet );
		}
		
		public override void Dispose() {
			StopProcessing = true;
			if( thread != null ) {
				thread.Join();
			}
			socket.Close();
			Disconnected = true;
		}
		
		Vector3 lastPos = new Vector3( float.MaxValue, float.MaxValue, float.MaxValue );
		float lastYaw = float.MaxValue, lastPitch = float.MaxValue;
		public override void Tick( double delta ) {
			if( Disconnected ) return;
			
			InboundPacket packet = null;
			while( readQueue.Dequeue( ref packet ) ) {
				try {
					packet.ReadCallback( Window );
				} catch( NotImplementedException ) {
					// TODO: Finish all missing packets.
				}
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
					SendPacket( new PlayerPosAndLookOutbound( true, lastPos, lastPos.Y + Player.EyeHeight, lastYaw, lastPitch ) );
				} else if( pos ) {
					SendPacket( new PlayerPosOutbound( true, lastPos, lastPos.Y + Player.EyeHeight ) );
				} else if( ori ) {
					SendPacket( new PlayerLookOutbound( true, lastYaw, lastPitch ) );
				} else {
					SendPacket( new PlayerOutbound( true ) );
				}
			}
		}
	}
}