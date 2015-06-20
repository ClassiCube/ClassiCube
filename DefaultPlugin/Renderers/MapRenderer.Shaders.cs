using System;
using ClassicalSharp.GraphicsAPI;

namespace DefaultPlugin {
	
	public sealed class MapShader : FogAndMVPShader {
		
		public MapShader() {
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
   if(finalColour.a < 0.5) {
      discard;
   }
   
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
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
		}
	}
}
