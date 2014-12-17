using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	/// <summary> Same as NormalMapEnvRenderer, but breaks edge and sides into 128 sized blocks. </summary>
	public class LegacyMapEnvRenderer : MapEnvRenderer {
		
		public LegacyMapEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		public override void Init() {
			base.Init();
			Window.ViewDistanceChanged += ResetEnv;
			Window.TerrainAtlasChanged += ResetTextures;
			MakeEdgeTexture();
			MakeSideTexture();
			ResetEnv( null, null );
		}
		
		void ResetTextures( object sender, EventArgs e ) {
			MakeEdgeTexture();
			MakeSideTexture();
		}

		void ResetEnv( object sender, EventArgs e ) {
			if( Window.Map.IsNotLoaded ) return;
			RebuildSides();
			RebuildEdges();
		}
		
		public override void RenderMapSides( double deltaTime ) {
			if( sidesVboId == -1 ) return;
			Graphics.Texturing = true;
			Graphics.Bind2DTexture( sideTexId );
			Graphics.DrawVbPos3fTex2fCol4b( DrawMode.Triangles, sidesVboId, sidesVertices );
			Graphics.Texturing = false;
		}
		
		public override void RenderMapEdges( double deltaTime ) {
			if( edgesVboId == -1 ) return;
			
			Graphics.Texturing = true;
			Graphics.AlphaBlending = true;
			Graphics.Bind2DTexture( edgeTexId );
			Graphics.DrawVbPos3fTex2fCol4b( DrawMode.Triangles, edgesVboId, edgeVertices );
			Graphics.AlphaBlending = false;
			Graphics.Texturing = false;
		}
		
		int index;
		VertexPos3fTex2fCol4b[] vertices;
		
		public override void OnNewMap( object sender, EventArgs e ) {
			Graphics.DeleteVb( sidesVboId );
			Graphics.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
			index = 0;
			MakeEdgeTexture();
			MakeSideTexture();
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			RebuildSides();
			RebuildEdges();
		}
		
		int sidesVertices, edgeVertices = 0;
		static readonly FastColour sidesCol = new FastColour( 128, 128, 128 ), edgeCol = FastColour.White;
		void RebuildSides() {
			if( sidesVboId != -1 ) {
				Graphics.DeleteVb( sidesVboId );
			}

			int groundLevel = Map.GroundHeight;
			sidesVertices = 0;			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				sidesVertices += CountVertices( rec.Width, rec.Height ); // YPlanes outside
			}
			sidesVertices += CountVertices( Map.Width, Map.Length ); // YPlane beneath map
			sidesVertices += CountVertices( Map.Width, groundLevel ) * 2; // ZPlanes
			sidesVertices += CountVertices( Map.Length, groundLevel ) * 2; // XPlanes
			vertices = new VertexPos3fTex2fCol4b[sidesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, sidesCol );
			}
			DrawYPlane( 0, 0, Map.Width, Map.Length, 0, sidesCol );
			DrawZPlane( 0, 0, Map.Width, 0, groundLevel, sidesCol );
			DrawZPlane( Map.Length, 0, Map.Width, 0, groundLevel, sidesCol );
			DrawXPlane( 0, 0, Map.Length, 0, groundLevel, sidesCol );
			DrawXPlane( Map.Width, 0, Map.Length, 0, groundLevel, sidesCol );
			sidesVboId = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
			index = 0;
		}		
		
		void RebuildEdges() {
			if( edgesVboId != -1 ) {
				Graphics.DeleteVb( edgesVboId );
			}
			
			int waterLevel = Map.WaterHeight;
			edgeVertices = 0;
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				edgeVertices += CountVertices( rec.Width, rec.Height ); // YPlanes outside
			}
			vertices = new VertexPos3fTex2fCol4b[edgeVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, edgeCol );
			}
			edgesVboId = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
			index = 0;
		}
		
		int sidesVboId = -1, edgesVboId = -1;
		int edgeTexId, sideTexId;
		
		public override void Dispose() {
			base.Dispose();
			index = 0;
			Graphics.DeleteTexture( edgeTexId );
			Graphics.DeleteTexture( sideTexId );
			Graphics.DeleteVb( sidesVboId );
			Graphics.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
			Window.ViewDistanceChanged -= ResetEnv;
		}
		
		protected override void EdgeBlockChanged() {
			MakeEdgeTexture();
		}
		
		protected override void SidesBlockChanged() {
			MakeSideTexture();
		}
		
		protected override void WaterLevelChanged() {
			ResetEnv( null, null );
		}
		
		void MakeEdgeTexture() {
			int texLoc = Window.BlockInfo.GetOptimTextureLoc( (byte)Map.EdgeBlock, TileSide.Top );
			Window.Graphics.DeleteTexture( edgeTexId );
			edgeTexId = Window.TerrainAtlas.LoadTextureElement( texLoc );
		}
		
		void MakeSideTexture() {
			int texLoc = Window.BlockInfo.GetOptimTextureLoc( (byte)Map.SidesBlock, TileSide.Top );
			Window.Graphics.DeleteTexture( sideTexId );
			sideTexId = Window.TerrainAtlas.LoadTextureElement( texLoc );
		}
			
		int CountVertices( int axis1Len, int axis2Len ) {
			int cellsAxis1 = axis1Len / 128 + ( axis1Len % 128 != 0 ? 1 : 0 );
			int cellsAxis2 = axis2Len / 128 + ( axis2Len % 128 != 0 ? 1 : 0 );
			return cellsAxis1 * cellsAxis2 * 6;
		}
		
		void DrawXPlane( int x, int z1, int z2, int y1, int y2, FastColour col ) {
			int length = z2 - z1, endZ = z2;
			int height = y2 - y1, endY = y2, startY = y1;
			for( ; z1 < endZ; z1 += 128 ) {
				z2 = z1 + 128;
				if( z2 > endZ ) z2 = endZ;
				y1 = startY;
				for( ; y1 < endY; y1 += 128 ) {
					y2 = y1 + 128;
					if( y2 > endY ) y2 = endY;
					
					TextureRectangle rec = new TextureRectangle( 0, 0, z2 - z1, y2 - y1 );
					vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V2, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
			
					vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V1, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
				}
			}
		}
		
		void DrawZPlane( int z, int x1, int x2, int y1, int y2, FastColour col ) {
			int width = x2 - x1, endX = x2;
			int height= y2 - y1, endY = y2, startY = y1;
			for( ; x1 < endX; x1 += 128 ) {
				x2 = x1 + 128;
				if( x2 > endX ) x2 = endX;
				y1 = startY;
				for( ; y1 < endY; y1 += 128 ) {
					y2 = y1 + 128;
					if( y2 > endY ) y2 = endY;
					
					TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, y2 - y1 );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V1, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y2, z, rec.U1, rec.V2, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V2, col );
			
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y2, z, rec.U2, rec.V2, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y1, z, rec.U2, rec.V1, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y1, z, rec.U1, rec.V1, col );
				}
			}
		}
		
		void DrawYPlane( int x1, int z1, int x2, int z2, int y, FastColour col ) {
			int width = x2 - x1, endX = x2;
			int length = z2 - z1, endZ = z2, startZ = z1;
			for( ; x1 < endX; x1 += 128 ) {
				x2 = x1 + 128;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += 128 ) {
					z2 = z1 + 128;
					if( z2 > endZ ) z2 = endZ;
					
					TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, z2 - z1 );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, rec.U1, rec.V2, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
			
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, rec.U2, rec.V2, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, rec.U2, rec.V1, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, rec.U1, rec.V1, col );
				}
			}
		}
	}
}