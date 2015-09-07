using System;

namespace ClassicalSharp {

	public class Map {

		Game game;
		BlockInfo info;
		internal byte[] mapData;
		public int Width, Height, Length;
		internal short[] heightmap;
		int maxY;
		int oneY;
		
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
		
		public Map( Game game ) {
			this.game = game;
			info = game.BlockInfo;
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
			
			game.RaiseOnNewMap();
			game.SelectionManager.Dispose();
		}
		
		public void SetSidesBlock( Block block ) {
			if( block == SidesBlock ) return;
			if( block == (Block)BlockInfo.MaxDefinedBlock ) {
				Utils.LogWarning( "Tried to set sides block to an invalid block: " + block );
				block = Block.Bedrock;
			}
			SidesBlock = block;
			game.RaiseEnvVariableChanged( EnvVariable.SidesBlock );
		}
		
		public void SetEdgeBlock( Block block ) {
			if( block == EdgeBlock ) return;
			if( block == (Block)BlockInfo.MaxDefinedBlock ) {
				Utils.LogWarning( "Tried to set edge block to an invalid block: " + block );
				block = Block.StillWater;
			}
			EdgeBlock = block;
			game.RaiseEnvVariableChanged( EnvVariable.EdgeBlock );
		}
		
		public void SetWaterLevel( int level ) {
			if( level == WaterHeight ) return;
			WaterHeight = level;
			game.RaiseEnvVariableChanged( EnvVariable.WaterLevel );
		}
		
		public void SetWeather( Weather weather ) {
			if( weather == Weather ) return;
			Weather = weather;
			game.RaiseEnvVariableChanged( EnvVariable.Weather );
		}
		
		public void SetSkyColour( FastColour col ) {
			if( col == SkyCol ) return;
			SkyCol = col;
			game.RaiseEnvVariableChanged( EnvVariable.SkyColour );
		}
		
		public void SetFogColour( FastColour col ) {
			if( col == FogCol ) return;
			FogCol = col;
			game.RaiseEnvVariableChanged( EnvVariable.FogColour );
		}
		
		public void SetCloudsColour( FastColour col ) {
			if( col == CloudsCol ) return;
			CloudsCol = col;
			game.RaiseEnvVariableChanged( EnvVariable.CloudsColour );
		}
		
		public void SetSunlight( FastColour col ) {
			if( col == Sunlight ) return;
			Sunlight = col;
			AdjustLight( Sunlight, ref SunlightXSide, ref SunlightZSide, ref SunlightYBottom );
			game.RaiseEnvVariableChanged( EnvVariable.SunlightColour );
		}
		
		public void SetShadowlight( FastColour col ) {
			if( col == Shadowlight ) return;
			Shadowlight = col;
			AdjustLight( Shadowlight, ref ShadowlightXSide, ref ShadowlightZSide, ref ShadowlightYBottom );
			game.RaiseEnvVariableChanged( EnvVariable.ShadowlightColour );
		}
		
		public int GetLightHeight( int x, int z ) {
			int index = ( z * Width ) + x;
			int height = heightmap[index];
			return height == short.MaxValue ? CalcHeightAt( x, maxY, z, index ) : height;
		}
		
		int CalcHeightAt( int x, int maxY, int z, int index ) {
			int mapIndex = ( maxY * Length + z ) * Width + x;
			for( int y = maxY; y >= 0; y-- ) {
				byte block = mapData[mapIndex];
				if( info.BlocksLight( block ) ) {
					heightmap[index] = (short)( y - 1 );
					return y;
				}
				mapIndex -= oneY;
			}
			
			heightmap[index] = -10;
			return -10;
		}
		
		void UpdateHeight( int x, int y, int z, byte oldBlock, byte newBlock ) {
			bool didBlock = info.BlocksLight( oldBlock );
			bool nowBlocks = info.BlocksLight( newBlock );
			if( didBlock == nowBlocks ) return;
			
			int index = ( z * Width ) + x;
			int height = heightmap[index];
			if( height == short.MaxValue ) {
				// We have to calculate the entire column for visibility, because the old/new block info is
				// useless if there is another block higher than block.y that stops rain.
				CalcHeightAt( x, maxY, z, index );
			} else if( y > height ) {
				if( nowBlocks ) {
					heightmap[index] = (short)( y - 1 );
				} else {
					// Part of the column is now visible to light, we don't know how exactly how high it should be though.
					// However, we know that if the old block was above or equal to light height, then the new light height must be <= old block.y
					CalcHeightAt( x, y, z, index );
				}
			}
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
			
			heightmap = new short[width * length];
			for( int i = 0; i < heightmap.Length; i++ ) {
				heightmap[i] = short.MaxValue;
			}
		}
		
