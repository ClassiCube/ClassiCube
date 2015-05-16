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
		public void Render( double deltaTime ) {
			graphics.Texturing = true;
			graphics.Bind2DTexture( rainTexture );
			graphics.PushMatrix();
			Vector3 pos = Window.LocalPlayer.Position;
			graphics.Translate( pos.X, pos.Y, pos.Z );
			graphics.SetMatrixMode( MatrixType.Texture );
			graphics.PushMatrix();
			graphics.Translate( 0, (float)( -Window.accumulator / 2 ), 0 );
			
			VertexPos3fTex2f[] vertices = new VertexPos3fTex2f[6];
			vertices[0] = new VertexPos3fTex2f( -1, 0, -0.5f, 0, 1 );
			vertices[1] = new VertexPos3fTex2f( -1, 6, -0.5f, 0, 0 );
			vertices[2] = new VertexPos3fTex2f( -1, 6, 0.5f, 1, 0 );
			
			vertices[3] = new VertexPos3fTex2f( -1, 6, 0.5f, 1, 0 );
			vertices[4] = new VertexPos3fTex2f( -1, 0, 0.5f, 1, 1 );
			vertices[5] = new VertexPos3fTex2f( -1, 0, -0.5f, 0, 1 );
			graphics.DrawVertices( DrawMode.Triangles, vertices );
			
			graphics.PopMatrix();
			graphics.SetMatrixMode( MatrixType.Modelview );
			graphics.Texturing = false;
			graphics.PopMatrix();
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
