using System;

namespace ClassicalSharp.GraphicsAPI {
	
	public abstract class Shader {
		public string FragSource, VertexSource;
		
		public int ProgramId;
		
		public void Initialise( OpenGLApi api ) {
			ProgramId = api.CreateProgram( VertexSource, FragSource );
			BindParameters( api );
		}
		
		protected virtual void BindParameters( OpenGLApi api ) {
			
		}
		
		public abstract void DrawVb( OpenGLApi graphics, DrawMode mode, int vb, int verticesCount );
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
		protected override void BindParameters( OpenGLApi api ) {
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			projLoc = api.GetUniformLocation( ProgramId, "proj" );
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		public override void DrawVb( OpenGLApi graphics, DrawMode mode, int vbId, int verticesCount ) {
			graphics.BindVb( vbId );
			graphics.EnableAndSetVertexAttribPointerF( positionLoc, 3, stride, 0 );
			graphics.EnableAndSetVertexAttribPointerF( texCoordsLoc, 2, stride, 12 );
			graphics.EnableAndSetVertexAttribPointer( colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			graphics.DrawVb( mode, 0, verticesCount );
			
			graphics.DisableVertexAttribArray( positionLoc );
			graphics.DisableVertexAttribArray( texCoordsLoc );
			graphics.DisableVertexAttribArray( colourLoc );
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
}";		
		}
		
		public int positionLoc;
		public int mvpLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
		}
		
		const int stride = 12;
		public override void DrawVb( OpenGLApi graphics, DrawMode mode, int vbId, int verticesCount ) {
			graphics.BindVb( vbId );
			graphics.EnableAndSetVertexAttribPointerF( positionLoc, 3, stride, 0 );			
			graphics.DrawVb( mode, 0, verticesCount );		
			graphics.DisableVertexAttribArray( positionLoc );
		}
	}
	
	public sealed class EnvShader : Shader {
		
