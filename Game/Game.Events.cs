using System;
using ClassicalSharp;
using OpenTK;

namespace ClassicalSharp {

	public partial class Game {
		
		/// <summary> Raised when a player is spawned in the current world. </summary>
		public event EventHandler<IdEventArgs> EntityAdded;
		
		/// <summary> Raised when information(display name and/or skin name) is changed about a player. </summary>
		/// <remarks> Note this event is only raised when using the CPE player list extension. </remarks>
		public event EventHandler<IdEventArgs> EntityInfoChanged;
		
		/// <summary> Raised when a player is despawned from the current world. </summary>
		public event EventHandler<IdEventArgs> EntityRemoved;
		
		public event EventHandler<TextEventArgs> CpeListInfoAdded;
		
		public event EventHandler<TextEventArgs> CpeListInfoChanged;
		
		public event EventHandler<TextEventArgs> CpeListInfoRemoved;
		
		public event EventHandler<MapLoadingEventArgs> MapLoading;
		
		public event EventHandler<EnvVariableEventArgs> EnvVariableChanged;
		
		public event EventHandler ViewDistanceChanged;
		
		public event EventHandler OnNewMap;
		
		public event EventHandler OnNewMapLoaded;
		
		/// <summary> Raised when the client's block permissions(can place or delete a block) change. </summary>
		public event EventHandler BlockPermissionsChanged;
		
		public event EventHandler<TextEventArgs> ChatReceived;
		
		internal void RaiseOnNewMap() {
			RaiseEvent( OnNewMap );
		}
		
		internal void RaiseEntityAdded( byte id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( EntityAdded, e );
		}
		
		internal void RaiseEntityInfoChange( byte id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( EntityInfoChanged, e );
		}
		
		internal void RaiseEntityRemoved( byte id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( EntityRemoved, e );
		}
		
		internal void RaiseCpeListInfoAdded( string name ) {
			TextEventArgs e = new TextEventArgs( name );
			RaiseEvent( CpeListInfoAdded, e );
		}
		
		internal void RaiseCpeListInfoChanged( string name ) {
			TextEventArgs e = new TextEventArgs( name );
			RaiseEvent( CpeListInfoChanged, e );
		}
		
		internal void RaiseCpeListInfoRemoved( string name ) {
			TextEventArgs e = new TextEventArgs( name );
			RaiseEvent( CpeListInfoRemoved, e );
		}
		
		internal void RaiseBlockPermissionsChanged() {
			RaiseEvent( BlockPermissionsChanged );
		}
		
		internal void RaiseOnNewMapLoaded() {
			RaiseEvent( OnNewMapLoaded );
		}
		
		internal void RaiseMapLoading( byte progress ) {
			MapLoadingEventArgs e = new MapLoadingEventArgs( progress );
			RaiseEvent( MapLoading, e );
		}
		
		internal void RaiseEnvVariableChanged( EnvVariable variable ) {
			EnvVariableEventArgs e = new EnvVariableEventArgs( variable );
			RaiseEvent( EnvVariableChanged, e );
		}
		
		void RaiseEvent( EventHandler handler ) {
			if( handler != null ) {
				handler( this, EventArgs.Empty );
			}
		}
		
		void RaiseEvent<T>( EventHandler<T> handler, T args ) where T : EventArgs {
			if( handler != null ) {
				handler( this, args );
			}
		}
	}
	
	public sealed class IdEventArgs : EventArgs {
		
		public readonly byte Id;
		
		public IdEventArgs( byte id ) {
			Id = id;
		}
	}
	
	public sealed class TextEventArgs : EventArgs {
		
		public readonly string Text;
		
		public TextEventArgs( string text ) {
			Text = text;
		}
	}
	
	public sealed class MapLoadingEventArgs : EventArgs {
		
		public readonly int Progress;
		
		public MapLoadingEventArgs( int progress ) {
			Progress = progress;
		}
	}
	
	public sealed class EnvVariableEventArgs : EventArgs {
		
		public readonly EnvVariable Variable;
		
		public EnvVariableEventArgs( EnvVariable variable ) {
			Variable = variable;
		}
	}
	
	public enum EnvVariable {
		SidesBlock,
		EdgeBlock,
		WaterLevel,

		SkyColour,
		CloudsColour,
		FogColour,
		SunlightColour,
		ShadowlightColour,
	}
}
