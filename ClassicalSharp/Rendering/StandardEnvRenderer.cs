using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public unsafe class StandardEnvRenderer : EnvRenderer {
		
		public StandardEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int cloudTexture = -1, cloudsVb = -1, cloudsIndices;
		int skyOffset = 10, skyVb = -1, skyIndices;
		public float CloudsSpeed = 1;
		bool legacy;
		
		public void SetUseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSky();
			ResetClouds();
		}
		
		public override void Render( double deltaTime ) {
			if( skyVb == -1 || cloudsVb == -1 ) return;
			
			Vector3 pos = Window.LocalPlayer.EyePosition;
			if( pos.Y < Map.Height + skyOffset ) {
				Graphics.BeginVbBatch( VertexFormat.Pos3fCol4b );
				Graphics.BindVb( skyVb );
				Graphics.DrawIndexedVb( DrawMode.Triangles, skyIndices, 0 );
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
			Graphics.DeleteVb( skyVb );
			Graphics.DeleteVb( cloudsVb );
			skyVb = cloudsVb = -1;
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			Graphics.Fog = true;
			ResetAllEnv( null, null );
		}
		
		public override void Init() {
			base.Init();
			Graphics.Fog = true;
			ResetAllEnv( null, null );
			cloudTexture = Graphics.CreateTexture( "clouds.png" );
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
			Graphics.DeleteVb( skyVb );
			Graphics.DeleteVb( cloudsVb );
			Graphics.DeleteTexture( ref cloudTexture );
		}
		
		void RenderClouds( double delta ) {
			double time = Window.accumulator;
			float offset = (float)( time / 2048f * 0.6f * CloudsSpeed );
			Graphics.SetMatrixMode( MatrixType.Texture );
			Matrix4 matrix = Matrix4.Translate( offset, 0, 0 );
			Graphics.LoadMatrix( ref matrix );
			Graphics.SetMatrixMode( MatrixType.Modelview );
			
			Graphics.AlphaTest = true;
			Graphics.Texturing = true;
			Graphics.BindTexture( cloudTexture );
			Graphics.BeginVbBatch( VertexFormat.Pos3fTex2fCol4b );
			Graphics.BindVb( cloudsVb );
			Graphics.DrawIndexedVb_TrisT2fC4b( cloudsIndices, 0 );
			Graphics.AlphaTest = false;
			Graphics.Texturing = false;
			
			Graphics.SetMatrixMode( MatrixType.Texture );
			Graphics.LoadIdentityMatrix();
			Graphics.SetMatrixMode( MatrixType.Modelview );
		}
		
		double BlendFactor( int x ) {
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
				float blend = (float)BlendFactor( Window.ViewDistance );
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
			Graphics.DeleteVb( cloudsVb );
			ResetClouds( Window.ViewDistance, legacy ? 128 : 65536 );
		}
		
		void ResetSky() {
			if( Map.IsNotLoaded ) return;
			Graphics.DeleteVb( skyVb );
			ResetSky( Window.ViewDistance, legacy ? 128 : 65536 );
		}
		
		void ResetClouds( int extent, int axisSize ) {
			int x1 = -extent, x2 = Map.Width + extent;
			int z1 = -extent, z2 = Map.Length + extent;
			cloudsIndices = Utils.CountIndices( x2 - x1, z2 - z1, axisSize );
			
			VertexPos3fTex2fCol4b* vertices = stackalloc VertexPos3fTex2fCol4b[cloudsIndices / 6 * 4];
			DrawCloudsY( x1, z1, x2, z2, Map.Height + 2, axisSize, Map.CloudsCol, vertices );
			cloudsVb = Graphics.CreateVb( (IntPtr)vertices, VertexFormat.Pos3fTex2fCol4b, cloudsIndices / 6 * 4 );
		}
		
		void ResetSky( int extent, int axisSize ) {
			int x1 = -extent, x2 = Map.Width + extent;
			int z1 = -extent, z2 = Map.Length + extent;
			skyIndices = Utils.CountIndices( x2 - x1, z2 - z1, axisSize );
			
			VertexPos3fCol4b* vertices = stackalloc VertexPos3fCol4b[skyIndices / 6 * 4];
			DrawSkyY( x1, z1, x2, z2, Map.Height + skyOffset, axisSize, Map.SkyCol, vertices );
			skyVb = Graphics.CreateVb( (IntPtr)vertices, VertexFormat.Pos3fCol4b, skyIndices / 6 * 4 );
		}
		
		void DrawSkyY( int x1, int z1, int x2, int z2, int y, int axisSize, FastColour col, VertexPos3fCol4b* vertices ) {
			int endX = x2, endZ = z2, startZ = z1;
			
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += axisSize ) {
					z2 = z1 + axisSize;
					if( z2 > endZ ) z2 = endZ;
					
					*vertices++ = new VertexPos3fCol4b( x1, y, z1, col );
					*vertices++ = new VertexPos3fCol4b( x1, y, z2, col );
					*vertices++ = new VertexPos3fCol4b( x2, y, z2, col );
					*vertices++ = new VertexPos3fCol4b( x2, y, z1, col );
				}
			}
		}
		
		void DrawCloudsY( int x1, int z1, int x2, int z2, int y, int axisSize, FastColour col, VertexPos3fTex2fCol4b* vertices ) {
			int endX = x2, endZ = z2, startZ = z1;
			
			for( ; x1 < endX; x1 += axisSize ) {
				x2 = x1 + axisSize;
				if( x2 > endX ) x2 = endX;
				z1 = startZ;
				for( ; z1 < endZ; z1 += axisSize ) {
					z2 = z1 + axisSize;
					if( z2 > endZ ) z2 = endZ;
					
					*vertices++ = new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x1, y, z2, x1 / 2048f, z2 / 2048f, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, col );
					*vertices++ = new VertexPos3fTex2fCol4b( x2, y, z1, x2 / 2048f, z1 / 2048f, col );
				}
			}
		}
	}
}