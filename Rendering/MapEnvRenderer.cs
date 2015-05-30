using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public sealed class MapEnvRenderer : IDisposable {
		
		public Map Map;
		public Game Window;
		public IGraphicsApi Graphics;
		
		public MapEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int sidesVboId = -1, edgesVboId = -1;
		int edgeTexId, sideTexId;
		int sidesVertices, edgesVertices, index;
		static readonly FastColour sidesCol = new FastColour( 128, 128, 128 ), edgesCol = FastColour.White;
		bool legacy = false;
		
		public void SetUseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSidesAndEdges( null, null );
		}
		
		public void Init() {
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
			Window.EnvVariableChanged += EnvVariableChanged;
			Window.ViewDistanceChanged += ResetSidesAndEdges;
			Window.TerrainAtlasChanged += ResetTextures;
			
			Graphics = Window.Graphics;
			MakeTexture( ref edgeTexId, ref lastEdgeTexLoc, Map.EdgeBlock );
			MakeTexture( ref sideTexId, ref lastSideTexLoc, Map.SidesBlock );
			ResetSidesAndEdges( null, null );
		}

		public void RenderMapSides( double deltaTime ) {
			if( sidesVboId == -1 ) return;
			
			Graphics.Texturing = true;
			Graphics.Bind2DTexture( sideTexId );
			Graphics.DrawVb( sidesVboId );
			Graphics.Texturing = false;
		}
		
		public void RenderMapEdges( double deltaTime ) {
			if( edgesVboId == -1 ) return;
			
			Graphics.Texturing = true;
			Graphics.AlphaBlending = true;
			Graphics.Bind2DTexture( edgeTexId );
			Graphics.DrawVb( edgesVboId );
			Graphics.AlphaBlending = false;
			Graphics.Texturing = false;
		}
		
		public void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
			Window.EnvVariableChanged -= EnvVariableChanged;
			Window.ViewDistanceChanged -= ResetSidesAndEdges;
			Window.TerrainAtlasChanged -= ResetTextures;
			
			Graphics.DeleteTexture( ref edgeTexId );
			Graphics.DeleteTexture( ref sideTexId );
			Graphics.DeleteVb( sidesVboId );
			Graphics.DeleteVb( edgesVboId );
			sidesVboId = edgesVboId = -1;
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			Graphics.DeleteVb( sidesVboId );
			Graphics.DeleteVb( edgesVboId );
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
			Graphics.DeleteVb( sidesVboId );
			if( legacy ) {
				RebuildSidesLegacy( Map.GroundHeight );
			} else {
				RebuildSidesModern( Map.GroundHeight );
			}
		}
		
		void RebuildEdges() {
			index = 0;
			Graphics.DeleteVb( edgesVboId );
			if( legacy ) {
				RebuildEdgesLegacy( Map.WaterHeight );
			} else {
				RebuildEdgesModern( Map.WaterHeight );
			}
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
			sidesVboId = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		void RebuildEdgesModern( int waterLevel ) {
			edgesVertices = 4 * 6;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[edgesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlane( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, edgesCol, vertices );
			}			
			edgesVboId = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		#endregion
		
		#region Legacy
		
		void RebuildSidesLegacy( int groundLevel ) {
			sidesVertices = 0;
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				sidesVertices += Utils.CountVertices( rec.Width, rec.Height, axisSize ); // YPlanes outside
			}
			sidesVertices += Utils.CountVertices( Map.Width, Map.Length, axisSize ); // YPlane beneath map
			sidesVertices += Utils.CountVertices( Map.Width, groundLevel, axisSize ) * 2; // ZPlanes
			sidesVertices += Utils.CountVertices( Map.Length, groundLevel, axisSize ) * 2; // XPlanes
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[sidesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlaneParts( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, groundLevel, sidesCol, vertices );
			}
			DrawYPlaneParts( 0, 0, Map.Width, Map.Length, 0, sidesCol, vertices );
			DrawZPlaneParts( 0, 0, Map.Width, 0, groundLevel, sidesCol, vertices );
			DrawZPlaneParts( Map.Length, 0, Map.Width, 0, groundLevel, sidesCol, vertices );
			DrawXPlaneParts( 0, 0, Map.Length, 0, groundLevel, sidesCol, vertices );
			DrawXPlaneParts( Map.Width, 0, Map.Length, 0, groundLevel, sidesCol, vertices );
			sidesVboId = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		void RebuildEdgesLegacy( int waterLevel ) {
			edgesVertices = 0;
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				edgesVertices += Utils.CountVertices( rec.Width, rec.Height, axisSize ); // YPlanes outside
			}
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[edgesVertices];
			
			foreach( Rectangle rec in OutsideMap( Window.ViewDistance ) ) {
				DrawYPlaneParts( rec.X, rec.Y, rec.X + rec.Width, rec.Y + rec.Height, waterLevel, edgesCol, vertices );
			}
			edgesVboId = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		const int axisSize = 128;	
		void DrawXPlaneParts( int x, int z1, int z2, int y1, int y2, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			int length = z2 - z1, endZ = z2;
			int height = y2 - y1, endY = y2, startY = y1;
			for( ; z1 < endZ; z1 += axisSize ) {
				z2 = z1 + axisSize;
				if( z2 > endZ ) z2 = endZ;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					DrawXPlane( x, z1, z2, y1, y2, col, vertices );
				}
			}
		}
		
		void DrawZPlaneParts( int z, int x1, int x2, int y1, int y2, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			int width = x2 - x1, endX = x2;
			int height= y2 - y1, endY = y2, startY = y1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				y1 = startY;
				for( ; y1 < endY; y1 += axisSize ) {
					y2 = y1 + axisSize;
					if( y2 > endY ) y2 = endY;
					DrawZPlane( z, x1, x2, y1, y2, col, vertices );
				}
			}
		}
		
		void DrawYPlaneParts( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			int width = x2 - x1, endX = x2;
			int length = z2 - z1, endZ = z2, startZ = z1;
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += axisSize ) {
					z2 = z1 + axisSize;
					if( z2 > endZ ) z2 = endZ;
					DrawYPlane( x1, z1, x2, z2, y, col, vertices );
				}
			}
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