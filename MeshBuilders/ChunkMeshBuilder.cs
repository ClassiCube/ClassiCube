using System;
using ClassicalSharp.Blocks.Model;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.World;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {
		
		public BlockInfo BlockInfo;
		protected Map map;
		public Game Window;
		public IGraphicsApi Graphics;
		FastColour[] colours;
		
		public ChunkMeshBuilder( Game window ) {
			Window = window;
			Graphics = window.Graphics;
			BlockInfo = window.BlockInfo;
			map = window.Map;
			colours = new FastColour[] {
				map.SunlightXSide,
				map.SunlightXSide,
				map.SunlightZSide,
				map.SunlightZSide,
				map.SunlightYBottom,
				map.Sunlight,				
			};
		}
		
		byte[] drawFlags = new byte[16 * 16 * 16 * 6];
		byte[] chunk = new byte[( 16 + 2 ) * ( 16 + 2 ) * ( 16 + 2 )];
		byte[] meta = new byte[( 16 + 2 ) * ( 16 + 2 ) * ( 16 + 2 )];
		const int minY = 0, maxY = 127;
		
		unsafe void BuildChunk( int x1, int y1, int z1 ) {
			PreStretchTiles( x1, y1, z1 );
			if( ReadChunkData( x1, y1, z1 ) ) return;
			
			int* offsets = stackalloc int[6];
			offsets[TileSide.Left] = -1; // x - 1
			offsets[TileSide.Right] = 1; // x + 1
			offsets[TileSide.Front] = -18; // z - 1
			offsets[TileSide.Back] = 18; // z + 1
			offsets[TileSide.Bottom] = -324; // y - 1
			offsets[TileSide.Top] = 324; // y + 1
			
			Stretch( x1, y1, z1, offsets );
			PostStretchTiles( x1, y1, z1 );
			
			for( int y = y1, yy = 0; y < y1 + 16; y++, yy++ ) {
				for( int z = z1, zz = 0; z < z1 + 16; z++, zz++ ) {
					
					int chunkIndex = ( yy + 1 ) * 324 + ( zz + 1 ) * 18 + ( -1 + 1 );
					int countIndex = ( ( yy << 8 ) + ( zz << 4 ) + 0 ) * 6;
					for( int x = x1, xx = 0; x < x1 + 16; x++, xx++ ) {
						chunkIndex++;
						RenderTile( chunkIndex, countIndex, x, y, z, offsets );
						countIndex += 6;
					}
				}
			}
		}
		
		unsafe bool ReadChunkData( int x1, int y1, int z1 ) {
			bool allAir = true, allSolid = true;
			fixed( byte* chunkPtr = chunk, metaPtr = meta ) {
				int* chunkIntPtr = (int*)chunkPtr;
				int* metaIntPtr = (int*)metaPtr;
				for( int i = 0; i < ( 18 * 18 * 18 ) / 4; i++ ) {
					*chunkIntPtr++ = 0;
					*metaIntPtr++ = 0;
				}
				CopyMainPart( x1, y1, z1, ref allAir, ref allSolid, chunkPtr, metaPtr );
				CopyXMinus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr, metaPtr );
				CopyXPlus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr, metaPtr );
				CopyZMinus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr, metaPtr );
				CopyZPlus( x1, y1, z1, ref allAir, ref allSolid, chunkPtr, metaPtr );
			}
			return allAir || allSolid;
		}
		
		public SectionDrawInfo GetDrawInfo( int x, int y, int z ) {
			BuildChunk( x, y, z );
			return GetChunkInfo( x, y, z );
		}

		unsafe void RenderTile( int chunkIndex, int countIndex, int x, int y, int z, int* offsets ) {
			byte tile = chunk[chunkIndex];
			if( tile == 0 ) return;
			IBlockModel model = BlockInfo.GetModel( tile );
			byte tileMeta = meta[chunkIndex];
			DrawInfo1DPart part = model.Pass == BlockPass.Solid ? Solid :
				model.Pass == BlockPass.Transluscent ? Transluscent : Sprite;
			Neighbours state = new Neighbours();
			if( model.NeedsNeighbourState ) {
				state.Above = chunk[chunkIndex + offsets[TileSide.Top]];
				state.Below = chunk[chunkIndex + offsets[TileSide.Bottom]];
				state.Left = chunk[chunkIndex + offsets[TileSide.Left]];
				state.Right = chunk[chunkIndex + offsets[TileSide.Right]];
				state.Front = chunk[chunkIndex + offsets[TileSide.Front]];
				state.Back = chunk[chunkIndex + offsets[TileSide.Back]];
			}
			
			for( int face = 0; face < 6; face ++ ) {
				int count = drawFlags[countIndex + face];
				if( count != 0 ) {
					model.DrawFace( face, tileMeta, state, ref part.index, x, y, z, part.vertices, colours[face] );
				}
			}
		}
		
		unsafe void Stretch( int x1, int y1, int z1, int* offsets ) {
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
						IBlockModel model = BlockInfo.GetModel( tile );
						byte tileMeta = meta[chunkIndex];
						CountVertices( countIndex, tile, tileMeta, chunkIndex, offsets, model );
					}
				}
			}
		}
		
		unsafe void CountVertices( int index, byte tile, byte tileMeta, int chunkIndex, int* offsets, IBlockModel model ) {
			Neighbours state = new Neighbours();
			if( model.NeedsNeighbourState ) {
				state.Above = chunk[chunkIndex + offsets[TileSide.Top]];
				state.Below = chunk[chunkIndex + offsets[TileSide.Bottom]];
				state.Left = chunk[chunkIndex + offsets[TileSide.Left]];
				state.Right = chunk[chunkIndex + offsets[TileSide.Right]];
				state.Front = chunk[chunkIndex + offsets[TileSide.Front]];
				state.Back = chunk[chunkIndex + offsets[TileSide.Back]];
			}
			for( int face = 0; face < 6; face++ ) {
				if( !model.HasFace( face ) ) {
					drawFlags[index + face] = 0;
					continue;
				}
				byte neighbour = chunk[chunkIndex + offsets[face]];
				if( model.FaceHidden( face, tileMeta, state, neighbour ) ) {
					drawFlags[index + face] = 0;
				} else {
					AddVertices( model.Pass, model.GetVerticesCount( face, tileMeta, state, neighbour ) );
					drawFlags[index + face] = 1;
				}
			}
		}
		
		DrawInfo1DPart Solid = new DrawInfo1DPart();
		DrawInfo1DPart Transluscent = new DrawInfo1DPart();
		DrawInfo1DPart Sprite = new DrawInfo1DPart();
		
		class DrawInfo1DPart {
			public VertexPos3fTex2fCol4b[] vertices;
			public int index, count;
			
			public DrawInfo1DPart() {
				vertices = new VertexPos3fTex2fCol4b[0];
			}
			
			public void ExpandToCapacity() {
				if( count > vertices.Length ) {
					vertices = new VertexPos3fTex2fCol4b[count];
				}
			}
			
			public void ResetState() {
				index = 0;
				count = 0;
			}
		}
		
		SectionDrawInfo GetChunkInfo( int x, int y, int z ) {
			SectionDrawInfo info = new SectionDrawInfo();
			info.SolidParts = GetPartInfo( Solid );
			info.TranslucentParts = GetPartInfo( Transluscent );
			info.SpriteParts = GetPartInfo( Sprite );
			return info;
		}
		
		ChunkPartInfo GetPartInfo( DrawInfo1DPart part ) {
			ChunkPartInfo info = new ChunkPartInfo( 0, part.count );
			if( part.count > 0 ) {
				info.VboID = Graphics.InitVb( part.vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b, part.count );
			}
			return info;
		}
		
		void PreStretchTiles( int x1, int y1, int z1 ) {
			Solid.ResetState();
			Sprite.ResetState();
			Transluscent.ResetState();
		}
		
		void PostStretchTiles( int x1, int y1, int z1 ) {
			Solid.ExpandToCapacity();
			Sprite.ExpandToCapacity();
			Transluscent.ExpandToCapacity();
		}
		
		public void BeginRender() {
			Graphics.BeginVbBatch( VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		public void Render( ChunkPartInfo info ) {
			Graphics.DrawVbBatch( DrawMode.Triangles, info.VboID, info.VerticesCount );
		}
		
		public void EndRender() {
			Graphics.EndVbBatch();
		}
		
		void AddVertices( BlockPass pass, int count ) {
			if( pass == BlockPass.Solid ) {
				Solid.count += count;
			} else if( pass == BlockPass.Transluscent ) {
				Transluscent.count += count;
			} else if( pass == BlockPass.Sprite ) {
				Sprite.count += count;
			}
		}
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