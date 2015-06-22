using System;
using ClassicalSharp.GraphicsAPI;

namespace DefaultPlugin {
	
	public sealed class MapShader : FogAndMVPShader {
		
		public MapShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec4 in_flags;
in vec2 in_texcoords;
flat out vec4 out_colour;
flat out float out_type;
out vec2 out_texcoords;
uniform mat4 MVP;
uniform vec4 lightColours[8];

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = lightColours[int(in_flags.x)];
   out_type = in_flags.y;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
flat in vec4 out_colour;
flat in float out_type;
out vec4 final_colour;
uniform sampler2D texImage;
--IMPORT fog_uniforms

void main() {
   vec4 texColour = texture2D(texImage, out_texcoords);
   if(texColour.a < 0.5) {
      discard;
   }
   vec4 finalColour = texColour * out_colour;
   
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int texImageLoc, lightsColourLoc;
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			texCoordsLoc = GetAttribLocation( "in_texcoords" );
			colourLoc = GetAttribLocation( "in_flags" );
			
			texImageLoc = GetUniformLocation( "texImage" );
			lightsColourLoc = GetUniformLocation( "lightColours" );
			base.GetLocations();
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, false, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
	
	public sealed class MapDepthPassShader : Shader {
		
		public MapDepthPassShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
}";
			
			FragmentSource = @"
#version 130
in float out_test;
out vec4 final_colour;

void main() {
   final_colour = vec4(1.0, 1.0, 1.0, 1.0);
}";
		}
		
		public int positionLoc;
		public int mvpLoc;
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			mvpLoc = GetUniformLocation( "MVP" );
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
		}
	}
}
