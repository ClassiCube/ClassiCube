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
			
			Graphics.Fog = false;
			Graphics.UseProgram( shader.ProgramId );
			Graphics.SetUniform( shader.mvpLoc, ref Window.mvp );
			ResetFog( true );
			
			Vector3 pos = Window.LocalPlayer.EyePosition;
			if( pos.Y < Map.Height + skyOffset ) {
				RenderSky();
			}
			RenderClouds( deltaTime );
			Graphics.UseProgram( 0 );
			Graphics.Fog = true;
		}

		public void SetSkyOffset( int offset ) {
			skyOffset = offset;
			ResetSky();
		}
		
		protected override void CloudsColourChanged() {
			ResetClouds();
		}
		
		protected override void FogColourChanged() {
			ResetFog( false );
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
			ResetFog( false );
			ResetSky();
			ResetClouds();
		}
		
		public override void Init() {
			base.Init();
			Graphics.Fog = true;
			ResetFog( false );
			ResetSky();
			ResetClouds();
			cloudTexture = Graphics.LoadTexture( "clouds.png" );
			Window.ViewDistanceChanged += delegate {
				ResetFog( false );
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
			
			Graphics.DrawArrays( DrawMode.Triangles, 0, skyVertices );
			Graphics.BindVb( 0 );
			
			Graphics.DisableVertexAttribArray( shader.positionLoc );
			Graphics.DisableVertexAttribArray( shader.texCoordsLoc );
			Graphics.DisableVertexAttribArray( shader.colourLoc );
		}
		
		void RenderClouds( double delta ) {
			double time = Window.accumulator;
			float offset = (float)( time / 2048f * 0.6f * CloudsSpeed );
			Graphics.SetUniform( shader.sOffsetLoc, offset );
			Graphics.Texturing = true;
			Graphics.Bind2DTexture( cloudTexture );
			
			Graphics.BindVb( cloudsVbo );
			Graphics.EnableAndSetVertexAttribPointerF( shader.positionLoc, 3, stride, 0 );
			Graphics.EnableAndSetVertexAttribPointerF( shader.texCoordsLoc, 2, stride, 12 );
			Graphics.EnableAndSetVertexAttribPointer( shader.colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			Graphics.DrawArrays( DrawMode.Triangles, 0, cloudsVertices );
			Graphics.BindVb( 0 );
			
			Graphics.DisableVertexAttribArray( shader.positionLoc );
			Graphics.DisableVertexAttribArray( shader.texCoordsLoc );
			Graphics.DisableVertexAttribArray( shader.colourLoc );
			Graphics.Texturing = false;
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
			
			VertexPos3fTex2fCol4b[] vertices = {
				new VertexPos3fTex2fCol4b( x2, y, z1, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x1, y, z1, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x1, y, z2, -1000, -1000, skyCol ),
				
				new VertexPos3fTex2fCol4b( x2, y, z1, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x2, y, z2, -1000, -1000, skyCol ),
				new VertexPos3fTex2fCol4b( x1, y, z2, -1000, -1000, skyCol ),
			};
			skyVbo = Graphics.InitVb( vertices, DrawMode.Triangles, VertexFormat.VertexPos3fTex2fCol4b );
		}
		
		double blendFactor( int x ) {
			//return -0.05 + 0.22 * Math.Log( Math.Pow( x, 0.25 ) );
			double blend = -0.13 + 0.28 * Math.Log( Math.Pow( x, 0.25 ) );
			if( blend < 0 ) blend = 0;
			return blend;
		}
		
		void ResetFog( bool updateShaders ) {
			if( Map.IsNotLoaded ) return;
			FastColour fogCol = Map.FogCol;
			FastColour skyCol = Map.SkyCol;
			FastColour adjFogCol = fogCol;
			Block headBlock = Window.LocalPlayer.BlockAtHead;
			
			if( headBlock == Block.Water || headBlock == Block.StillWater ) {
				Graphics.SetFogMode( Fog.Exp );
				Graphics.SetFogDensity( 0.1f );
				adjFogCol = new FastColour( 5, 5, 51 );
				if( updateShaders ) {
					Graphics.SetUniform( shader.fogModeLoc, 1 );
					Graphics.SetUniform( shader.fogDensityLoc, 0.1f );
				}
			} else if( headBlock == Block.Lava || headBlock == Block.StillLava ) {
				Graphics.SetFogMode( Fog.Exp );
				Graphics.SetFogDensity( 2f );
				adjFogCol = new FastColour( 153, 25, 0 );
				if( updateShaders ) {
					Graphics.SetUniform( shader.fogModeLoc, 1 );
					Graphics.SetUniform( shader.fogDensityLoc, 2f );
				}
			} else {
				// Blend fog and sky together
				float blend = (float)blendFactor( Window.ViewDistance );
				adjFogCol.R = (byte)( Utils.Lerp( fogCol.R / 255f, skyCol.R / 255f, blend ) * 255 );
				adjFogCol.G = (byte)( Utils.Lerp( fogCol.G / 255f, skyCol.G / 255f, blend ) * 255 );
				adjFogCol.B = (byte)( Utils.Lerp( fogCol.B / 255f, skyCol.B / 255f, blend ) * 255 );
				Graphics.SetFogMode( Fog.Linear );
				Graphics.SetFogStart( 0 );
				Graphics.SetFogEnd( Window.ViewDistance );
				if( updateShaders ) {
					Graphics.SetUniform( shader.fogModeLoc, 0 );
					Graphics.SetUniform( shader.fogEndLoc, Window.ViewDistance );
				}
			}
			Graphics.ClearColour( adjFogCol );
			Graphics.SetFogColour( adjFogCol );
			if( updateShaders ) {
				Vector4 colVec4 = new Vector4( adjFogCol.R / 255f, adjFogCol.G / 255f, adjFogCol.B / 255f, 1 );
				Graphics.SetUniform( shader.fogColLoc, ref colVec4 );
			}
		}
	}
}