// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.TexturePack;

namespace ClassicalSharp {
	
	public unsafe partial class ChunkMeshBuilder {
		
		DrawInfo[] drawInfoNormal, drawInfoTranslucent;
		TerrainAtlas1D atlas;
		int arraysCount = 0;
		bool fullBright;
		int lightFlags;

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = game.TerrainAtlas1D.TexIds.Length;
			if( arraysCount == newArraysCount ) return;
			arraysCount = newArraysCount;
			Array.Resize( ref drawInfoNormal, arraysCount );
			Array.Resize( ref drawInfoTranslucent, arraysCount );
			
			for( int i = 0; i < drawInfoNormal.Length; i++ ) {
				if( drawInfoNormal[i] != null ) continue;
				drawInfoNormal[i] = new DrawInfo();
				drawInfoTranslucent[i] = new DrawInfo();
			}
		}
		
		public void Dispose() {
			game.Events.TerrainAtlasChanged -= TerrainAtlasChanged;
		}
		
		[StructLayout( LayoutKind.Sequential )]
		struct DrawInfoFaceData {
			public int left, right, front, back, bottom, top;
		}
		
		class DrawInfo {
			public VertexP3fT2fC4b[] vertices;
			public DrawInfoFaceData vIndex, sIndex, vCount;
			public int iCount, spriteCount;
			
			public void ExpandToCapacity() {
				int vertCount = iCount / 6 * 4;
				if( vertices == null || (vertCount + 2) > vertices.Length ) {
					vertices = new VertexP3fT2fC4b[vertCount + 2]; 
					// ensure buffer is up to 64 bits aligned for last element
				}	
				
				// Adjust for the fact that we group all vertices by face.
				sIndex.left = (spriteCount / 6) * 0;
				sIndex.right = (spriteCount / 6) * 1;
				sIndex.front = (spriteCount / 6) * 2;
				sIndex.back = (spriteCount / 6) * 3;
				
				vIndex.left = (spriteCount / 6) * 4;
				vIndex.right = vIndex.left + vCount.left / 6 * 4;
				vIndex.front = vIndex.right + vCount.right / 6 * 4;
				vIndex.back = vIndex.front + vCount.front / 6 * 4;
				vIndex.bottom = vIndex.back + vCount.back / 6 * 4;
				vIndex.top = vIndex.bottom + vCount.bottom / 6 * 4;
			}
			
			public void ResetState() {
				iCount = 0; spriteCount = 0;
				vIndex = new DrawInfoFaceData();
				vCount = new DrawInfoFaceData();
				sIndex = new DrawInfoFaceData();
			}
		}
		
		int[] offsets = { -1, 1, -extChunkSize, extChunkSize, -extChunkSize2, extChunkSize2 };
		bool CanStretch( byte initialTile, int chunkIndex, int x, int y, int z, int face ) {
			byte tile = chunk[chunkIndex];
			return tile == initialTile && !info.IsFaceHidden( tile, chunk[chunkIndex + offsets[face]], face )
				&& (fullBright || IsLit( X, Y, Z, face, initialTile ) == IsLit( x, y, z, face, tile ) );
		}
		
		bool IsLit( int x, int y, int z, int face, byte type ) {
			int offset = (info.LightOffset[type] >> face) & 1;
			switch( face ) {
				case TileSide.Left:
					return x < offset || y > map.heightmap[(z * width) + (x - offset)];
				case TileSide.Right:
					return x > (maxX - offset) || y > map.heightmap[(z * width) + (x + offset)];
				case TileSide.Front:
					return z < offset || y > map.heightmap[((z - offset) * width) + x];
				case TileSide.Back:
					return z > (maxZ - offset) || y > map.heightmap[((z + offset) * width) + x];
				case TileSide.Bottom:
					return y <= 0 || (y - 1 - offset) >= (map.heightmap[(z * width) + x]);
				case TileSide.Top:
					return y >= maxY || (y - offset) >= (map.heightmap[(z * width) + x]);
			}
			return true;
		}
		
