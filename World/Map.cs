using System;
using System.Collections.Generic;
using ClassicalSharp.World;

namespace ClassicalSharp {

	public class Map {

		public Game Window;
		public long WorldTime;
		public long Seed;
		public byte Dimension;
		
		public static readonly FastColour DefaultSunlight = new FastColour( 255, 255, 255 );
		public static readonly FastColour DefaultShadowlight = new FastColour( 162, 162, 162 );
		public static readonly FastColour DefaultSkyColour = new FastColour( 0x99, 0xCC, 0xFF );		
		public static readonly FastColour DefaultCloudsColour =  new FastColour( 0xFF, 0xFF, 0xFF );		
		public static readonly FastColour DefaultFogColour = new FastColour( 0xFF, 0xFF, 0xFF );
		
		public FastColour SkyCol = DefaultSkyColour, FogCol = DefaultFogColour, CloudsCol = DefaultCloudsColour;
		public FastColour Sunlight, SunlightXSide, SunlightZSide, SunlightYBottom;
		public FastColour Shadowlight, ShadowlightXSide, ShadowlightZSide, ShadowlightYBottom;
		
		public Map( Game window ) {
			IsNotLoaded = true;
			Window = window;
			SetSunlight( DefaultSunlight );
			SetShadowlight( DefaultShadowlight );
		}
		
		public bool IsNotLoaded { get; set; }
		
		public const int Height = Chunk.Height;
		
		public void Reset() {
			SetShadowlight( DefaultShadowlight );
			SetSunlight( DefaultSunlight );
			SkyCol = DefaultSkyColour;
			FogCol = DefaultFogColour;
			CloudsCol = DefaultCloudsColour;
			chunks.Clear();
		}
		
		public void SetSkyColour( FastColour col ) {
			SkyCol = col;
			Window.RaiseEnvVariableChanged( EnvVariable.SkyColour );
		}
		
		public void SetFogColour( FastColour col ) {
			FogCol = col;
			Window.RaiseEnvVariableChanged( EnvVariable.FogColour );
		}
		
		public void SetCloudsColour( FastColour col ) {
			CloudsCol = col;
			Window.RaiseEnvVariableChanged( EnvVariable.CloudsColour );
		}
		
		public void SetSunlight( FastColour value ) {
			Sunlight = value;
			AdjustLight( Sunlight, ref SunlightXSide, ref SunlightZSide, ref SunlightYBottom );
			Window.RaiseEnvVariableChanged( EnvVariable.SunlightColour );
		}
		
		public void SetShadowlight( FastColour value ) {
			Shadowlight = value;
			AdjustLight( Shadowlight, ref ShadowlightXSide, ref ShadowlightZSide, ref ShadowlightYBottom );
			Window.RaiseEnvVariableChanged( EnvVariable.ShadowlightColour );
		}
		
		void AdjustLight( FastColour normal, ref FastColour xSide, ref FastColour zSide, ref FastColour yBottom ) {
			xSide = FastColour.Scale( normal, 0.6f );
			zSide = FastColour.Scale( normal, 0.8f );
			yBottom = FastColour.Scale( normal, 0.5f );
		}
		
		Dictionary<Vector2I, Chunk> chunks = new Dictionary<Vector2I, Chunk>();
		
		public void SetBlock( int x, int y, int z, byte blockId ) {
			int chunkX = x >> 4;
			int chunkZ = z >> 4;
			Chunk chunk;
			if( chunks.TryGetValue( new Vector2I( chunkX, chunkZ ), out chunk ) ) {
				int blockX = x & 0x0F;
				int blockZ = z & 0x0F;
				byte oldBlock = chunk.GetBlock( blockX, y, blockZ );
				chunk.SetBlock( blockX, y, blockZ, blockId );
			}			
		}
		
		public void SetBlock( int x, int y, int z, byte blockId, byte blockMeta ) {
			int chunkX = x >> 4;
			int chunkZ = z >> 4;
			Chunk chunk;
			if( chunks.TryGetValue( new Vector2I( chunkX, chunkZ ), out chunk ) ) {
				int blockX = x & 0x0F;
				int blockZ = z & 0x0F;
				chunk.SetBlock( blockX, y, blockZ, blockId );
				chunk.SetBlockMetadata( blockX, y, blockZ, blockMeta );
			}
		}
		
		public void LoadChunk( Chunk chunk ) {
			chunks.Add( new Vector2I( chunk.ChunkX, chunk.ChunkZ ), chunk );
			Window.MapRenderer.LoadChunk( chunk );
		}
		
		public Chunk GetChunk( int x, int z ) {
			int chunkX = x >> 4;
			int chunkZ = z >> 4;
			Chunk chunk;
			if( chunks.TryGetValue( new Vector2I( chunkX, chunkZ ), out chunk ) ) {
				return chunk;
			}
			return null;
		}
		
		public int GetLightHeight( int x, int z ) {
			return -1;
		}
		
		public void SetBlock( Vector3I p, byte blockId ) {
			SetBlock( p.X, p.Y, p.Z, blockId );
		}
		
		public byte GetBlock( int x, int y, int z ) {
			int chunkX = x >> 4;
			int chunkZ = z >> 4;
			Chunk chunk;
			if( chunks.TryGetValue( new Vector2I( chunkX, chunkZ ), out chunk ) ) {
				int blockX = x & 0x0F;
				int blockZ = z & 0x0F;
				return chunk.GetBlock( blockX, y, blockZ );
			}
			return 0;
		}
		
		public byte GetBlock( Vector3I p ) {
			return GetBlock( p.X, p.Y, p.Z );
		}
		
		public bool IsValidPos( int x, int y, int z ) {
			int chunkX = x >> 4;
			int chunkZ = z >> 4;
			return y >= 0 && y < Height &&
				chunks.ContainsKey( new Vector2I( chunkX, chunkZ ) );
		}
		
		public bool IsValidPos( Vector3I p ) {
			return IsValidPos( p.X, p.Y, p.Z );
		}
		
		public void UnloadChunk( int chunkX, int chunkZ ) {
			if( chunks.Remove( new Vector2I( chunkX, chunkZ ) ) ) {
				Window.MapRenderer.UnloadChunk( chunkX, chunkZ );
			}
		}
	}
}