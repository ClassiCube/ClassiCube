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
			Graphics.DeleteTexture( cloudTexture );
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
			if( cloudsVbo != -1 ) {
				Graphics.DeleteVb( cloudsVbo );
			}
			if( Map.IsNotLoaded ) return;
			// Calculate the number of possible visible 'cloud grid cells'.
			int dist = Window.ViewDistance;
			const int cloudSize = 512;
			int extent = cloudSize * ( dist / cloudSize + ( dist % cloudSize != 0 ? 1 : 0 ) ); // ceiling division
			
			// Calculate how many 'cloud grid cells' are actually visible.
			int cellsCount = 0;
			for( int x = -extent; x < Map.Width + extent; x += cloudSize ) {
				for( int z = -extent; z < Map.Length + extent; z += cloudSize ) {
					if( x + cloudSize < -dist || z + cloudSize < -dist ) continue;
					if( x > Map.Width + dist || z > Map.Length + dist ) continue;
					cellsCount++;
				}
			}
			cloudsVertices = cellsCount * 6;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[cloudsVertices];
			int index = 0;
			int y = Map.Height + 2;
			FastColour cloudsCol = Map.CloudsCol;
			
			for( int x = -extent; x < Map.Width + extent; x += cloudSize ) {
				for( int z = -extent; z < Map.Length + extent; z += cloudSize ) {
					int x2 = x + cloudSize;
					int z2 = z + cloudSize;
					if( x2 < -dist || z2 < -dist ) continue;
					if( x > Map.Width + dist || z > Map.Length + dist ) continue;
					// Clip clouds to visible view distance (avoid overdrawing)
					x2 = Math.Min( x2, Map.Width + dist );
					z2 = Math.Min( z2, Map.Length + dist );
					int x1 = Math.Max( x, -dist );
					int z1 = Math.Max( z, -dist );
					
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, cloudsCol );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, x2 / 2048f, z1 / 2048f, cloudsCol );
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, cloudsCol );
					
					vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, cloudsCol );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, x1 / 2048f, z2 / 2048f, cloudsCol );
					vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, cloudsCol );
				}
			}
			cloudsVbo = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		void ResetSky() {
			if( skyVbo != -1 ) {
				Graphics.DeleteVb( skyVbo );
			}
			if( Map.IsNotLoaded ) return;
			VertexPos3fCol4b[] vertices = new VertexPos3fCol4b[skyVertices];
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = Map.Width + extent;
			int y = Map.Height + skyOffset;
			int z1 = 0 - extent, z2 = Map.Length + extent;
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