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
			
			const int oy = -1;
			int a0_0 = Lit( X - 1, Y + oy, Z + 1 ) + Lit( X - 1, Y + 1 + oy, Z + 1 ) + Lit( X - 1, Y + 1 + oy, Z ) + Lit( X - 1, Y + oy, Z );
			int a0_1 = Lit( X - 1, Y + oy, Z - 1 ) + Lit( X - 1, Y + 1 + oy, Z - 1 ) + Lit( X - 1, Y + 1 + oy, Z ) + Lit( X - 1, Y + oy, Z );
			int a1_1 = Lit( X - 1, Y + oy, Z - 1 ) + Lit( X - 1, Y - 1 + oy, Z - 1 ) + Lit( X - 1, Y - 1 + oy, Z ) + Lit( X - 1, Y + oy, Z );
			int a1_0 = Lit( X - 1, Y - 1 + oy, Z ) + Lit( X - 1, Y - 1 + oy, Z + 1 ) + Lit( X - 1, Y + oy, Z + 1 ) + Lit( X - 1, Y + oy, Z );
			
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
			
			const int oy = -1;
			int a0_1 = Lit( X + 1, Y + oy, Z + 1 ) + Lit( X + 1, Y + 1 + oy, Z + 1 ) + Lit( X + 1, Y + 1 + oy, Z ) + Lit( X + 1, Y + oy, Z );
			int a0_0 = Lit( X + 1, Y + oy, Z - 1 ) + Lit( X + 1, Y + 1 + oy, Z - 1 ) + Lit( X + 1, Y + 1 + oy, Z ) + Lit( X + 1, Y + oy, Z );
			int a1_0 = Lit( X + 1, Y + oy, Z - 1 ) + Lit( X + 1, Y - 1 + oy, Z - 1 ) + Lit( X + 1, Y - 1 + oy, Z ) + Lit( X + 1, Y + oy, Z );
			int a1_1 = Lit( X + 1, Y - 1 + oy, Z ) + Lit( X + 1, Y - 1 + oy, Z + 1 ) + Lit( X + 1, Y + oy, Z + 1 ) + Lit( X + 1, Y + oy, Z );
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
			
			const int oy = -1;
			int a1_0 = Lit( X + 1, Y + oy, Z - 1 ) + Lit( X + 1, Y + 1 + oy, Z - 1 ) + Lit( X, Y + 1 + oy, Z - 1 ) + Lit( X, Y + oy, Z - 1 );
			int a1_1 = Lit( X, Y + 1 + oy, Z - 1 ) + Lit( X - 1, Y + 1 + oy, Z - 1 ) + Lit( X - 1, Y + oy, Z - 1 ) + Lit( X, Y + oy, Z - 1 );
			int a0_1 = Lit( X - 1, Y + oy, Z - 1 ) + Lit( X - 1, Y - 1 + oy, Z - 1 ) + Lit( X, Y - 1 + oy, Z - 1 ) + Lit( X, Y + oy, Z - 1 );
			int a0_0 = Lit( X, Y - 1 + oy, Z - 1 ) + Lit( X + 1, Y - 1 + oy, Z - 1 ) + Lit( X + 1, Y + oy, Z - 1 ) + Lit( X, Y + oy, Z - 1 );
			
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
			
			const int oy = -1;
			int a0_0 = Lit( X + 1, Y + oy, Z + 1 ) + Lit( X + 1, Y + 1 + oy, Z + 1 ) + Lit( X, Y + 1 + oy, Z + 1 ) + Lit( X, Y + oy, Z + 1 );
			int a0_1 = Lit( X, Y + 1 + oy, Z + 1 ) + Lit( X - 1, Y + 1 + oy, Z + 1 ) + Lit( X - 1, Y + oy, Z + 1 ) + Lit( X, Y + oy, Z + 1 );
			int a1_1 = Lit( X - 1, Y + oy, Z + 1 ) + Lit( X - 1, Y - 1 + oy, Z + 1 ) + Lit( X, Y - 1 + oy, Z + 1 ) + Lit( X, Y + oy, Z + 1 );
			int a1_0 = Lit( X, Y - 1 + oy, Z + 1 ) + Lit( X + 1, Y - 1 + oy, Z + 1 ) + Lit( X + 1, Y + oy, Z + 1 ) + Lit( X, Y + oy, Z + 1 );
			
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
			
			int aX0_Z0 = Lit( X - 1, Y - 1, Z ) + Lit( X - 1, Y - 1, Z - 1 ) + Lit( X, Y - 1, Z - 1 ) + Lit( X, Y - 1, Z );
			int aX0_Z1 = Lit( X - 1, Y - 1, Z ) + Lit( X - 1, Y - 1, Z + 1 ) + Lit( X, Y - 1, Z + 1 ) + Lit( X, Y - 1, Z );
			int aX1_Z0 = Lit( X + 1, Y - 1, Z ) + Lit( X + 1, Y - 1, Z - 1 ) + Lit( X, Y - 1, Z - 1 ) + Lit( X, Y - 1, Z );
			int aX1_Z1 = Lit( X + 1, Y - 1, Z ) + Lit( X + 1, Y - 1, Z + 1 ) + Lit( X, Y - 1, Z + 1 ) + Lit( X, Y - 1, Z );
			int col0_1 = fullBright ? FastColour.WhitePacked : MakeYSide( aX0_Z1 ), col1_1 = fullBright ? FastColour.WhitePacked : MakeYSide( aX1_Z1 );
			int col1_0 = fullBright ? FastColour.WhitePacked : MakeYSide( aX1_Z0 ), col0_0 = fullBright ? FastColour.WhitePacked : MakeYSide( aX0_Z0 );
			
			if( aX0_Z1 + aX1_Z0 > aX0_Z0 + aX1_Z1 ) {
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_1 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col0_1 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col0_0 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col1_0 );
			} else {
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_1 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col0_1 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v1, col0_0 );
				part.vertices[part.vIndex.bottom++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u2, v1, col1_0 );
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
			
			int F = ComputeLightFlags( X, Y, Z );		
			int aX0_Z0 = ((F >> xMyCzM) & 1) + ((F >> xMyCzC) & 1) + ((F >> xCyCzM) & 1) + ((F >> xCyCzC) & 1);
			int aX1_Z0 = ((F >> xPyCzM) & 1) + ((F >> xPyCzC) & 1) + ((F >> xCyCzM) & 1) + ((F >> xCyCzC) & 1);
			int aX0_Z1 = ((F >> xMyCzP) & 1) + ((F >> xMyCzC) & 1) + ((F >> xCyCzP) & 1) + ((F >> xCyCzC) & 1);
			int aX1_Z1 = ((F >> xPyCzP) & 1) + ((F >> xPyCzC) & 1) + ((F >> xCyCzP) & 1) + ((F >> xCyCzC) & 1);
			int col0_0 = fullBright ? FastColour.WhitePacked : Make( aX0_Z0 ), col1_0 = fullBright ? FastColour.WhitePacked : Make( aX1_Z0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : Make( aX1_Z1 ), col0_1 = fullBright ? FastColour.WhitePacked : Make( aX0_Z1 );
			
			if( aX0_Z0 + aX1_Z1 > aX0_Z1 + aX1_Z0 ) {
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
		
		#region Light computation
		
		int ComputeLightFlags( int x, int y, int z ) {
			if( fullBright ) return (1 << xPyPzP) - 1; // all faces fully bright
			
			return
				// Layer Y - 1 light flags
				Lit( x - 1, y - 1, z - 1 ) << xMyMzM |
				Lit( x - 1, y - 1, z )     << xMyMzC |
				Lit( x - 1, y - 1, z + 1 ) << xMyMzP |
				Lit( x, y - 1, z - 1 )     << xCyMzM |
				Lit( x, y - 1, z )         << xCyMzC |
				Lit( x, y - 1, z + 1 )     << xCyMzP |
				Lit( x + 1, y - 1, z - 1 ) << xPyMzM |
				Lit( x + 1, y - 1, z )     << xPyMzC |
				Lit( x + 1, y - 1, z + 1 ) << xPyMzP |
				// Layer Y light flags
				Lit( x - 1, y, z - 1 )     << xMyCzM |
				Lit( x - 1, y, z )         << xMyCzC |
				Lit( x - 1, y, z + 1 )     << xMyCzP |
				Lit( x, y, z - 1 )         << xCyCzM |
				Lit( x, y, z )             << xCyCzC |
				Lit( x, y, z + 1 )         << xCyCzP |
				Lit( x + 1, y, z - 1 )     << xPyCzM |
				Lit( x + 1, y, z )         << xPyCzC |
				Lit( x + 1, y, z + 1 )     << xPyCzP |
				// Layer Y + 1 light flags
				Lit( x - 1, y + 1, z - 1 ) << xMyPzM |
				Lit( x - 1, y + 1, z )     << xMyPzC |
				Lit( x - 1, y + 1, z + 1 ) << xMyPzP |
				Lit( x, y + 1, z - 1 )     << xCyPzM |
				Lit( x, y, z - 1 )         << xCyPzC |
				Lit( x, y + 1, z + 1 )     << xCyPzP |
				Lit( x + 1, y + 1, z - 1 ) << xPyPzM |
				Lit( x + 1, y + 1, z )     << xPyPzC |
				Lit( x + 1, y + 1, z + 1 ) << xPyPzP ;
		}
		
		const int xMyMzM = 0,  xMyMzC = 1,  xMyMzP = 2;
		const int xCyMzM = 3,  xCyMzC = 4,  xCyMzP = 5;
		const int xPyMzM = 6,  xPyMzC = 7,  xPyMzP = 8;

		const int xMyCzM = 9,  xMyCzC = 10, xMyCzP = 11;
		const int xCyCzM = 12, xCyCzC = 13, xCyCzP = 14;
		const int xPyCzM = 15, xPyCzC = 16, xPyCzP = 17;

		const int xMyPzM = 18, xMyPzC = 19, xMyPzP = 20;
		const int xCyPzM = 21, xCyPzC = 22, xCyPzP = 23;
		const int xPyPzM = 24, xPyPzC = 25, xPyPzP = 26;		
		
		int Lit( int x, int y, int z ) {
			if( fullBright || x < 0 || y < 0 || z < 0 
			   || x >= width || y >= height || z >= length ) return 1;
			
			byte block = map.GetBlock( x, y, z );
			int offset = (lightFlags >> Side.Top) & 1;
			return (y - offset) >= map.heightmap[(z * width) + x] ? 1 : 0;
		}
		
		#endregion
	}
}