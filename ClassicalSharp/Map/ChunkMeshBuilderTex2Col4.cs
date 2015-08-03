using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {
		
		DrawInfo[] drawInfoNormal, drawInfoTranslucent;
		TerrainAtlas1D atlas;
		int arraysCount = 0;

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = Window.TerrainAtlas1D.TexIds.Length;
			if( arraysCount > 0 && arraysCount != newArraysCount ) {
				Array.Resize( ref drawInfoNormal, newArraysCount );
				Array.Resize( ref drawInfoTranslucent, newArraysCount );
				for( int i = arraysCount; i < drawInfoNormal.Length; i++ ) {
					drawInfoNormal[i] = new DrawInfo();
					drawInfoTranslucent[i] = new DrawInfo();
				}
			}
			arraysCount = newArraysCount;
		}
		
		public void Dispose() {
			Window.TerrainAtlasChanged -= TerrainAtlasChanged;
		}
		
		[StructLayout( LayoutKind.Sequential )]
		struct DrawInfoFaceData {
			public int left, right, front, back, bottom, top;
		}
		
		class DrawInfo {
			public VertexPos3fTex2fCol4b[] vertices;
			public int vCount, iCount;
			public DrawInfoFaceData vIndex;
			public DrawInfoFaceData Count;
			public int spriteIndex, spriteCount;
			
			public void ExpandToCapacity() {
				vCount = ( iCount / 6 ) * 4;

				if( vertices == null || vCount > vertices.Length ) {
					vertices = new VertexPos3fTex2fCol4b[vCount];
				}				
				vIndex.left = spriteCount / 6 * 4;
				vIndex.right = vIndex.left + Count.left / 6 * 4;
				vIndex.front = vIndex.right + Count.right / 6 * 4;
				vIndex.back = vIndex.front + Count.front / 6 * 4;
				vIndex.bottom = vIndex.back + Count.back / 6 * 4;
				vIndex.top = vIndex.bottom + Count.bottom / 6 * 4;
			}
			
			public void ResetState() {
				vCount = iCount = 0;
				spriteIndex = spriteCount = 0;
				vIndex = new DrawInfoFaceData();
				Count = new DrawInfoFaceData();
			}
		}
		
		bool CanStretch( byte initialTile, int chunkIndex, int x, int y, int z, int face ) {
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
		
		void GetChunkInfo( int x, int y, int z, ref ChunkPartInfo[] normalParts, ref ChunkPartInfo[] translucentParts ) {
			for( int i = 0; i < arraysCount; i++ ) {
				SetPartInfo( drawInfoNormal[i], i, ref normalParts );
				SetPartInfo( drawInfoTranslucent[i], i, ref translucentParts );
			}
		}
		
		void SetPartInfo( DrawInfo part, int i, ref ChunkPartInfo[] parts ) {
			if( part.iCount == 0 ) return;
			
			ChunkPartInfo info;
			info.VbId = Graphics.InitVb( part.vertices, VertexFormat.Pos3fTex2fCol4b, part.vCount );
			info.IndicesCount = part.iCount;
			info.leftCount = (ushort)part.Count.left; info.rightCount = (ushort)part.Count.right;
			info.frontCount = (ushort)part.Count.front; info.backCount = (ushort)part.Count.back;
			info.bottomCount = (ushort)part.Count.bottom; info.topCount = (ushort)part.Count.top;
			info.spriteCount = (ushort)part.spriteCount;
			
			info.leftIndex = info.spriteCount;
			info.rightIndex = info.leftIndex + info.leftCount;
			info.frontIndex = info.rightIndex + info.rightCount;
			info.backIndex = info.frontIndex + info.frontCount;
			info.bottomIndex = info.backIndex + info.backCount;
			info.topIndex = info.bottomIndex + info.bottomCount;
			
			// Lazy initalize part arrays so we can save time in MapRenderer for chunks that only contain 1 or 2 part types.
			if( parts == null )
				parts = new ChunkPartInfo[arraysCount];
			parts[i] = info;
		}
		
		bool isTranslucent;
		float blockHeight;
		float invVerElementSize;
		
		void PreStretchTiles( int x1, int y1, int z1 ) {
			invVerElementSize = Window.TerrainAtlas1D.invElementSize;
			arraysCount = Window.TerrainAtlas1D.TexIds.Length;
			atlas = Window.TerrainAtlas1D;
			
			if( drawInfoNormal == null ) {
				drawInfoNormal = new DrawInfo[arraysCount];
				drawInfoTranslucent = new DrawInfo[arraysCount];
				for( int i = 0; i < drawInfoNormal.Length; i++ ) {
					drawInfoNormal[i] = new DrawInfo();
					drawInfoTranslucent[i] = new DrawInfo();
				}
			} else {
				for( int i = 0; i < drawInfoNormal.Length; i++ ) {
					drawInfoNormal[i].ResetState();
					drawInfoTranslucent[i].ResetState();
				}
			}
		}
		
		void PostStretchTiles( int x1, int y1, int z1 ) {
			for( int i = 0; i < drawInfoNormal.Length; i++ ) {
				drawInfoNormal[i].ExpandToCapacity();
				drawInfoTranslucent[i].ExpandToCapacity();
			}
		}
		
		void AddSpriteVertices( byte tile, int count ) {
			int i = atlas.Get1DIndex( BlockInfo.GetOptimTextureLoc( tile, TileSide.Left ) );
			DrawInfo part = drawInfoNormal[i];
			part.spriteCount += 6 + 6 * count;
			part.iCount += 6 + 6 * count;
		}
		
		unsafe void AddVertices( byte tile, int count, int face ) {
			int i = atlas.Get1DIndex( BlockInfo.GetOptimTextureLoc( tile, face ) );
			DrawInfo part = BlockInfo.IsTranslucent( tile ) ? drawInfoTranslucent[i] : drawInfoNormal[i];
			part.iCount += 6;
			
			DrawInfoFaceData counts = part.Count;
			int* ptr = &counts.left;
			ptr += face;
			*ptr += 6;
			part.Count = counts;
		}
		
		void DrawLeftFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Left );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColourAdj( X - 1, Y, Z, ref map.SunlightXSide, ref map.ShadowlightXSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			
			part.vertices[part.vIndex.left++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + count, rec.U2, rec.V1, col );
			part.vertices[part.vIndex.left++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.left++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.left++] = new VertexPos3fTex2fCol4b( X, Y, Z + count, rec.U2, rec.V2, col );
		}

		void DrawRightFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColourAdj( X + 1, Y, Z, ref map.SunlightXSide, ref map.ShadowlightXSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			
			part.vertices[part.vIndex.right++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex.right++] = new VertexPos3fTex2fCol4b( X + 1, Y + blockHeight, Z + count, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.right++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z + count, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.right++] = new VertexPos3fTex2fCol4b( X + 1, Y, Z, rec.U2, rec.V2, col );
		}
		
		void DrawBackFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Back );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColourAdj( X, Y, Z + 1, ref map.SunlightZSide, ref map.ShadowlightZSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			
			part.vertices[part.vIndex.back++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
			part.vertices[part.vIndex.back++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.back++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.back++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
		}

		void DrawFrontFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Front );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColourAdj( X, Y, Z - 1, ref map.SunlightZSide, ref map.ShadowlightZSide );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			if( blockHeight != 1 ) {
				rec.V2 = rec.V1 + blockHeight * invVerElementSize;
			}
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			
			part.vertices[part.vIndex.front++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.front++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U2, rec.V2, col );
			part.vertices[part.vIndex.front++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex.front++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U1, rec.V1, col );
		}
		
		void DrawBottomFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Bottom );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColour( X, Y - 1, Z, ref map.SunlightYBottom, ref map.ShadowlightYBottom );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U2, rec.V1, col );
		}

		void DrawTopFace( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Top );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
				isTranslucent = BlockInfo.IsTranslucent( tile );
			}
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];

			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
		}
		
		void DrawSprite( int count ) {
			int texId = BlockInfo.GetOptimTextureLoc( tile, TileSide.Right );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = GetColour( X, Y + 1, Z, ref map.Sunlight, ref map.Shadowlight );
			if( blockHeight == -1 ) {
				blockHeight = BlockInfo.BlockHeight( tile );
			}
			DrawInfo part = drawInfoNormal[i];
			
			// Draw stretched Z axis
			part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X, Y, Z + 0.5f, rec.U2, rec.V2, col );
			part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 0.5f, rec.U2, rec.V1, col );
			part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 0.5f, rec.U1, rec.V1, col );
			part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 0.5f, rec.U1, rec.V2, col );
			
			// Draw X axis
			rec.U2 = 1;
			int startX = X;
			
			for( int j = 0; j < count; j++ ) {
				part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z, rec.U1, rec.V2, col );
				part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z, rec.U1, rec.V1, col );
				part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y + blockHeight, Z + 1, rec.U2, rec.V1, col );
				part.vertices[part.spriteIndex++] = new VertexPos3fTex2fCol4b( X + 0.5f, Y, Z + 1, rec.U2, rec.V2, col );
				X++;
			}
			X = startX;
		}
	}
}