using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	/// <summary> Same as NormalEnvRenderer, but breaks sky and clouds into 128 sized blocks. </summary>
	public class LegacyEnvRenderer : EnvRenderer {
		
		public LegacyEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int cloudTexture = -1, cloudsVbo = -1, cloudsVertices = 0;
		int skyOffset = 10, skyVbo = -1, skyVertices = 6;
		public float CloudsSpeed = 1f;
		
		public override void Render( double deltaTime ) {
			if( skyVbo != -1 ) {
				Vector3 pos = Window.LocalPlayer.EyePosition;
				if( pos.Y < Map.Height + skyOffset ) {
					Graphics.DrawVbPos3fCol4b( DrawMode.Triangles, skyVbo, skyVertices );
				}
			}
			if( cloudsVbo != -1 ) {
				RenderClouds( deltaTime );
			}
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
			skyVbo = -1;
			cloudsVbo = -1;
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			Graphics.Fog = true;
			ResetFog();
			ResetSky();
			ResetClouds();
		}
		
		public override void Init() {
			base.Init();
			Graphics.Fog = true;
			ResetFog();
			ResetSky();
			ResetClouds();
			cloudTexture = Graphics.LoadTexture( "clouds.png" );
			Window.ViewDistanceChanged += delegate {
				ResetFog();
				ResetSky();
				ResetClouds();
			};
		}
		
		public override void Dispose() {
			base.Dispose();
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
			Graphics.DrawVbPos3fTex2fCol4b( DrawMode.Triangles, cloudsVbo, cloudsVertices );
			Graphics.AlphaTest = false;
			Graphics.Texturing = false;
			
			Graphics.SetMatrixMode( MatrixType.Texture );
			Graphics.LoadIdentityMatrix();
			Graphics.SetMatrixMode( MatrixType.Modelview );
		}
		
		void ResetClouds() {
			Graphics.DeleteVb( cloudsVbo );
			if( Map.IsNotLoaded ) return;
			
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = Map.Width + extent;
			int y = Map.Height + 2;
			int z1 = 0 - extent, z2 = Map.Length + extent;
			FastColour cloudsCol = Map.CloudsCol;
			
			cloudsVertices = CountVertices( x2 - x1, z2 - z1 );
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[cloudsVertices];
			DrawCloudsYPlane( x1, z1, x2, z2, y, cloudsCol, vertices );
			cloudsVbo = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		void ResetSky() {
			Graphics.DeleteVb( skyVbo );
			if( Map.IsNotLoaded ) return;
			
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = Map.Width + extent;
			int y = Map.Height + skyOffset;
			int z1 = 0 - extent, z2 = Map.Length + extent;
			FastColour skyCol = Map.SkyCol;
			
			skyVertices = CountVertices( x2 - x1, z2 - z1 );
			VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[skyVertices];
			DrawSkyYPlane( x1, z1, x2, z2, y, skyCol, vertices );
			skyVbo = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fCol4b );
		}
		
		int CountVertices( int axis1Len, int axis2Len ) {
			int cellsAxis1 = axis1Len / 128 + ( axis1Len % 128 != 0 ? 1 : 0 );
			int cellsAxis2 = axis2Len / 128 + ( axis2Len % 128 != 0 ? 1 : 0 );
			return cellsAxis1 * cellsAxis2 * 6;
		}
		
		void DrawSkyYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fCol4b[] vertices ) {
			int width = x2 - x1, endX = x2;
			int length = z2 - z1, endZ = z2, startZ = z1;
			int index = 0;
			
			for( ; x1 < endX; x1 += 128 ) {
				x2 = x1 + 128;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += 128 ) {
					z2 = z1 + 128;
					if( z2 > endZ ) z2 = endZ;
					
					vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
					vertices[index++] = new VertexPos3fCol4b( x1, y, z2, col );
					vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
			
					vertices[index++] = new VertexPos3fCol4b( x2, y, z2, col );
					vertices[index++] = new VertexPos3fCol4b( x2, y, z1, col );
					vertices[index++] = new VertexPos3fCol4b( x1, y, z1, col );
				}
			}
		}
		
		void DrawCloudsYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			int width = x2 - x1, endX = x2;
			int length = z2 - z1, endZ = z2, startZ = z1;
			int index = 0;
			
			for( ; x1 < endX; x1 += 128 ) {
				x2 = x1 + 128;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += 128 ) {
					z2 = z1 + 128;
					if( z2 > endZ ) z2 = endZ;
					
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, x1 / 2048f, z2 / 2048f, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, col );
			
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, x2 / 2048f, z1 / 2048f, col );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, col );
				}
			}
		}
		
		double blendFactor( int x ) {
			//return -0.05 + 0.22 * Math.Log( Math.Pow( x, 0.25 ) );
			double blend = -0.13 + 0.28 * Math.Log( Math.Pow( x, 0.25 ) );
			if( blend < 0 ) blend = 0;
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
				adjFogCol.R = (byte)( Utils.Lerp( fogCol.R / 255f, skyCol.R / 255f, blend ) * 255 );
				adjFogCol.G = (byte)( Utils.Lerp( fogCol.G / 255f, skyCol.G / 255f, blend ) * 255 );
				adjFogCol.B = (byte)( Utils.Lerp( fogCol.B / 255f, skyCol.B / 255f, blend ) * 255 );
				Graphics.SetFogMode( Fog.Linear );
				Graphics.SetFogStart( 0 );
				Graphics.SetFogEnd( Window.ViewDistance );
			}
			Graphics.ClearColour( adjFogCol );
			Graphics.SetFogColour( adjFogCol );
		}
	}
}