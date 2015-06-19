using System;

namespace ClassicalSharp.GraphicsAPI {

	public abstract class Shader {
		
		public string VertexSource;
		
		public string FragmentSource;
		
		public int ProgramId;
		
		public void Init( OpenGLApi api ) {
			ProgramId = api.CreateProgram( VertexSource, FragmentSource );
			GetLocations( api );
		}
		
		public void Draw( OpenGLApi api, DrawMode mode, VertexFormat format, int id, int startVertex, int verticesCount ) {
			EnableVertexAttribStates( api );
			api.DrawVb( mode, format, id, startVertex, verticesCount );
			DisableVertexAttribStates( api );
		}
		
		public void DrawIndexedVbBatch( OpenGLApi api, DrawMode mode, int vb, int ib, int indicesCount,
		                               int startVertex, int startIndex ) {
			EnableVertexAttribStates( api );
			api.DrawIndexedVbBatch( mode, vb, ib, indicesCount, startVertex, startIndex );
			DisableVertexAttribStates( api );
		}
		
		protected virtual void GetLocations( OpenGLApi api ) {
		}
		
		protected virtual void EnableVertexAttribStates( OpenGLApi api ) {
		}
		
		protected virtual void DisableVertexAttribStates( OpenGLApi api ) {
		}
	}
	
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
		public void DrawVb( OpenGLApi graphics, int vbId, int verticesCount ) {
			graphics.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			graphics.EnableVertexAttribF( texCoordsLoc, 2, stride, 12 );
			graphics.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			graphics.DrawModernVb( DrawMode.TriangleStrip, vbId, 0, verticesCount );
			
			graphics.DisableVertexAttrib( positionLoc );
			graphics.DisableVertexAttrib( texCoordsLoc );
			graphics.DisableVertexAttrib( colourLoc );
		}
	}
	
}
