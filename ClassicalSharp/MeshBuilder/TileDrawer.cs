// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.TexturePack;
using OpenTK;

namespace ClassicalSharp {
	
	public unsafe partial class ChunkMeshBuilder {
		
		DrawInfo[] normalParts, translucentParts;
		TerrainAtlas1D atlas;
		int arraysCount = 0;
		bool fullBright;
		int lightFlags;

		void TerrainAtlasChanged( object sender, EventArgs e ) {
			int newArraysCount = game.TerrainAtlas1D.TexIds.Length;
			if( arraysCount == newArraysCount ) return;
			arraysCount = newArraysCount;
			Array.Resize( ref normalParts, arraysCount );
			Array.Resize( ref translucentParts, arraysCount );
			
			for( int i = 0; i < normalParts.Length; i++ ) {
				if( normalParts[i] != null ) continue;
				normalParts[i] = new DrawInfo();
				translucentParts[i] = new DrawInfo();
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
		
		Vector3 minBB, maxBB;
		public void RenderTile( int index, int x, int y, int z ) {
			X = x; Y = y; Z = z;
			
			if( info.IsSprite[curBlock] ) {
				fullBright = info.FullBright[curBlock];
				int count = counts[index + Side.Top];
				if( count != 0 )
					DrawSprite( count );
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if( leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			   backCount == 0 && bottomCount == 0 && topCount == 0 ) return;
			
			fullBright = info.FullBright[curBlock];
			isTranslucent = info.IsTranslucent[curBlock];
			lightFlags = info.LightOffset[curBlock];
			
			Vector3 min = info.MinBB[curBlock], max = info.MaxBB[curBlock];
			x1 = x + min.X; y1 = y + min.Y; z1 = z + min.Z;
			x2 = x + max.X; y2 = y + max.Y; z2 = z + max.Z;

			if( curBlock >= Block.Water && curBlock <= Block.StillLava ) {
				x1 -= 0.1f/16; x2 -= 0.1f/16f; 
				z1 -= 0.1f/16f; z2 -= 0.1f/16f;
				y1 -= 1.5f/16; y2 -= 1.5f/16;
			} else if( isTranslucent && info.Collide[curBlock] != CollideType.Solid ) {
				x1 += 0.1f/16; x2 += 0.1f/16f; 
				z1 += 0.1f/16f; z2 += 0.1f/16f;
				y1 -= 0.1f/16; y2 -= 0.1f/16f;
			}
			
			this.minBB = min; this.maxBB = max;
			minBB.Y = 1 - minBB.Y; maxBB.Y = 1 - maxBB.Y;
			
			if( leftCount != 0 ) DrawLeftFace( leftCount );
			if( rightCount != 0 ) DrawRightFace( rightCount );
			if( frontCount != 0 ) DrawFrontFace( frontCount );
			if( backCount != 0 ) DrawBackFace( backCount );
			if( bottomCount != 0 ) DrawBottomFace( bottomCount );
			if( topCount != 0 ) DrawTopFace( topCount );
		}
		
		bool isTranslucent;
		float invVerElementSize;
		int elementsPerAtlas1D;
		
		void PreStretchTiles( int x1, int y1, int z1 ) {
			atlas = game.TerrainAtlas1D;
			invVerElementSize = atlas.invElementSize;
			elementsPerAtlas1D = atlas.elementsPerAtlas1D;
			arraysCount = atlas.TexIds.Length;
			
			if( normalParts == null ) {
				normalParts = new DrawInfo[arraysCount];
				translucentParts = new DrawInfo[arraysCount];
				for( int i = 0; i < normalParts.Length; i++ ) {
					normalParts[i] = new DrawInfo();
					translucentParts[i] = new DrawInfo();
				}
			} else {
				for( int i = 0; i < normalParts.Length; i++ ) {
					normalParts[i].ResetState();
					translucentParts[i].ResetState();
				}
			}
		}
		
		void PostStretchTiles( int x1, int y1, int z1 ) {
			for( int i = 0; i < normalParts.Length; i++ ) {
				normalParts[i].ExpandToCapacity();
				translucentParts[i].ExpandToCapacity();
			}
		}
		
		void AddSpriteVertices( byte block ) {
			int i = atlas.Get1DIndex( info.GetTextureLoc( block, Side.Left ) );
			DrawInfo part = normalParts[i];
			part.spriteCount += 6 * 4;
			part.iCount += 6 * 4;
		}
		
		unsafe void AddVertices( byte block, int count, int face ) {
			int i = atlas.Get1DIndex( info.GetTextureLoc( block, face ) );
			DrawInfo part = info.IsTranslucent[block] ? translucentParts[i] : normalParts[i];
			part.iCount += 6;

			DrawInfoFaceData counts = part.vCount;
			*(&counts.left + face) += 6;
			part.vCount = counts;
		}
		
		void DrawLeftFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Left];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Left) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White :
				X >= offset ? (Y > map.heightmap[(Z * width) + (X - offset)] ? env.SunlightXSide : env.ShadowlightXSide) : env.SunlightXSide;
			
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2 + (count - 1), u2, v1, col );
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col );
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v2, col );
			part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2 + (count - 1), u2, v2, col );
		}

		void DrawRightFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Right) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White :
				X <= (maxX - offset) ? (Y > map.heightmap[(Z * width) + (X + offset)] ? env.SunlightXSide : env.ShadowlightXSide) : env.SunlightXSide;
			
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col );
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col );
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col );
			part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col );
		}

		void DrawFrontFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Front];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Front) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White :
				Z >= offset ? (Y > map.heightmap[((Z - offset) * width) + X] ? env.SunlightZSide : env.ShadowlightZSide) : env.SunlightZSide;
			
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col );
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col );
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col );
			part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col );
		}
		
		void DrawBackFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Back];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Back) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White :
				Z <= (maxZ - offset) ? (Y > map.heightmap[((Z + offset) * width) + X] ? env.SunlightZSide : env.ShadowlightZSide) : env.SunlightZSide;
			
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col );
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col );
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
		}
		
		void DrawBottomFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Bottom];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Bottom) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White : ((Y - 1 - offset) >= map.heightmap[(Z * width) + X] ? env.SunlightYBottom : env.ShadowlightYBottom);
			
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col );
			part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col );
		}

		void DrawTopFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Top];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Top) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White : ((Y - offset) >= map.heightmap[(Z * width) + X] ? env.Sunlight : env.Shadowlight);

			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u2, v1, col );
			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col );
			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v2, col );
			part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v2, col );
		}
		
		void DrawSprite( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			const float blockHeight = 1;
			
			float u1 = 0, u2 = 1 * 15.99f/16f;
			float v1 = vOrigin, v2 = vOrigin + invVerElementSize * 15.99f/16f;
			DrawInfo part = normalParts[i];
			FastColour col = fullBright ? FastColour.White : (Y > map.heightmap[(Z * width) + X] ? env.Sunlight : env.Shadowlight);
			
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