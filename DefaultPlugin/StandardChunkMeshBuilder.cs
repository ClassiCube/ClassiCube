using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;

namespace DefaultPlugin {
	
	public class StandardChunkMeshBuilder : ChunkMeshBuilder {
		
		TerrainAtlas2D atlas;
		public StandardChunkMeshBuilder( Game window ) : base( window ) {
		}
		
		public override void Dispose() {
			base.Dispose();
		}
		
		DrawInfo1DPart Solid = new DrawInfo1DPart();
		DrawInfo1DPart Translucent = new DrawInfo1DPart();
		DrawInfo1DPart Sprite = new DrawInfo1DPart();
		
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
		
		protected override void GetChunkInfo( int x, int y, int z, ref ChunkPartInfo solidPart,
		                  ref ChunkPartInfo spritePart, ref ChunkPartInfo translucentPart ) {
			SetPartInfo( Solid,  ref solidPart );
			SetPartInfo( Sprite, ref spritePart );
			SetPartInfo( Translucent, ref translucentPart );
		}
		
		void SetPartInfo( DrawInfo1DPart part, ref ChunkPartInfo info ) {
			if( part.iCount == 0 ) return;
			
			info.VbId = Graphics.InitVb( part.vertices, VertexPos3fTex2fCol4b.Size, part.vCount );
			info.IbId = Graphics.InitIb( part.indices, part.iCount );
			info.IndicesCount = part.iCount;
		}
		
		bool isTranslucent;
		float invVerElementSize;	
		protected override void PreStretchTiles( int x1, int y1, int z1 ) {
			invVerElementSize = TerrainAtlas2D.usedInvVerElemSize;
			atlas = Window.TerrainAtlas;
			
			Solid.ResetState();
			Sprite.ResetState();
			Translucent.ResetState();
		}
		
		protected override void PostStretchTiles( int x1, int y1, int z1 ) {
			Solid.ExpandToCapacity();
			Sprite.ExpandToCapacity();
			Translucent.ExpandToCapacity();
		}
		
		protected override void AddSpriteVertices( byte tile, int count ) {
			Sprite.iCount += 6 + 6 * count;
		}
		
		protected override void AddVertices( byte tile, int count, int face ) {
			DrawInfo1DPart part = BlockInfo.IsTranslucent( tile ) ? Translucent : Solid;
			part.iCount += 6;
		}

		const byte baseCol = 0, xCol = 1, zCol = 2, yBottomCol = 3;
		protected override void DrawTopFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Top );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColour( X, Y + 1, Z, baseCol, baseCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;

			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U1, rec.V2, col, tile, (byte)count, 0 );
		}

		protected override void DrawBottomFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Bottom );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColour( X, Y - 1, Z, yBottomCol, yBottomCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U1, rec.V2, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V1, col, tile, (byte)count, 0 );
		}

		protected override void DrawBackFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Back );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColourAdj( X, Y, Z + 1, zCol, zCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U1, rec.V1, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U1, rec.V2, col, tile, (byte)count, 0 );
		}

		protected override void DrawFrontFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Front );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColourAdj( X, Y, Z - 1, zCol, zCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
		}

		protected override void DrawLeftFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColourAdj( X - 1, Y, Z, xCol, xCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U1, rec.V1, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + count, rec.U1, rec.V2, col, tile, (byte)count, 0 );
		}

		protected override void DrawRightFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColourAdj( X + 1, Y, Z, xCol, xCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo1DPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + count, rec.U1, rec.V2, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U1, rec.V2, col, tile, (byte)count, 0 );
		}
		
		protected override void DrawSprite( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			TextureRectangle rec = atlas.GetTexRec( texId );
			byte col = GetColour( X, Y + 1, Z, baseCol, baseCol + 4 );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
			}
			DrawInfo1DPart part = Sprite;
			
			// Draw stretched Z axis
			AddIndices( part );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U1, rec.V2, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col, tile, (byte)count, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col, tile, 0, 0 );
			part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 0.5f, rec.U1, rec.V2, col, tile, 0, 0 );
			
			// Draw X axis
			int startX = X;
			
			for( int i = 0; i < count; i++ ) {
				AddIndices( part );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col, tile, 0, 0 );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col, tile, 0, 0 );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U1, rec.V1, col, tile, (byte)count, 0 );
				part.vertices[part.vIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U1, rec.V2, col, tile, (byte)count, 0 );
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