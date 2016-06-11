using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp {

	public static class IsometricBlockDrawer {
		
		static BlockInfo info;
		static ModelCache cache;
		static TerrainAtlas2D atlas;
		static int index;
		static float scale;
		static Vector3 minBB, maxBB;
		const float invElemSize = TerrainAtlas2D.invElementSize;
		static bool fullBright;
		
		static FastColour colNormal, colXSide, colZSide, colYBottom;
		static float cosX, sinX, cosY, sinY;
		static IsometricBlockDrawer() {
			colNormal = FastColour.White;
			FastColour.GetShaded( colNormal, ref colXSide, ref colZSide, ref colYBottom );
			
			cosX = (float)Math.Cos( 26.565f * Utils.Deg2Rad );
			sinX = (float)Math.Sin( 26.565f * Utils.Deg2Rad );
			cosY = (float)Math.Cos( -45f * Utils.Deg2Rad );
			sinY = (float)Math.Sin( -45f * Utils.Deg2Rad );
		}
		
		public static void Draw( Game game, byte block, float size, float x, float y ) {
			info = game.BlockInfo;
			cache = game.ModelCache;
			atlas = game.TerrainAtlas;
			minBB = info.MinBB[block];
			maxBB = info.MaxBB[block];
			fullBright = info.FullBright[block];
			if( info.IsSprite[block] ) {
				minBB = Vector3.Zero; maxBB = Vector3.One;
			}
			if( info.IsAir[block] ) return;
			index = 0;
			
			// isometric coords size: cosY * -scale - sinY * scale
			// we need to divide by (2 * cosY), as the calling function expects size to be in pixels.
			scale = size / (2 * cosY);		
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;			
			pos = Utils.RotateY( Utils.RotateX( pos, cosX, -sinX ), cosY, -sinY );
			
			if( info.IsSprite[block] ) {
				XQuad( block, 0f, TileSide.Right );
				ZQuad( block, 0f, TileSide.Back );
			} else {
				XQuad( block, Make( maxBB.X ), TileSide.Left );
				ZQuad( block, Make( minBB.Z ), TileSide.Back );
				YQuad( block, Make( maxBB.Y ), TileSide.Top );
			}
			
			for( int i = 0; i < index; i++ )
				TransformVertex( ref cache.vertices[i] );
			game.Graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb,
			                                   cache.vertices, index, index * 6 / 4 );
		}
		
		static void TransformVertex( ref VertexPos3fTex2fCol4b vertex ) {
			Vector3 p = new Vector3( vertex.X, vertex.Y, vertex.Z );
			//p = Utils.RotateY( p - pos, time ) + pos;
			// See comment in IGraphicsApi.Draw2DTexture()
			p.X -= 0.5f; p.Y -= 0.5f;
			p = Utils.RotateX( Utils.RotateY( p, cosY, sinY ), cosX, sinX );
			vertex.X = p.X; vertex.Y = p.Y; vertex.Z = p.Z;
		}

		static Vector3 pos = Vector3.Zero;
		static void YQuad( byte block, float y, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texId );
			FastColour col = colNormal;		
			float uOrigin = rec.U1, vOrigin = rec.V1;
			
			rec.U1 = uOrigin + minBB.Z * invElemSize;
			rec.U2 = uOrigin + maxBB.Z * invElemSize * 15.99f/16f;
			rec.V1 = vOrigin + minBB.X * invElemSize;
			rec.V2 = vOrigin + maxBB.X * invElemSize * 15.99f/16f;

			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( minBB.X ), pos.Y + y,
			                                                    pos.Z + Make( minBB.Z ), rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( minBB.X ), pos.Y + y,
			                                                    pos.Z + Make( maxBB.Z ), rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( maxBB.X ), pos.Y + y,
			                                                    pos.Z + Make( maxBB.Z ), rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( maxBB.X ), pos.Y + y,
			                                                    pos.Z + Make( minBB.Z ), rec.U2, rec.V1, col );
		}

		static void ZQuad( byte block, float z, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texId );
			FastColour col = fullBright ? colNormal : colZSide;
			float uOrigin = rec.U1, vOrigin = rec.V1;
			
			rec.U1 = uOrigin + minBB.X * invElemSize;
			rec.U2 = uOrigin + maxBB.X * invElemSize * 15.99f/16f;
			rec.V1 = vOrigin + (1 - maxBB.Y) * invElemSize;
			rec.V2 = vOrigin + (1 - minBB.Y) * invElemSize;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( minBB.X ), pos.Y + Make( minBB.Y ),
			                                                    pos.Z + z, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( minBB.X ), pos.Y + Make( maxBB.Y ),
			                                                    pos.Z + z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( maxBB.X ), pos.Y + Make( maxBB.Y ),
			                                                    pos.Z + z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + Make( maxBB.X ), pos.Y + Make( minBB.Y ),
			                                                    pos.Z + z, rec.U1, rec.V2, col );
		}
		
		static float Make( float value ) {
			return scale - (scale * value * 2);
		}

		static void XQuad( byte block, float x, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRec rec = atlas.GetTexRec( texId );
			FastColour col = fullBright ? colNormal : colXSide;
			float uOrigin = rec.U1, vOrigin = rec.V1;
			
			rec.U1 = uOrigin + minBB.Z * invElemSize;
			rec.U2 = uOrigin + maxBB.Z * invElemSize * 15.99f/16f;
			rec.V1 = vOrigin + (1 - maxBB.Y) * invElemSize;
			rec.V2 = vOrigin + (1 - minBB.Y) * invElemSize * 15.99f/16f;		
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + Make( minBB.Y ),
			                                                    pos.Z + Make( minBB.Z ), rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + Make( maxBB.Y ),
			                                                    pos.Z + Make( minBB.Z ), rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + Make( maxBB.Y ),
			                                                    pos.Z + Make( maxBB.Z ), rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + Make( minBB.Y ),
			                                                    pos.Z + Make( maxBB.Z ), rec.U1, rec.V2, col );
		}
	}
}