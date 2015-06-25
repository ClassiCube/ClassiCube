//#define NET_DEBUG
using System;
using System.Drawing;
using System.IO;
#if __MonoCS__
using Ionic.Zlib;
#else
using System.IO.Compression;
#endif
using System.Net;
using System.Net.Sockets;
using ClassicalSharp.Network;
using OpenTK;

namespace ClassicalSharp {

	public abstract class NetworkProcessor {
		
		public NetworkProcessor( Game window ) {
			Window = window;
		}
		
		protected Socket socket;
		protected NetworkStream stream;
		public Game Window;
		public bool Disconnected = false;
		public bool UsingExtPlayerList; // TODO: remove this
		
		public virtual void Connect( IPAddress address, int port) {
			socket = new Socket( address.AddressFamily, SocketType.Stream, ProtocolType.Tcp );
			try {
				socket.Connect( address, port );
			} catch( SocketException ex ) {
				Utils.LogError( "Error while trying to connect: {0}{1}", Environment.NewLine, ex );
				Window.Disconnect( "&eUnable to reach " + address + ":" +port,
				                  "Unable to establish an underlying connection" );
				Dispose();
				return;
			}
			stream = new NetworkStream( socket, true );
		}
		
		public abstract void SendChat( string text );
		
		public abstract void SendPosition( Vector3 pos, byte yaw, byte pitch );
		
		public abstract void SendSetBlock( int x, int y, int z, byte block );
		
		public virtual void Dispose() {
			socket.Close();
			Disconnected = true;
		}
		
		public abstract void Tick( double delta );
	}
}