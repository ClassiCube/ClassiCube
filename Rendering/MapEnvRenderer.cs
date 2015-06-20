using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public sealed class MapEnvRenderer : IDisposable {
		
		public Map Map;
		public Game Window;
		public OpenGLApi api;
		MapEnvShader shader;
		
		public MapEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int sidesVboId = -1, edgesVboId = -1;
		int edgeTexId, sideTexId;
		int sidesVertices, edgesVertices, index;
		static readonly FastColour sidesCol = new FastColour( 128, 128, 128 ), edgesCol = FastColour.White;
		
		public void Init() {
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
			Window.EnvVariableChanged += EnvVariableChanged;
			Window.ViewDistanceChanged += ResetSidesAndEdges;
			Window.TerrainAtlasChanged += ResetTextures;
			
			api = Window.Graphics;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, Map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, Map.SidesBlock );
			ResetSidesAndEdges( null, null );
			shader = new MapEnvShader();
			shader.Init( api );
		}

		public void RenderMapSides( double deltaTime ) {
			if( sidesVboId == -1 ) return;
			
			api.UseProgram( shader.ProgramId );
			shader.UpdateFogAndMVPState( api, ref Window.MVP );
			
			api.Bind2DTexture( sideTexId );
			shader.Draw( api, DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, sidesVboId, 0, sidesVertices );
		}
		
		public void RenderMapEdges( double deltaTime ) {
			if( edgesVboId == -1 ) return;
			// Do not draw water when we cannot see it.
			// Fixes 'depth bleeding through' issues with 16 bit depth buffers on large maps.
			if( Window.LocalPlayer.EyePosition.Y < 0 ) return;
			
			api.AlphaBlending = true;
			api.Bind2DTexture( edgeTexId );
			shader.Draw( api, DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, edgesVboId, 0, edgesVertices );
			api.AlphaBlending = false;
		}
		
		public void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			Window.EnvVariableChanged -= EnvVariableChanged;
			Window.ViewDistanceChanged -= ResetSidesAndEdges;
			Window.TerrainAtlasChanged -= ResetTextures;
			
			api.DeleteTexture( ref edgeTexId );
			api.DeleteTexture( ref sideTexId );
			api.DeleteVb( sidesVboId );
			api.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			api.DeleteVb( sidesVboId );
			api.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, Map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, Map.SidesBlock );
		}
		
		void OnNewMapLoaded( object sender, EventArgs e ) {
			RebuildSides();
			RebuildEdges();
		}
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Variable == EnvVariable.EdgeBlock ) {
				MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, Map.EdgeBlock );
			} else if( e.Variable == EnvVariable.SidesBlock ) {
				MakeTexture( ref sideTexId, ref lastSideTexLoc, Map.SidesBlock );
			} else if( e.Variable == EnvVariable.WaterLevel ) {
				ResetSidesAndEdges( null, null );
			}
		}
		
		void ResetTextures( object sender, EventArgs e ) {
			lastEdgeTexLoc = lastSideTexLoc = -1;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, Map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, Map.SidesBlock );
		}

		void ResetSidesAndEdges( object sender, EventArgs e ) {
			if( Window.Map.IsNotLoaded ) return;
			RebuildSides();
			RebuildEdges();
		}
		
		void RebuildSides() {
			index = 0;
			api.DeleteVb( sidesVboId );
			RebuildSidesModern( Map.GroundHeight );
		}
		
		void RebuildEdges() {
			index = 0;
			api.DeleteVb( edgesVboId );
			RebuildEdgesModern( Map.WaterHeight );
		}
		
		// |-----------|
		// |     2     |
		// |---*****---|
		// | 3 *Map* 4 |
		// |   *   *   |
		// |---*****---|
		// |     1     |
		// |-----------|
		IEnumerable<Rectangle> OutsideMap( int extent ) {
			yield return new Rectangle( -extent, -extent, extent + Map.Width + extent, extent );
			yield return new Rectangle( -extent, Map.Length, extent + Map.Width + extent, extent );
			yield return new Rectangle( -extent, 0, extent, Map.Length );
			yield return new Rectangle( Map.Width, 0, extent, Map.Length );
		}
		
		#region Modern
		
		void RebuildSidesModern( int groundLevel ) {
			sidesVertices = 5 * 6 + 4 * 6;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[sidesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, sidesCol, vertices );
			}
			DrawYPlane( 0, 0, Map.Width, Map.Length, 0, sidesCol, vertices );
			DrawZPlane( 0, 0, Map.Width, 0, groundLevel, sidesCol, vertices );
			DrawZPlane( Map.Length, 0, Map.Width, 0, groundLevel, sidesCol, vertices );
			DrawXPlane( 0, 0, Map.Length, 0, groundLevel, sidesCol, vertices );
			DrawXPlane( Map.Width, 0, Map.Length, 0, groundLevel, sidesCol, vertices  );			
			sidesVboId = api.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
		}
		
		void RebuildEdgesModern( int waterLevel ) {
			edgesVertices = 4 * 6;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[edgesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, edgesCol, vertices );
			}			
			edgesVboId = api.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
		}
		
		#endregion
		
		int lastEdgeTexLoc, lastSideTexLoc;
		void MakeTexture( ref int texId, ref int lastTexLoc, Block block ) {
			int texLoc = Window.BlockInfo.GetOptimTextureLoc( (byte)block, TileSide.Top );
			if( texLoc != lastTexLoc ) {
				lastTexLoc = texLoc;
				Window.Graphics.DeleteTexture( ref texId );
				texId = Window.TerrainAtlas.LoadTextureElement( texLoc );
			}
		}	
		
		void DrawXPlane( int x, int z1, int z2, int y1, int y2, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			TextureRectangle rec = new TextureRectangle( 0, 0, z2 - z1, y2 - y1 );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z1, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y2, z2, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z2, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y1, z1, rec.U1, rec.V1, col );
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
		
		void DrawZPlane( int z, int x1, int x2, int y1, int y2, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
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