using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class NormalMapEnvRenderer : MapEnvRenderer {
		
		MapShader shader;
		public NormalMapEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
			shader = window.MapRenderer.shader;
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

			Graphics.Bind2DTexture( sideTexId );
			shader.DrawVb( Graphics, sidesVboId, sidesVertices, DrawMode.Triangles );
		}
		
		public override void RenderMapEdges( double deltaTime ) {
			if( edgesVboId == -1 ) return;
			
			Graphics.AlphaBlending = true;
			Graphics.Bind2DTexture( edgeTexId );
			shader.DrawVb( Graphics, edgesVboId, edgesVertices, DrawMode.Triangles );
			Graphics.AlphaBlending = false;
		}
		
		const int sidesVertices = 5 * 6 + 4 * 6, edgesVertices = 4 * 6;
		int index;
		
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
		
		static readonly FastColour sidesCol = new FastColour( 128, 128, 128 ), edgesCol = FastColour.White;
		void RebuildSides() {
			Graphics.DeleteVb( sidesVboId );
			int groundLevel = Map.GroundHeight;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[sidesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, sidesCol, vertices );
			}
			DrawYPlane( 0, 0, Map.Width, Map.Length, 0, sidesCol, vertices );
			DrawZPlane( 0, 0, Map.Width, groundLevel, sidesCol, vertices );
			DrawZPlane( Map.Length, 0, Map.Width, groundLevel, sidesCol, vertices );
			DrawXPlane( 0, 0, Map.Length, groundLevel, sidesCol, vertices );
			DrawXPlane( Map.Width, 0, Map.Length, groundLevel, sidesCol, vertices  );
			
			sidesVboId = Graphics.InitVb( vertices, VertexPos3fTex2fCol4b.Size );
			index = 0;
		}
		
		void RebuildEdges() {
			Graphics.DeleteVb( edgesVboId );
			int waterLevel = Map.WaterHeight;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[edgesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, edgesCol, vertices );
			}
			
			edgesVboId = Graphics.InitVb( vertices, VertexPos3fTex2fCol4b.Size );
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
		
		void DrawXPlane( int x, int z1, int z2, int height, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = new TextureRectangle( 0, 0, z2 - z1, height );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, 0, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, height, z1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, height, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, height, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, 0, z2, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, 0, z1, rec.U1, rec.V1, col );
		}
		
		void DrawZPlane( int z, int x1, int x2, int height, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = new TextureRectangle( 0, 0, x2 - x1, height );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, 0, z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, height, z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, height, z, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, height, z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, 0, z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, 0, z, rec.U1, rec.V1, col );
		}
		
		void DrawYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
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