using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp {

	public class WeatherRenderer {
		
		public Game Window;
		Map map;
		IGraphicsApi graphics;
		public WeatherRenderer( Game window ) {
			Window = window;
			map = Window.Map;
			graphics = window.Graphics;
		}
		
		int rainTexture;
		//short[] rainHeightmap;
		VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[12 * 9 * 9];
		public void Render( double deltaTime ) {
			graphics.Texturing = true;
			graphics.Bind2DTexture( rainTexture );
			Vector3 pos = Window.LocalPlayer.Position;
			graphics.SetMatrixMode( MatrixType.Texture );
			graphics.PushMatrix();
			graphics.Translate( 0, (float)( -Window.accumulator / 2 ), 0 );
			
			int index = 0;
			graphics.AlphaBlending = true;
			FastColour col = FastColour.White;
			col.A = 110;
			MakeRainForSquare( (int)pos.X + 4, (int)pos.Y, 6, (int)pos.Z, col, ref index );
			graphics.DrawVertices( DrawMode.Triangles, vertices, 12 );
			graphics.AlphaBlending = false;
			
			graphics.PopMatrix();
			graphics.SetMatrixMode( MatrixType.Modelview );
			graphics.Texturing = false;
		}
		
		void MakeRainForSquare( int x, int y, int height, int z, FastColour col, ref int index ) {
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, 0, 1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z, 0, 0, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z + 1, 2, 0, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z + 1, 2, 0, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, 2, 1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, 0, 1, col );
			
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, 2, 1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z, 2, 0, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z + 1, 0, 0, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z + 1, 0, 0, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, 0, 1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, 2, 1, col );
		}
		
		void OnNewMap( object sender, EventArgs e ) {
			//rainHeightmap = null;
		}
		
		void OnNewMapLoaded( object sender, EventArgs e ) {
			//rainHeightmap = new short[map.Width * map.Length];
		}
		
		public void Init() {
			rainTexture = graphics.LoadTexture( "rain.png" );
			Window.OnNewMap += OnNewMap;
			Window.OnNewMapLoaded += OnNewMapLoaded;
		}
		
		public void Dispose() {
			Window.OnNewMap -= OnNewMap;
			Window.OnNewMapLoaded -= OnNewMapLoaded;
		}
		
		// rain takes about 2 seconds to fall
		// rain is around 6 blocks high
	}
}
