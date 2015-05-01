using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ChunkMeshBuilderTex2Col4 : ChunkMeshBuilder {
		
		DrawInfo1D[] drawInfoBuffer;
		TextureAtlas1D atlas;
		int arraysCount = 0;
		const int maxIndices = 65536;
		
		public ChunkMeshBuilderTex2Col4( Game window ) : base( window ) {
			Window.TerrainAtlasChanged += TerrainAtlasChanged;
		}

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = Window.TerrainAtlas1DTexIds.Length;
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
			public VertexPos3fTex2fCol4b[] vertices1, vertices2, vertices;
			public ushort[] indices1, indices2, indices;
			public int vIndex, vCount;
			public int iIndex, iCount;
			public int vCount1, vCount2;
			public int iCount1, iCount2;
			
			public DrawInfo1DPart() {
				vertices1 = new VertexPos3fTex2fCol4b[0];
				indices1 = new ushort[0];
				vertices2 = new VertexPos3fTex2fCol4b[0];
				indices2 = new ushort[0];
			}
			
			public void ExpandToCapacity() {
				vCount = ( iCount / 6 ) * 4;
				
				vCount1 = Math.Min( vCount, maxIndices );
				iCount1 = ( vCount1 / 4 ) * 6;
				if( vCount1 > vertices1.Length ) {
					vertices1 = new VertexPos3fTex2fCol4b[vCount1];					
					indices1 = new ushort[iCount1];
				}
				
				vCount2 = Math.Max( 0, vCount - maxIndices );
				iCount2 = ( vCount2 / 4 ) * 6;
				if( vCount2 > vertices2.Length ) {
					vertices2 = new VertexPos3fTex2fCol4b[vCount2];					
					indices2 = new ushort[iCount2];
				}
				vertices = vertices1;
				indices = indices1;
			}
			
			public void ResetState() {
				vIndex = iIndex = 0;
				vCount = iCount = 0;
				vCount1 = vCount2 = 0;
				iCount1 = iCount2 = 0;
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
		
		FastColour GetColour( int x, int y, int z, ref FastColour sunlight, ref FastColour shadow ) {
			if( !map.IsValidPos( x, y, z ) ) return sunlight;
			return y > GetLightHeight( x, z ) ? sunlight : shadow;
		}
		
		FastColour GetColourAdj( int x, int y, int z, ref FastColour sunlight, ref FastColour shadow ) {
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
		
		protected override ChunkDrawInfo GetChunkInfo( int x, int y, int z ) {
			ChunkDrawInfo info = new ChunkDrawInfo( arraysCount );
			for( int i = 0; i < arraysCount; i++ ) {
				DrawInfo1D drawInfo = drawInfoBuffer[i];
				SetPartInfo( drawInfo.Solid, i, info.SolidParts );
				SetPartInfo( drawInfo.Translucent, i, info.TranslucentParts );
				SetPartInfo( drawInfo.Sprite, i, info.SpriteParts );
			}
			return info;
		}
		
		void SetPartInfo( DrawInfo1DPart part, int i, ChunkPartInfo[] parts ) {
			ChunkPartInfo info = default( ChunkPartInfo );
			if( part.iCount1 > 0 ) {
				info.VbId = Graphics.InitIndexedVb( part.vertices1, part.indices1, DrawMode.Triangles, part.vCount1, part.iCount1 );
				info.IndicesCount = part.iCount1;
			}
			if( part.iCount2 > 0 ) {
				info.VbId2 = Graphics.InitIndexedVb( part.vertices2, part.indices2, DrawMode.Triangles, part.vCount2, part.iCount2 );
				info.IndicesCount2 = part.iCount2;
			}
			parts[i] = info;
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
		
		public override void BeginRender() {
			Graphics.BeginIndexedVbBatch();
		}
		
		public override void Render( ChunkPartInfo info ) {
			Graphics.DrawIndexedVbBatch( DrawMode.Triangles, info.VbId, info.IndicesCount );
		}
		
		public override void Render2( ChunkPartInfo info ) {
			Graphics.DrawIndexedVbBatch( DrawMode.Triangles, info.VbId2, info.IndicesCount2 );
		}
		
		public override void EndRender() {
			Graphics.EndIndexedVbBatch();
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
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;

			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
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
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U2, rec.V1, col );
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
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
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
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col );
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
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U2, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + count, rec.U2, rec.V2, col );
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
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1D info = drawInfoBuffer[drawInfoIndex];
			DrawInfo1DPart part = isTranslucent ? info.Translucent : info.Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + count, rec.U1, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col );
		}
		
		protected override void DrawSprite( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int drawInfoIndex;
			TextureRectangle rec = atlas.GetTexRec( texId, out drawInfoIndex );
			rec.U2 = count;
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
			}
			DrawInfo1DPart part = drawInfoBuffer[drawInfoIndex].Sprite;
			
			// Draw stretched Z axis
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U2, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 0.5f, rec.U1, rec.V2, col );
			
			// Draw X axis
			rec.U2 = 1;
			int startX = X;
			
			for( int i = 0; i < count; i++ ) {
				AddIndices( part );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U2, rec.V2, col );
				X++;
			}
			X = startX;
		}
		
		void AddIndices( DrawInfo1DPart part ) {
			int element = part.vIndex;
			if( element == maxIndices ) {
				part.indices = part.indices2;
				part.vertices = part.vertices2;
				part.iIndex = 0;
				part.vIndex = 0;
				element = 0;
			}
			part.indices[part.iIndex++] = (ushort)( element + 0 );
			part.indices[part.iIndex++] = (ushort)( element + 1 );
			part.indices[part.iIndex++] = (ushort)( element + 2 );
			
			part.indices[part.iIndex++] = (ushort)( element + 2 );
			part.indices[part.iIndex++] = (ushort)( element + 3 );
			part.indices[part.iIndex++] = (ushort)( element + 0 );
		}
	}
}