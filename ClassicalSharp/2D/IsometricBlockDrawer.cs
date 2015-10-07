using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;
using ClassicalSharp.Model;

namespace ClassicalSharp {

	public static class IsometricBlockDrawer {
		
		static BlockInfo info;
		static ModelCache cache;
		static TerrainAtlas2D atlas;
		static float blockHeight;
		static int index;
		static float scale;
		
		static FastColour colNormal, colXSide, colZSide, colYBottom;
		static IsometricBlockDrawer() {
			colNormal = FastColour.White;
			FastColour.GetShaded( colNormal, ref colXSide, ref colZSide, ref colYBottom );
		}
		
		public static void Draw( Game game, byte block, float size, float x, float y ) {
			info = game.BlockInfo;
			cache = game.ModelCache;
			atlas = game.TerrainAtlas;
			blockHeight = info.Height[block];
			index = 0;
			scale = size;
					
			// screen to isometric coords			
			pos.X = x; pos.Y = -y; pos.Z = 0;
			pos = Utils.RotateX( pos, (float)Utils.DegreesToRadians( -35.264f ) );
			pos = Utils.RotateY( pos, (float)Utils.DegreesToRadians( 45f ) );
			
			if( info.IsSprite[block] ) {
				DrawXFace( block, 0f, TileSide.Right );
				DrawZFace( block, 0f, TileSide.Back );
			} else {
				DrawXFace( block, scale, TileSide.Left );
				DrawZFace( block, -scale, TileSide.Back );
				DrawYFace( block, scale * blockHeight, TileSide.Top );
			}
			game.Graphics.DrawDynamicIndexedVb( DrawMode.Triangles, cache.vb, 
			                                   cache.vertices, index, index * 6 / 4 );
		}
		
		
		public static void SetupState( IGraphicsApi graphics, bool setFog ) {
			graphics.PushMatrix();
			Matrix4 m = Matrix4.RotateY( (float)Utils.DegreesToRadians( 45f ) ) * 
				Matrix4.RotateX( (float)Utils.RadiansToDegrees( -35.264f ) );
			graphics.LoadMatrix( ref m );
			if( setFog )
				graphics.Fog = false;
			graphics.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
		}
		
		public static void RestoreState( IGraphicsApi graphics, bool setFog ) {
			graphics.PopMatrix();
			if( setFog )
				graphics.Fog = true;			
		}

		static Vector3 pos = Vector3.Zero;		
		static void DrawYFace( byte block, float y, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			FastColour col = colNormal;

			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y + y, 
			                                                    pos.Z - scale, rec.U2, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y + y, 
			                                                    pos.Z + scale, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y + y, 
			                                                    pos.Z + scale, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y + y, 
			                                                    pos.Z - scale, rec.U2, rec.V1, col );
		}

		static void DrawZFace( byte block, float z, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize;
			FastColour col = colZSide;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y - scale * blockHeight, 
			                                                    pos.Z + z, rec.U1, rec.V2, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X - scale, pos.Y + scale * blockHeight, 
			                                                    pos.Z + z, rec.U1, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y + scale * blockHeight, 
			                                                    pos.Z + z, rec.U2, rec.V1, col );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + scale, pos.Y - scale * blockHeight, 
			                                                    pos.Z + z, rec.U2, rec.V2, col );
		}

		static void DrawXFace( byte block, float x, int side ) {
			int texId = info.GetTextureLoc( block, side );
			TextureRectangle rec = atlas.GetTexRec( texId );
			if( blockHeight != 1 )
				rec.V2 = rec.V1 + blockHeight * TerrainAtlas2D.invElementSize;
			FastColour col = colXSide;
			
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y - scale * blockHeight, 
			                                                    pos.Z - scale, rec.U1, rec.V2, colNormal );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + scale * blockHeight, 
			                                                    pos.Z - scale, rec.U1, rec.V1, colNormal );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y + scale * blockHeight, 
			                                                    pos.Z + scale, rec.U2, rec.V1, colNormal );
			cache.vertices[index++] = new VertexPos3fTex2fCol4b( pos.X + x, pos.Y - scale * blockHeight, 
			                                                    pos.Z + scale, rec.U2, rec.V2, colNormal );
		}
	}
}