		public EnvShader() {
			VertexSource = @"
#version 120
attribute vec3 in_position;
attribute vec2 in_texcoords;
attribute vec4 in_colour;
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragSource = @"
#version 120
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform sampler2D texImage;
uniform vec4 fogColour;
uniform float fogEnd;
uniform float fogDensity;
uniform float fogMode;
uniform float sOffset;
// 0 = linear, 1 = exp

void main() {
   vec4 finalColour;
   if(out_texcoords.s >= -900) {
      vec2 texcoords = out_texcoords;
      texcoords.s = texcoords.s + sOffset;
      finalColour = texture2D(texImage, texcoords) * out_colour;
   } else {
      finalColour = out_colour;
   }
   if(finalColour.a < 0.5) {
      discard;
   }
   
   float alpha = finalColour.a;
   float depth = (gl_FragCoord.z / gl_FragCoord.w);
   float f = 0;
   if(fogMode == 0) {
      f = (fogEnd - depth) / fogEnd; // omit (end - start) since start is 0
   } else {
      f = exp(-fogDensity * depth);
   }
   f = clamp(f, 0.0, 1.0);
   finalColour = mix(fogColour, finalColour, f);     
   finalColour.a = alpha;
   gl_FragColor = finalColour;
}";	
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		public int sOffsetLoc, texImageLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
			fogColLoc = api.GetUniformLocation( ProgramId, "fogColour" );
			fogEndLoc = api.GetUniformLocation( ProgramId, "fogEnd" );
			fogDensityLoc = api.GetUniformLocation( ProgramId, "fogDensity" );
			fogModeLoc = api.GetUniformLocation( ProgramId, "fogMode" );
			sOffsetLoc = api.GetUniformLocation( ProgramId, "sOffset" );
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		public override void DrawVb( OpenGLApi graphics, DrawMode mode, int vb, int verticesCount ) {
			graphics.BindVb( vb );
			graphics.EnableAndSetVertexAttribPointerF( positionLoc, 3, stride, 0 );
			graphics.EnableAndSetVertexAttribPointerF( texCoordsLoc, 2, stride, 12 );
			graphics.EnableAndSetVertexAttribPointer( colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			graphics.DrawVb( mode, 0, verticesCount );
			
			graphics.DisableVertexAttribArray( positionLoc );
			graphics.DisableVertexAttribArray( texCoordsLoc );
			graphics.DisableVertexAttribArray( colourLoc );
		}
	}
	
	public sealed class MapShader : Shader {
		
		public MapShader() {
			VertexSource = @"
#version 120
attribute vec3 in_position;
attribute vec2 in_texcoords;
attribute vec4 in_colour;
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   out_texcoords = in_texcoords;
   out_colour = in_colour;
}";
			
			FragSource = @"
#version 120
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform sampler2D texImage;
uniform vec4 fogColour;
uniform float fogEnd;
uniform float fogDensity;
uniform float fogMode;

void main() {
   vec4 finalColour = texture2D(texImage, out_texcoords) * out_colour;
   if(finalColour.a < 0.5) {
      discard;
   }
   if(fogMode != 0) {
      float alpha = finalColour.a;
      float depth = (gl_FragCoord.z / gl_FragCoord.w);
      float f = 0;
      if(fogMode == 1 ) {
         f = (fogEnd - depth) / fogEnd; // omit (end - start) since start is 0
      } else if(fogMode == 2) {
         f = exp(-fogDensity * depth);
      }

      f = clamp(f, 0.0, 1.0);
      finalColour = mix(fogColour, finalColour, f);     
      finalColour.a = alpha; // fog shouldn't affect alpha
   }
   gl_FragColor = finalColour;
}";	
		}
		
		public int positionLoc, texCoordsLoc, colourLoc;
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		public int texImageLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
			fogColLoc = api.GetUniformLocation( ProgramId, "fogColour" );
			fogEndLoc = api.GetUniformLocation( ProgramId, "fogEnd" );
			fogDensityLoc = api.GetUniformLocation( ProgramId, "fogDensity" );
			fogModeLoc = api.GetUniformLocation( ProgramId, "fogMode" );
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		public override void DrawVb( OpenGLApi graphics, DrawMode mode, int vbId, int verticesCount ) {
			graphics.BindVb( vbId );
			graphics.EnableAndSetVertexAttribPointerF( positionLoc, 3, stride, 0 );
			graphics.EnableAndSetVertexAttribPointerF( texCoordsLoc, 2, stride, 12 );
			graphics.EnableAndSetVertexAttribPointer( colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			graphics.DrawVb( mode, 0, verticesCount );
			
			graphics.DisableVertexAttribArray( positionLoc );
			graphics.DisableVertexAttribArray( texCoordsLoc );
			graphics.DisableVertexAttribArray( colourLoc );
		}
	}
	
		public sealed class MapPackedShader : Shader {
		
		public MapPackedShader() {
			VertexSource = @"
#version 130
in uint in_packed;
uniform vec3 offset; // MAKE WITH VERTEX DIVISOR
out vec2 out_texcoords;
flat out float out_texU1;
out vec4 out_colour;
uniform mat4 MVP;
uniform vec4 colour_indices[8] = vec4[](
// In sunlight
vec4(1.0, 1.0, 1.0, 1.0),// Y top
vec4(0.8, 0.8, 0.8, 1.0),// Z side
vec4(0.6, 0.6, 0.6, 1.0),// X side
vec4(0.5, 0.5, 0.5, 1.0),// Y bottom
// In shadow
vec4(0.64, 0.64, 0.64, 1.0),
vec4(0.512, 0.512, 0.512, 1.0),
vec4(0.384, 0.384, 0.384, 1.0),
vec4(0.32, 0.32, 0.32, 1.0)
);
uniform float y_offsets[4] = float[](0.0, 0.125, 0.5, 1.0);

void main() {
   vec3 pos = offset;
   pos.x += in_packed & 0x1Fu;
   pos.y += (in_packed & 0x3E0u ) >> 5u;
   pos.z += (in_packed & 0x7C00u ) >> 10u;
   pos.y += y_offsets[(in_packed & 0x18000u) >> 15u];
      
   gl_Position = MVP * vec4(pos, 1.0);
   
   uint meta = in_packed >> 17u;  
   out_texcoords = vec2( float(meta & 0x0Fu), float((meta & 0x70u) >> 4u) );
   out_texU1 = out_texcoords.x;
   out_texcoords.x += float((meta & 0xF80u) >> 7u);
   out_colour = colour_indices[meta >> 12u];
}";
			
			FragSource = @"
#version 130
flat in float out_texU1;
in vec2 out_texcoords;
in vec4 out_colour;
out vec4 frag_colour;

uniform sampler2D texImage;
uniform vec4 fogColour;
uniform float fogEnd;
uniform float fogDensity;
uniform float fogMode;

void main() {
   vec2 texCoords = out_texcoords;
   texCoords.x = (out_texU1 + fract(texCoords.x - out_texU1)) / 16.0;
   texCoords.y /= 16.0;
   vec4 finalColour = texture2D(texImage, texCoords) * out_colour;
   if(finalColour.a < 0.5) {
      discard;
   }
   
   if(fogMode != 0) {
      float alpha = finalColour.a;
      float depth = (gl_FragCoord.z / gl_FragCoord.w);
      float f = 0;
      if(fogMode == 1 ) {
         f = (fogEnd - depth) / fogEnd; // omit (end - start) since start is 0
      } else if(fogMode == 2) {
         f = exp(-fogDensity * depth);
      }

      f = clamp(f, 0.0, 1.0);
      finalColour = mix(fogColour, finalColour, f);     
      finalColour.a = alpha; // fog shouldn't affect alpha
   }
   frag_colour = finalColour;
}";	
		}
		
		public int flagsLoc, offsetLoc;
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		public int texImageLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			flagsLoc = api.GetAttribLocation( ProgramId, "in_packed" );
			offsetLoc = api.GetUniformLocation( ProgramId, "offset" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
			fogColLoc = api.GetUniformLocation( ProgramId, "fogColour" );
			fogEndLoc = api.GetUniformLocation( ProgramId, "fogEnd" );
			fogDensityLoc = api.GetUniformLocation( ProgramId, "fogDensity" );
			fogModeLoc = api.GetUniformLocation( ProgramId, "fogMode" );
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		public override void DrawVb( OpenGLApi graphics, DrawMode mode, int vbId, int verticesCount ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class MapPackedLiquidDepthShader : Shader {
		
		public MapPackedLiquidDepthShader() {
			VertexSource = @"
#version 130
in uint in_packed;
uniform vec3 offset; // MAKE WITH VERTEX DIVISOR
uniform mat4 MVP;
uniform float y_offsets[4] = float[](0.0, 0.125, 0.5, 1.0);

void main() {
   vec3 pos = offset;
   pos.x += in_packed & 0x1Fu;
   pos.y += (in_packed & 0x3E0u ) >> 5u;
   pos.z += (in_packed & 0x7C00u ) >> 10u;
   pos.y += y_offsets[(in_packed & 0x18000u) >> 15u];
      
   gl_Position = MVP * vec4(pos, 1.0);
}";
			
			FragSource = @"
#version 130
out vec4 frag_colour;

void main() {
   frag_colour = vec4(1.0, 1.0, 1.0, 1.0);
}";	
		}
		
		public int flagsLoc, offsetLoc, mvpLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			flagsLoc = api.GetAttribLocation( ProgramId, "in_packed" );
			offsetLoc = api.GetUniformLocation( ProgramId, "offset" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
		}
		
		const int stride = VertexPos3fTex2fCol4b.Size;
		public override void DrawVb( OpenGLApi graphics, DrawMode mode, int vbId, int verticesCount ) {
			throw new NotImplementedException();
		}
	}
}