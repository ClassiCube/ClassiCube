// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;

namespace ClassicalSharp.Events {

	public class WorldEvents : EventsBase {
		
		/// <summary> Raised when the player joins and begins loading a new world. </summary>
		public event EventHandler OnNewMap;
		public void RaiseOnNewMap() { Raise(OnNewMap); }
		
		/// <summary> Raised when a portion of the world is read and decompressed, or generated. </summary>
		public event EventHandler<LoadingEventArgs> Loading;
		public void RaiseLoading(float progress) { loadingArgs.Progress = progress; Raise(Loading, loadingArgs); }
		
		/// <summary> Raised when new world has finished loading and the player can now interact with it. </summary>
		public event EventHandler OnNewMapLoaded;
		public void RaiseOnNewMapLoaded() { Raise(OnNewMapLoaded); }		
		
		/// <summary> Raised when an environment variable of the world is changed by the user, CPE, or WoM config. </summary>
		public event EventHandler<EnvVarEventArgs> EnvVariableChanged;
		public void RaiseEnvVariableChanged(EnvVar envVar) { envArgs.Var = envVar; Raise(EnvVariableChanged, envArgs); }
	
		LoadingEventArgs loadingArgs = new LoadingEventArgs();
		EnvVarEventArgs envArgs = new EnvVarEventArgs();
	}
	
	public sealed class LoadingEventArgs : EventArgs {
		
		/// <summary> Percentage of the map that has been fully decompressed, or generated. </summary>
		public float Progress;
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
		
		WeatherSpeed,
		WeatherFade,
		ExpFog,
		SidesOffset,
		SkyboxHorSpeed,
		SkyboxVerSpeed,
	}
}
