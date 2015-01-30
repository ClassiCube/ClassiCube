using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.World;

namespace ClassicalSharp {
	
	public abstract class ChunkMeshBuilder {
		
		protected int X, Y, Z;
		protected byte tile;
		public BlockInfo BlockInfo;
		protected Map map;
		public Game Window;
		public IGraphicsApi Graphics;
		
		public ChunkMeshBuilder( Game window ) {
			Window = window;
			Graphics = window.Graphics;
			BlockInfo = window.BlockInfo;
			map = window.Map;
		}
		
		protected byte[] drawFlags = new byte[16 * 16 * 16 * 6];
		protected byte[] chunk = new byte[( 16 + 2 ) * ( 16 + 2 ) * ( 16 + 2 )];
		const int minY = 0, maxY = 127;
		
		public virtual bool UsesLighting {
			get { return false; }
		}
		
		void BuildChunk( int x1, int y1, int z1 ) {
			PreStretchTiles( x1, y1, z1 );
			if( ReadChunkData( x1, y1, z1 ) ) return;
			
			Stretch( x1, y1, z1 );
			PostStretchTiles( x1, y1, z1 );
			
			for( int y = y1, yy = 0; y < y1 + 16; y++, yy++ ) {
				for( int z = z1, zz = 0; z < z1 + 16; z++, zz++ ) {
					
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( -1 + 1 );
					for( int x = x1, xx = 0; x < x1 + 16; x++, xx++ ) {
						chunkIndex++;
						RenderTile( chunkIndex, xx, yy, zz, x, y, z );
					}
				}
			}
		}
		
