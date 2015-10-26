using System;

namespace ClassicalSharp {

	/// <summary> Represents a fixed size map of blocks. Stores the raw block data,
	/// heightmap, dimensions and various metadata such as environment settings. </summary>
	public sealed partial class Map {

		Game game;
		BlockInfo info;
		internal byte[] mapData;
		public int Width, Height, Length;
		internal short[] heightmap;
		int maxY, oneY;
		
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
		
		/// <summary> Colour applied to blocks located in direct sunlight. </summary>
		public FastColour Sunlight;
		public FastColour SunlightXSide, SunlightZSide, SunlightYBottom;
		public static readonly FastColour DefaultSunlight = new FastColour( 255, 255, 255 );
		
		/// <summary> Colour applied to blocks located in shadow / hidden from direct sunlight. </summary>
		public FastColour Shadowlight;
		public FastColour ShadowlightXSide, ShadowlightZSide, ShadowlightYBottom;
		public static readonly FastColour DefaultShadowlight = new FastColour( 162, 162, 162 );
		
		/// <summary> Current weather for this particular map. </summary>
		public Weather Weather = Weather.Sunny;
		
		/// <summary> Unique uuid/guid of this particular map. </summary>
		public Guid Uuid;
		
		/// <summary> Block that surrounds map and is perpendicular to the Y plane. (default water) </summary>
		public Block EdgeBlock = Block.StillWater;
		
		/// <summary> Height of the map edge in world space. </summary>
		public int EdgeHeight;
		
		/// <summary> Block that surrounds the map that is below the map, fills part of the vertical sides,
		/// and also perpendicular to the Y plane. (default bedrock) </summary>
		public Block SidesBlock = Block.Bedrock;
		
		/// <summary> Maximum height of the various parts of the map sides, in world space. </summary>
		public int SidesHeight {
			get { return EdgeHeight - 2; }
		}
		
		/// <summary> Whether this map is empty. </summary>
		public bool IsNotLoaded {
			get { return Width == 0 && Height == 0 && Length == 0; }
		}
		
		public Map( Game game ) {
			this.game = game;
			info = game.BlockInfo;
			ResetLight();
		}

		/// <summary> Resets all of the properties to their defaults and raises the 'OnNewMap' event. </summary>
		public void Reset() {
			EdgeHeight = -1;
			CloudHeight = -1;
			Width = Height = Length = 0;
			Uuid = Guid.NewGuid();
			EdgeBlock = Block.StillWater;
			SidesBlock = Block.Bedrock;
			
			ResetLight();
			SkyCol = DefaultSkyColour;
			FogCol = DefaultFogColour;
			CloudsCol = DefaultCloudsColour;
			Weather = Weather.Sunny;
			game.Events.RaiseOnNewMap();
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
			game.Events.RaiseEnvVariableChanged( EnvVar.SidesBlock );
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
			game.Events.RaiseEnvVariableChanged( EnvVar.EdgeBlock );
		}
		
		/// <summary> Sets the height of the clouds in world space, and raises the
		/// EnvVariableChanged event with the variable 'CloudsLevel'. </summary>
		public void SetCloudsLevel( int level ) { Set( level, ref CloudHeight, EnvVar.CloudsLevel ); }
		
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
			Set( col, ref Sunlight, EnvVar.SunlightColour );
			FastColour.GetShaded( Sunlight, ref SunlightXSide,
			                     ref SunlightZSide, ref SunlightYBottom );
		}
		
		/// <summary> Sets the current shadowlight colour, and raises the
		/// EnvVariableChanged event with the variable 'ShadowlightColour'. </summary>
		public void SetShadowlight( FastColour col ) {
			Set( col, ref Shadowlight, EnvVar.ShadowlightColour );
			FastColour.GetShaded( Shadowlight, ref ShadowlightXSide,
			                     ref ShadowlightZSide, ref ShadowlightYBottom );
		}
		
