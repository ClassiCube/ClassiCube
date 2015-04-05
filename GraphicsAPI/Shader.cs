using System;

namespace ClassicalSharp.GraphicsAPI {
	
	public abstract class Shader {
		public string FragSource, VertexSource;
		
		public int ProgramId;
		
		public void Initialise( ModernOpenGLApi api ) {
			ProgramId = api.CreateProgram( VertexSource, FragSource );
			BindParameters( api );
		}
		
		protected virtual void BindParameters( ModernOpenGLApi api ) {
			
		}
	}
	
	public sealed class GuiShader : Shader {
		
		public GuiShader() {
			VertexSource = @"
#version 120
attribute vec3 in_position;
attribute vec2 in_texcoords;
attribute vec4 in_colour;
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform mat4 proj;

void main() {
   gl_Position = proj * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragSource = @"
#version 120
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform sampler2D texImage;

void main() {
   vec4 finalColour;
   if(out_texcoords.r >= 0) {
      finalColour = texture2D(texImage, out_texcoords) * out_colour;
   } else {
      finalColour = out_colour;
   }
   if(finalColour.a < 0.05) {
      discard;
   }
   gl_FragColor = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int texImageLoc, projLoc;
		protected override void BindParameters( ModernOpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			projLoc = api.GetUniformLocation( ProgramId, "proj" );
		}
	}
	
	public sealed class PickingShader : Shader {
		
		public PickingShader() {
			VertexSource = @"
#version 120
attribute vec3 in_position;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
}";
			
			FragSource = @"
#version 120

void main() {
   gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}";		}
		
		public int positionLoc;
		public int mvpLoc;
		protected override void BindParameters( ModernOpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
		}
	}
}