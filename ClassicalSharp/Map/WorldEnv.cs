// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;

namespace ClassicalSharp.Map {

	public enum Weather { Sunny, Rainy, Snowy, }
	
	/// <summary> Contains  the environment metadata for a world. </summary>
	public sealed class WorldEnv {
		
		/// <summary> Colour of the sky located behind/above clouds. </summary>
		public FastColour SkyCol = DefaultSkyColour;
		public static readonly FastColour DefaultSkyColour = new FastColour( 0x99, 0xCC, 0xFF );
		
		/// <summary> Colour applied to the fog/horizon looking out horizontally.
		/// Note the true horizon colour is a blend of this and sky colour. </summary>
		public FastColour FogCol = DefaultFogColour;
		public static readonly FastColour DefaultFogColour = new FastColour( 0xFF, 0xFF, 0xFF );
		
		/// <summary> Colour applied to the clouds. </summary>
		public FastColour CloudsCol = DefaultCloudsColour;
		public static readonly FastColour DefaultCloudsColour =  new FastColour( 0xFF, 0xFF, 0xFF );
		
		/// <summary> Height of the clouds in world space. </summary>
		public int CloudHeight;
		
		/// <summary> How fast clouds should travel across the map, defaults to 1. </summary>
		public float CloudsSpeed = 1;
		
		/// <summary> Colour applied to blocks located in direct sunlight. </summary>
		public FastColour Sunlight;
		public FastColour SunlightXSide, SunlightZSide, SunlightYBottom;
		public static readonly FastColour DefaultSunlight = new FastColour( 0xFF, 0xFF, 0xFF );
		
		/// <summary> Colour applied to blocks located in shadow / hidden from direct sunlight. </summary>
		public FastColour Shadowlight;
		public FastColour ShadowlightXSide, ShadowlightZSide, ShadowlightYBottom;
		public static readonly FastColour DefaultShadowlight = new FastColour( 0x9B, 0x9B, 0x9B );
		
		/// <summary> Current weather for this particular map. </summary>
		public Weather Weather = Weather.Sunny;
		
		/// <summary> Block that surrounds map the map horizontally (default water) </summary>
		public Block EdgeBlock = Block.StillWater;
		
		/// <summary> Height of the map edge in world space. </summary>
		public int EdgeHeight;
		
		/// <summary> Block that surrounds the map that fills the bottom of the map horizontally, 
		/// fills part of the vertical sides of the map, and also surrounds map the map horizontally. (default bedrock) </summary>
		public Block SidesBlock = Block.Bedrock;
		
		/// <summary> Maximum height of the various parts of the map sides, in world space. </summary>
		public int SidesHeight { get { return EdgeHeight - 2; } }
		
		Game game;
		public WorldEnv( Game game ) {
			this.game = game;
			ResetLight();
		}

		/// <summary> Resets all of the environment properties to their defaults. </summary>
		public void Reset() {
			EdgeHeight = -1;
			CloudHeight = -1;
			EdgeBlock = Block.StillWater;
			SidesBlock = Block.Bedrock;
			CloudsSpeed = 1;
			
			ResetLight();
			SkyCol = DefaultSkyColour;
			FogCol = DefaultFogColour;
			CloudsCol = DefaultCloudsColour;
			Weather = Weather.Sunny;
		}
		
		void ResetLight() {
			Shadowlight = DefaultShadowlight;
			FastColour.GetShaded( Shadowlight, ref ShadowlightXSide,
			                     ref ShadowlightZSide, ref ShadowlightYBottom );
			Sunlight = DefaultSunlight;
			FastColour.GetShaded( Sunlight, ref SunlightXSide,
			                     ref SunlightZSide, ref SunlightYBottom );
		}
		
		/// <summary> Sets the sides block to the given block, and raises the
		/// EnvVariableChanged event with the variable 'SidesBlock'. </summary>
		public void SetSidesBlock( Block block ) {
			if( block == SidesBlock ) return;
			if( block == (Block)BlockInfo.MaxDefinedBlock ) {
				Utils.LogDebug( "Tried to set sides block to an invalid block: " + block );
				block = Block.Bedrock;
			}
			SidesBlock = block;
			game.WorldEvents.RaiseEnvVariableChanged( EnvVar.SidesBlock );
		}
		
