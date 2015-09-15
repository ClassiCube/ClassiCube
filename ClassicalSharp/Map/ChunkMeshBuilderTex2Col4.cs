using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public partial class ChunkMeshBuilder {
		
		DrawInfo[] drawInfoNormal, drawInfoTranslucent;
		TerrainAtlas1D atlas;
		int arraysCount = 0;
		bool emitsLight;

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = game.TerrainAtlas1D.TexIds.Length;
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
			game.TerrainAtlasChanged -= TerrainAtlasChanged;
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
			return tile == initialTile && !info.IsFaceHidden( tile, GetNeighbour( chunkIndex, face ), face ) &&
				( IsLit( X, Y, Z, face ) == IsLit( x, y, z, face ) );
		}
		
		bool IsLit( int x, int y, int z, int face ) {
			switch( face ) {
				case TileSide.Left:
					return emitsLight || x <= 0 || y > map.heightmap[( z * width ) + (x - 1)];
					
				case TileSide.Right:
					return emitsLight || x >= maxX || y > map.heightmap[( z * width ) + (x + 1)];
					
				case TileSide.Front:
					return emitsLight || z <= 0 || y > map.heightmap[( (z - 1) * width ) + x];
					
				case TileSide.Back:
					return emitsLight || z >= maxZ || y > map.heightmap[( (z + 1) * width ) + x];
					
				case TileSide.Bottom:
					return emitsLight || y <= 0 || y > map.heightmap[( z * width ) + x];
					
				case TileSide.Top:
					return emitsLight || y >= maxY || y > map.heightmap[( z * width ) + x];
			}
			return true;
		}
		
		void SetPartInfo( DrawInfo part, int i, ref ChunkPartInfo[] parts ) {
			if( part.iCount == 0 ) return;
			
			ChunkPartInfo info;
			info.VbId = graphics.CreateVb( part.vertices, VertexFormat.Pos3fTex2fCol4b, part.vCount );
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
			invVerElementSize = game.TerrainAtlas1D.invElementSize;
			arraysCount = game.TerrainAtlas1D.TexIds.Length;
			atlas = game.TerrainAtlas1D;
			
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
			int i = atlas.Get1DIndex( info.GetTextureLoc( tile, TileSide.Left ) );
			DrawInfo part = drawInfoNormal[i];
			part.spriteCount += 6 + 6 * count;
			part.iCount += 6 + 6 * count;
		}
		
		unsafe void AddVertices( byte tile, int count, int face ) {
			int i = atlas.Get1DIndex( info.GetTextureLoc( tile, face ) );
			DrawInfo part = info.IsTranslucent( tile ) ? drawInfoTranslucent[i] : drawInfoNormal[i];
			part.iCount += 6;

			DrawInfoFaceData counts = part.Count;
			*( &counts.left + face ) += 6;
			part.Count = counts;
		}
		
		void DrawLeftFace( int count ) {
			int texId = info.GetTextureLoc( tile, TileSide.Left );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = emitsLight ? FastColour.White : 
				X > 0 ? ( Y > map.heightmap[( Z * width ) + (X - 1)] ? map.SunlightXSide : map.ShadowlightXSide ) : map.SunlightXSide;
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
			int texId = info.GetTextureLoc( tile, TileSide.Right );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = emitsLight ? FastColour.White : 
				X < maxX ? ( Y > map.heightmap[( Z * width ) + (X + 1)] ? map.SunlightXSide : map.ShadowlightXSide ) : map.SunlightXSide;
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
			int texId = info.GetTextureLoc( tile, TileSide.Back );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = emitsLight ? FastColour.White : 
				Z < maxZ ? ( Y > map.heightmap[( (Z + 1) * width ) + X] ? map.SunlightZSide : map.ShadowlightZSide ) : map.SunlightZSide;
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
			int texId = info.GetTextureLoc( tile, TileSide.Front );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = emitsLight ? FastColour.White : 
				Z > 0 ? ( Y > map.heightmap[( (Z - 1) * width ) + X] ? map.SunlightZSide : map.ShadowlightZSide ) : map.SunlightZSide;
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
			int texId = info.GetTextureLoc( tile, TileSide.Bottom );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = emitsLight ? FastColour.White : 
				Y > 0 ? ( (Y - 1) > map.heightmap[( Z * width ) + X] ? map.SunlightYBottom : map.ShadowlightYBottom ) : map.SunlightYBottom;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X + count, Y, Z + 1, rec.U2, rec.V2, col );
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X, Y, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X, Y, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.bottom++] = new VertexPos3fTex2fCol4b( X + count, Y, Z, rec.U2, rec.V1, col );
		}

		void DrawTopFace( int count ) {
			int texId = info.GetTextureLoc( tile, TileSide.Top );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = emitsLight ? FastColour.White : 
				Y < maxY ? ( Y > map.heightmap[( Z * width ) + X] ? map.Sunlight : map.Shadowlight ) : map.Sunlight;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];

			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z, rec.U2, rec.V1, col );
			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z, rec.U1, rec.V1, col );
			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X, Y + blockHeight, Z + 1, rec.U1, rec.V2, col );
			part.vertices[part.vIndex.top++] = new VertexPos3fTex2fCol4b( X + count, Y + blockHeight, Z + 1, rec.U2, rec.V2, col );
		}
		
		void DrawSprite( int count ) {
			int texId = info.GetTextureLoc( tile, TileSide.Right );
			int i;
			TextureRectangle rec = atlas.GetTexRec( texId, count, out i );
			FastColour col = Y < maxY ? ( emitsLight || Y > map.heightmap[( Z * width ) + X] ? map.Sunlight : map.Shadowlight ) 
				: map.Sunlight;
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