		/// <summary> Sets the current weather, and raises the
		/// EnvVariableChanged event with the variable 'Weather'. </summary>
		public void SetWeather( Weather weather ) {
			if( weather == Weather ) return;
			Weather = weather;
			game.Events.RaiseEnvVariableChanged( EnvVar.Weather );
		}
		
		void Set<T>( T value, ref T target, EnvVar var ) where T : IEquatable<T> {
			if( value.Equals( target ) ) return;
			target = value;
			game.Events.RaiseEnvVariableChanged( var );
		}
		
		/// <summary> Updates the underlying block array, heightmap, and dimensions of this map. </summary>
		public void SetData( byte[] blocks, int width, int height, int length ) {
			mapData = blocks;
			this.Width = width;
			this.Height = height;
			this.Length = length;
			if( EdgeHeight == -1 ) EdgeHeight = height / 2;
			maxY = height - 1;
			oneY = length * width;
			if( CloudHeight == -1 ) CloudHeight = height + 2;
			
			heightmap = new short[width * length];
			for( int i = 0; i < heightmap.Length; i++ ) {
				heightmap[i] = short.MaxValue;
			}
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking,
		/// and also recalculates the heightmap for the given (x,z) column.	</summary>
		public void SetBlock( int x, int y, int z, byte blockId ) {
			int index = (y * Length + z) * Width + x;
			byte oldBlock = mapData[index];
			mapData[index] = blockId;
			UpdateHeight( x, y, z, oldBlock, blockId );
			game.WeatherRenderer.UpdateHeight( x, y, z, oldBlock, blockId );
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking,
		/// and also recalculates the heightmap for the given (x,z) column.	</summary>
		public void SetBlock( Vector3I p, byte blockId ) {
			SetBlock( p.X, p.Y, p.Z, blockId );
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public byte GetBlock( int x, int y, int z ) {
			return mapData[(y * Length + z) * Width + x];
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public byte GetBlock( Vector3I p ) {
			return mapData[(p.Y * Length + p.Z) * Width + p.X];
		}
		
		/// <summary> Returns the block at the given world coordinates withbounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public byte SafeGetBlock( int x, int y, int z ) {
			return IsValidPos( x, y, z ) ?
				mapData[(y * Length + z) * Width + x] : (byte)0;
		}
		
		/// <summary> Returns the block at the given world coordinates withbounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public byte SafeGetBlock( Vector3I p ) {
			return IsValidPos( p.X, p.Y, p.Z ) ?
				mapData[(p.Y * Length + p.Z) * Width + p.X] : (byte)0;
		}
		
		/// <summary> Returns whether the given world coordinates are contained
		/// within the dimensions of the map. </summary>
		public bool IsValidPos( int x, int y, int z ) {
			return x >= 0 && y >= 0 && z >= 0 &&
				x < Width && y < Height && z < Length;
		}
		
		/// <summary> Returns whether the given world coordinates are contained
		/// within the dimensions of the map. </summary>
		public bool IsValidPos( Vector3I p ) {
			return p.X >= 0 && p.Y >= 0 && p.Z >= 0 &&
				p.X < Width && p.Y < Height && p.Z < Length;
		}
		
		/// <summary> Returns whether the given world coordinates are fully not in sunlight. </summary>
		public bool IsLit( int x, int y, int z ) {
			if( !IsValidPos( x, y, z ) ) return true;
			return y > GetLightHeight( x, z );
		}
		
		/// <summary> Returns whether the given world coordinatse are fully not in sunlight. </summary>
		public bool IsLit( Vector3I p ) {
			if( !IsValidPos( p.X, p.Y, p.Z ) ) return true;
			return p.Y > GetLightHeight( p.X, p.Z );
		}
		
		/// <summary> Returns the y coordinate of the highest block that is fully not in sunlight. </summary>
		/// <remarks> e.g. if cobblestone was at y = 5, this method would return 4. </remarks>
		public int GetLightHeight( int x, int z ) {
			int index = ( z * Width ) + x;
			int height = heightmap[index];
			return height == short.MaxValue ? CalcHeightAt( x, maxY, z, index ) : height;
		}
	}
}