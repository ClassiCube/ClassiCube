using System;
using System.Net;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	/// <summary> Represents a connection to either a multiplayer server, or an internal single player server. </summary>
	public abstract class INetworkProcessor {
		
		public abstract bool IsSinglePlayer { get; }
		
		/// <summary> Opens a connection to the given IP address and port, and prepares the initial state of the client. </summary>
		public abstract void Connect( IPAddress address, int port );
		
		public abstract void SendChat( string text, bool partial );
		
		/// <summary> Informs the server of the client's current position and orientation. </summary>
		public abstract void SendPosition( Vector3 pos, float yaw, float pitch );
		
		/// <summary> Informs the server that the client placed or deleted a block at the given coordinates. </summary>
		public abstract void SendSetBlock( int x, int y, int z, bool place, byte block );
		
		/// <summary> Informs the server that using the given mouse button,
		/// the client clicked on a particular block or entity. </summary>
		public abstract void SendPlayerClick( MouseButton button, bool buttonDown, byte targetId, PickedPos pos );		
		
		public abstract void Tick( double delta );
		
		public abstract void Dispose();		
		
		public string ServerName;
		public string ServerMotd;
		
		/// <summary> Whether the network processor is currently connected to a server. </summary>
		public bool Disconnected;
		
		/// <summary> Whether the client should use extended player list management, with group names and ranks. </summary>
		public bool UsingExtPlayerList;
		
		/// <summary> Whether the server supports handling PlayerClick packets from the client. </summary>
		public bool UsingPlayerClick;
		
		/// <summary> Whether the server can handle partial message packets or not. </summary>
		public bool ServerSupportsPartialMessages;
		
		/// <summary> Whether the server supports receiving all code page 437 characters from this client. </summary>
		public bool ServerSupportsFullCP437;
	}
}