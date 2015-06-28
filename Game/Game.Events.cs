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
		
		public void RaiseOnNewMap() {
			RaiseEvent( OnNewMap );
		}
		
		public void RaiseHeldBlockChanged() {
			RaiseEvent( HeldBlockChanged );
		}
		
		public void RaiseEntityAdded( int id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( EntityAdded, e );
		}
		
		public void RaiseEntityRemoved( int id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( EntityRemoved, e );
		}
		
		public void RaiseCpeListInfoAdded( int id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( CpeListInfoAdded, e );
		}
		
		public void RaiseCpeListInfoChanged( int id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( CpeListInfoChanged, e );
		}
		
		public void RaiseCpeListInfoRemoved( int id ) {
			IdEventArgs e = new IdEventArgs( id );
			RaiseEvent( CpeListInfoRemoved, e );
		}
		
		public void RaiseBlockPermissionsChanged() {
			RaiseEvent( BlockPermissionsChanged );
		}
		
		public void RaiseOnNewMapLoaded() {
			RaiseEvent( OnNewMapLoaded );
		}
		
		public void RaiseMapLoading( byte progress ) {
			MapLoadingEventArgs e = new MapLoadingEventArgs( progress );
			RaiseEvent( MapLoading, e );
		}
		
		public void RaiseEnvVariableChanged( EnvVariable variable ) {
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
		
		public readonly int Id;
		
		public IdEventArgs( int id ) {
			Id = id;
		}
	}
	
	public sealed class ChatEventArgs : EventArgs {
		
		public byte Type;
		
		public string Text;
		
		public ChatEventArgs( string text, byte type ) {
			Type = type;
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
