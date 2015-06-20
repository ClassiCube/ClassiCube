using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class StandardChunkMeshBuilder : ChunkMeshBuilder {
		
		DrawInfo1D[] drawInfoBuffer;
		TerrainAtlas1D atlas;
		int arraysCount = 0;
		public StandardChunkMeshBuilder( Game window ) : base( window ) {
			window.TerrainAtlasChanged += TerrainAtlasChanged;
		}
		
		public override void Dispose() {
			base.Dispose();
			Window.TerrainAtlasChanged -= TerrainAtlasChanged;
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = Window.TerrainAtlas1D.TexIds.Length;
			if( arraysCount > 0 && arraysCount != newArraysCount ) {
				Array.Resize( ref drawInfoBuffer, newArraysCount );
				for( int i = arraysCount; i < drawInfoBuffer.Length; i++ ) {
					drawInfoBuffer[i] = new DrawInfo1D();
				}
			}
			arraysCount = newArraysCount;
		}
		
		class DrawInfo1D {
			public DrawInfo1DPart Solid = new DrawInfo1DPart();
			public DrawInfo1DPart Translucent = new DrawInfo1DPart();
			public DrawInfo1DPart Sprite = new DrawInfo1DPart();
		}
		
		class DrawInfo1DPart {
			public VertexPos3fTex2fCol4b[] vertices;
			public ushort[] indices;
			public int vIndex, vCount;
			public int iIndex, iCount;
			
			public DrawInfo1DPart() {
				vertices = new VertexPos3fTex2fCol4b[0];
				indices = new ushort[0];
			}
			
			public void ExpandToCapacity() {
				vCount = ( iCount / 6 ) * 4;

				if( vCount > vertices.Length ) {
					vertices = new VertexPos3fTex2fCol4b[vCount];
					indices = new ushort[iCount];
				}
			}
			
			public void ResetState() {
				vIndex = iIndex = 0;
				vCount = iCount = 0;
			}
		}
		
		protected override bool CanStretch( byte initialTile, int chunkIndex, int x, int y, int z, int face ) {
			byte tile = chunk[chunkIndex];
			return tile == initialTile && !BlockInfo.IsFaceHidden( tile, GetNeighbour( chunkIndex, face ), face ) &&
				( IsLit( startX, startY, startZ, face ) == IsLit( x, y, z, face ) );
		}
		
		bool IsLit( int x, int y, int z, int face ) {
			switch( face ) {
				case TileSide.Left:
					return x <= 0 || IsLitAdj( x - 1, y, z );
					
				case TileSide.Right:
					return x >= maxX || IsLitAdj( x + 1, y, z );
					
				case TileSide.Front:
					return z <= 0 || IsLitAdj( x, y, z - 1 );
					
				case TileSide.Back:
					return z >= maxZ || IsLitAdj( x, y, z + 1 );
					
				case TileSide.Bottom:
					return y <= 0 || IsLit( x, y - 1, z );
					
				case TileSide.Top:
					return y >= maxY || IsLit( x, y + 1, z );
			}
			return true;
		}
		
		bool IsLit( int x, int y, int z ) {
			return y > GetLightHeight( x, z );
		}
		
		bool IsLitAdj( int x, int y, int z ) {
			return y > GetLightHeightAdj( x, z );
		}
		
		byte GetColour( int x, int y, int z, byte sunlight, byte shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeight( x, z ) ? sunlight : shadow;
		}
		
		byte GetColourAdj( int x, int y, int z, byte sunlight, byte shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeightAdj( x, z ) ? sunlight : shadow;
		}
		
		int GetLightHeight( int x, int z ) {
			return map.GetLightHeight( x, z );
		}
		
		int GetLightHeightAdj( int x, int z ) {
			int y = map.GetLightHeight( x, z );
			return y == -1 ? -1 :
				( BlockInfo.BlockHeight( map.GetBlock( x, y, z ) ) == 1 ? y : y - 1 );
		}
		
		protected override void GetChunkInfo( int x, int y, int z, ref ChunkPartInfo[] solidParts,
		                  ref ChunkPartInfo[] spriteParts, ref ChunkPartInfo[] translucentParts ) {
			for( int i = 0; i < arraysCount; i++ ) {
				DrawInfo1D info = drawInfoBuffer[i];
				SetPartInfo( info.Solid, i, ref solidParts );
				SetPartInfo( info.Sprite, i, ref spriteParts );
				SetPartInfo( info.Translucent, i, ref translucentParts );
			}
		}
		
		void SetPartInfo( DrawInfo1DPart part, int i, ref ChunkPartInfo[] parts ) {
			if( part.iCount == 0 ) return;
			
			ChunkPartInfo info = default( ChunkPartInfo );
			info.VbId = Graphics.InitVb( part.vertices, VertexPos3fTex2fCol4b.Size, part.vCount );
			info.IbId = Graphics.InitIb( part.indices, part.iCount );
			info.IndicesCount = part.iCount;
			
			// Lazy initalize part arrays so we can save time in MapRenderer for chunks that only contain 1 or 2 part types.
			if( parts == null ) 
				parts = new ChunkPartInfo[arraysCount];
			parts[i] = info;
		}
		
		bool isTranslucent;
		float invVerElementSize;	
		protected override void PreStretchTiles( int x1, int y1, int z1 ) {
			invVerElementSize = Window.TerrainAtlas1D.invElementSize;
			arraysCount = Window.TerrainAtlas1D.TexIds.Length;
			atlas = Window.TerrainAtlas1D;
			
			if( drawInfoBuffer == null ) {
				drawInfoBuffer = new DrawInfo1D[arraysCount];
				for( int i = 0; i < drawInfoBuffer.Length; i++ ) {
					drawInfoBuffer[i] = new DrawInfo1D();
				}
			} else {
				for( int i = 0; i < drawInfoBuffer.Length; i++ ) {
					DrawInfo1D info = drawInfoBuffer[i];
					info.Solid.ResetState();
					info.Sprite.ResetState();
					info.Translucent.ResetState();
				}
			}
		}
		
		protected override void PostStretchTiles( int x1, int y1, int z1 ) {
			for( int i = 0; i < drawInfoBuffer.Length; i++ ) {
				DrawInfo1D info = drawInfoBuffer[i];
				info.Solid.ExpandToCapacity();
				info.Sprite.ExpandToCapacity();
				info.Translucent.ExpandToCapacity();
			}
		}
		
		protected override void AddSpriteVertices( byte tile, int count ) {
			int i = atlas.Get1DIndex( BlockInfo.GetOptimTextureLoc( tile, TileSide.Left ) );
			drawInfoBuffer[i].Sprite.iCount += 6 + 6 * count;
		}
		
		protected override void AddVertices( byte tile, int count, int face ) {
			int i = atlas.Get1DIndex( BlockInfo.GetOptimTextureLoc( tile, face ) );
			if( BlockInfo.IsTranslucent( tile ) ) {
				drawInfoBuffer[i].Translucent.iCount += 6;
			} else {
				drawInfoBuffer[i].Solid.iCount += 6;
			}
		}

		const byte baseCol = 0, xCol = 1, zCol = 2, yBottomCol = 3;
		protected override void DrawTopFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Top );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColour( X, Y + 1, Z, baseCol, baseCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;

			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U2, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col, tile, 0, 0 );
		}

		protected override void DrawBottomFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Bottom );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColour( X, Y - 1, Z, yBottomCol, yBottomCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U2, rec.V1, col, tile, 0, 0 );
		}

		protected override void DrawBackFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Back );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColourAdj( X, Y, Z + 1, zCol, zCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col, tile, 0, 0 );
		}

		protected override void DrawFrontFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Front );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColourAdj( X, Y, Z - 1, zCol, zCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
		}

		protected override void DrawLeftFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColourAdj( X - 1, Y, Z, xCol, xCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U2, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + count, rec.U2, rec.V2, col, tile, 0, 0 );
		}

		protected override void DrawRightFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColourAdj( X + 1, Y, Z, xCol, xCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + count, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col, tile, 0, 0 );
		}
		
		protected override void DrawSprite( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out drawInfoIndex );
			byte col = GetColour( X, Y + 1, Z, baseCol, baseCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
			}
			DrawInfo1DPart part = drawInfoBuffer[drawInfoIndex].Sprite;
			
			// Draw stretched Z axis
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U2, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 0.5f, rec.U1, rec.V2, col, tile, 0, 0 );
			
			// Draw X axis
			rec.U2 = 1;
			int startX = X;
			
			for( int i = 0; i < count; i++ ) {
				AddIndices( part );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col, tile, 0, 0 );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col, tile, 0, 0 );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U2, rec.V2, col, tile, 0, 0 );
				X++;
			}
			X = startX;
		}
		
		void AddIndices( DrawInfo1DPart part ) {
			int element = part.vIndex;
			part.indices[part.iIndex++] = (ushort)( element + 0 );
			part.indices[part.iIndex++] = (ushort)( element + 1 );
			part.indices[part.iIndex++] = (ushort)( element + 2 );
			
			part.indices[part.iIndex++] = (ushort)( element + 2 );
			part.indices[part.iIndex++] = (ushort)( element + 3 );
			part.indices[part.iIndex++] = (ushort)( element + 0 );
		}
	}
}