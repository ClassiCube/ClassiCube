using System;
using System.Net;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	/// <summary> Represents a connection to either a multiplayer server, or an internal single player server. </summary>
	public abstract class INetworkProcessor {
		
		public abstract bool IsSinglePlayer { get; }
		
		public abstract void Connect( IPAddress address, int port );
		
		public abstract void SendChat( string text );
		
		public abstract void SendPosition( Vector3 pos, float yaw, float pitch );
		
		public abstract void SendSetBlock( int x, int y, int z, bool place, byte block );
		
		public abstract void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos );		
		
		public abstract void Tick( double delta );
		
		public abstract void Dispose();		
		
		public string ServerName, ServerMotd;
		public bool Disconnected;
		public bool UsingExtPlayerList, UsingPlayerClick;
	}
}