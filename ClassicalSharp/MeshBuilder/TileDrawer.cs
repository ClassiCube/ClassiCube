// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Runtime.InteropServices;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Textures;
using OpenTK;

namespace ClassicalSharp {
	
	public unsafe partial class ChunkMeshBuilder {
		
		protected DrawInfo[] normalParts, translucentParts;
		protected TerrainAtlas1D atlas;
		protected int arraysCount = 0;
		protected bool fullBright;
		protected int lightFlags;

		protected Vector3 minBB, maxBB;		
		protected bool isTranslucent;
		protected float invVerElementSize;
		protected int elementsPerAtlas1D;
		
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
		protected struct DrawInfoFaceData {
			public int left, right, front, back, bottom, top;
		}
		
		protected class DrawInfo {
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

		void RenderTile( int index ) {
			if( info.Draw[curBlock] == DrawType.Sprite ) {
				fullBright = info.FullBright[curBlock];
				int count = counts[index + Side.Top];
				if( count != 0 ) DrawSprite( count );
				return;
			}
			
			int leftCount = counts[index++], rightCount = counts[index++],
			frontCount = counts[index++], backCount = counts[index++],
			bottomCount = counts[index++], topCount = counts[index++];
			if( leftCount == 0 && rightCount == 0 && frontCount == 0 &&
			   backCount == 0 && bottomCount == 0 && topCount == 0 ) return;
			
			fullBright = info.FullBright[curBlock];
			isTranslucent = info.Draw[curBlock] == DrawType.Translucent;
			lightFlags = info.LightOffset[curBlock];
			
			Vector3 min = info.MinBB[curBlock], max = info.MaxBB[curBlock];
			x1 = X + min.X; y1 = Y + min.Y; z1 = Z + min.Z;
			x2 = X + max.X; y2 = Y + max.Y; z2 = Z + max.Z;

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
			DrawInfo part = info.Draw[block] == DrawType.Translucent ? translucentParts[i] : normalParts[i];
			part.iCount += 6;

			DrawInfoFaceData counts = part.vCount;
			*(&counts.left + face) += 6;
			part.vCount = counts;
		}
		
		protected abstract void DrawLeftFace( int count );
		protected abstract void DrawRightFace( int count );
		protected abstract void DrawFrontFace( int count );
		protected abstract void DrawBackFace( int count );
		protected abstract void DrawTopFace( int count );
		protected abstract void DrawBottomFace( int count );	
		
		void DrawSprite( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			const float blockHeight = 1;
			
			const float u1 = 0, u2 = 15.99f/16f;
			float v1 = vOrigin, v2 = vOrigin + invVerElementSize * 15.99f/16f;
			DrawInfo part = normalParts[i];
			int col = fullBright ? FastColour.WhitePacked : (Y > map.heightmap[(Z * width) + X] ? env.Sun : env.Shadow);
			
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