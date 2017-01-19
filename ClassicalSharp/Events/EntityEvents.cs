// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Events {
	
	/// <summary> Contains events related to the spawning/despawning of entities, 
	/// and the creation/removal of tab list entries. </summary>
	public sealed class EntityEvents : EventsBase {
		
		IdEventArgs idArgs = new IdEventArgs();
		
		/// <summary> Raised when an entity is spawned in the current world. </summary>
		public event EventHandler<IdEventArgs> Added;
		public void RaiseAdded(byte id) { idArgs.Id = id; Raise(Added, idArgs); }
		
		/// <summary> Raised when an entity is despawned from the current world. </summary>
		public event EventHandler<IdEventArgs> Removed;
		public void RaiseRemoved(byte id) { idArgs.Id = id; Raise(Removed, idArgs); }
		
		/// <summary> Raised when a tab list entry is created. </summary>
		public event EventHandler<IdEventArgs> TabListEntryAdded;
		public void RaiseTabEntryAdded(byte id) { idArgs.Id = id; Raise(TabListEntryAdded, idArgs); }
		
		/// <summary> Raised when a tab list entry is modified. </summary>
		public event EventHandler<IdEventArgs> TabListEntryChanged;
		public void RaiseTabListEntryChanged(byte id) { idArgs.Id = id; Raise(TabListEntryChanged, idArgs); }
		
		/// <summary> Raised when a tab list entry is removed. </summary>
		public event EventHandler<IdEventArgs> TabListEntryRemoved;
		public void RaiseTabEntryRemoved(byte id) { idArgs.Id = id; Raise(TabListEntryRemoved, idArgs); }
	}
	
	public sealed class IdEventArgs : EventArgs {
		
		public byte Id;
	}
}
