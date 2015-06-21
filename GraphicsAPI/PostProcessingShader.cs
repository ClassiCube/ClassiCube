using System;

namespace ClassicalSharp.GraphicsAPI {
	
	public abstract class PostProcessingShader : Shader {
		
		public PostProcessingShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec2 in_texcoords;
out vec2 out_texcoords;

uniform mat4 proj;

void main() {
   gl_Position = proj * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
}";
		}
		
		public int positionLoc, texCoordsLoc;
		public int texSceneLoc, projLoc;
		protected override void GetLocations() {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			
			texSceneLoc = api.GetUniformLocation( ProgramId, "texScene" );
			projLoc = api.GetUniformLocation( ProgramId, "proj" );
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
		}
		
		protected internal abstract void RenderTargetResized( int newWidth, int newHeight );
	}
}
