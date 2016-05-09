// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Events {
	
	/// <summary> Contains events related to the spawning/despawning of entities, 
	/// and the creation/removal of CPE player list entries. </summary>
	public sealed class EntityEvents : OtherEvents {
		
		IdEventArgs idArgs = new IdEventArgs();
		
		/// <summary> Raised when an entity is spawned in the current world. </summary>
		public event EventHandler<IdEventArgs> Added;
		internal void RaiseAdded( byte id ) { idArgs.Id = id; Raise( Added, idArgs ); }
		
		/// <summary> Raised when an entity is despawned from the current world. </summary>
		public event EventHandler<IdEventArgs> Removed;
		internal void RaiseRemoved( byte id ) { idArgs.Id = id; Raise( Removed, idArgs ); }
		
		/// <summary> Raised when a new CPE player list entry is created. </summary>
		public event EventHandler<IdEventArgs> CpeListInfoAdded;
		internal void RaiseCpeListInfoAdded( byte id ) { idArgs.Id = id; Raise( CpeListInfoAdded, idArgs ); }
		
		/// <summary> Raised when a CPE player list entry is modified. </summary>
		public event EventHandler<IdEventArgs> CpeListInfoChanged;
		internal void RaiseCpeListInfoChanged( byte id ) { idArgs.Id = id; Raise( CpeListInfoChanged, idArgs ); }
		
		/// <summary> Raised when a CPE player list entry is removed. </summary>
		public event EventHandler<IdEventArgs> CpeListInfoRemoved;
		internal void RaiseCpeListInfoRemoved( byte id ) { idArgs.Id = id; Raise( CpeListInfoRemoved, idArgs ); }
	}
	
	public sealed class IdEventArgs : EventArgs {
		
		public byte Id;
	}
}
