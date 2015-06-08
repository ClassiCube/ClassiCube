using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public class StandardEnvRenderer : EnvRenderer {
		
		public StandardEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int cloudTexture = -1, cloudsVbo = -1, cloudsVertices;
		int skyOffset = 10, skyVbo = -1, skyVertices;
		int index;
		public float CloudsSpeed = 1f;
		bool legacy;
		
		public void SetUseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSky();
			ResetClouds();
		}
		
		public override void Render( double deltaTime ) {
			if( skyVbo == -1 || cloudsVbo == -1 ) return;
			
			Vector3 pos = Window.LocalPlayer.EyePosition;
			if( pos.Y < Map.Height + skyOffset ) {
				Graphics.DrawVb( DrawMode.Triangles, VertexFormat.Pos3fCol4b, skyVbo, 0, skyVertices );
			}
			RenderClouds( deltaTime );
			ResetFog();
		}

		public void SetSkyOffset( int offset ) {
			skyOffset = offset;
			ResetSky();
		}
		
		protected override void CloudsColourChanged() {
			ResetClouds();
		}
		
		protected override void FogColourChanged() {
			ResetFog();
		}
		
		protected override void SkyColourChanged() {
			ResetSky();
		}
		
		public override void OnNewMap( object sender, EventArgs e ) {
			Graphics.Fog = false;
			Graphics.DeleteVb( skyVbo );
			Graphics.DeleteVb( cloudsVbo );
			skyVbo = cloudsVbo = -1;
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			Graphics.Fog = true;
			ResetAllEnv( null, null );
		}
		
		public override void Init() {
			base.Init();
			Graphics.Fog = true;
			ResetAllEnv( null, null );
			cloudTexture = Graphics.LoadTexture( "clouds.png" );
			Window.ViewDistanceChanged += ResetAllEnv;
		}
		
		void ResetAllEnv( object sender, EventArgs e ) {
			ResetFog();
			ResetSky();
			ResetClouds();
		}
		
		public override void Dispose() {
			base.Dispose();
			Window.ViewDistanceChanged -= ResetAllEnv;
			Graphics.DeleteVb( skyVbo );
			Graphics.DeleteVb( cloudsVbo );
			Graphics.DeleteTexture( ref cloudTexture );
		}
		
		void RenderClouds( double delta ) {
			double time = Window.accumulator;
			float offset = (float)( time / 2048f * 0.6f * CloudsSpeed );
			Graphics.SetMatrixMode( MatrixType.Texture );
			Matrix4 matrix = Matrix4.CreateTranslation( offset, 0, 0 );
			Graphics.LoadMatrix( ref matrix );
			Graphics.SetMatrixMode( MatrixType.Modelview );
			
			Graphics.AlphaTest = true;
			Graphics.Texturing = true;
			Graphics.Bind2DTexture( cloudTexture );
			Graphics.DrawVb( DrawMode.Triangles, VertexFormat.Pos3fTex2fCol4b, cloudsVbo, 0, cloudsVertices );
			Graphics.AlphaTest = false;
			Graphics.Texturing = false;
			
			Graphics.SetMatrixMode( MatrixType.Texture );
			Graphics.LoadIdentityMatrix();
			Graphics.SetMatrixMode( MatrixType.Modelview );
		}
		
		double blendFactor( int x ) {
			//return -0.05 + 0.22 * Math.Log( Math.Pow( x, 0.25 ) );
			double blend = -0.13 + 0.28 * Math.Log( Math.Pow( x, 0.25 ) );
			if( blend < 0 ) blend = 0;
			if( blend > 1 ) blend = 1;
			return blend;
		}
		
		void ResetFog() {
			if( Map.IsNotLoaded ) return;
			FastColour fogCol = Map.FogCol;
			FastColour skyCol = Map.SkyCol;
			FastColour adjFogCol = fogCol;
			Block headBlock = Window.LocalPlayer.BlockAtHead;
			
			if( headBlock == Block.Water || headBlock == Block.StillWater ) {
				Graphics.SetFogMode( Fog.Exp );
				Graphics.SetFogDensity( 0.1f );
				adjFogCol = new FastColour( 5, 5, 51 );
			} else if( headBlock == Block.Lava || headBlock == Block.StillLava ) {
				Graphics.SetFogMode( Fog.Exp );
				Graphics.SetFogDensity( 2f );
				adjFogCol = new FastColour( 153, 25, 0 );
			} else {
				// Blend fog and sky together
				float blend = (float)blendFactor( Window.ViewDistance );
				adjFogCol.R = (byte)Utils.Lerp( fogCol.R, skyCol.R, blend );
				adjFogCol.G = (byte)Utils.Lerp( fogCol.G, skyCol.G, blend );
				adjFogCol.B = (byte)Utils.Lerp( fogCol.B, skyCol.B, blend );
				Graphics.SetFogMode( Fog.Linear );
				Graphics.SetFogStart( 0 );
				Graphics.SetFogEnd( Window.ViewDistance );
			}
			Graphics.ClearColour( adjFogCol );
			Graphics.SetFogColour( adjFogCol );
		}
		
		void ResetClouds() {
			if( Map.IsNotLoaded ) return;
			index = 0;
			Graphics.DeleteVb( cloudsVbo );
			int extent = Window.ViewDistance;
			
			if( legacy ) {
				ResetCloudsLegacy( Map.Height + 2, -extent, -extent, Map.Width + extent, Map.Length + extent );
			} else {
				ResetCloudsModern( Map.Height + 2, -extent, -extent, Map.Width + extent, Map.Length + extent );
			}
		}
		
		void ResetSky() {
			if( Map.IsNotLoaded ) return;
			index = 0;
			Graphics.DeleteVb( skyVbo );
			int extent = Window.ViewDistance;
			
			if( legacy ) {
				ResetSkyLegacy( Map.Height + skyOffset, -extent, -extent, Map.Width + extent, Map.Length + extent );
			} else {
				ResetSkyModern( Map.Height + skyOffset, -extent, -extent, Map.Width + extent, Map.Length + extent );
			}
		}
		
		#region Modern
		
		void ResetCloudsModern( int y, int x1, int z1, int x2, int z2 ) {
			cloudsVertices = 6;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[cloudsVertices];		
			DrawCloudsYPlane( x1, z1, x2, z2, y, Map.CloudsCol, vertices );
			cloudsVbo = Graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
		}
		
		void ResetSkyModern( int y, int x1, int z1, int x2, int z2 ) {
			skyVertices = 6;
			VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[skyVertices];
			DrawSkyYPlane( x1, z1, x2, z2, y, Map.SkyCol, vertices );
			skyVbo = Graphics.InitVb( vertices, VertexFormat.Pos3fCol4b );
		}
		
		#endregion
		
		#region Legacy
		
		void ResetCloudsLegacy( int y, int x1, int z1, int x2, int z2 ) {
			cloudsVertices = Utils.CountVertices( x2 - x1, z2 - z1, 128 );
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[cloudsVertices];
			DrawCloudsYPlaneParts( x1, z1, x2, z2, y, Map.CloudsCol, vertices );
			cloudsVbo = Graphics.InitVb( vertices, VertexFormat.Pos3fTex2fCol4b );
		}
		
		void ResetSkyLegacy( int y, int x1, int z1, int x2, int z2 ) {
			skyVertices = Utils.CountVertices( x2 - x1, z2 - z1, 128 );
			VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[skyVertices];
			DrawSkyYPlaneParts( x1, z1, x2, z2, y, Map.SkyCol, vertices );
			skyVbo = Graphics.InitVb( vertices, VertexFormat.Pos3fCol4b );
		}
		
		void DrawSkyYPlaneParts( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fCol4b[] vertices ) {
			int endX = x2, endZ = z2, startZ = z1;
			
			for( ; x1 < endX; x1 += 128 ) {
				x2 = x1 + 128;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += 128 ) {
					z2 = z1 + 128;
					if( z2 > endZ ) z2 = endZ;
					DrawSkyYPlane( x1, z1, x2, z2, y, col, vertices );
				}
			}
		}
		
		void DrawCloudsYPlaneParts( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			int endX = x2, endZ = z2, startZ = z1;
			
			for( ; x1 < endX; x1 += 128 ) {
				x2 = x1 + 128;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += 128 ) {
					z2 = z1 + 128;
					if( z2 > endZ ) z2 = endZ;
					DrawCloudsYPlane( x1, z1, x2, z2, y, col, vertices );
				}
			}
		}
		
		#endregion
		
		void DrawCloudsYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, x1 / 2048f, z2 / 2048f, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, x2 / 2048f, z1 / 2048f, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, col );
		}
		
		void DrawSkyYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fCol4b[] vertices ) {
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			
			vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			vertices[index++] = new VertexPos3fCol4b( x2, y, z1, col );
			vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
		}
	}
}