		unsafe bool ReadChunkData( int x1, int y1, int z1 ) {
			for( int i = 0; i < chunk.Length; i++ ) {
				chunk[i] = 0;
			}
			
			bool allAir = true, allSolid = true;
			fixed( byte* chunkPtr = chunk ) {
				CopyMainPart( x1, y1, z1, ref allAir, ref allSolid, chunkPtr );
				CopyXMinus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr );
				CopyXPlus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr );
				CopyZMinus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr );
				CopyZPlus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr );
			}
			return allAir || allSolid;
		}
		
		unsafe void CopyMainPart( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr ) {
			Chunk chunk = map.GetChunk( x1 >> 4, z1 >> 4 );
			if( chunk == null ) return;
			
			for( int yy = -1; yy < 17; yy++ ) {
				int y = yy + y1;
				if( y < minY ) continue;
				if( y > maxY ) break;
				
				for( int zz = 0; zz < 16; zz++ ) {
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( 0 + 1 );
					for( int xx = 0; xx < 16; xx++ ) {
						byte block = chunk.GetBlock( xx, y, zz );
						if( block != 0 ) allAir = false;
						if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
						
						chunkPtr[chunkIndex] = block;
						chunkIndex++;
					}
				}
			}
		}
		
		unsafe void CopyXMinus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr ) {
			Chunk chunk = map.GetChunk( ( x1 >> 4 ) - 1, z1 >> 4 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( 0 + 1 ) * 18 + ( -1 + 1 );
				
				for( int zz = 0; zz < 16; zz++ ) {
					
					byte block = chunk.GetBlock( 15, y, zz );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					chunkIndex += 18;
				}
			}
		}
		
		unsafe void CopyXPlus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr ) {
			Chunk chunk = map.GetChunk( ( x1 >> 4 ) + 1, z1 >> 4 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( 0 + 1 ) * 18 + ( 16 + 1 );
				
				for( int zz = 0; zz < 16; zz++ ) {
					byte block = chunk.GetBlock( 0, y, zz );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					chunkIndex += 18;
				}
			}
		}
		
		unsafe void CopyZMinus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr ) {
			Chunk chunk = map.GetChunk( x1 >> 4, ( z1 >> 4 ) - 1 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( -1 + 1 ) * 18 + ( 0 + 1 );
				for( int xx = 0; xx < 16; xx++ ) {
					
					byte block = chunk.GetBlock( xx, y, 15 );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					chunkIndex++;
				}
			}
		}
		
		unsafe void CopyZPlus( int x1, int y1, int z1, ref bool allAir, ref bool allSolid, byte* chunkPtr ) {
			Chunk chunk = map.GetChunk( x1 >> 4, ( z1 >> 4 ) + 1 );
			if( chunk == null ) return;
			
			for( int yy = 0; yy < 16; yy++ ) {
				int y = yy + y1;
				int chunkIndex = ( yy + 1 ) * 324 + ( 16 + 1 ) * 18 + ( 0 + 1 );
				
				for( int xx = 0; xx < 16; xx++ ) {
					byte block = chunk.GetBlock( xx, y, 0 );
					if( block != 0 ) allAir = false;
					if( !BlockInfo.IsOpaque( block ) ) allSolid = false;
					
					chunkPtr[chunkIndex] = block;
					chunkIndex++;
				}
			}
		}
		
		public SectionDrawInfo GetDrawInfo( int x, int y, int z ) {
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
				count = drawFlags[index + TileSide.Top];
				if( count != 0 ) {
					DrawSprite();
				}
				return;
			}
			
			count = drawFlags[index + TileSide.Left];
			if( count != 0 ) {
				DrawLeftFace();
			}
			count = drawFlags[index + TileSide.Right];
			if( count != 0 ) {
				DrawRightFace();
			}
			count = drawFlags[index + TileSide.Front];
			if( count != 0 ) {
				DrawFrontFace();
			}
			count = drawFlags[index + TileSide.Back];
			if( count != 0 ) {
				DrawBackFace();
			}
			count = drawFlags[index + TileSide.Bottom];
			if( count != 0 ) {
				DrawBottomFace();
			}
			count = drawFlags[index + TileSide.Top];
			if( count != 0 ) {
				DrawTopFace();
			}
		}
		
		
		void Stretch( int x1, int y1, int z1 ) {
			for( int i = 0; i < drawFlags.Length; i++ ) {
				drawFlags[i] = 1;
			}
			
			for( int y = y1, yy = 0; y < y1 + 16; y++, yy++ ) {
				for( int z = z1, zz = 0; z < z1 + 16; z++, zz++ ) {
					
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( -1 + 1 );
					for( int x = x1, xx = 0; x < x1 + 16; x++, xx++ ) {
						chunkIndex++;
						byte tile = chunk[chunkIndex];
						if( tile == 0 ) continue;
						int countIndex = ( ( yy << 8 ) + ( zz << 4 ) + xx ) * 6;
						
						// Sprites only use the top face to indicate stretching count, so we can take a shortcut here.
						// Note that sprites are not drawn with any of the DrawXFace, they are drawn using DrawSprite.
						if( BlockInfo.IsSprite( tile ) ) {
							if( drawFlags[countIndex + TileSide.Top] != 0 ) {
								AddSpriteVertices( tile );
								drawFlags[countIndex + TileSide.Top] = 1;
							}
						} else { // Do the full calculation for all six faces.
							DoStretchTerrain( xx, yy, zz, x, y, z, countIndex, tile, chunkIndex );
						}
					}
				}
			}
		}
		
		protected virtual void AddSpriteVertices( byte tile ) {
		}
		
		protected virtual void AddVertices( byte tile, int face ) {
		}
		
		protected int startX, startY, startZ;
		void DoStretchTerrain( int xx, int yy, int zz, int x, int y, int z, int index, byte tile, int chunkIndex ) {
			startX = x;
			startY = y;
			startZ = z;
			
			if( drawFlags[index + TileSide.Left] != 0 ) {
				byte left = chunk[chunkIndex - 1]; // x - 1
				if( BlockInfo.IsFaceHidden( tile, left, TileSide.Left ) ) {
					drawFlags[index + TileSide.Left] = 0;
				} else {
					AddVertices( tile, TileSide.Left );
					drawFlags[index + TileSide.Left] = 1;
				}
			}
			
			if( drawFlags[index + TileSide.Right] != 0 ) {
				byte right = chunk[chunkIndex + 1]; // x + 1
				if( BlockInfo.IsFaceHidden( tile, right, TileSide.Right ) ) {
					drawFlags[index + TileSide.Right] = 0;
				} else {
					AddVertices( tile, TileSide.Right );
					drawFlags[index + TileSide.Right] = 1;
				}
			}
			
			if( drawFlags[index + TileSide.Front] != 0 ) {
				byte front = chunk[chunkIndex - 18]; // z - 1
				if( BlockInfo.IsFaceHidden( tile, front, TileSide.Front ) ) {
					drawFlags[index + TileSide.Front] = 0;
				} else {
					AddVertices( tile, TileSide.Front );
					drawFlags[index + TileSide.Front] = 1;
				}
			}
			
			if( drawFlags[index + TileSide.Back] != 0 ) {
				byte back = chunk[chunkIndex + 18]; // z + 1
				if( BlockInfo.IsFaceHidden( tile, back, TileSide.Back ) ) {
					drawFlags[index + TileSide.Back] = 0;
				} else {
					AddVertices( tile, TileSide.Back );
					drawFlags[index + TileSide.Back] = 1;
				}
			}
			
			if( drawFlags[index + TileSide.Bottom] != 0 ) {
				byte below = chunk[chunkIndex - 324]; // y - 1
				if( BlockInfo.IsFaceHidden( tile, below, TileSide.Bottom ) ) {
					drawFlags[index + TileSide.Bottom] = 0;
				} else {
					AddVertices( tile, TileSide.Bottom );
					drawFlags[index + TileSide.Bottom] = 1;
				}
			}
			
			if( drawFlags[index + TileSide.Top] != 0 ) {
				byte above = chunk[chunkIndex + 324]; // y + 1
				if( BlockInfo.IsFaceHidden( tile, above, TileSide.Top ) ) {
					drawFlags[index + TileSide.Top] = 0;
				} else {
					AddVertices( tile, TileSide.Top );
					drawFlags[index + TileSide.Top] = 1;
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
		
		public abstract void BeginRender();
		
		public abstract void Render( ChunkPartInfo drawInfo );
		
		public abstract void EndRender();
		
		public virtual void OnNewMap() {
		}
		
		public virtual void OnNewMapLoaded() {
			map = Window.Map;
		}
		
		protected abstract SectionDrawInfo GetChunkInfo( int x, int y, int z );

		protected abstract void DrawTopFace();

		protected abstract void DrawBottomFace();

		protected abstract void DrawBackFace();

		protected abstract void DrawFrontFace();

		protected abstract void DrawLeftFace();

		protected abstract void DrawRightFace();
		
		protected abstract void DrawSprite();
	}
	
	public class SectionDrawInfo {
		public ChunkPartInfo SolidParts;
		public ChunkPartInfo TranslucentParts;
		public ChunkPartInfo SpriteParts;
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