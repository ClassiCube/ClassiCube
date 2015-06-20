using System;

namespace ClassicalSharp {

	public class Map {

		public Game Window;
		BlockInfo info;
		public byte[] mapData;
		public int Width, Height, Length;	
		short[] lightmap, solidmap;
		int maxY, oneY;
		
		public static readonly FastColour DefaultSunlight = new FastColour( 255, 255, 255 );
		public static readonly FastColour DefaultShadowlight = new FastColour( 162, 162, 162 );
		public static readonly FastColour DefaultSkyColour = new FastColour( 0x99, 0xCC, 0xFF );		
		public static readonly FastColour DefaultCloudsColour =  new FastColour( 0xFF, 0xFF, 0xFF );		
		public static readonly FastColour DefaultFogColour = new FastColour( 0xFF, 0xFF, 0xFF );
		
		public Block SidesBlock = Block.Bedrock, EdgeBlock = Block.StillWater;
		public int WaterHeight;
		public FastColour SkyCol = DefaultSkyColour, FogCol = DefaultFogColour, CloudsCol = DefaultCloudsColour;
		public FastColour Sunlight, SunlightXSide, SunlightZSide, SunlightYBottom;
		public FastColour Shadowlight, ShadowlightXSide, ShadowlightZSide, ShadowlightYBottom;
		public Weather Weather = Weather.Sunny;
		
		public int GroundHeight {
			get { return WaterHeight - 2; }
		}
		
		public Map( Game window ) {
			Window = window;
			info = window.BlockInfo;
			SetSunlight( DefaultSunlight );
			SetShadowlight( DefaultShadowlight );
		}
		
		public bool IsNotLoaded {
			get { return Width == 0 && Height == 0 && Length == 0; }
		}
		
		public void Reset() {
			WaterHeight = -1;
			SetShadowlight( DefaultShadowlight );
			SetSunlight( DefaultSunlight );
			Width = Height = Length = 0;
			EdgeBlock = Block.StillWater;
			SidesBlock = Block.Bedrock;
			SkyCol = DefaultSkyColour;
			FogCol = DefaultFogColour;
			CloudsCol = DefaultCloudsColour;
			Weather = Weather.Sunny;
		}
		
		public void SetSidesBlock( Block block ) {
			SidesBlock = block;
			Window.RaiseEnvVariableChanged( EnvVariable.SidesBlock );
		}
		
		public void SetEdgeBlock( Block block ) {
			EdgeBlock = block;
			Window.RaiseEnvVariableChanged( EnvVariable.EdgeBlock );
		}
		
		public void SetWaterLevel( int level ) {
			WaterHeight = level;
			Window.RaiseEnvVariableChanged( EnvVariable.WaterLevel );
		}
		
		public void SetWeather( Weather weather ) {
			Weather = weather;
			Window.RaiseEnvVariableChanged( EnvVariable.Weather );
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
		
		public void UseRawMap( byte[] map, int width, int height, int length ) {
			mapData = map;
			this.Width = width;
			this.Height = height;
			this.Length = length;
			if( WaterHeight == -1 ) WaterHeight = height / 2;
			maxY = height - 1;
			oneY = length * width;
			
			lightmap = new short[width * length];
			for( int i = 0; i < lightmap.Length; i++ ) {
				lightmap[i] = short.MaxValue;
			}
			solidmap = new short[width * length];
			for( int i = 0; i < solidmap.Length; i++ ) {
				solidmap[i] = short.MaxValue;
			}
		}
		
		public void SetBlock( int x, int y, int z, byte blockId ) {
			int index = ( y * Length + z ) * Width + x;
			byte oldBlock = mapData[index];
			mapData[index] = blockId;
			UpdateLightHeight( x, y, z, oldBlock, blockId );
			UpdateSolidHeight( x, y, z, oldBlock, blockId );
		}
		
		public void SetBlock( Vector3I p, byte blockId ) {
			SetBlock( p.X, p.Y, p.Z, blockId );
		}
		
		public byte GetBlock( int x, int y, int z ) {
			return mapData[( y * Length + z ) * Width + x];
		}
		
		public byte GetBlock( Vector3I p ) {
			return mapData[( p.Y * Length + p.Z ) * Width + p.X];
		}
		
		public bool IsValidPos( int x, int y, int z ) {
			return x >= 0 && y >= 0 && z >= 0 &&
				x < Width && y < Height && z < Length;
		}
		
		public bool IsValidPos( Vector3I p ) {
			return p.X >= 0 && p.Y >= 0 && p.Z >= 0 &&
				p.X < Width && p.Y < Height && p.Z < Length;
		}
		
		#region Heightmap for rain/snow
		
		public int GetSolidHeight( int x, int z ) {
			if( x < 0 || z < 0 || x >= Width || z >= Length ) return WaterHeight - 1;
			int index = ( x * Length ) + z;
			int height = lightmap[index];			
			return height == short.MaxValue ? CalcLightHeight( x, maxY, z, index ) : height;
		}
		
		int CalcSolidHeight( int x, int maxY, int z, int index ) {
			int mapIndex = ( maxY * Length + z ) * Width + x;
			for( int y = maxY; y >= 0; y-- ) {
				if( IsSolid( mapData[mapIndex] ) ) {
					lightmap[index] = (short)y;
					return y;
				}
				mapIndex -= oneY;
			}		
			lightmap[index] = -1;
			return -1;
		}
		
		bool IsSolid( byte block ) {	
			return !( block == 0 || info.IsSprite( block ) || info.IsLiquid( block ) );
		}
		
		void UpdateSolidHeight( int x, int y, int z, byte oldBlock, byte newBlock ) {
			bool didBlock = IsSolid( oldBlock );
			bool nowBlocks = IsSolid( newBlock );
			if( didBlock == nowBlocks ) return;
			
			int index = ( x * Length ) + z;
			if( nowBlocks ) {
				if( y > GetSolidHeight( x, z ) ) {
					solidmap[index] = (short)y;
				}
			} else {
				if( y >= GetSolidHeight( x, z ) ) {
					CalcSolidHeight( x, y, z, index );
				}
			}
		}
		
		#endregion
		
		
		#region Heightmap for light
		
		public int GetLightHeight( int x,  int z ) {
			int index = ( x * Length ) + z;
			int height = lightmap[index];			
			return height == short.MaxValue ? CalcLightHeight( x, maxY, z, index ) : height;
		}
		
		int CalcLightHeight( int x, int maxY, int z, int index ) {
			int mapIndex = ( maxY * Length + z ) * Width + x;
			for( int y = maxY; y >= 0; y-- ) {
				byte block = mapData[mapIndex];
				if( info.BlocksLight( block ) ) {
					lightmap[index] = (short)y;
					return y;
				}
				mapIndex -= oneY;
			}
			
			lightmap[index] = -1;
			return -1;
		}
		
		void UpdateLightHeight( int x, int y, int z, byte oldBlock, byte newBlock ) {
			bool didBlock = info.BlocksLight( oldBlock );
			bool nowBlocks = info.BlocksLight( newBlock );
			if( didBlock == nowBlocks ) return;
			
			int index = ( x * Length ) + z;
			if( nowBlocks ) {
				if( y > GetLightHeight( x, z ) ) {
					lightmap[index] = (short)y;
				}
			} else {
				if( y >= GetLightHeight( x, z ) ) {
					CalcLightHeight( x, y, z, index );
				}
			}
		}
		
		#endregion
	}
}