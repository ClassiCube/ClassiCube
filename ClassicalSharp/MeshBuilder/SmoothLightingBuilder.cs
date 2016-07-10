// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	public sealed class SmoothLightingMeshBuilder : ChunkMeshBuilder {
		
		protected override int StretchXLiquid( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block ) {
			if( OccludedLiquid( chunkIndex ) ) return 0;
			return 1;
		}
		
		protected override int StretchX( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
			return 1;
		}
		
		protected override int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
			return 1;
		}
		
		protected override void DrawLeftFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Left];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Left) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2 + (count - 1), u2, v1, col );
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col );
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v2, col );
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2 + (count - 1), u2, v2, col );
			
			const int ox = -1, oy = -1;			
			int a0_0 = Lit( X + ox, Y + oy, Z + 1 ) + Lit( X + ox, Y + 1 + oy, Z + 1 ) + Lit( X + ox, Y + 1 + oy, Z ) + Lit( X + ox, Y + oy, Z );			
			int a0_1 = Lit( X + ox, Y + oy, Z - 1 ) + Lit( X + ox, Y + 1 + oy, Z - 1 ) + Lit( X + ox, Y + 1 + oy, Z ) + Lit( X + ox, Y + oy, Z );
			int a1_1 = Lit( X + ox, Y + oy, Z - 1 ) + Lit( X + ox, Y - 1 + oy, Z - 1 ) + Lit( X + ox, Y - 1 + oy, Z ) + Lit( X + ox, Y + oy, Z );			
			int a1_0 = Lit( X + ox, Y - 1 + oy, Z ) + Lit( X + ox, Y - 1 + oy, Z + 1 ) + Lit( X + ox, Y + oy, Z + 1 ) + Lit( X + ox, Y + oy, Z );
			
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeXSide( a0_0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeXSide( a1_0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeXSide( a1_1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeXSide( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2, u2, v1, col0_0 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1 + (count - 1), u1, v1, col0_1 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1 + (count - 1), u1, v2, col1_1 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2, u2, v2, col1_0 );
			} else {
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2, u2, v1, col0_0 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1 + (count - 1), u1, v1, col0_1 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1 + (count - 1), u1, v2, col1_1 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2, u2, v2, col1_0 );
			}
		}

		protected override void DrawRightFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Right];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Right) & 1;
			
			float u1 = minBB.Z, u2 = (count - 1) + maxBB.Z * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col );
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col );
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col );
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col );
			
			const int ox = +1, oy = -1;
			int a0_1 = Lit( X + ox, Y + oy, Z + 1 ) + Lit( X + ox, Y + 1 + oy, Z + 1 ) + Lit( X +ox, Y +1 + oy, Z ) + Lit( X + ox, Y + oy, Z );
			int a0_0 = Lit( X + ox, Y + oy, Z - 1 ) + Lit( X + ox, Y + 1 + oy, Z - 1 ) + Lit( X +ox, Y +1 + oy, Z ) + Lit( X + ox, Y + oy, Z );
			int a1_0 = Lit( X + ox, Y + oy, Z - 1 ) + Lit( X + ox, Y - 1 + oy, Z - 1 ) + Lit( X +ox, Y -1 + oy, Z ) + Lit( X + ox, Y + oy, Z );	
			int a1_1 = Lit( X + ox, Y - 1 + oy, Z ) + Lit( X + ox, Y - 1 + oy, Z + 1 ) + Lit( X +ox, Y + oy, Z +1 ) + Lit( X + ox, Y + oy, Z );
			
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeXSide( a0_0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeXSide( a1_0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeXSide( a1_1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeXSide( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col0_0 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col0_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col1_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col1_0 );
			} else {
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col0_0 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col0_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col1_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col1_0 );
			}
		}

		protected override void DrawFrontFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Front];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Front) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col );
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col );
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col );
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col );
			
			const int oz = -1, oy = -1;		
			int a1_0 = Lit( X + 1, Y + oy, Z + oz ) + Lit( X + 1, Y + 1 + oy, Z + oz ) + Lit( X, Y + 1 + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );	
			int a1_1 = Lit( X, Y + 1 + oy, Z + oz ) + Lit( X - 1, Y + 1 + oy, Z + oz ) + Lit( X - 1, Y + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );
			int a0_1 = Lit( X - 1, Y + oy, Z + oz ) + Lit( X - 1, Y - 1 + oy, Z + oz ) + Lit( X, Y - 1 + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );
			int a0_0 = Lit( X, Y - 1 + oy, Z + oz ) + Lit( X + 1, Y - 1 + oy, Z + oz ) + Lit( X + 1, Y + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );
			
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeZSide( a0_0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeZSide( a1_0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeZSide( a1_1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeZSide( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col0_0 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col0_1 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col1_1 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col1_0 );
			} else {
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col0_0 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col0_1 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col1_1 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col1_0 );
			}
		}
		
		protected override void DrawBackFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Back];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Back) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + maxBB.Y * invVerElementSize;
			float v2 = vOrigin + minBB.Y * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col );
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col );
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
			
			const int oz = +1, oy = -1;
			int a0_0 = Lit( X + 1, Y + oy, Z +oz ) + Lit( X + 1, Y + 1 + oy, Z + oz ) + Lit( X, Y + 1 + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );	
			int a0_1 = Lit( X, Y + 1 + oy, Z +oz ) + Lit( X - 1, Y + 1 + oy, Z + oz ) + Lit( X - 1, Y + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );	
			int a1_1 = Lit( X - 1, Y + oy, Z +oz ) + Lit( X - 1, Y - 1 + oy, Z + oz ) + Lit( X, Y - 1 + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );
			int a1_0 = Lit( X, Y - 1 + oy, Z +oz ) + Lit( X + 1, Y - 1 + oy, Z + oz ) + Lit( X + 1, Y + oy, Z + oz ) + Lit( X, Y + oy, Z + oz );
			
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeZSide( a0_0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeZSide( a1_0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeZSide( a1_1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeZSide( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col0_0 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col0_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col1_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_0 );
			} else {
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col0_0 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col0_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col1_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_0 );
			}
		}
		
		protected override void DrawBottomFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Bottom];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Bottom) & 1;
			
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col );
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col );
			
			const int oy = -1;
			int a1_1 = Lit( X + 1, Y + oy, Z ) + Lit( X + 1, Y + oy, Z - 1 ) + Lit( X, Y + oy, Z - 1 ) + Lit( X, Y + oy, Z );
			int a0_1 = Lit( X, Y + oy, Z - 1 ) + Lit( X - 1, Y + oy, Z - 1 ) + Lit( X - 1, Y + oy, Z ) + Lit( X, Y + oy, Z );
			int a0_0 = Lit( X - 1, Y + oy, Z ) + Lit( X - 1, Y + oy, Z + 1 ) + Lit( X, Y + oy, Z + 1 ) + Lit( X, Y + oy, Z );
			int a1_0 = Lit( X, Y + oy, Z + 1 ) + Lit( X + 1, Y + oy, Z + 1 ) + Lit( X + 1, Y + oy, Z ) + Lit( X, Y + oy, Z );
			
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeYSide( a0_0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeYSide( a1_0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeYSide( a1_1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeYSide( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_0 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col0_0 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col0_1 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col1_1 );
			} else {
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_0 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col0_0 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col0_1 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col1_1 );
			}
		}

		protected override void DrawTopFace( int count ) {
			int texId = info.textures[curBlock * Side.Sides + Side.Top];
			int i = texId / elementsPerAtlas1D;
			float vOrigin = (texId % elementsPerAtlas1D) * invVerElementSize;
			int offset = (lightFlags >> Side.Top) & 1;

			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White : ((Y - offset) >= map.heightmap[(Z * width) + X] ? env.Sunlight : env.Shadowlight);
			
			int a0_0 = Lit( X - 1, Y, Z ) + Lit( X - 1, Y, Z - 1 ) + Lit( X, Y, Z - 1 ) + Lit( X, Y, Z );
			int a1_0 = Lit( X + 1, Y, Z ) + Lit( X + 1, Y, Z - 1 ) + Lit( X, Y, Z - 1 ) + Lit( X, Y, Z );
			int a0_1 = Lit( X - 1, Y, Z ) + Lit( X - 1, Y, Z + 1 ) + Lit( X, Y, Z + 1 ) + Lit( X, Y, Z );
			int a1_1 = Lit( X + 1, Y, Z ) + Lit( X + 1, Y, Z + 1 ) + Lit( X, Y, Z + 1 ) + Lit( X, Y, Z );
			int col0_0 = fullBright ? FastColour.WhitePacked : Make( a0_0 ), col1_0 = fullBright ? FastColour.WhitePacked : Make( a1_0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : Make( a1_1 ), col0_1 = fullBright ? FastColour.WhitePacked : Make( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col0_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v2, col0_1 );	
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v2, col1_1 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u2, v1, col1_0 );	
			} else {
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u2, v1, col1_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col0_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v2, col0_1 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v2, col1_1 );
			}
		}		
		
		int Make( int count ) { return Lerp( env.Shadow, env.Sun, count / 4f ); }
		int MakeZSide( int count ) { return Lerp( env.ShadowZSide, env.SunZSide, count / 4f ); }
		int MakeXSide( int count ) { return Lerp( env.ShadowXSide, env.SunXSide, count / 4f ); }
		int MakeYSide( int count ) { return Lerp( env.ShadowYBottom, env.SunYBottom, count / 4f ); }
		
		static int Lerp( int a, int b, float t ) {
			int c = FastColour.BlackPacked;
			c |= (byte)Utils.Lerp( (a & 0xFF0000) >> 16, (b & 0xFF0000) >> 16, t ) << 16;
			c |= (byte)Utils.Lerp( (a & 0x00FF00) >> 8, (b & 0x00FF00) >> 8, t ) << 8;
			c |= (byte)Utils.Lerp( a & 0x0000FF, b & 0x0000FF, t );
			return c;
		}
		
		int Lit( int x, int y, int z ) {
			return (fullBright || y >= maxY || x < 0 || z < 0 
			        || x >= width || z >= length || y > map.heightmap[( z * width ) + x]) ? 1 : 0;
		}
	}
}