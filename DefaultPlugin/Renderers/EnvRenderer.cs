using System;
using System.Drawing;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;
using OpenTK;

namespace DefaultPlugin {

	public sealed class StandardEnvRenderer : EnvRenderer {
		
		public StandardEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		int cloudTexture = -1, cloudsVbo = -1, cloudsVertices = 6;
		int skyOffset = 10, skyVbo = -1, skyVertices = 6;
		int index;
		public float CloudsSpeed = 1f;
		EnvShader shader;
		
		public void SetUseLegacyMode( bool legacy ) {
			ResetSky();
			ResetClouds();
		}
		
		public override void Render( double deltaTime ) {
			if( skyVbo == -1 || cloudsVbo == -1 ) return;
			
			shader.Bind();
			shader.UpdateFogAndMVPState( ref Window.MVP );
			
			Vector3 pos = Window.LocalPlayer.EyePosition;
			if( pos.Y < Map.Height + skyOffset ) {
				shader.Draw( DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, skyVbo, 0, skyVertices );
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
			Graphics.DeleteVb( skyVbo );
			Graphics.DeleteVb( cloudsVbo );
			skyVbo = cloudsVbo = -1;
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			ResetAllEnv( null, null );
		}
		
		public override void Init() {
			base.Init();
			ResetAllEnv( null, null );
			cloudTexture = Graphics.LoadTexture( "clouds.png" );
			Window.ViewDistanceChanged += ResetAllEnv;
			shader = new EnvShader();
			shader.Init( Graphics );
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
			shader.Dispose();
		}
		
		void RenderClouds( double delta ) {
			double time = Window.accumulator;
			float offset = (float)( time / 2048f * 0.6f * CloudsSpeed );
			shader.SetUniform( shader.sOffsetLoc, offset );
			Graphics.Bind2DTexture( cloudTexture );
			shader.Draw(  DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, cloudsVbo, 0, cloudsVertices );
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
			ResetCloudsModern( Map.Height + 2, -extent, -extent, Map.Width + extent, Map.Length + extent );
		}
		
		void ResetSky() {
			if( Map.IsNotLoaded ) return;
			index = 0;
			Graphics.DeleteVb( skyVbo );
			int extent = Window.ViewDistance;
			ResetSkyModern( Map.Height + skyOffset, -extent, -extent, Map.Width + extent, Map.Length + extent );
		}
		
		#region Modern
		
		void ResetCloudsModern( int y, int x1, int z1, int x2, int z2 ) {
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[cloudsVertices];
			DrawCloudsYPlane( x1, z1, x2, z2, y, Map.CloudsCol, vertices );
			cloudsVbo = Graphics.InitVb( vertices, VertexPos3fTex2fCol4b.Size );
		}
		
		void ResetSkyModern( int y, int x1, int z1, int x2, int z2 ) {
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[skyVertices];
			DrawSkyYPlane( x1, z1, x2, z2, y, Map.SkyCol, vertices );
			skyVbo = Graphics.InitVb( vertices, VertexPos3fTex2fCol4b.Size );
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
		
		void DrawSkyYPlane( int x1, int z1, int x2, int z2, int y, FastColour col, VertexPos3fTex2fCol4b[] vertices ) {
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, -100000, -100000, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z2, -100000, -100000, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, -100000, -100000, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z2, -100000, -100000, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x2, y, z1, -100000, -100000, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x1, y, z1, -100000, -100000, col );
		}
	}
	
	public sealed class EnvShader : FogAndMVPShader {
		
		public EnvShader() {
			VertexSource = @"
#version 130
--IMPORT pos3fTex2fCol4b_attributes
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_colour = in_colour;
   out_texcoords = in_texcoords;
}";
			
			FragmentSource = @"
#version 130
flat in vec4 out_colour;
in vec2 out_texcoords;
out vec4 final_colour;
uniform sampler2D texImage;
uniform float sOffset;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour;
   if(out_texcoords.s >= -90000) {
      vec2 texcoords = out_texcoords;
      texcoords.s = texcoords.s + sOffset;
      finalColour = texture2D(texImage, texcoords) * out_colour;
   } else {
      finalColour = out_colour;
   }
   if(finalColour.a < 0.5) {
      discard;
   }
   
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int sOffsetLoc, texImageLoc;
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			texCoordsLoc = GetAttribLocation( "in_texcoords" );
			colourLoc = GetAttribLocation( "in_colour" );
			
			texImageLoc = GetUniformLocation( "texImage" );
			sOffsetLoc = GetUniformLocation( "sOffset" );
			base.GetLocations();
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
}