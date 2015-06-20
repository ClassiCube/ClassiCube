using System;

namespace ClassicalSharp.GraphicsAPI {
	
	public sealed class GuiShader : Shader {
		
		public GuiShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec4 in_colour;
in vec2 in_texcoords;
out vec4 out_colour;
out vec2 out_texcoords;

uniform mat4 proj;

void main() {
   gl_Position = proj * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
in vec4 out_colour;
in vec2 out_texcoords;
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
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
	
	public abstract class FogAndMVPShader : Shader {
		
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		protected override void GetLocations( OpenGLApi api ) {
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
			
			fogColLoc = api.GetUniformLocation( ProgramId, "fogColour" );
			fogEndLoc = api.GetUniformLocation( ProgramId, "fogEnd" );
			fogDensityLoc = api.GetUniformLocation( ProgramId, "fogDensity" );
			fogModeLoc = api.GetUniformLocation( ProgramId, "fogMode" );
		}
	}

	public sealed class PickingShader : FogAndMVPShader {
		
		public PickingShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
}";
			
			FragmentSource = @"
#version 130
out vec4 final_colour;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour = vec4(1.0, 1.0, 1.0, 1.0);
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc;
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			base.GetLocations( api );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
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
in vec4 out_colour;
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
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			sOffsetLoc = api.GetUniformLocation( ProgramId, "sOffset" );
			base.GetLocations( api );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
	
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
in vec4 out_colour;
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
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			base.GetLocations( api );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
	
	public sealed class ParticleShader : FogAndMVPShader {
		
		public ParticleShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec2 in_texcoords;
out vec2 out_texcoords;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
out vec4 final_colour;
uniform sampler2D texImage;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour = texture2D(texImage, out_texcoords);
   if(finalColour.a < 0.5) {
      discard;
   }
   
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc;
		public int texImageLoc;
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			base.GetLocations( api );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 12 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
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
in vec4 out_colour;
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
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			base.GetLocations( api );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
	
	public sealed class MapEnvShader : FogAndMVPShader {
		
		public MapEnvShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec4 in_colour;
in vec2 in_texcoords;
out vec4 out_colour;
out vec2 out_texcoords;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
in vec4 out_colour;
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
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			base.GetLocations( api );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 16 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
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
		protected override void GetLocations( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
		}
		
		protected override void EnableVertexAttribStates( OpenGLApi api, int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
		}
		
		protected override void DisableVertexAttribStates( OpenGLApi api, int stride ) {
			api.DisableVertexAttrib( positionLoc );
		}
	}
}
