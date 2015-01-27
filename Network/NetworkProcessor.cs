//#define NET_DEBUG
using System;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Net.Sockets;
using System.Text;
using OpenTK;

namespace ClassicalSharp {

	public partial class NetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			Window = window;
		}
		
		Socket socket;
		NetworkStream stream;
		public Game Window;
		public bool Disconnected;
		bool receivedFirstPosition = false;
		
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
			//WritePacket( MakeLoginPacket( Window.Username, Window.Mppass ) );
		}
		
		public void SendChat( string text ) {

		}
		
		public void SendPosition( Vector3 pos, byte yaw, byte pitch ) {

		}
		
		public void SendSetBlock( int x, int y, int z, byte block ) {

		}
		
		public void Dispose() {
			socket.Close();
			Disconnected = true;
		}
		
		public void Tick( double delta ) {
			if( Disconnected ) return;
		}
	}
}