		/// <summary> Sets the edge block to the given block, and raises the
		/// EnvVariableChanged event with the variable 'EdgeBlock'. </summary>
		public void SetEdgeBlock( Block block ) {
			if( block == EdgeBlock ) return;
			if( block == (Block)BlockInfo.MaxDefinedBlock ) {
				Utils.LogDebug( "Tried to set edge block to an invalid block: " + block );
				block = Block.StillWater;
			}
			EdgeBlock = block;
			game.WorldEvents.RaiseEnvVariableChanged( EnvVar.EdgeBlock );
		}
		
		/// <summary> Sets the height of the clouds in world space, and raises the
		/// EnvVariableChanged event with the variable 'CloudsLevel'. </summary>
		public void SetCloudsLevel( int level ) { Set( level, ref CloudHeight, EnvVar.CloudsLevel ); }
		
		/// <summary> Sets the current clouds speed, and raises the
		/// EnvVariableChanged event with the variable 'CloudsSpeed'. </summary>
		public void SetCloudsSpeed( float speed ) { Set( speed, ref CloudsSpeed, EnvVar.CloudsSpeed ); }
		
		/// <summary> Sets the height of the map edges in world space, and raises the
		/// EnvVariableChanged event with the variable 'EdgeLevel'. </summary>
		public void SetEdgeLevel( int level ) { Set( level, ref EdgeHeight, EnvVar.EdgeLevel ); }
		
		/// <summary> Sets the current sky colour, and raises the
		/// EnvVariableChanged event with the variable 'SkyColour'. </summary>
		public void SetSkyColour( FastColour col ) { Set( col, ref SkyCol, EnvVar.SkyColour ); }
		
		/// <summary> Sets the current fog colour, and raises the
		/// EnvVariableChanged event with the variable 'FogColour'. </summary>
		public void SetFogColour( FastColour col ) { Set( col, ref FogCol, EnvVar.FogColour ); }
		
		/// <summary> Sets the current clouds colour, and raises the
		/// EnvVariableChanged event with the variable 'CloudsColour'. </summary>
		public void SetCloudsColour( FastColour col ) { Set( col, ref CloudsCol, EnvVar.CloudsColour ); }
		
		/// <summary> Sets the current sunlight colour, and raises the
		/// EnvVariableChanged event with the variable 'SunlightColour'. </summary>
		public void SetSunlight( FastColour col ) {
			if( col == Sunlight ) return;
			Sunlight = col;
			Set( col, ref Sunlight, EnvVar.SunlightColour );
			FastColour.GetShaded( Sunlight, ref SunlightXSide,
			                     ref SunlightZSide, ref SunlightYBottom );
			game.WorldEvents.RaiseEnvVariableChanged( EnvVar.SunlightColour );
		}
		
		/// <summary> Sets the current shadowlight colour, and raises the
		/// EnvVariableChanged event with the variable 'ShadowlightColour'. </summary>
		public void SetShadowlight( FastColour col ) {
			if( col == Shadowlight ) return;
			Shadowlight = col;
			Set( col, ref Shadowlight, EnvVar.ShadowlightColour );
			FastColour.GetShaded( Shadowlight, ref ShadowlightXSide,
			                     ref ShadowlightZSide, ref ShadowlightYBottom );
			game.WorldEvents.RaiseEnvVariableChanged( EnvVar.ShadowlightColour );
		}
		
		/// <summary> Sets the current weather, and raises the
		/// EnvVariableChanged event with the variable 'Weather'. </summary>
		public void SetWeather( Weather weather ) {
			if( weather == Weather ) return;
			Weather = weather;
			game.WorldEvents.RaiseEnvVariableChanged( EnvVar.Weather );
		}
		
		void Set<T>( T value, ref T target, EnvVar var ) where T : IEquatable<T> {
			if( value.Equals( target ) ) return;
			target = value;
			game.WorldEvents.RaiseEnvVariableChanged( var );
		}
	}
}