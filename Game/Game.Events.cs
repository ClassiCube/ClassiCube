using System;
using ClassicalSharp;
using OpenTK;

namespace ClassicalSharp {

	public partial class Game {
		
		/// <summary> Raised when a player is spawned in the current world. </summary>
		public event EventHandler<IdEventArgs> EntityAdded;
		
		/// <summary> Raised when a player is despawned from the current world. </summary>
		public event EventHandler<IdEventArgs> EntityRemoved;
		
		public event EventHandler<IdEventArgs> CpeListInfoAdded;
		
		public event EventHandler<IdEventArgs> CpeListInfoChanged;
		
		public event EventHandler<IdEventArgs> CpeListInfoRemoved;
		
		public event EventHandler<MapLoadingEventArgs> MapLoading;
		
		public event EventHandler<EnvVariableEventArgs> EnvVariableChanged;
		
		public event EventHandler ViewDistanceChanged;
		
		public event EventHandler OnNewMap;
		
		public event EventHandler OnNewMapLoaded;
		
		public event EventHandler HeldBlockChanged;
		
		/// <summary> Raised when the client's block permissions(can place or delete a block) change. </summary>
		public event EventHandler BlockPermissionsChanged;
		
		/// <summary> Raised when the terrain atlas("terrain.png") is changed. </summary>
		public event EventHandler TerrainAtlasChanged;
		
		public event EventHandler<ChatEventArgs> ChatReceived;
		
		internal void RaiseOnNewMap() {
			RaiseEvent( OnNewMap );
		}
		
		internal void RaiseHeldBlockChanged() {
			RaiseEvent( HeldBlockChanged );
		}
		
		IdEventArgs idArgs = new IdEventArgs( 0 );
		internal void RaiseEntityAdded( byte id ) {
			idArgs.Id = id;
			RaiseEvent( EntityAdded, idArgs );
		}
		
		internal void RaiseEntityRemoved( byte id ) {
			idArgs.Id = id;
			RaiseEvent( EntityRemoved, idArgs );
		}
		
		internal void RaiseCpeListInfoAdded( byte id ) {
			idArgs.Id = id;
			RaiseEvent( CpeListInfoAdded, idArgs );
		}
		
		internal void RaiseCpeListInfoChanged( byte id ) {
			idArgs.Id = id;
			RaiseEvent( CpeListInfoChanged, idArgs );
		}
		
		internal void RaiseCpeListInfoRemoved( byte id ) {
			idArgs.Id = id;
			RaiseEvent( CpeListInfoRemoved, idArgs );
		}
		
		internal void RaiseBlockPermissionsChanged() {
			RaiseEvent( BlockPermissionsChanged );
		}
		
		internal void RaiseOnNewMapLoaded() {
			RaiseEvent( OnNewMapLoaded );
		}
		
		MapLoadingEventArgs loadingArgs = new MapLoadingEventArgs( 0 );
		internal void RaiseMapLoading( byte progress ) {
			loadingArgs.Progress = progress;
			RaiseEvent( MapLoading, loadingArgs );
		}
		
		EnvVariableEventArgs envArgs = new EnvVariableEventArgs( 0 );
		internal void RaiseEnvVariableChanged( EnvVariable variable ) {
			envArgs.Variable = variable;
			RaiseEvent( EnvVariableChanged, envArgs );
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
		
		public byte Id;
		
		public IdEventArgs( byte id ) {
			Id = id;
		}
	}
	
	public sealed class ChatEventArgs : EventArgs {
		
		public CpeMessage Type;
		
		public string Text;
	}
	
	public sealed class MapLoadingEventArgs : EventArgs {
		
		public int Progress;
		
		public MapLoadingEventArgs( int progress ) {
			Progress = progress;
		}
	}
	
	public sealed class EnvVariableEventArgs : EventArgs {
		
		public EnvVariable Variable;
		
		public EnvVariableEventArgs( EnvVariable variable ) {
			Variable = variable;
		}
	}
	
	public enum EnvVariable {
		SidesBlock,
		EdgeBlock,
		WaterLevel,
		Weather,

		SkyColour,
		CloudsColour,
		FogColour,
		SunlightColour,
		ShadowlightColour,	
	}
	
	public enum Weather {
		Sunny,
		Rainy,
		Snowy,
	}
}
