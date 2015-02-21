using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;

namespace Injector {
	
	class Program {
		
		public static void Main( string[] args ) {
			TcpListener listener = new TcpListener( IPAddress.Loopback, 25564 );
			listener.Start();
			while( true ) {
				TcpClient client = listener.AcceptTcpClient();
				InjectorThread injector = new InjectorThread( client, new IPEndPoint( IPAddress.Loopback, 25565 ) );
				Thread thread = new Thread( injector.Run );
				thread.Name = "Client XY";
				thread.Start();
			}
		}
	}
}