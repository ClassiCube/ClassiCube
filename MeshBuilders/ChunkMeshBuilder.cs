using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public abstract class ChunkMeshBuilder {
		
		protected int X, Y, Z;
		protected byte tile;
		public BlockInfo BlockInfo;
		protected Map map;
		public Game Window;
		public IGraphicsApi Graphics;
		byte[] adjBlockLookup;
		
		public ChunkMeshBuilder( Game window ) {
			Window = window;
			Graphics = window.Graphics;
			BlockInfo = window.BlockInfo;
			
			adjBlockLookup = new byte[256];
			for( int i = 0; i < adjBlockLookup.Length; i++ ) {
				adjBlockLookup[i] = (byte)i;
			}
			adjBlockLookup[9] = 8;
			adjBlockLookup[11] = 10;
		}
		
		protected int width, length, height;
		protected int maxX, maxY, maxZ;
		protected byte[] counts = new byte[16 * 16 * 16 * 6];
		protected byte[] chunk = new byte[( 16 + 2 ) * ( 16 + 2 ) * ( 16 + 2 )];
		
		public virtual bool UsesLighting {
			get { return false; }
		}
		
		void BuildChunk( int x1, int y1, int z1 ) {
			PreStretchTiles( x1, y1, z1 );
			if( ReadChunkData( x1, y1, z1 ) ) return;
			
			Stretch( x1, y1, z1 );
			PostStretchTiles( x1, y1, z1 );
			
			int xMax = Math.Min( width, x1 + 16 );
			int yMax = Math.Min( height, y1 + 16 );
			int zMax = Math.Min( length, z1 + 16 );
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( -1 + 1 );
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						chunkIndex++;
						RenderTile( chunkIndex, xx, yy, zz, x, y, z );
					}
				}
			}
		}
		
		unsafe bool ReadChunkData( int x1, int y1, int z1 ) {
			bool allAir = true, allSolid = true;
			fixed( byte* chunkPtr = chunk, mapPtr = map.mapData ) {
				
				int* chunkIntPtr = (int*)chunkPtr;
				for( int i = 0; i < ( 18 * 18 * 18 ) / 4; i++ ) {
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
						int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( -1 );
						
						for( int xx = -1; xx < 17; xx++ ) {
							int x = xx + x1;
							index++;
							chunkIndex++;
							if( x < 0 ) continue;
							if( x > maxX ) break;
							
							byte block = mapPtr[index];
							if( block == 9 ) block = 8; // Still water --> Water
							if( block == 11 ) block = 10; // Still lava --> Lava
							//byte block = adjBlockLookup[map.GetBlock( index )];
							
							if( allAir && block != 0 ) allAir = false;
							if( allSolid && !BlockInfo.IsOpaque( block ) ) allSolid = false;
							chunkPtr[chunkIndex] = block;
						}
					}
				}
			}
			return allAir || allSolid;
		}
		
		public ChunkDrawInfo GetDrawInfo( int x, int y, int z ) {
			BuildChunk( x, y, z );
			return GetChunkInfo( x, y, z );
		}
		
		protected virtual void PreStretchTiles( int x1, int y1, int z1 ) {
		}
		
		protected virtual void PostStretchTiles( int x1, int y1, int z1 ) {
		}
		
		protected virtual void PreRenderTile() {
		}

		public void RenderTile( int chunkIndex, int xx, int yy, int zz, int x, int y, int z ) {
			tile = chunk[chunkIndex];
			if( tile == 0 ) return;
			X = x;
			Y = y;
			Z = z;
			PreRenderTile();
			int index = ( ( yy << 8 ) + ( zz << 4 ) + xx ) * 6;
			int count = 0;
			
			if( BlockInfo.IsSprite( tile ) ) {
				count = counts[index + TileSide.Top];
				if( count != 0 ) {
					DrawSprite( count );
				}
				return;
			}
			
			count = counts[index + TileSide.Left];
			if( count != 0 ) {
				DrawLeftFace( count );
			}
			count = counts[index + TileSide.Right];
			if( count != 0 ) {
				DrawRightFace( count );
			}
			count = counts[index + TileSide.Front];
			if( count != 0 ) {
				DrawFrontFace( count );
			}
			count = counts[index + TileSide.Back];
			if( count != 0 ) {
				DrawBackFace( count );
			}
			count = counts[index + TileSide.Bottom];
			if( count != 0 ) {
				DrawBottomFace( count );
			}
			count = counts[index + TileSide.Top];
			if( count != 0 ) {
				DrawTopFace( count );
			}
		}
		
		
		void Stretch( int x1, int y1, int z1 ) {
			for( int i = 0; i < counts.Length; i++ ) {
				counts[i] = 1;
			}
			
			int xMax = Math.Min( width, x1 + 16 );
			int yMax = Math.Min( height, y1 + 16 );
			int zMax = Math.Min( length, z1 + 16 );
			for( int y = y1, yy = 0; y < yMax; y++, yy++ ) {
				for( int z = z1, zz = 0; z < zMax; z++, zz++ ) {
					
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( -1 + 1 );
					for( int x = x1, xx = 0; x < xMax; x++, xx++ ) {
						chunkIndex++;
						byte tile = chunk[chunkIndex];
						if( tile == 0 ) continue;
						int countIndex = ( ( yy << 8 ) + ( zz << 4 ) + xx ) * 6;
						
						// Sprites only use the top face to indicate stretching count, so we can take a shortcut here.
						// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
						if( BlockInfo.IsSprite( tile ) ) {
							if( counts[countIndex + TileSide.Top] != 0 ) {
								startX = x;
								startY = y;
								startZ = z;
								int count = StretchX( xx, countIndex, x, y, z, chunkIndex, tile, TileSide.Top );
								AddSpriteVertices( tile, count );
								counts[countIndex + TileSide.Top] = (byte)count;
							}
						} else { // Do the full calculation for all six faces.
							DoStretchTerrain( xx, yy, zz, x, y, z, countIndex, tile, chunkIndex );
						}
					}
				}
			}
		}
		
		protected virtual void AddSpriteVertices( byte tile, int count ) {
		}
		
		protected virtual void AddVertices( byte tile, int count, int face ) {
		}
		
		protected int startX, startY, startZ;
		void DoStretchTerrain( int xx, int yy, int zz, int x, int y, int z, int index, byte tile, int chunkIndex ) {
			startX = x;
			startY = y;
			startZ = z;
			
			if( x == 0 ) {
				counts[index + TileSide.Left] = 0;
			} else if( counts[index + TileSide.Left] != 0 ) {
				byte left = chunk[chunkIndex - 1]; // x - 1
				if( BlockInfo.IsFaceHidden( tile, left, TileSide.Left ) ) {
					counts[index + TileSide.Left] = 0;
				} else {
					int count = StretchZ( zz, index, x, y, z, chunkIndex, tile, TileSide.Left );
					AddVertices( tile, count, TileSide.Left );
					counts[index + TileSide.Left] = (byte)count;
				}
			}
			
			if( x == maxX ) {
				counts[index + TileSide.Right] = 0;
			} else if( counts[index + TileSide.Right] != 0 ) {
				byte right = chunk[chunkIndex + 1]; // x + 1
				if( BlockInfo.IsFaceHidden( tile, right, TileSide.Right ) ) {
					counts[index + TileSide.Right] = 0;
				} else {
					int count = StretchZ( zz, index, x, y, z, chunkIndex, tile, TileSide.Right );
					AddVertices( tile, count, TileSide.Right );
					counts[index + TileSide.Right] = (byte)count;
				}
			}
			
			if( z == 0 ) {
				counts[index + TileSide.Front] = 0;
			} else if( counts[index + TileSide.Front] != 0 ) {
				byte front = chunk[chunkIndex - 18]; // z - 1
				if( BlockInfo.IsFaceHidden( tile, front, TileSide.Front ) ) {
					counts[index + TileSide.Front] = 0;
				} else {
					int count = StretchX( xx, index, x, y, z, chunkIndex, tile, TileSide.Front );
					AddVertices( tile, count, TileSide.Front );
					counts[index + TileSide.Front] = (byte)count;
				}
			}
			
			if( z == maxZ ) {
				counts[index + TileSide.Back] = 0;
			} else if( counts[index + TileSide.Back] != 0 ) {
				byte back = chunk[chunkIndex + 18]; // z + 1
				if( BlockInfo.IsFaceHidden( tile, back, TileSide.Right ) ) {
					counts[index + TileSide.Back] = 0;
				} else {
					int count = StretchX( xx, index, x, y, z, chunkIndex, tile, TileSide.Back );
					AddVertices( tile, count, TileSide.Back );
					counts[index + TileSide.Back] = (byte)count;
				}
			}
			
			if( y == 0 ) {
				counts[index + TileSide.Bottom] = 0;
			} else if( counts[index + TileSide.Bottom] != 0 ) {
				byte below = chunk[chunkIndex - 324]; // y - 1
				if( BlockInfo.IsFaceHidden( tile, below, TileSide.Bottom ) ) {
					counts[index + TileSide.Bottom] = 0;
				} else {
					int count = StretchX( xx, index, x, y, z, chunkIndex, tile, TileSide.Bottom );
					AddVertices( tile, count, TileSide.Bottom );
					counts[index + TileSide.Bottom] = (byte)count;
				}
			}
			
			if( counts[index + TileSide.Top] != 0 ) {
				byte above = chunk[chunkIndex + 324]; // y + 1
				if( BlockInfo.IsFaceHidden( tile, above, TileSide.Top ) ) {
					counts[index + TileSide.Top] = 0;
				} else {
					int count = StretchX( xx, index, x, y, z, chunkIndex, tile, TileSide.Top );
					AddVertices( tile, count, TileSide.Top );
					counts[index + TileSide.Top] = (byte)count;
				}
			}
		}
		
		protected virtual bool CanStretch( byte initialTile, int chunkIndex, int x, int y, int z, int face ) {
			byte tile = chunk[chunkIndex];
			return tile == initialTile && !BlockInfo.IsFaceHidden( tile, GetNeighbour( chunkIndex, face ), face );
		}
		
		protected byte GetNeighbour( int chunkIndex, int face ) {
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
			countIndex += 6;
			int max = 16 - xx;
			while( count < max && x < width && CanStretch( tile, chunkIndex, x, y, z, face ) ) {
				count++;
				counts[countIndex + face] = 0;
				x++;
				chunkIndex++;
				countIndex += 6;
			}
			return count;
		}
		
		int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte tile, int face ) {
			int count = 1;
			z++;
			chunkIndex += 18;
			countIndex += 16 * 6;
			int max = 16 - zz;
			while( count < max && z < length && CanStretch( tile, chunkIndex, x, y, z, face ) ) {
				count++;
				counts[countIndex + face] = 0;
				z++;
				chunkIndex += 18;
				countIndex += 16 * 6;
			}
			return count;
		}
		
		public abstract void BeginRender();
		
		public abstract void Render( ChunkPartInfo drawInfo );
		
		public abstract void EndRender();
		
		public virtual void OnNewMap() {
		}
		
		public virtual void OnNewMapLoaded() {
			map = Window.Map;
			width = map.Width;
			height = map.Height;
			length = map.Length;
			maxX = width - 1;
			maxY = height - 1;
			maxZ = length - 1;
		}
		
		protected abstract ChunkDrawInfo GetChunkInfo( int x, int y, int z );

		protected abstract void DrawTopFace( int count );

		protected abstract void DrawBottomFace( int count );

		protected abstract void DrawBackFace( int count );

		protected abstract void DrawFrontFace( int count );

		protected abstract void DrawLeftFace( int count );

		protected abstract void DrawRightFace( int count );
		
		protected abstract void DrawSprite( int count );
	}
	
	public class ChunkDrawInfo {
		
		public ChunkPartInfo[] SolidParts;
		public ChunkPartInfo[] TranslucentParts;
		public ChunkPartInfo[] SpriteParts;
		
		public ChunkDrawInfo( int partsCount ) {
			SolidParts = new ChunkPartInfo[partsCount];
			TranslucentParts = new ChunkPartInfo[partsCount];
			SpriteParts = new ChunkPartInfo[partsCount];
		}
	}
	
	public struct ChunkPartInfo {
		
		public int VboID;
		public int VerticesCount;
		
		public ChunkPartInfo( int vbo, int vertices ) {
			VboID = vbo;
			VerticesCount = vertices;
		}
	}
}