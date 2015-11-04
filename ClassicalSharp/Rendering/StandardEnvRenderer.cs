using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public unsafe class StandardEnvRenderer : EnvRenderer {
		
		public StandardEnvRenderer( Game game ) {
			this.game = game;
			map = game.Map;
		}
		
		int cloudsVb = -1, cloudVertices, skyVb = -1, skyVertices;
		bool legacy;
		
		public void SetUseLegacyMode( bool legacy ) {
			this.legacy = legacy;
			ResetSky();
			ResetClouds();
		}
		
		public override void Render( double deltaTime ) {
			if( skyVb == -1 || cloudsVb == -1 ) return;
			
			Vector3 eyePos = game.LocalPlayer.EyePosition;
			Vector3 pos = game.Camera.GetCameraPos( eyePos );
			float normalY = map.Height + 8;
			float skyY = Math.Max( pos.Y + 8, normalY );
			
			graphics.SetBatchFormat( VertexFormat.Pos3fCol4b );
			graphics.BindVb( skyVb );
			if( skyY == normalY ) {
				graphics.DrawIndexedVb( DrawMode.Triangles, skyVertices * 6 / 4, 0 );
			} else {
				Matrix4 m = Matrix4.Translate( 0, skyY - normalY, 0 );
				graphics.PushMatrix();
				graphics.MultiplyMatrix( ref m );
				graphics.DrawIndexedVb( DrawMode.Triangles, skyVertices * 6 / 4, 0 );
				graphics.PopMatrix();
			}
			
			RenderClouds( deltaTime );
			ResetFog();
		}
		
		protected override void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.SkyColour ) {
				ResetSky();
			} else if( e.Var == EnvVar.FogColour ) {
				ResetFog();
			} else if( e.Var == EnvVar.CloudsColour ) {
				ResetClouds();
			} else if( e.Var == EnvVar.CloudsLevel ) {
				ResetSky();
				ResetClouds();
			}
		}
		
		public override void OnNewMap( object sender, EventArgs e ) {
			graphics.Fog = false;
			graphics.DeleteVb( skyVb );
			graphics.DeleteVb( cloudsVb );
			skyVb = cloudsVb = -1;
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			graphics.Fog = true;
			ResetAllEnv( null, null );
		}
		
		public override void Init() {
			base.Init();
			graphics.Fog = true;
			ResetAllEnv( null, null );
			game.Events.ViewDistanceChanged += ResetAllEnv;
		}
		
		void ResetAllEnv( object sender, EventArgs e ) {
			ResetFog();
			ResetSky();
			ResetClouds();
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.ViewDistanceChanged -= ResetAllEnv;
			graphics.DeleteVb( skyVb );
			graphics.DeleteVb( cloudsVb );
		}
		
		void RenderClouds( double delta ) {
			double time = game.accumulator;
			float offset = (float)( time / 2048f * 0.6f * map.CloudsSpeed );
			graphics.SetMatrixMode( MatrixType.Texture );
			Matrix4 matrix = Matrix4.Translate( offset, 0, 0 );
			graphics.LoadMatrix( ref matrix );
			graphics.SetMatrixMode( MatrixType.Modelview );
			
			graphics.AlphaTest = true;
			graphics.Texturing = true;
			graphics.BindTexture( game.CloudsTextureId );
			graphics.SetBatchFormat( VertexFormat.Pos3fTex2fCol4b );
			graphics.BindVb( cloudsVb );
			graphics.DrawIndexedVb_TrisT2fC4b( cloudVertices * 6 / 4, 0 );
			graphics.AlphaTest = false;
			graphics.Texturing = false;
			
			graphics.SetMatrixMode( MatrixType.Texture );
			graphics.LoadIdentityMatrix();
			graphics.SetMatrixMode( MatrixType.Modelview );
		}
		
		double BlendFactor( int x ) {
			//return -0.05 + 0.22 * Math.Log( Math.Pow( x, 0.25 ) );
			double blend = -0.13 + 0.28 * Math.Log( Math.Pow( x, 0.25 ) );
			if( blend < 0 ) blend = 0;
			if( blend > 1 ) blend = 1;
			return blend;
		}
		
		void ResetFog() {
			if( map.IsNotLoaded ) return;
			FastColour adjFogCol = FastColour.White;
			Block headBlock = game.LocalPlayer.BlockAtHead;
			BlockInfo info = game.BlockInfo;
			
			if( info.FogDensity[(byte)headBlock] != 0 ) {
				graphics.SetFogMode( Fog.Exp );
				graphics.SetFogDensity( info.FogDensity[(byte)headBlock] );
				adjFogCol = info.FogColour[(byte)headBlock];
			} else {
				// Blend fog and sky together
				FastColour fogCol = map.FogCol;
				FastColour skyCol = map.SkyCol;
				float blend = (float)BlendFactor( game.ViewDistance );
				adjFogCol.R = (byte)Utils.Lerp( fogCol.R, skyCol.R, blend );
				adjFogCol.G = (byte)Utils.Lerp( fogCol.G, skyCol.G, blend );
				adjFogCol.B = (byte)Utils.Lerp( fogCol.B, skyCol.B, blend );
				graphics.SetFogMode( Fog.Linear );
				graphics.SetFogStart( 0 );
				graphics.SetFogEnd( game.ViewDistance );
			}
			graphics.ClearColour( adjFogCol );
			graphics.SetFogColour( adjFogCol );
		}
		
		void ResetClouds() {
			if( map.IsNotLoaded ) return;
			graphics.DeleteVb( cloudsVb );
			ResetClouds( game.ViewDistance, legacy ? 128 : 65536 );
		}
		
		void ResetSky() {
			if( map.IsNotLoaded ) return;
			graphics.DeleteVb( skyVb );
			ResetSky( game.ViewDistance, legacy ? 128 : 65536 );
		}
		
		void ResetClouds( int extent, int axisSize ) {
			extent = Utils.AdjViewDist( extent );
			int x1 = -extent, x2 = map.Width + extent;
			int z1 = -extent, z2 = map.Length + extent;
			cloudVertices = Utils.CountVertices( x2 - x1, z2 - z1, axisSize );
			
			VertexPos3fTex2fCol4b* vertices = stackalloc VertexPos3fTex2fCol4b[cloudVertices];
			DrawCloudsY( x1, z1, x2, z2, map.CloudHeight, axisSize, map.CloudsCol, vertices );
			cloudsVb = graphics.CreateVb( (IntPtr)vertices, VertexFormat.Pos3fTex2fCol4b, cloudVertices );
		}
		
		void ResetSky( int extent, int axisSize ) {
			extent = Utils.AdjViewDist( extent );
			int x1 = -extent, x2 = map.Width + extent;
			int z1 = -extent, z2 = map.Length + extent;
			skyVertices = Utils.CountVertices( x2 - x1, z2 - z1, axisSize );
			
			VertexPos3fCol4b* vertices = stackalloc VertexPos3fCol4b[skyVertices];
			DrawSkyY( x1, z1, x2, z2, map.Height + 8, axisSize, map.SkyCol, vertices );
			skyVb = graphics.CreateVb( (IntPtr)vertices, VertexFormat.Pos3fCol4b, skyVertices );
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