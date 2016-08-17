// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Events;
using ClassicalSharp.Renderers;
using OpenTK;

namespace ClassicalSharp.Map {
	
	/// <summary> Represents a fixed size map of blocks. Stores the raw block data,
	/// heightmap, dimensions and various metadata such as environment settings. </summary>
	public sealed partial class World {

		Game game;
		BlockInfo info;
		internal byte[] blocks;
		public int Width, Height, Length;
		internal short[] heightmap;
		int maxY, oneY;
		
		/// <summary> Contains the environment metadata for this world. </summary>
		public WorldEnv Env;
		
		/// <summary> Unique uuid/guid of this particular world. </summary>
		public Guid Uuid;
		
		/// <summary> Whether this map is empty. </summary>
		public bool IsNotLoaded = true;
		
		/// <summary> Current terrain.png or texture pack url of this map. </summary>
		public string TextureUrl = null;
		
		public World( Game game ) {
			Env = new WorldEnv( game );
			this.game = game;
			info = game.BlockInfo;
		}

		/// <summary> Resets all of the properties to their defaults and raises the 'OnNewMap' event. </summary>
		public void Reset() {
			Env.Reset();
			Width = Height = Length = 0;
			IsNotLoaded = true;
			
			Uuid = Guid.NewGuid();
			game.WorldEvents.RaiseOnNewMap();
		}	
		
		/// <summary> Updates the underlying block array, heightmap, and dimensions of this map. </summary>
		public void SetNewMap( byte[] blocks, int width, int height, int length ) {
			this.blocks = blocks;
			this.Width = width;
			this.Height = height;
			this.Length = length;
			IsNotLoaded = width == 0 || length == 0 || height == 0;
			
			if( Env.EdgeHeight == -1 ) Env.EdgeHeight = height / 2;
			maxY = height - 1;
			oneY = length * width;
			if( Env.CloudHeight == -1 ) Env.CloudHeight = height + 2;
			
			heightmap = new short[width * length];
			for( int i = 0; i < heightmap.Length; i++ )
				heightmap[i] = short.MaxValue;
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking,
		/// and also recalculates the heightmap for the given (x,z) column.	</summary>
		public void SetBlock( int x, int y, int z, byte blockId ) {
			int index = (y * Length + z) * Width + x;
			byte oldBlock = blocks[index];
			blocks[index] = blockId;
			UpdateHeight( x, y, z, oldBlock, blockId );
			
			WeatherRenderer weather = game.WeatherRenderer;
			if( weather.heightmap != null && !IsNotLoaded )
				weather.UpdateHeight( x, y, z, oldBlock, blockId );
		}
		
		/// <summary> Sets the block at the given world coordinates without bounds checking,
		/// and also recalculates the heightmap for the given (x,z) column.	</summary>
		public void SetBlock( Vector3I p, byte blockId ) {
			SetBlock( p.X, p.Y, p.Z, blockId );
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public byte GetBlock( int x, int y, int z ) {
			return blocks[(y * Length + z) * Width + x];
		}
		
		/// <summary> Returns the block at the given world coordinates without bounds checking. </summary>
		public byte GetBlock( Vector3I p ) {
			return blocks[(p.Y * Length + p.Z) * Width + p.X];
		}
		
		/// <summary> Returns the block at the given world coordinates with bounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public byte SafeGetBlock( int x, int y, int z ) {
			return IsValidPos( x, y, z ) ?
				blocks[(y * Length + z) * Width + x] : Block.Air;
		}
		
		/// <summary> Returns the block at the given world coordinates with bounds checking,
		/// returning 0 is the coordinates were outside the map. </summary>
		public byte SafeGetBlock( Vector3I p ) {
			return IsValidPos( p.X, p.Y, p.Z ) ?
				blocks[(p.Y * Length + p.Z) * Width + p.X] : Block.Air;
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
		
		/// <summary> Returns whether the given world coordinatse are fully not in sunlight. </summary>
		public bool IsLit( Vector3 p ) {
			int x = Utils.Floor( p.X ), y = Utils.Floor( p.Y ), z = Utils.Floor( p.Z );
			if( !IsValidPos( x, y, z ) ) return true;
			return y > GetLightHeight( x, z );
		}
		
		/// <summary> Returns the y coordinate of the highest block that is fully not in sunlight. </summary>
		/// <remarks> e.g. if cobblestone was at y = 5, this method would return 4. </remarks>
		public int GetLightHeight( int x, int z ) {
			int index = ( z * Width ) + x;
			int height = heightmap[index];
			return height == short.MaxValue ? CalcHeightAt( x, maxY, z, index ) : height;
		}
		
		/// <summary> Unpacks the given index into the map's block array into its original world coordinates. </summary>
		public Vector3I GetCoords( int index ) {
			if( index < 0 || index >= blocks.Length )
				return new Vector3I( -1 );
			int x = index % Width;
			int y = index / oneY; // index / (width * length)
			int z = (index / Width) % Length;
			return new Vector3I( x, y, z );
		}
	}
}