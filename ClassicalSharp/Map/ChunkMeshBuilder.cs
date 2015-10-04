using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {
		
		int X, Y, Z;
		byte tile;
		BlockInfo info;
		Map map;
		Game game;
		IGraphicsApi graphics;
		const int chunkSize = 16, extChunkSize = 18;
		const int chunkSize2 = 16 * 16, extChunkSize2 = 18 * 18;
		const int chunkSize3 = 16 * 16 * 16, extChunkSize3 = 18 * 18 * 18;
		
		public ChunkMeshBuilder( Game game ) {
			this.game = game;
			graphics = game.Graphics;
			info = game.BlockInfo;
			game.Events.TerrainAtlasChanged += TerrainAtlasChanged;
		}
		
		internal int width, length, height, clipLevel;
		int maxX, maxY, maxZ;
		byte[] counts = new byte[chunkSize3 * TileSide.Sides];
		byte[] chunk = new byte[extChunkSize3];
		
		bool BuildChunk( int x1, int y1, int z1 ) {
			PreStretchTiles( x1, y1, z1 );
			if( ReadChunkData( x1, y1, z1 ) ) return false;

			Stretch( x1, y1, z1 );
			PostStretchTiles( x1, y1, z1 );
			
			int xMax = Math.Min( width, x1 + chunkSize );
			int yMax = Math.Min( height, y1 + chunkSize );
			int zMax = Math.Min( length, z1 + chunkSize );
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = ( yy + 1 ) * extChunkSize2 + ( zz + 1 ) * extChunkSize + ( 0 + 1 ) - 1;
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						chunkIndex++;
						tile = chunk[chunkIndex];
						if( tile != 0 ) {
							RenderTile( chunkIndex, xx, yy, zz, x, y, z );
						}
					}
				}
			}
			return true;
		}
		
		unsafe bool ReadChunkData( int x1, int y1, int z1 ) {
			bool allAir = true, allSolid = true;
			fixed( byte* chunkPtr = chunk, mapPtr = map.mapData ) {
				
				int* chunkIntPtr = (int*)chunkPtr;
				for( int i = 0; i < extChunkSize3 / sizeof( int ); i++ ) {
					*chunkIntPtr++ = 0;
				}
				
				for( int yy = -1; yy < 17; yy++ ) {
					int y = yy + y1;
					if( y < 0 ) continue;
					if( y > maxY ) break;
					for( int zz = -1; zz < 17; zz++ ) {
						int z = zz + z1;
						if( z < 0 ) continue;
						if( z > maxZ ) break;
						
						int index = ( y * length + z ) * width + ( x1 - 1 - 1 );
						int chunkIndex = ( yy + 1 ) * extChunkSize2 + ( zz + 1 ) * extChunkSize + ( -1 + 1 ) - 1;
						
						for( int xx = -1; xx < 17; xx++ ) {
							int x = xx + x1;
							index++;
							chunkIndex++;
							if( x < 0 ) continue;
							if( x > maxX ) break;
							
							byte block = mapPtr[index];
							if( block == 9 ) block = 8; // Still water --> Water
							if( block == 11 ) block = 10; // Still lava --> Lava
							
							if( allAir && block != 0 ) allAir = false;
							if( allSolid && !info.IsOpaque[block] ) allSolid = false;
							chunkPtr[chunkIndex] = block;
						}
					}
				}
				
				if( x1 == 0 || y1 == 0 || z1 == 0 || x1 + chunkSize >= width || 
				   y1 + chunkSize >= height || z1 + chunkSize >= length ) allSolid = false;
				if( !( allAir || allSolid ) ) {
					map.HeightmapHint( x1 - 1, z1 - 1, mapPtr );
				}
			}
			return allAir || allSolid;
		}
		
		public void GetDrawInfo( int x, int y, int z, ref ChunkPartInfo[] normalParts, 
		                        ref ChunkPartInfo[] translucentParts, ref byte occlusionFlags ) {
			if( !BuildChunk( x, y, z ) )
				return;
						
			for( int i = 0; i < arraysCount; i++ ) {
				SetPartInfo( drawInfoNormal[i], i, ref normalParts );
				SetPartInfo( drawInfoTranslucent[i], i, ref translucentParts );
			}
			if( normalParts != null || translucentParts != null )
				occlusionFlags = 0;//(byte)ComputeOcclusion();
		}

		public void RenderTile( int chunkIndex, int xx, int yy, int zz, int x, int y, int z ) {
			X = x; Y = y; Z = z;
			int index = ( ( yy << 8 ) + ( zz << 4 ) + xx ) * TileSide.Sides;
			
			if( info.IsSprite[tile] ) {
				int count = counts[index + TileSide.Top];
				if( count != 0 ) {
					blockHeight = info.Height[tile];
					DrawSprite( count );
				}
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if( leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			   backCount == 0 && bottomCount == 0 && topCount == 0 ) return;
			
			emitsLight = info.EmitsLight[tile];
			blockHeight = info.Height[tile];
			isTranslucent = info.IsTranslucent[tile];
			
			if( leftCount != 0 )
				DrawLeftFace( leftCount );
			if( rightCount != 0 )
				DrawRightFace( rightCount );
			if( frontCount != 0 )
				DrawFrontFace( frontCount );
			if( backCount != 0 )
				DrawBackFace( backCount );
			if( bottomCount != 0 )
				DrawBottomFace( bottomCount );
			if( topCount != 0 )
				DrawTopFace( topCount );
		}
		
		void Stretch( int x1, int y1, int z1 ) {
			for( int i = 0; i < counts.Length; i++ ) {
				counts[i] = 1;
			}
			
			int xMax = Math.Min( width, x1 + chunkSize );
			int yMax = Math.Min( height, y1 + chunkSize );
			int zMax = Math.Min( length, z1 + chunkSize );
			
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = (yy + 1) * extChunkSize2 + (zz + 1) * extChunkSize + (-1 + 1);
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						chunkIndex++;
						byte tile = chunk[chunkIndex];
						if( tile == 0 ) continue;
						int countIndex = ((yy << 8) + (zz << 4) + xx) * TileSide.Sides;
						
						// Sprites only use one face to indicate stretching count, so we can take a shortcut here.
						// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
						if( info.IsSprite[tile] ) {
							countIndex += TileSide.Top;
							if( counts[countIndex] != 0 ) {
								X = x; Y = y; Z = z;
								AddSpriteVertices( tile, 1 );
								counts[countIndex] = 1;
							}
						} else {
							X = x; Y = y; Z = z;
							emitsLight = info.EmitsLight[tile];
							TestAndStretchZ( zz, countIndex, tile, chunkIndex, x, 0, TileSide.Left, -1 );
							TestAndStretchZ( zz, countIndex, tile, chunkIndex, x, maxX, TileSide.Right, 1 );
							TestAndStretchX( xx, countIndex, tile, chunkIndex, z, 0, TileSide.Front, -extChunkSize );
							TestAndStretchX( xx, countIndex, tile, chunkIndex, z, maxZ, TileSide.Back, extChunkSize );
							
							if( y > 0 )
								TestAndStretchX( xx, countIndex, tile, chunkIndex, y, 0, TileSide.Bottom, -extChunkSize2 );
							else
								counts[countIndex + TileSide.Bottom] = 0;
							TestAndStretchX( xx, countIndex, tile, chunkIndex, y, maxY + 2, TileSide.Top, extChunkSize2 );
						}
					}
				}
			}
		}
		
		void TestAndStretchX( int xx, int index, byte tile, int chunkIndex, int value, int test, int tileSide, int offset ) {
			index += tileSide;
			if( counts[index] != 0 ) {
				if( (value == test && Y < clipLevel) || 
				   (value != test && info.IsFaceHidden( tile, chunk[chunkIndex + offset], tileSide )) ) {
					counts[index] = 0;
				} else {
					int count = StretchX( xx, index, X, Y, Z, chunkIndex, tile, tileSide );
					AddVertices( tile, count, tileSide );
					counts[index] = (byte)count;
				}
			}
		}
		
		void TestAndStretchZ( int zz, int index, byte tile, int chunkIndex, int value, int test, int tileSide, int offset ) {
			index += tileSide;
			if( counts[index] != 0 ) {
				if( (value == test && Y < clipLevel) || 
				   (value != test && info.IsFaceHidden( tile, chunk[chunkIndex + offset], tileSide )) ) {
					counts[index] = 0;
				} else {
					int count = StretchZ( zz, index, X, Y, Z, chunkIndex, tile, tileSide );
					AddVertices( tile, count, tileSide );
					counts[index] = (byte)count;
				}
			}
		}
		
		byte GetNeighbour( int chunkIndex, int face ) {			
			switch( face ) {
				case TileSide.Left:
					return chunk[chunkIndex - 1]; // x - 1
					
				case TileSide.Right:
					return chunk[chunkIndex + 1]; // x + 1
					
				case TileSide.Front:
					return chunk[chunkIndex - 18]; // z - 1
					
				case TileSide.Back:
					return chunk[chunkIndex + 18]; // z + 1
					
				case TileSide.Bottom:
					return chunk[chunkIndex - 324]; // y - 1
					
				case TileSide.Top:
					return chunk[chunkIndex + 324]; // y + 1
			}
			return 0;
		}
		
		int StretchX( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte tile, int face ) {
			int count = 1;
			x++;
			chunkIndex++;
			countIndex += TileSide.Sides;
			int max = chunkSize - xx;
			while( count < max && x < width && CanStretch( tile, chunkIndex, x, y, z, face ) ) {
				counts[countIndex] = 0;
				count++;
				x++;
				chunkIndex++;
				countIndex += TileSide.Sides;
			}
			return count;
		}
		
		int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte tile, int face ) {
			int count = 1;
			z++;
			chunkIndex += extChunkSize;
			countIndex += chunkSize * TileSide.Sides;
			int max = chunkSize - zz;
			while( count < max && z < length && CanStretch( tile, chunkIndex, x, y, z, face ) ) {
				counts[countIndex] = 0;
				count++;
				z++;
				chunkIndex += extChunkSize;
				countIndex += chunkSize * TileSide.Sides;
			}
			return count;
		}
		
		public void OnNewMap() {
		}
		
		public void OnNewMapLoaded() {
			map = game.Map;
			width = map.Width;
			height = map.Height;
			length = map.Length;
			clipLevel = Math.Max( 0, game.Map.GroundHeight );
			maxX = width - 1;
			maxY = height - 1;
			maxZ = length - 1;
		}
	}
	
	public struct ChunkPartInfo {
		
		public int VbId, IndicesCount;
		public int leftIndex, rightIndex, frontIndex,
		backIndex, bottomIndex, topIndex;
		public ushort leftCount, rightCount, frontCount,
		backCount, bottomCount, topCount, spriteCount;
	}
}