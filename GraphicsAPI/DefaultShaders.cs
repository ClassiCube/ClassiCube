using System;

namespace ClassicalSharp.GraphicsAPI {
	
	public sealed class GuiShader : Shader {
		
		public GuiShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec2 in_texcoords;
in vec4 in_colour;
out vec2 out_texcoords;
out vec4 out_colour;
uniform mat4 proj;

void main() {
   gl_Position = proj * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
in vec4 out_colour;
out vec4 final_colour;
uniform sampler2D texImage;

void main() {
   vec4 col;
   if(out_texcoords.r >= 0) {
      col = texture2D(texImage, out_texcoords) * out_colour;
   } else {
      col = out_colour;
   }
   if(col.a < 0.05) {
      discard;
   }
   final_colour = col;
}";
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int texImageLoc, projLoc;
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			projLoc = api.GetUniformLocation( ProgramId, "proj" );
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		protected override void EnableVertexAttribStates( OpenGLApi api ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );		
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}	
	}	
}
