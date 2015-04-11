using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ChunkMeshBuilderTex2Col4 : ChunkMeshBuilder {
		
		MapPackedShader shader;
		MapPackedLiquidDepthShader liquidShader;
		TextureAtlas2D atlas;
		
		public ChunkMeshBuilderTex2Col4( Game window, MapRenderer renderer ) : base( window ) {
			shader = renderer.packedShader;
			liquidShader = renderer.transluscentShader;
			atlas = window.TerrainAtlas;
		}

		DrawInfoPart Solid = new DrawInfoPart();
		DrawInfoPart Translucent = new DrawInfoPart();
		DrawInfoPart Sprite = new DrawInfoPart();
		
		class DrawInfoPart {
			public uint[] vertices;
			public int verticesIndex, verticesCount;
			public ushort[] indices;
			public int indicesIndex, indicesCount;
			
			public DrawInfoPart() {
				vertices = new uint[0];
				indices = new ushort[0];
			}
			
			public void ExpandToCapacity() {
				if( verticesCount > vertices.Length ) {
					vertices = new uint[verticesCount];
				}
				if( indicesCount > indices.Length ) {
					indices = new ushort[indicesCount];
				}
				if( indicesCount >= ushort.MaxValue ) {
					Utils.LogWarning( "Indices overflow: " + indicesCount );
				}
			}
			
			public void ResetState() {
				verticesIndex = 0;
				verticesCount = 0;
				indicesIndex = 0;
				indicesCount = 0;
			}
		}
		
		public override bool UsesLighting {
			get { return true; }
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
		
		int GetColour( int x, int y, int z, int type ) {
			if( !map.IsValidPos( x, y, z ) ) return type;
			return y > GetLightHeight( x, z ) ? type : type + 4;
		}
		
		int GetColourAdj( int x, int y, int z, int type ) {
			if( !map.IsValidPos( x, y, z ) ) return type;
			return y > GetLightHeightAdj( x, z ) ? type : type + 4;
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
			ChunkDrawInfo info = new ChunkDrawInfo();
			info.SolidPart = GetPartInfo( Solid );
			info.TranslucentPart = GetPartInfo( Translucent );
			info.SpritePart = GetPartInfo( Sprite );
			return info;
		}
		
		ChunkPartInfo GetPartInfo( DrawInfoPart part ) {
			ChunkPartInfo info = default( ChunkPartInfo );
			if( part.verticesCount > 0 ) {
				info.Id = Graphics.InitIndexedVb( part.vertices, part.verticesCount,
				                                 VertexFormat.VertexPos3fTex2fCol4b, part.indices, part.indicesCount );
				info.IndicesCount = part.indicesCount;
			}
			return info;
		}
		
		bool isTranslucent;
		byte yOffsetType;
		protected override void PreRenderTile() {
			yOffsetType = 0xFF;
		}
		
		protected override void PreStretchTiles( int x1, int y1, int z1 ) {
			Solid.ResetState();
			Sprite.ResetState();
			Translucent.ResetState();
		}
		
		protected override void PostStretchTiles( int x1, int y1, int z1 ) {
			Solid.ExpandToCapacity();
			Sprite.ExpandToCapacity();
			Translucent.ExpandToCapacity();
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		public override void Render( ChunkPartInfo info ) {
			Graphics.BindVb( info.Id.VbId );
			Graphics.BindIb( info.Id.IbId );
			
			Graphics.EnableAndUIntSetVertexAttribPointer( shader.flagsLoc, 1, 4, 0 );
			Graphics.DrawIndexedVb( DrawMode.Triangles, info.IndicesCount );
			Graphics.DisableVertexAttribArray( shader.flagsLoc );
		}
		
		public override void RenderLiquidDepthPass( ChunkPartInfo info ) {
			Graphics.BindVb( info.Id.VbId );
			Graphics.BindIb( info.Id.IbId );
			
			Graphics.EnableAndUIntSetVertexAttribPointer( liquidShader.flagsLoc, 1, 4, 0 );
			Graphics.DrawIndexedVb( DrawMode.Triangles, info.IndicesCount );
			Graphics.DisableVertexAttribArray( liquidShader.flagsLoc );
		}
		
		protected override void AddSpriteVertices( byte tile, int count ) {
			Sprite.verticesCount += 4 + 4 * count;
			Sprite.indicesCount += 6 + 6 * count;
		}
		
		protected override void AddVertices( byte tile, int count, int face ) {
			DrawInfoPart part = BlockInfo.IsTranslucent( tile ) ? Translucent : Solid;
			part.indicesCount += 6;
			part.verticesCount += 4;
		}

		const int typeYTop = 0, typeZSide = 1, typeXSide = 2, typeYBottom = 3;
		protected override void DrawTopFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Top );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColour( X, Y + 1, Z, typeYTop );
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfoPart part = isTranslucent ? Translucent : Solid;

			AddIndices( part );
			byte type = yOffsetType;
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ, type, texX, texY, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ, type, texX, texY, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ + 1, type, texX, texY + 1, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ + 1, type, texX, texY + 1, count, col );
			
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U2, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
		}

		protected override void DrawBottomFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Bottom );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColour( X, Y - 1, Z, typeYBottom );
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfoPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ + 1, 0, texX, texY + 1, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ + 1, 0, texX, texY + 1, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ, 0, texX, texY, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ, 0, texX, texY, count, col );
			
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U2, rec.V1, col );
		}

		protected override void DrawBackFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Back );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColourAdj( X, Y, Z + 1, typeZSide );
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfoPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			byte type = yOffsetType;
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ + 1, type, texX, texY, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ + 1, type, texX, texY, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ + 1, 0, texX, texY + 1, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ + 1, 0, texX, texY + 1, count, col );
			
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
		}

		protected override void DrawFrontFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Front );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColourAdj( X, Y, Z - 1, typeZSide );
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfoPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			byte type = yOffsetType;
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ, 0, texX, texY + 1, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ, 0, texX, texY + 1, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ, type, texX, texY, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX + count, YY, ZZ, type, texX, texY, 0, col );
			
			
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col );
		}

		protected override void DrawLeftFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColourAdj( X - 1, Y, Z, typeXSide );
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfoPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			byte type = yOffsetType;
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ + count, type, texX, texY, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ, type, texX, texY, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ, 0, texX, texY + 1, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX, YY, ZZ + count, 0, texX, texY + 1, count, col );
			
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U2, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + count, rec.U2, rec.V2, col );
		}

		protected override void DrawRightFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColourAdj( X + 1, Y, Z, typeXSide );
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfoPart part = isTranslucent ? Translucent : Solid;
			
			AddIndices( part );
			byte type = yOffsetType;
			part.vertices[part.verticesIndex++] = Pack( XX + 1, YY, ZZ, type, texX, texY, count, col );
			part.vertices[part.verticesIndex++] = Pack( XX + 1, YY, ZZ + count, type, texX, texY, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX + 1, YY, ZZ + count, 0, texX, texY + 1, 0, col );
			part.vertices[part.verticesIndex++] = Pack( XX + 1, YY, ZZ, 0, texX, texY + 1, count, col );
			
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + count, rec.U1, rec.V2, col );
			//part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col );
		}
		
		protected override void DrawSprite( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int texX, texY;
			atlas.GetCoords( texId, out texX, out texY );
			int col = GetColour( X, Y + 1, Z, typeYTop ) != 0 ? 1 : 0;
			if( yOffsetType == 0xFF ) {
				yOffsetType = BlockInfo.YOffsetType( tile );
			}
			DrawInfoPart part = Sprite;
			
			// Draw stretched Z axis
			AddIndices( part );
			byte type = yOffsetType;
			part.vertices[part.verticesIndex++] = PackSprite( XX, YY, ZZ, 0, texX, texY + 1, count, col, 0, 1 );
			part.vertices[part.verticesIndex++] = PackSprite( XX, YY, ZZ, type, texX, texY, count, col, 0, 1 );
			part.vertices[part.verticesIndex++] = PackSprite( XX + count, YY, ZZ, type, texX, texY, 0, col, 0, 1 );
			part.vertices[part.verticesIndex++] = PackSprite( XX + count, YY, ZZ, 0, texX, texY + 1, 0, col, 0, 1 );
			
			/*part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U2, rec.V1, col );
			part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 0.5f, rec.U1, rec.V2, col );*/
			
			// Draw X axis
			int startX = XX;			
			for( int i = 0; i < count; i++ ) {
				AddIndices( part );
				part.vertices[part.verticesIndex++] = PackSprite( XX, YY, ZZ, 0, texX, texY + 1, 0, col, 1, 0 );
				part.vertices[part.verticesIndex++] = PackSprite( XX, YY, ZZ, type, texX, texY, 0, col, 1, 0 );
				part.vertices[part.verticesIndex++] = PackSprite( XX, YY, ZZ + 1, type, texX, texY, 1, col, 1, 0 );
				part.vertices[part.verticesIndex++] = PackSprite( XX, YY, ZZ + 1, 0, texX, texY + 1, 1, col, 1, 0 );
				
				/*part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
				part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col );
				part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
				part.vertices[part.verticesIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U2, rec.V2, col );*/
				XX++;
			}
			XX = startX;
		}
		
		void AddIndices( DrawInfoPart part ) {
			int startIndex = part.verticesIndex;
			part.indices[part.indicesIndex++] = (ushort)( startIndex + 0 );
			part.indices[part.indicesIndex++] = (ushort)( startIndex + 1 );
			part.indices[part.indicesIndex++] = (ushort)( startIndex + 2 );
			
			part.indices[part.indicesIndex++] = (ushort)( startIndex + 2 );
			part.indices[part.indicesIndex++] = (ushort)( startIndex + 3 );
			part.indices[part.indicesIndex++] = (ushort)( startIndex + 0 );
		}
		
		static uint Pack( int xx, int yy, int zz, int yType, int texX, int texY, int texCount, int colourType ) {
			// Main position data (16 bits)
			//5: X in chunk
			//4: Y in chunk
			//2: Y offset (00: 0, 01: 0.125, 10: 0.5, 11: 1.0)
			//5: Z in chunk
			
			// Meta (16 bits)
			//4: tex X
			//3: tex Y
			//5: texCount
			//3: colour
			//1: unused
			return (uint)( xx | ( yy << 5 ) | ( yType << 9 ) | ( zz << 11 ) |
			              ( texX << 16 ) | ( texY << 20 ) | ( texCount << 23 ) |
			              ( colourType << 28 ) );
		}
		
		static uint PackSprite( int xx, int yy, int zz, int yType, int texX, int texY, int texCount, int colourType,
		                       int offsetX, int offsetZ ) {
			//1: colour
			//1: offsetX
			//1: offsetZ
			//1: unused
			return (uint)( xx | ( yy << 5 ) | ( yType << 9 ) | ( zz << 11 ) |
			              ( texX << 16 ) | ( texY << 20 ) | ( texCount << 23 ) |
			              ( colourType << 28 ) | ( offsetX << 29 ) | ( offsetZ << 30 ) );
		}
	}
}