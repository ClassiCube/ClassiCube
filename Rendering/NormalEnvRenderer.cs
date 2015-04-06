using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {

	public class NormalEnvRenderer : EnvRenderer {
		
		EnvShader shader;
		public NormalEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
			shader = new EnvShader();
			shader.Initialise( window.Graphics );
		}
		
		int cloudTexture = -1, cloudsVbo = -1, cloudsVertices = 6;
		int skyOffset = 10, skyVbo = -1, skyVertices = 6;
		public float CloudsSpeed = 1f;
		const int stride = VertexPos3fTex2fCol4b.Size;
		
		public override void Render( double deltaTime ) {
			if( Map.IsNotLoaded || skyVbo == -1 ) return;
			
			Graphics.UseProgram( shader.ProgramId );
			Graphics.SetUniform( shader.mvpLoc, ref Window.mvp );
			ResetFog();
			Graphics.SetUniform( shader.fogColLoc, ref Graphics.FogColour );
			Graphics.SetUniform( shader.fogDensityLoc, Graphics.FogDensity );
			Graphics.SetUniform( shader.fogEndLoc, Graphics.FogEnd );
			Graphics.SetUniform( shader.fogModeLoc, Graphics.FogMode == Fog.Linear ? 0 : 1 );
			
			Vector3 pos = Window.LocalPlayer.EyePosition;
			if( pos.Y < Map.Height + skyOffset ) {
				RenderSky();
			}
			RenderClouds( deltaTime );
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
			Graphics.DeleteVb( skyVbo );
			Graphics.DeleteVb( cloudsVbo );
			skyVbo = -1;
			cloudsVbo = -1;
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			ResetFog();
			ResetSky();
			ResetClouds();
		}
		
		public override void Init() {
			base.Init();
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
		
		void RenderSky() {
			Graphics.BindVb( skyVbo );
			Graphics.EnableAndSetVertexAttribPointerF( shader.positionLoc, 3, stride, 0 );
			Graphics.EnableAndSetVertexAttribPointerF( shader.texCoordsLoc, 2, stride, 12 );
			Graphics.EnableAndSetVertexAttribPointer( shader.colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			Graphics.DrawVb( DrawMode.Triangles, 0, skyVertices );
			
			Graphics.DisableVertexAttribArray( shader.positionLoc );
			Graphics.DisableVertexAttribArray( shader.texCoordsLoc );
			Graphics.DisableVertexAttribArray( shader.colourLoc );
		}
		
		void RenderClouds( double delta ) {
			double time = Window.accumulator;
			float offset = (float)( time / 2048f * 0.6f * CloudsSpeed );
			Graphics.SetUniform( shader.sOffsetLoc, offset );
			Graphics.Bind2DTexture( cloudTexture );
			
			Graphics.BindVb( cloudsVbo );
			Graphics.EnableAndSetVertexAttribPointerF( shader.positionLoc, 3, stride, 0 );
			Graphics.EnableAndSetVertexAttribPointerF( shader.texCoordsLoc, 2, stride, 12 );
			Graphics.EnableAndSetVertexAttribPointer( shader.colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			Graphics.DrawVb( DrawMode.Triangles, 0, cloudsVertices );
			
			Graphics.DisableVertexAttribArray( shader.positionLoc );
			Graphics.DisableVertexAttribArray( shader.texCoordsLoc );
			Graphics.DisableVertexAttribArray( shader.colourLoc );
		}
		
		void ResetClouds() {
			Graphics.DeleteVb( cloudsVbo );
			if( Map.IsNotLoaded ) return;
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = Map.Width + extent;
			int y = Map.Height + 2;
			int z1 = 0 - extent, z2 = Map.Length + extent;
			FastColour cloudsCol = Map.CloudsCol;
			
			VertexPos3fTex2fCol4b[] vertices = {
				new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, cloudsCol ),
				new VertexPos3fTex2fCol4b( x2, y, z1, x2 / 2048f, z1 / 2048f, cloudsCol ),
				new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, cloudsCol ),
				
				new VertexPos3fTex2fCol4b( x2, y, z2, x2 / 2048f, z2 / 2048f, cloudsCol ),
				new VertexPos3fTex2fCol4b( x1, y, z2, x1 / 2048f, z2 / 2048f, cloudsCol ),
				new VertexPos3fTex2fCol4b( x1, y, z1, x1 / 2048f, z1 / 2048f, cloudsCol ),
			};
			cloudsVbo = Graphics.InitVb( vertices, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		void ResetSky() {
			Graphics.DeleteVb( skyVbo );
			if( Map.IsNotLoaded ) return;
			int extent = Window.ViewDistance;
			int x1 = 0 - extent, x2 = Map.Width + extent;
			int y = Map.Height + skyOffset;
			int z1 = 0 - extent, z2 = Map.Length + extent;
			FastColour skyCol = Map.SkyCol;
			
			VertexPos3fTex2fCol4b[] vertices = {
				new VertexPos3fTex2fCol4b( x2, y, z1, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x1, y, z1, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x1, y, z2, -1000, -1000, skyCol ),
				
				new VertexPos3fTex2fCol4b( x2, y, z1, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x2, y, z2, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x1, y, z2, -1000, -1000, skyCol ),
			};
			skyVbo = Graphics.InitVb( vertices, VertexFormat.VertexPos3fTex2fCol4b );
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
				Graphics.SetFogEnd( Window.ViewDistance );
			}
			Graphics.ClearColour( adjFogCol );
			Graphics.SetFogColour( adjFogCol );
		}
	}
}