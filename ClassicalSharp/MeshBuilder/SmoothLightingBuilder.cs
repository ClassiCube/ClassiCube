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
			FastColour col = fullBright ? FastColour.White :
				X >= offset ? (Y > map.heightmap[(Z * width) + (X - offset)] ? env.SunlightXSide : env.ShadowlightXSide) : env.SunlightXSide;
			
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2 + (count - 1), u2, v1, col );
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col );
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v2, col );
			//part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2 + (count - 1), u2, v2, col );
			int offsetX = -1;
			int offsetY = -1;
			
			int a0_0 = Count( Lit( X +offsetX, Y +offsetY, Z +1 ), Lit( X +offsetX, Y +1 +offsetY, Z +1 ), Lit( X +offsetX, Y +1 +offsetY, Z ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);
			
			int a0_1 = Count( Lit( X +offsetX, Y +offsetY, Z -1 ), Lit( X +offsetX, Y +1 +offsetY, Z -1 ), Lit( X +offsetX, Y +1 +offsetY, Z ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);

			int a1_1 = Count( Lit( X +offsetX, Y +offsetY, Z -1 ), Lit( X +offsetX, Y -1 +offsetY, Z -1 ), Lit( X +offsetX, Y -1 +offsetY, Z ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);
			
			int a1_0 = Count( Lit( X +offsetX, Y -1 +offsetY, Z ), Lit( X +offsetX, Y -1 +offsetY, Z +1 ), Lit( X +offsetX, Y +offsetY, Z +1 ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);
			
			FastColour col0_0 = fullBright ? FastColour.White : MakeXSide( a0_0 ), col1_0 = fullBright ? FastColour.White : MakeXSide( a1_0 );
			FastColour col1_1 = fullBright ? FastColour.White : MakeXSide( a1_1 ), col0_1 = fullBright ? FastColour.White : MakeXSide( a0_1 );
			
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
			FastColour col = fullBright ? FastColour.White :
				X <= (maxX - offset) ? (Y > map.heightmap[(Z * width) + (X + offset)] ? env.SunlightXSide : env.ShadowlightXSide) : env.SunlightXSide;
			
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col );
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col );
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col );
			//part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col );
			int offsetX = +1;
			int offsetY = -1;

			int a0_1 = Count( Lit( X +offsetX, Y +offsetY, Z +1 ), Lit( X +offsetX, Y +1 +offsetY, Z +1 ), Lit( X +offsetX, Y +1 +offsetY, Z ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);
			
			int a0_0 = Count( Lit( X +offsetX, Y +offsetY, Z -1 ), Lit( X +offsetX, Y +1 +offsetY, Z -1 ), Lit( X +offsetX, Y +1 +offsetY, Z ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);

			int a1_0 = Count( Lit( X +offsetX, Y +offsetY, Z -1 ), Lit( X +offsetX, Y -1 +offsetY, Z -1 ), Lit( X +offsetX, Y -1 +offsetY, Z ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);
			
			int a1_1 = Count( Lit( X +offsetX, Y -1 +offsetY, Z ), Lit( X +offsetX, Y -1 +offsetY, Z +1 ), Lit( X +offsetX, Y +offsetY, Z +1 ) ) + (Lit( X +offsetX, Y +offsetY, Z ) ? 1 : 0);
			
			FastColour col0_0 = fullBright ? FastColour.White : MakeXSide( a0_0 ), col1_0 = fullBright ? FastColour.White : MakeXSide( a1_0 );
			FastColour col1_1 = fullBright ? FastColour.White : MakeXSide( a1_1 ), col0_1 = fullBright ? FastColour.White : MakeXSide( a0_1 );
			
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
			FastColour col = fullBright ? FastColour.White :
				Z >= offset ? (Y > map.heightmap[((Z - offset) * width) + X] ? env.SunlightZSide : env.ShadowlightZSide) : env.SunlightZSide;
			
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col );
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col );
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col );
			//part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col );
			int offsetZ = -1;
			int offsetY = -1;
			
			int a1_0 = Count( Lit( X + 1, Y +offsetY, Z +offsetZ ), Lit( X +1, Y + 1 +offsetY, Z +offsetZ), Lit( X, Y + 1 +offsetY, Z +offsetZ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			int a1_1 = Count( Lit( X, Y + 1 +offsetY, Z +offsetZ ), Lit( X -1, Y + 1 +offsetY, Z +offsetZ ), Lit( X -1, Y +offsetY, Z +offsetZ ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			int a0_1 = Count( Lit( X -1, Y +offsetY, Z +offsetZ ), Lit( X -1, Y -1 +offsetY, Z +offsetZ ), Lit( X, Y -1 +offsetY, Z +offsetZ ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			int a0_0 = Count( Lit( X, Y-1 +offsetY, Z +offsetZ ), Lit( X +1, Y -1 +offsetY, Z +offsetZ ), Lit( X + 1, Y +offsetY, Z +offsetZ ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			FastColour col0_0 = fullBright ? FastColour.White : MakeZSide( a0_0 ), col1_0 = fullBright ? FastColour.White : MakeZSide( a1_0 );
			FastColour col1_1 = fullBright ? FastColour.White : MakeZSide( a1_1 ), col0_1 = fullBright ? FastColour.White : MakeZSide( a0_1 );
			
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
			FastColour col = fullBright ? FastColour.White :
				Z <= (maxZ - offset) ? (Y > map.heightmap[((Z + offset) * width) + X] ? env.SunlightZSide : env.ShadowlightZSide) : env.SunlightZSide;
			
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col );
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col );
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			//part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
			
			int offsetZ = +1;
			int offsetY = -1;


			int a0_0 = Count( Lit( X + 1, Y +offsetY, Z +offsetZ ), Lit( X +1, Y + 1 +offsetY, Z +offsetZ), Lit( X, Y + 1 +offsetY, Z +offsetZ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			int a0_1 = Count( Lit( X, Y + 1 +offsetY, Z +offsetZ ), Lit( X -1, Y + 1 +offsetY, Z +offsetZ ), Lit( X -1, Y +offsetY, Z +offsetZ ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			int a1_1 = Count( Lit( X -1, Y +offsetY, Z +offsetZ ), Lit( X -1, Y -1 +offsetY, Z +offsetZ ), Lit( X, Y -1 +offsetY, Z +offsetZ ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			int a1_0 = Count( Lit( X, Y-1 +offsetY, Z +offsetZ ), Lit( X +1, Y -1 +offsetY, Z +offsetZ ), Lit( X + 1, Y +offsetY, Z +offsetZ ) ) + (Lit( X, Y +offsetY, Z +offsetZ ) ? 1 : 0);
			
			FastColour col0_0 = fullBright ? FastColour.White : MakeZSide( a0_0 ), col1_0 = fullBright ? FastColour.White : MakeZSide( a1_0 );
			FastColour col1_1 = fullBright ? FastColour.White : MakeZSide( a1_1 ), col0_1 = fullBright ? FastColour.White : MakeZSide( a0_1 );
			
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
			FastColour col = fullBright ? FastColour.White : ((Y - 1 - offset) >= map.heightmap[(Z * width) + X] ? env.SunlightYBottom : env.ShadowlightYBottom);
			
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col );
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col );
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col );
			//part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col );
			
			int offsetY = -1;
			int a1_1 = Count( Lit( X +1, Y +offsetY, Z ), Lit( X +1, Y +offsetY, Z -1 ), Lit( X, Y +offsetY, Z -1 ) ) + (Lit( X, Y +offsetY, Z ) ? 1 : 0);
			int a0_1 = Count( Lit( X , Y +offsetY, Z -1 ), Lit( X -1, Y +offsetY, Z -1  ), Lit( X -1, Y +offsetY, Z  ) ) + (Lit( X, Y +offsetY, Z ) ? 1 : 0);
			int a0_0 = Count( Lit( X -1, Y +offsetY, Z ), Lit( X -1, Y +offsetY, Z +1 ), Lit( X, Y +offsetY, Z +1 ) ) + (Lit( X, Y +offsetY, Z ) ? 1 : 0);
			int a1_0 = Count( Lit( X , Y +offsetY, Z +1 ), Lit( X +1, Y +offsetY, Z +1 ), Lit( X+1, Y +offsetY, Z  ) ) + (Lit( X, Y +offsetY, Z ) ? 1 : 0);
			
			FastColour col0_0 = fullBright ? FastColour.White : MakeYSide( a0_0 ), col1_0 = fullBright ? FastColour.White : MakeYSide( a1_0 );
			FastColour col1_1 = fullBright ? FastColour.White : MakeYSide( a1_1 ), col0_1 = fullBright ? FastColour.White : MakeYSide( a0_1 );
			
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
			const float blockHeight = 1;
			float u1 = minBB.X, u2 = (count - 1) + maxBB.X * 15.99f/16f;
			float v1 = vOrigin + minBB.Z * invVerElementSize;
			float v2 = vOrigin + maxBB.Z * invVerElementSize * 15.99f/16f;
			DrawInfo part = isTranslucent ? translucentParts[i] : normalParts[i];
			FastColour col = fullBright ? FastColour.White : ((Y - offset) >= map.heightmap[(Z * width) + X] ? env.Sunlight : env.Shadowlight);
			
			int a0_0 = Count( Lit( X - 1, Y, Z ), Lit( X - 1, Y, Z - 1 ), Lit( X, Y, Z - 1 ) ) + (Lit( X, Y, Z ) ? 1 : 0);
			int a1_0 = Count( Lit( X + 1, Y, Z ), Lit( X + 1, Y, Z - 1 ), Lit( X, Y, Z - 1 ) ) + (Lit( X, Y, Z ) ? 1 : 0);
			int a0_1 = Count( Lit( X - 1, Y, Z ), Lit( X - 1, Y, Z + 1 ), Lit( X, Y, Z + 1 ) ) + (Lit( X, Y, Z ) ? 1 : 0);
			int a1_1 = Count( Lit( X + 1, Y, Z ), Lit( X + 1, Y, Z + 1 ), Lit( X, Y, Z + 1 ) ) + (Lit( X, Y, Z ) ? 1 : 0);
			FastColour col0_0 = fullBright ? FastColour.White : Make( a0_0 ), col1_0 = fullBright ? FastColour.White : Make( a1_0 );
			FastColour col1_1 = fullBright ? FastColour.White : Make( a1_1 ), col0_1 = fullBright ? FastColour.White : Make( a0_1 );
			
			if( a0_0 + a1_1 > a0_1 + a1_0 ) {
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u2, v1, col1_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col0_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v2, col0_1 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v2, col1_1 );
			} else {
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u2, v1, col1_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col0_0 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v2, col0_1 );
				part.vertices[part.vIndex.top++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v2, col1_1 );
			}
		}		

		int Count( bool side1, bool corner, bool side2 ) {
			///if( side1 && side2 ) return 3;
			return ((side1 ? 1 : 0) + (side2 ? 1 : 0) + (corner ? 1 : 0));
		}
		
		FastColour Make( int count ) {
			return Lerp( env. Shadowlight, env.Sunlight, count / 4f );
		}
		FastColour MakeZSide( int count ) {
			return Lerp( env.ShadowlightZSide, env.SunlightZSide, count / 4f );
		}
		FastColour MakeXSide( int count ) {
			return Lerp( env.ShadowlightXSide, env.SunlightXSide, count / 4f );
		}
		FastColour MakeYSide( int count ) {
			return Lerp( env.ShadowlightYBottom, env.SunlightYBottom, count / 4f );
		}
		
		static FastColour Lerp( FastColour a, FastColour b, float t ) {
			return new FastColour((byte)Utils.Lerp( a.R, b.R, t ),(byte)Utils.Lerp( a.G, b.G, t ),(byte)Utils.Lerp( a.B, b.B, t ) );
		}
		
		bool Lit( int x, int y, int z ) {
			return fullBright || y >= maxY || x < 0 || z < 0 || x >= width || z >= length || y > map.heightmap[( z * width ) + x];
		}
	}
}