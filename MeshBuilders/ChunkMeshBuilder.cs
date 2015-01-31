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
			if( chunk == null ) {
				Utils.LogWarning( "um what? " + ( x1 >> 4 ) + " , " + ( z1 >> 4 ) + "(" + x1 + " , " + z1 + ")" );
				return;
			}
			
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
		
		unsafe void Stretch( int x1, int y1, int z1 ) {
			for( int i = 0; i < drawFlags.Length; i++ ) {
				drawFlags[i] = 1;
			}
			int* offsets = stackalloc int[6];
			offsets[TileSide.Left] = -1; // x - 1
			offsets[TileSide.Right] = 1; // x + 1
			offsets[TileSide.Front] = -18; // z - 1
			offsets[TileSide.Back] = 18; // z + 1
			offsets[TileSide.Bottom] = -324; // y - 1
			offsets[TileSide.Top] = 324; // y + 1
			
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
							DoStretchTerrain( countIndex, tile, chunkIndex, offsets );
						}
					}
				}
			}
		}
		
		protected virtual void AddSpriteVertices( byte tile ) {
		}
		
		protected virtual void AddVertices( byte tile, int face ) {
		}
		
		unsafe void DoStretchTerrain( int index, byte tile, int chunkIndex, int* offsets ) {
			for( int face = 0; face < 6; face++ ) {
				if( drawFlags[index + face] != 0 ) {
					byte neighbour = chunk[chunkIndex + offsets[face]];
					if( BlockInfo.IsFaceHidden( tile, neighbour, face ) ) {
						drawFlags[index + face] = 0;
					} else {
						AddVertices( tile, face );
						drawFlags[index + face] = 1;
					}
				}
			}
		}
		
		public abstract void BeginRender();
		
		public abstract void Render( ChunkPartInfo drawInfo );
		
		public abstract void EndRender();
		
		public virtual void OnNewMap() {
		}
		
		public virtual void OnNewMapLoaded() {
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