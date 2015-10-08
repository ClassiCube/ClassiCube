using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Model;
using OpenTK;

namespace ClassicalSharp {

	public static class IsometricBlockDrawer {
		
		static BlockInfo info;
		static ModelCache cache;
		static TerrainAtlas2D atlas;
		static float blockHeight;
		static int index;
		static float scale;
		
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
			blockHeight = info.Height[block];
			index = 0;
			scale = size;
			
			// screen to isometric coords (cos(-x) = cos(x), sin(-x) = -sin(x))
			pos.X = x; pos.Y = y; pos.Z = 0;			
			pos = Utils.RotateY( Utils.RotateX( pos, cosX, -sinX ), cosY, -sinY );
			
			if( info.IsSprite[block] ) {
				DrawXFace( block, 0f, TileSide.Right );
				DrawZFace( block, 0f, TileSide.Back );
			} else {
				DrawXFace( block, scale, TileSide.Left );
				DrawZFace( block, -scale, TileSide.Back );
				DrawYFace( block, scale * blockHeight, TileSide.Top );
			}
			
			for( int i = 0; i < index; i++ )
				TransformVertex( ref cache.vertices[i] );
			game.Graphics.DrawDynamicIndexedVb( DrawMode.Triangles, cache.vb,
			                                   cache.vertices, index, index * 6 / 4 );
		}
		
		static void TransformVertex( ref VertexPos3fTex2fCol4b vertex ) {
			Vector3 pos = new Vector3( vertex.X, vertex.Y, vertex.Z );
			#if USE_DX
			// See comment in IGraphicsApi.Draw2DTexture()
			pos.X -= 0.5f;
			pos.Y -= 0.5f;
			#endif
			pos = Utils.RotateX( Utils.RotateY( pos, cosY, sinY ), cosX, sinX );
			vertex.X = pos.X; vertex.Y = pos.Y; vertex.Z = pos.Z;
		}

		static Vector3 pos = Vector3.Zero;
		static void DrawYFace( byte block, float y, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = colNormal;

			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y - y,
			                                                    pos.Z - scale, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y - y,
			                                                    pos.Z + scale, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y - y,
			                                                    pos.Z + scale, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y - y,
			                                                    pos.Z - scale, rec.U2, rec.V1, col );
		}

		static void DrawZFace( byte block, float z, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize;
			FastColour col = colZSide;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y + scale * blockHeight,
			                                                    pos.Z - z, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y - scale * blockHeight,
			                                                    pos.Z - z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y - scale * blockHeight,
			                                                    pos.Z - z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y + scale * blockHeight,
			                                                    pos.Z - z, rec.U2, rec.V2, col );
		}

		static void DrawXFace( byte block, float x, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize;
			FastColour col = colXSide;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - x, pos.Y + scale * blockHeight,
			                                                    pos.Z - scale, rec.U1, rec.V2, colNormal );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - x, pos.Y - scale * blockHeight,
			                                                    pos.Z - scale, rec.U1, rec.V1, colNormal );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - x, pos.Y - scale * blockHeight,
			                                                    pos.Z + scale, rec.U2, rec.V1, colNormal );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - x, pos.Y + scale * blockHeight,
			                                                    pos.Z + scale, rec.U2, rec.V2, colNormal );
		}
	}
}