using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace DefaultPlugin {

	public sealed class StandardWeatherRenderer : WeatherRenderer {
		
		WeatherShader shader;	
		public StandardWeatherRenderer( Game window ) : base( window ) {	
		}
		
		int weatherVb;
		int rainTexture, snowTexture;
		float vOffset;
		VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[12 * 9 * 9];
		public override void Render( double deltaTime ) {
			Weather weather = map.Weather;
			if( weather == Weather.Sunny ) return;
			
			api.UseProgram( shader.ProgramId );
			shader.UpdateFogAndMVPState( ref Window.MVP );
			
			api.Bind2DTexture( weather == Weather.Rainy ? rainTexture : snowTexture );
			Vector3I pos = Vector3I.Floor( Window.LocalPlayer.Position );
			float speed = weather == Weather.Rainy ? 1f : 0.25f;
			vOffset = -(float)Window.accumulator * speed;
			
			int index = 0;
			api.AlphaBlending = true;
			FastColour col = FastColour.White;
			for( int dx = - 4; dx <= 4; dx++ ) {
				for( int dz = -4; dz <= 4; dz++ ) {
					int rainY = Math.Max( pos.Y, map.GetSolidHeight( pos.X + dx, pos.Z + dz ) + 1 );
					int height = Math.Min( 6 - ( rainY - pos.Y ), 6 );
					if( height <= 0 ) continue;
					
					col.A = (byte)Math.Max( 0, AlphaAt( dx * dx + dz * dz ) );
					MakeRainForSquare( pos.X + dx, rainY, height, pos.Z + dz, col, ref index );
				}
			}
			shader.DrawDynamic( DrawMode.Triangles, VertexPos3fTex2fCol4b.Size, weatherVb, vertices, index );
			api.AlphaBlending = false;
		}
		
		float AlphaAt( float x ) {
			// Wolfram Alpha: fit {0,178},{1,169},{4,147},{9,114},{16,59},{25,9}
			return (float)( 0.05 * x * x - 8 * x + 178 );
		}
		
		void MakeRainForSquare( int x, int y, int height, int z, FastColour col, ref int index ) {
			float v1 = vOffset + ( z & 0x01 ) * 0.5f - ( x & 0x0F ) * 0.0625f;
			float v2 = height / 6f + v1;
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, 0, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z, 0, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z + 1, 2, v1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z + 1, 2, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z + 1, 2, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z, 0, v2, col );
			
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, 2, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y + height, z, 2, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z + 1, 0, v1, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y + height, z + 1, 0, v1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x, y, z + 1, 0, v2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + 1, y, z, 2, v2, col );
		}
		
		public override void Init() {
			rainTexture = api.LoadTexture( "rain.png" );
			snowTexture = api.LoadTexture( "snow.png" );
			weatherVb = api.CreateDynamicVb( VertexPos3fTex2fCol4b.Size, 12 * 9 * 9 );
			shader = new WeatherShader();
			shader.Init( api );
		}
		
		public override void Dispose() {
			api.DeleteTexture( ref rainTexture );
			api.DeleteTexture( ref snowTexture );
		}
	}
	
	public sealed class WeatherShader : FogAndMVPShader {
		
		public WeatherShader() {
			VertexSource = @"
#version 130
--IMPORT pos3fTex2fCol4b_attributes
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
flat in vec4 out_colour;
out vec4 final_colour;
uniform sampler2D texImage;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour = texture2D(texImage, out_texcoords) * out_colour;

--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int texImageLoc;
		protected override void GetLocations() {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
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
