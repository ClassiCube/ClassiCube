using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ChunkMeshBuilderTex2Col4 : ChunkMeshBuilder {
		
		DrawInfo1D[] drawInfoBuffer;
		TextureAtlas1D atlas;
		int arraysCount = 0;
		
		public ChunkMeshBuilderTex2Col4( Game window ) : base( window ) {
			Window.TerrainAtlasChanged += TerrainAtlasChanged;
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = Window.TerrainAtlas1DTexIds.Length;
			if( arraysCount > 0 && arraysCount != newArraysCount ) {
				Array.Resize( ref drawInfoBuffer, newArraysCount * 2 );
				for( int i = arraysCount * 2; i < drawInfoBuffer.Length; i++ ) {
					drawInfoBuffer[i] = new DrawInfo1D();
				}
			}
			arraysCount = newArraysCount;
		}
		
		class DrawInfo1D {
			public VertexPos3fTex2fCol4b[] vertices;
			public int index;
			public int totalVertices;
			
			public DrawInfo1D() {
				vertices = new VertexPos3fTex2fCol4b[0];
			}
		}
		
		public override bool UsesLighting {
			get { return true; }
		}
		
		protected override bool CanStretch( byte initialTile, int chunkIndex, int x, int y, int z, int face ) {
			byte tile = chunk[chunkIndex];
			return tile == initialTile && !IsFaceHidden( tile, GetNeighbour( chunkIndex, face ) ) &&
				( IsLit( startX, startY, startZ, face ) == IsLit( x, y, z, face ) );
		}
		
		bool IsLit( int x, int y, int z, int face ) {
			switch( face ) {
				case TileSide.Left:
					return x <= 0 ? true : IsLitAdj( x - 1, y, z );
					
				case TileSide.Right:
					return x >= maxX ? true : IsLitAdj( x + 1, y, z );
					
				case TileSide.Front:
					return z <= 0 ? true : IsLitAdj( x, y, z - 1 );
					
				case TileSide.Back:
					return z >= maxZ ? true : IsLitAdj( x, y, z + 1 );
					
				case TileSide.Bottom:
					return y <= 0 ? true : IsLit( x, y - 1, z );
					
				case TileSide.Top:
					return y >= maxY ? true : IsLit( x, y + 1, z );
			}
			return true;
		}
		
		bool IsLit( int x, int y, int z ) {
			return y > GetLightHeight( x, z );
		}
		
		bool IsLitAdj( int x, int y, int z ) {
			return y > GetLightHeightAdj( x, z );
		}
		
		FastColour GetColour( int x, int y, int z, ref FastColour sunlight, ref FastColour shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeight( x, z ) ? sunlight : shadow;
		}
		
		FastColour GetColourAdj( int x, int y, int z, ref FastColour sunlight, ref FastColour shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeightAdj( x, z ) ? sunlight : shadow;
		}
		
		int GetLightHeight( int x, int z ) {
			int y = map.GetLightHeight( x, z );
			if( y == -1 ) return -1;
			return y;
		}
		
		int GetLightHeightAdj( int x, int z ) {
			int y = map.GetLightHeight( x, z );
			if( y == -1 ) return -1;
			return BlockInfo.BlockHeight( map.GetBlock( x, y, z ) ) == 1 ? y : y - 1;
		}
		
		protected override ChunkDrawInfo[] GetChunkInfo( int x, int y, int z ) {
			ChunkDrawInfo[] info = new ChunkDrawInfo[arraysCount * 2];
			for( int i = 0; i < info.Length; i++ ) {
				DrawInfo1D drawInfo = drawInfoBuffer[i];
				int vboId = 0, totalVertices = drawInfo.totalVertices;
				if( totalVertices > 0 ) {
					vboId = Graphics.InitVb( drawInfo.vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b, totalVertices );
				}
				info[i] = new ChunkDrawInfo( vboId, totalVertices );
			}
			return info;
		}
		
		bool isTranslucent;
		float blockHeight;
		float invVerElementSize;
		protected override void PreRenderTile() {
			blockHeight = -1;
		}
		
		protected override void PreStretchTiles( int x1, int y1, int z1 ) {
			invVerElementSize = Window.TerrainAtlas1D.invElementSize;
			arraysCount = Window.TerrainAtlas1DTexIds.Length;
			atlas = Window.TerrainAtlas1D;
			
			if( drawInfoBuffer == null ) {
				drawInfoBuffer = new DrawInfo1D[arraysCount * 2];
				for( int i = 0; i < drawInfoBuffer.Length; i++ ) {
					drawInfoBuffer[i] = new DrawInfo1D();
				}
			} else {
				for( int i = 0; i < drawInfoBuffer.Length; i++ ) {
					DrawInfo1D info = drawInfoBuffer[i];
					info.index = 0;
					info.totalVertices = 0;
				}
			}
		}
		
		protected override void PostStretchTiles( int x1, int y1, int z1 ) {
			for( int i = 0; i < drawInfoBuffer.Length; i++ ) {
				DrawInfo1D info = drawInfoBuffer[i];
				if( info.totalVertices > info.vertices.Length ) {
					Array.Resize( ref info.vertices, info.totalVertices );
				}
			}
		}
		
		public override void BeginRender() {
			Graphics.BeginVbBatch( VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		public override void Render( ChunkDrawInfo info ) {
			Graphics.DrawVbBatch( DrawMode.Triangles, info.VboID, info.VerticesCount );
		}
		
		public override void EndRender() {
			Graphics.EndVbBatch();
		}
		
		protected override void AddSpriteVertices( byte tile, int count ) {
			int i = atlas.Get1DIndex( BlockInfo.GetOptimTextureLoc( tile, TileSide.Left ) );
			drawInfoBuffer[i].totalVertices += 6 + 6 * count;
		}
		
		protected override void AddVertices( byte tile, int count, int face ) {
			int i = atlas.Get1DIndex( BlockInfo.GetOptimTextureLoc( tile, face ) );
			if( BlockInfo.IsTranslucent( tile ) ) {
				i += arraysCount;
			}
			drawInfoBuffer[i].totalVertices += 6;
		}

		protected override void DrawTopFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Top );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];

			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
		}

		protected override void DrawBottomFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Bottom );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColour( X, Y - 1, Z, ref map.SunlightYBottom, ref map.ShadowlightYBottom );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
		}

		protected override void DrawBackFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Back );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColourAdj( X, Y, Z + 1, ref map.SunlightZSide, ref map.ShadowlightZSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
		}

		protected override void DrawFrontFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Front );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColourAdj( X, Y, Z - 1, ref map.SunlightZSide, ref map.ShadowlightZSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col );
		}

		protected override void DrawLeftFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColourAdj( X - 1, Y, Z, ref map.SunlightXSide, ref map.ShadowlightXSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U2, rec.V1, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + count, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
		}

		protected override void DrawRightFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColourAdj( X + 1, Y, Z, ref map.SunlightXSide, ref map.ShadowlightXSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + count, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col );
		}
		
		protected override void DrawSprite( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, 1 );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( isTranslucent ) {
				drawInfoIndex += arraysCount;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			
			// Draw stretched Z axis
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U2, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 0.5f, rec.U1, rec.V2, col );
			info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			
			// Draw X axis
			rec.U2 = 1;
			int startX = X;
			
			for( int i = 0; i < count; i++ ) {
				info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
				info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col );
				info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
				
				info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
				info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U2, rec.V2, col );
				info.vertices[info.index++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
				X++;
			}
			X = startX;
		}
	}
}