		void SetPartInfo( DrawInfo part, int i, ref ChunkPartInfo[] parts ) {
			if( part.iCount == 0 ) return;
			
			ChunkPartInfo info;
			int vertCount = (part.iCount / 6 * 4) + 2;
			info.VbId = graphics.CreateVb( part.vertices, VertexFormat.P3fT2fC4b, vertCount );
			info.IndicesCount = part.iCount;
			info.LeftCount = (ushort)part.vCount.left; info.RightCount = (ushort)part.vCount.right;
			info.FrontCount = (ushort)part.vCount.front; info.BackCount = (ushort)part.vCount.back;
			info.BottomCount = (ushort)part.vCount.bottom; info.TopCount = (ushort)part.vCount.top;
			info.SpriteCount = part.spriteCount;
			
			info.LeftIndex = info.SpriteCount;
			info.RightIndex = info.LeftIndex + info.LeftCount;
			info.FrontIndex = info.RightIndex + info.RightCount;
			info.BackIndex = info.FrontIndex + info.FrontCount;
			info.BottomIndex = info.BackIndex + info.BackCount;
			info.TopIndex = info.BottomIndex + info.BottomCount;
			
			// Lazy initalize part arrays so we can save time in MapRenderer for chunks that only contain 1 or 2 part types.
			if( parts == null )
				parts = new ChunkPartInfo[arraysCount];
			parts[i] = info;
		}
		
		bool isTranslucent;
		float invVerElementSize;
		int elementsPerAtlas1D;
		
