// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp {

	public class MapEvents : Events {
		
		/// <summary> Raised when the player joins and begins loading a new map. </summary>
		public event EventHandler OnNewMap;
		internal void RaiseOnNewMap() { Raise( OnNewMap ); }
		
		/// <summary> Raised when a portion of the map is read and decompressed, or generated. </summary>
		public event EventHandler<MapLoadingEventArgs> MapLoading;
		internal void RaiseMapLoading( byte progress ) { loadingArgs.Progress = progress; Raise( MapLoading, loadingArgs ); }
		
		/// <summary> Raised when new map has finished loading and the player can now interact with it. </summary>
		public event EventHandler OnNewMapLoaded;
		internal void RaiseOnNewMapLoaded() { Raise( OnNewMapLoaded ); }		
		
		/// <summary> Raised when an environment variable of the map is changed by the user, CPE, or WoM config. </summary>
		public event EventHandler<EnvVarEventArgs> EnvVariableChanged;
		internal void RaiseEnvVariableChanged( EnvVar envVar ) { envArgs.Var = envVar; Raise( EnvVariableChanged, envArgs ); }
	
		MapLoadingEventArgs loadingArgs = new MapLoadingEventArgs();
		EnvVarEventArgs envArgs = new EnvVarEventArgs();
	}
	
	public sealed class MapLoadingEventArgs : EventArgs {
		
		/// <summary> Percentage of the map that has been fully decompressed, or generated. </summary>
		public int Progress;
	}
	
	public sealed class EnvVarEventArgs : EventArgs {
		
		/// <summary> Map environment variable that was changed. </summary>
		public EnvVar Var;
	}
	
	public enum EnvVar {
		SidesBlock,
		EdgeBlock,
		EdgeLevel,
		CloudsLevel,
		CloudsSpeed,
		Weather,

		SkyColour,
		CloudsColour,
		FogColour,
		SunlightColour,
		ShadowlightColour,	
	}
}