		public bool IsLit( int x, int y, int z ) {
			if( !IsValidPos( x, y, z ) ) return true;
			return y > heightmap[z * Width + x];
		}
		
		public bool IsLit( Vector3I p ) {
			if( !IsValidPos( p.X, p.Y, p.Z ) ) return true;
			return p.Y > heightmap[p.Z * Width + p.X];
		}
		
		public void SetBlock( int x, int y, int z, byte blockId ) {
			int index = ( y * Length + z ) * Width + x;
			byte oldBlock = mapData[index];
			mapData[index] = blockId;
			UpdateHeight( x, y, z, oldBlock, blockId );
			game.WeatherRenderer.UpdateHeight( x, y, z, oldBlock, blockId );
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
		
		// Equivalent to
		// for x = startX; x < startX + 18; x++
		//    for z = startZ; z < startZ + 18; z++
		//       CalcHeightAt(x, maxY, z) if height == short.MaxValue
		// Except this function is a lot more optimised and minimises cache misses.
		internal unsafe void HeightmapHint( int startX, int startZ, byte* mapPtr ) {
			int x1 = Math.Max( startX, 0 ), x2 = Math.Min( Width, startX + 18 );
			int z1 = Math.Max( startZ, 0 ), z2 = Math.Min( Length, startZ + 18 );
			int xCount = x2 - x1, zCount = z2 - z1;
			int* skip = stackalloc int[xCount * zCount];
			
			int elemsLeft = InitialHeightmapCoverage( x1, z1, xCount, zCount, skip );
			if( !CalculateHeightmapCoverage( x1, z1, xCount, zCount, elemsLeft, skip, mapPtr ) ) {
				FinishHeightmapCoverage( x1, z1, xCount, zCount, skip );
			}
		}
		
		unsafe int InitialHeightmapCoverage( int x1, int z1, int xCount, int zCount, int* skip ) {
			int elemsLeft = 0, index = 0, curRunCount = 0;
			for( int z = 0; z < zCount; z++ ) {
				int heightmapIndex = ( z1 + z ) * Width + x1;
				for( int x = 0; x < xCount; x++ ) {
					int height = heightmap[heightmapIndex++];
					skip[index] = 0;
					if( height == short.MaxValue ) {
						elemsLeft++;
						curRunCount = 0;
					} else {
						skip[index - curRunCount]++;
						curRunCount++;
					}
					index++;
				}
				curRunCount = 0; // We can only skip an entire X row at most.
			}
			return elemsLeft;
		}
		
		unsafe bool CalculateHeightmapCoverage( int x1, int z1, int xCount, int zCount, int elemsLeft, int* skip, byte* mapPtr ) {
			int prevRunCount = 0;
			for( int y = maxY; y >= 0; y-- ) {
				if( elemsLeft <= 0 ) return true;
				int mapIndex = ( y * Length + z1 ) * Width + x1;
				int heightmapIndex = z1 * Width + x1;
				
				for( int z = 0; z < zCount; z++ ) {
					int baseIndex = mapIndex;
					int index = z * xCount;
					for( int x = 0; x < xCount; ) {
						int curRunCount = skip[index];
						x += curRunCount; mapIndex += curRunCount; index += curRunCount;
						
						if( x < xCount && info.blocksLight[mapPtr[mapIndex]] ) {
							heightmap[heightmapIndex + x] = (short)( y - 1 );
							elemsLeft--;
							skip[index] = 0;
							int offset = prevRunCount + curRunCount;
							int newRunCount = skip[index - offset] + 1;
							
							// consider case 1 0 1 0, where we are at 0
							// we need to make this 3 0 0 0 and advance by 1
							int oldRunCount = ( x - offset + newRunCount ) < xCount ? skip[index - offset + newRunCount] : 0;
							if( oldRunCount != 0 ) {
								skip[index - offset + newRunCount] = 0;
								newRunCount += oldRunCount;
							}				
							skip[index - offset] = newRunCount;
							x += oldRunCount; index += oldRunCount; mapIndex += oldRunCount;
							prevRunCount = newRunCount;
						} else {
							prevRunCount = 0;
						}
						x++; mapIndex++; index++;
					}
					prevRunCount = 0;
					heightmapIndex += Width;
					mapIndex = baseIndex + Width; // advance one Z
				}
			}
			return false;
		}
		
		unsafe void FinishHeightmapCoverage( int x1, int z1, int xCount, int zCount, int* skip ) {
			for( int z = 0; z < zCount; z++ ) {
				int heightmapIndex = ( z1 + z ) * Width + x1;
				for( int x = 0; x < xCount; x++ ) {
					int height = heightmap[heightmapIndex];
					if( height == short.MaxValue )
						heightmap[heightmapIndex] = -10;
					heightmapIndex++;
				}
			}
		}
	}
}