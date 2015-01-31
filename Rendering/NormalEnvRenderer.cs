using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public class NormalEnvRenderer : EnvRenderer {
		
		public NormalEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int cloudTexture = -1, cloudsVbo = -1, cloudsVertices = 6;
		int skyOffset = 10, skyVbo = -1, skyVertices = 6;
		public float CloudsSpeed = 1f;
		
		public override void Render( double deltaTime ) {
			Vector3 pos = Window.LocalPlayer.EyePosition;
			Graphics.PushMatrix();
			Graphics.Translate( pos.X, 0, pos.Z );
			if( skyVbo != -1 ) {			
				if( pos.Y < Map.Height + skyOffset ) {
					Graphics.DrawVbPos3fCol4b( DrawMode.Triangles, skyVbo, skyVertices );
				}
			}
			if( cloudsVbo != -1 ) {
				RenderClouds( deltaTime, pos );
			}
			ResetFog();
			Graphics.PopMatrix();
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
			Graphics.DeleteTexture( cloudTexture );
		}
		
		void RenderClouds( double delta, Vector3 pos ) {
			double time = Window.accumulator;
			float offset = (float)( time / 2048f * 0.6f * CloudsSpeed );
			Graphics.SetMatrixMode( MatrixType.Texture );
			Graphics.Translate( offset, 0, 0 );
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
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[cloudsVertices];
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = 0 + extent;
			int y = Map.Height + 2;
			int z1 = 0 - extent, z2 = 0 + extent;
			FastColour cloudsCol = Map.CloudsCol;
			
			vertices[0] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, cloudsCol );
			vertices[1] = new VertexPos3fTex2fCol4b( x2, y, z1, x2 / 2048f, z1 / 2048f, cloudsCol );
			vertices[2] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, cloudsCol );
			
			vertices[3] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, cloudsCol );
			vertices[4] = new VertexPos3fTex2fCol4b( x1, y, z2, x1 / 2048f, z2 / 2048f, cloudsCol );
			vertices[5] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, cloudsCol );
			cloudsVbo = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		void ResetSky() {
			Graphics.DeleteVb( skyVbo );
			if( Map.IsNotLoaded ) return;
			VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[skyVertices];
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = 0 + extent;
			int y = Map.Height + skyOffset;
			int z1 = 0 - extent, z2 = 0 + extent;
			FastColour skyCol = Map.SkyCol;
			
			vertices[0] = new VertexPos3fCol4b( x2, y, z1, skyCol );
			vertices[1] = new VertexPos3fCol4b( x1, y, z1, skyCol );
			vertices[2] = new VertexPos3fCol4b( x1, y, z2, skyCol );
			
			vertices[3] = new VertexPos3fCol4b( x2, y, z1, skyCol );
			vertices[4] = new VertexPos3fCol4b( x2, y, z2, skyCol );
			vertices[5] = new VertexPos3fCol4b( x1, y, z2, skyCol );
			skyVbo = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fCol4b );
		}
		
		double blendFactor( int x ) {
			//return -0.05 + 0.22 * Math.Log( Math.Pow( x, 0.25 ) );
			double blend = -0.13 + 0.28 * Math.Log( Math.Pow( x, 0.25 ) );
			if( blend < 0 ) blend = 0;
			return blend;
		}
		
		public override void EnableAmbientLighting() {
			Block headBlock = Window.LocalPlayer.BlockAtHead;
			if( headBlock == Block.Water || headBlock == Block.StillWater ) {
				Graphics.AmbientLighting = true;
				Graphics.SetAmbientColour( new FastColour( 102, 102, 230 ) );
			} else if( headBlock == Block.Lava || headBlock == Block.StillLava ) {
				Graphics.AmbientLighting = true;
				Graphics.SetAmbientColour( new FastColour( 102, 77, 77 ) );
			} else {
				// don't turn on unnecessary ambient colour of 255,255,255
			}
		}
		
		public override void DisableAmbientLighting() {
			Graphics.AmbientLighting = false;
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