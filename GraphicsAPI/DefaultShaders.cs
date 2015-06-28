using System;

namespace ClassicalSharp.GraphicsAPI {
	
	public sealed class GuiShader : Shader {
		
		public GuiShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec4 in_colour;
in vec2 in_texcoords;
flat out vec4 out_colour;
out vec2 out_texcoords;

uniform mat4 proj;

void main() {
   gl_Position = proj * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
flat in vec4 out_colour;
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
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			texCoordsLoc = GetAttribLocation( "in_texcoords" );
			colourLoc = GetAttribLocation( "in_colour" );
			
			texImageLoc = GetUniformLocation( "texImage" );
			projLoc = GetUniformLocation( "proj" );
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
	
	public sealed class EntityShader : FogAndMVPShader {
		
		public EntityShader() {
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
uniform float colour;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour = texture2D(texImage, out_texcoords) * vec4(colour, colour, colour, 1.0);
   if(finalColour.a < 0.5) {
      discard;
   }
   
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, texCoordsLoc;
		public int texImageLoc, colourLoc;
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			texCoordsLoc = GetAttribLocation( "in_texcoords" );
			
			texImageLoc = GetUniformLocation( "texImage" );
			colourLoc = GetUniformLocation( "colour" );
			base.GetLocations();
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 12 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
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
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			texCoordsLoc = GetAttribLocation( "in_texcoords" );
			
			texImageLoc = GetUniformLocation( "texImage" );
			base.GetLocations();
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( texCoordsLoc, 2, stride, 12 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( texCoordsLoc );
		}
	}
	
	public sealed class SelectionShader : FogAndMVPShader {
		
		public SelectionShader() {
			VertexSource = @"
#version 130
in vec3 in_position;
in vec4 in_colour;
flat out vec4 out_colour;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_colour = in_colour;
}";
			
			FragmentSource = @"
#version 130
in vec2 out_texcoords;
flat in vec4 out_colour;
out vec4 final_colour;
--IMPORT fog_uniforms

void main() {
   vec4 finalColour = out_colour;
   
--IMPORT fog_code
   final_colour = finalColour;
}";
		}
		
		public int positionLoc, colourLoc;
		protected override void GetLocations() {
			positionLoc = GetAttribLocation( "in_position" );
			colourLoc = GetAttribLocation( "in_colour" );
			base.GetLocations();
		}
		
		protected override void EnableVertexAttribStates( int stride ) {
			api.EnableVertexAttribF( positionLoc, 3, stride, 0 );
			api.EnableVertexAttribF( colourLoc, 4, VertexAttribType.UInt8, true, stride, 12 );
		}
		
		protected override void DisableVertexAttribStates( int stride ) {
			api.DisableVertexAttrib( positionLoc );
			api.DisableVertexAttrib( colourLoc );
		}
	}
}
