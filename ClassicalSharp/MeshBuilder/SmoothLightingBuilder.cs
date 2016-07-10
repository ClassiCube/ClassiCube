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
			
			int F = ComputeLightFlags( X, Y, Z );
			int aY0_Z0 = ((F >> xM1yM1zM1) & 1) + ((F >> xM1yCCzM1) & 1) + ((F >> xM1yM1zCC) & 1) + ((F >> xM1yCCzCC) & 1);
			int aY0_Z1 = ((F >> xM1yM1zP1) & 1) + ((F >> xM1yCCzP1) & 1) + ((F >> xM1yM1zCC) & 1) + ((F >> xM1yCCzCC) & 1);
			int aY1_Z0 = ((F >> xM1yP1zM1) & 1) + ((F >> xM1yCCzM1) & 1) + ((F >> xM1yP1zCC) & 1) + ((F >> xM1yCCzCC) & 1);
			int aY1_Z1 = ((F >> xM1yP1zP1) & 1) + ((F >> xM1yCCzP1) & 1) + ((F >> xM1yP1zCC) & 1) + ((F >> xM1yCCzCC) & 1);
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeXSide( aY0_Z0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeXSide( aY1_Z0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeXSide( aY1_Z1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeXSide( aY0_Z1 );
			
			if( aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0 ) {
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2 + (count - 1), u2, v1, col1_1 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col1_0 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v2, col0_0 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2 + (count - 1), u2, v2, col0_1 );
			} else {
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z2 + (count - 1), u2, v1, col1_1 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y2, z1, u1, v1, col1_0 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z1, u1, v2, col0_0 );
				part.vertices[part.vIndex.left++] = new VertexP3fT2fC4b( x1, y1, z2 + (count - 1), u2, v2, col0_1 );
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
			
			int F = ComputeLightFlags( X, Y, Z );
			int aY0_Z0 = ((F >> xP1yM1zM1) & 1) + ((F >> xP1yCCzM1) & 1) + ((F >> xP1yM1zCC) & 1) + ((F >> xP1yCCzCC) & 1);
			int aY0_Z1 = ((F >> xP1yM1zP1) & 1) + ((F >> xP1yCCzP1) & 1) + ((F >> xP1yM1zCC) & 1) + ((F >> xP1yCCzCC) & 1);
			int aY1_Z0 = ((F >> xP1yP1zM1) & 1) + ((F >> xP1yCCzM1) & 1) + ((F >> xP1yP1zCC) & 1) + ((F >> xP1yCCzCC) & 1);
			int aY1_Z1 = ((F >> xP1yP1zP1) & 1) + ((F >> xP1yCCzP1) & 1) + ((F >> xP1yP1zCC) & 1) + ((F >> xP1yCCzCC) & 1);
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeXSide( aY0_Z0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeXSide( aY1_Z0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeXSide( aY1_Z1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeXSide( aY0_Z1 );
			
			if( aY0_Z0 + aY1_Z1 > aY0_Z1 + aY1_Z0 ) {
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col1_0 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col1_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col0_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col0_0 );
			} else {
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z1, u2, v1, col1_0 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y2, z2 + (count - 1), u1, v1, col1_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z2 + (count - 1), u1, v2, col0_1 );
				part.vertices[part.vIndex.right++] = new VertexP3fT2fC4b( x2, y1, z1, u2, v2, col0_0 );
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
			
			int F = ComputeLightFlags( X, Y, Z );
			int aX0_Y0 = ((F >> xM1yM1zM1) & 1) + ((F >> xM1yCCzM1) & 1) + ((F >> xCCyM1zM1) & 1) + ((F >> xCCyCCzM1) & 1);
			int aX0_Y1 = ((F >> xM1yP1zM1) & 1) + ((F >> xM1yCCzM1) & 1) + ((F >> xCCyP1zM1) & 1) + ((F >> xCCyCCzM1) & 1);
			int aX1_Y0 = ((F >> xP1yM1zM1) & 1) + ((F >> xP1yCCzM1) & 1) + ((F >> xCCyM1zM1) & 1) + ((F >> xCCyCCzM1) & 1);
			int aX1_Y1 = ((F >> xP1yP1zM1) & 1) + ((F >> xP1yCCzM1) & 1) + ((F >> xCCyP1zM1) & 1) + ((F >> xCCyCCzM1) & 1);
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeZSide( aX0_Y0 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeZSide( aX1_Y0 );
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeZSide( aX1_Y1 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeZSide( aX0_Y1 );
			
			if( aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0 ) {
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col1_0 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col0_0 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col0_1 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col1_1 );
			} else {
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z1, u1, v2, col1_0 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y1, z1, u2, v2, col0_0 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x1, y2, z1, u2, v1, col0_1 );
				part.vertices[part.vIndex.front++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z1, u1, v1, col1_1 );
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
			
			int F = ComputeLightFlags( X, Y, Z );
			int aX0_Y0 = ((F >> xM1yM1zP1) & 1) + ((F >> xM1yCCzP1) & 1) + ((F >> xCCyM1zP1) & 1) + ((F >> xCCyCCzP1) & 1);
			int aX1_Y0 = ((F >> xP1yM1zP1) & 1) + ((F >> xP1yCCzP1) & 1) + ((F >> xCCyM1zP1) & 1) + ((F >> xCCyCCzP1) & 1);
			int aX0_Y1 = ((F >> xM1yP1zP1) & 1) + ((F >> xM1yCCzP1) & 1) + ((F >> xCCyP1zP1) & 1) + ((F >> xCCyCCzP1) & 1);
			int aX1_Y1 = ((F >> xP1yP1zP1) & 1) + ((F >> xP1yCCzP1) & 1) + ((F >> xCCyP1zP1) & 1) + ((F >> xCCyCCzP1) & 1);
			int col1_1 = fullBright ? FastColour.WhitePacked : MakeZSide( aX1_Y1 ), col1_0 = fullBright ? FastColour.WhitePacked : MakeZSide( aX1_Y0 );
			int col0_0 = fullBright ? FastColour.WhitePacked : MakeZSide( aX0_Y0 ), col0_1 = fullBright ? FastColour.WhitePacked : MakeZSide( aX0_Y1 );
			
			if( aX1_Y1 + aX0_Y0 > aX0_Y1 + aX1_Y0 ) {
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col1_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col0_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col0_0 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y1, z2, u2, v2, col1_0 );
			} else {
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x2 + (count - 1), y2, z2, u2, v1, col1_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y2, z2, u1, v1, col0_1 );
				part.vertices[part.vIndex.back++] = new VertexP3fT2fC4b( x1, y1, z2, u1, v2, col0_0 );
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
			
			int F = ComputeLightFlags( X, Y, Z );
			int aX0_Z0 = ((F >> xM1yM1zM1) & 1) + ((F >> xM1yM1zCC) & 1) + ((F >> xCCyM1zM1) & 1) + ((F >> xCCyM1zCC) & 1);
			int aX1_Z0 = ((F >> xP1yM1zM1) & 1) + ((F >> xP1yM1zCC) & 1) + ((F >> xCCyM1zM1) & 1) + ((F >> xCCyM1zCC) & 1);
			int aX0_Z1 = ((F >> xM1yM1zP1) & 1) + ((F >> xM1yM1zCC) & 1) + ((F >> xCCyM1zP1) & 1) + ((F >> xCCyM1zCC) & 1);
			int aX1_Z1 = ((F >> xP1yM1zP1) & 1) + ((F >> xP1yM1zCC) & 1) + ((F >> xCCyM1zP1) & 1) + ((F >> xCCyM1zCC) & 1);
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
			int aX0_Z0 = ((F >> xM1yCCzM1) & 1) + ((F >> xM1yCCzCC) & 1) + ((F >> xCCyCCzM1) & 1) + ((F >> xCCyCCzCC) & 1);
			int aX1_Z0 = ((F >> xP1yCCzM1) & 1) + ((F >> xP1yCCzCC) & 1) + ((F >> xCCyCCzM1) & 1) + ((F >> xCCyCCzCC) & 1);
			int aX0_Z1 = ((F >> xM1yCCzP1) & 1) + ((F >> xM1yCCzCC) & 1) + ((F >> xCCyCCzP1) & 1) + ((F >> xCCyCCzCC) & 1);
			int aX1_Z1 = ((F >> xP1yCCzP1) & 1) + ((F >> xP1yCCzCC) & 1) + ((F >> xCCyCCzP1) & 1) + ((F >> xCCyCCzCC) & 1);
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
			if( fullBright ) return (1 << xP1yP1zP1) - 1; // all faces fully bright
			
			return
				// Layer Y - 1 light flags
				Lit( x - 1, y - 1, z - 1 ) << xM1yM1zM1 |
				Lit( x - 1, y - 1, z )     << xM1yM1zCC |
				Lit( x - 1, y - 1, z + 1 ) << xM1yM1zP1 |
				Lit( x, y - 1, z - 1 )     << xCCyM1zM1 |
				Lit( x, y - 1, z )         << xCCyM1zCC |
				Lit( x, y - 1, z + 1 )     << xCCyM1zP1 |
				Lit( x + 1, y - 1, z - 1 ) << xP1yM1zM1 |
				Lit( x + 1, y - 1, z )     << xP1yM1zCC |
				Lit( x + 1, y - 1, z + 1 ) << xP1yM1zP1 |
				// Layer Y light flags
				Lit( x - 1, y, z - 1 )     << xM1yCCzM1 |
				Lit( x - 1, y, z )         << xM1yCCzCC |
				Lit( x - 1, y, z + 1 )     << xM1yCCzP1 |
				Lit( x, y, z - 1 )         << xCCyCCzM1 |
				Lit( x, y, z )             << xCCyCCzCC |
				Lit( x, y, z + 1 )         << xCCyCCzP1 |
				Lit( x + 1, y, z - 1 )     << xP1yCCzM1 |
				Lit( x + 1, y, z )         << xP1yCCzCC |
				Lit( x + 1, y, z + 1 )     << xP1yCCzP1 |
				// Layer Y + 1 light flags
				Lit( x - 1, y + 1, z - 1 ) << xM1yP1zM1 |
				Lit( x - 1, y + 1, z )     << xM1yP1zCC |
				Lit( x - 1, y + 1, z + 1 ) << xM1yP1zP1 |
				Lit( x, y + 1, z - 1 )     << xCCyP1zM1 |
				Lit( x, y, z - 1 )         << xCCyP1zCC |
				Lit( x, y + 1, z + 1 )     << xCCyP1zP1 |
				Lit( x + 1, y + 1, z - 1 ) << xP1yP1zM1 |
				Lit( x + 1, y + 1, z )     << xP1yP1zCC |
				Lit( x + 1, y + 1, z + 1 ) << xP1yP1zP1 ;
		}
		
		const int xM1yM1zM1 = 0,  xM1yM1zCC = 1,  xM1yM1zP1 = 2;
		const int xCCyM1zM1 = 3,  xCCyM1zCC = 4,  xCCyM1zP1 = 5;
		const int xP1yM1zM1 = 6,  xP1yM1zCC = 7,  xP1yM1zP1 = 8;

		const int xM1yCCzM1 = 9,  xM1yCCzCC = 10, xM1yCCzP1 = 11;
		const int xCCyCCzM1 = 12, xCCyCCzCC = 13, xCCyCCzP1 = 14;
		const int xP1yCCzM1 = 15, xP1yCCzCC = 16, xP1yCCzP1 = 17;

		const int xM1yP1zM1 = 18, xM1yP1zCC = 19, xM1yP1zP1 = 20;
		const int xCCyP1zM1 = 21, xCCyP1zCC = 22, xCCyP1zP1 = 23;
		const int xP1yP1zM1 = 24, xP1yP1zCC = 25, xP1yP1zP1 = 26;
		
		int Lit( int x, int y, int z ) {
			if( x < 0 || y < 0 || z < 0
			   || x >= width || y >= height || z >= length ) return 1;
			
			byte block = map.GetBlock( x, y, z );
			int offset = (lightFlags >> Side.Top) & 1;
			return (y - offset) >= map.heightmap[(z * width) + x] ? 1 : 0;
		}
		
		#endregion
	}
}