		void PreStretchTiles( int x1, int y1, int z1 ) {
			atlas = game.TerrainAtlas1D;
			invVerElementSize = atlas.invElementSize;
			elementsPerAtlas1D = atlas.elementsPerAtlas1D;
			arraysCount = atlas.TexIds.Length;
			
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
		
		void AddSpriteVertices( byte tile ) {
			int i = atlas.Get1DIndex( info.GetTextureLoc( tile, TileSide.Left ) );
			DrawInfo part = drawInfoNormal[i];
			part.spriteCount += 6 * 4;
			part.iCount += 6 * 4;
		}
		
		unsafe void AddVertices( byte tile, int count, int face ) {
			int i = atlas.Get1DIndex( info.GetTextureLoc( tile, face ) );
			DrawInfo part = info.IsTranslucent[tile] ? drawInfoTranslucent[i] : drawInfoNormal[i];
			part.iCount += 6;

			DrawInfoFaceData counts = part.vCount;
			*(&counts.left + face) += 6;
			part.vCount = counts;
		}
		
		void DrawLeftFace( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Left];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> TileSide.Left) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White :
				X >= offset ? (Y > map.heightmap[(Z * width) + (X - offset)] ? map.SunlightXSide : map.ShadowlightXSide) : map.SunlightXSide;
			
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2 + (count - 1), u2, v1, col );
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col );
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v2, col );
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2 + (count - 1), u2, v2, col );
		}

		void DrawRightFace( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> TileSide.Right) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White :
				X <= (maxX - offset) ? (Y > map.heightmap[(Z * width) + (X + offset)] ? map.SunlightXSide : map.ShadowlightXSide) : map.SunlightXSide;
			
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col );
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col );
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col );
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col );
		}

		void DrawFrontFace( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Front];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> TileSide.Front) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White :
				Z >= offset ? (Y > map.heightmap[((Z - offset) * width) + X] ? map.SunlightZSide : map.ShadowlightZSide) : map.SunlightZSide;
			
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col );
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col );
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col );
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col );
		}
		
		void DrawBackFace( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Back];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> TileSide.Back) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White :
				Z <= (maxZ - offset) ? (Y > map.heightmap[((Z + offset) * width) + X] ? map.SunlightZSide : map.ShadowlightZSide) : map.SunlightZSide;
			
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col );
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col );
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
		}
		
		void DrawBottomFace( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Bottom];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> TileSide.Bottom) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White : ((Y - 1 - offset) >= map.heightmap[(Z * width) + X] ? map.SunlightYBottom : map.ShadowlightYBottom);
			
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col );
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col );
		}

		void DrawTopFace( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Top];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> TileSide.Top) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? drawInfoTranslucent[i] : drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White : ((Y - offset) >= map.heightmap[(Z * width) + X] ? map.Sunlight : map.Shadowlight);

			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u2, v1, col );
			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col );
			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v2, col );
			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v2, col );
		}
		
		void DrawSprite( int count ) {
			int texId = info.textures[tile * TileSide.Sides + TileSide.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			float blockHeight = 1;
			
			float u1 = 0, u2 = 1 * 15.99f/16f;
			float v1 = vOrigin, v2 = vOrigin + invVerElementSize * 15.99f/16f;
			DrawInfo part = drawInfoNormal[i];
			FastColour col = fullBright ? FastColour.White : (Y > map.heightmap[(Z * width) + X] ? map.Sunlight : map.Shadowlight);
			
			// Draw Z axis
			part.vertices[part.sIndex.left++] = new VertexP3fT2fC4b( X + 2.50f/16, Y, Z + 2.5f/16, u2, v2, col );
			part.vertices[part.sIndex.left++] = new VertexP3fT2fC4b( X + 2.50f/16, Y + blockHeight, Z + 2.5f/16, u2, v1, col );
			part.vertices[part.sIndex.left++] = new VertexP3fT2fC4b( X + 13.5f/16, Y + blockHeight, Z + 13.5f/16, u1, v1, col );
			part.vertices[part.sIndex.left++] = new VertexP3fT2fC4b( X + 13.5f/16, Y, Z + 13.5f/16, u1, v2, col );
			
			// Draw Z axis mirrored
			part.vertices[part.sIndex.right++] = new VertexP3fT2fC4b( X + 13.5f/16, Y, Z + 13.5f/16, u2, v2, col );
			part.vertices[part.sIndex.right++] = new VertexP3fT2fC4b( X + 13.5f/16, Y + blockHeight, Z + 13.5f/16, u2, v1, col );
			part.vertices[part.sIndex.right++] = new VertexP3fT2fC4b( X + 2.50f/16, Y + blockHeight, Z + 2.5f/16, u1, v1, col );
			part.vertices[part.sIndex.right++] = new VertexP3fT2fC4b( X + 2.50f/16, Y, Z + 2.5f/16, u1, v2, col );

			// Draw X axis
			part.vertices[part.sIndex.front++] = new VertexP3fT2fC4b( X + 2.50f/16, Y, Z + 13.5f/16, u2, v2, col );
			part.vertices[part.sIndex.front++] = new VertexP3fT2fC4b( X + 2.50f/16, Y + blockHeight, Z + 13.5f/16, u2, v1, col );
			part.vertices[part.sIndex.front++] = new VertexP3fT2fC4b( X + 13.5f/16, Y + blockHeight, Z + 2.5f/16, u1, v1, col );
			part.vertices[part.sIndex.front++] = new VertexP3fT2fC4b( X + 13.5f/16, Y, Z + 2.5f/16, u1, v2, col );
			
			// Draw X axis mirrored
			part.vertices[part.sIndex.back++] = new VertexP3fT2fC4b( X + 13.5f/16, Y, Z + 2.5f/16, u2, v2, col );
			part.vertices[part.sIndex.back++] = new VertexP3fT2fC4b( X + 13.5f/16, Y + blockHeight, Z + 2.5f/16, u2, v1, col );
			part.vertices[part.sIndex.back++] = new VertexP3fT2fC4b( X + 2.50f/16, Y + blockHeight, Z + 13.5f/16, u1, v1, col );
			part.vertices[part.sIndex.back++] = new VertexP3fT2fC4b( X + 2.50f/16, Y, Z + 13.5f/16, u1, v2, col );
		}
	}
}