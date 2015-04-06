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
		public void DrawVb( OpenGLApi graphics, int vbId, int verticesCount ) {
			graphics.BindVb( vbId );
			graphics.EnableAndSetVertexAttribPointerF( positionLoc, 3, stride, 0 );
			graphics.EnableAndSetVertexAttribPointerF( texCoordsLoc, 2, stride, 12 );
			graphics.EnableAndSetVertexAttribPointer( colourLoc, 4, VertexAttribType.UInt8, true, stride, 20 );
			
			graphics.DrawVb( DrawMode.TriangleStrip, 0, verticesCount );
			
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
		public void DrawVb( OpenGLApi graphics, int vbId, int verticesCount, DrawMode mode ) {
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
		public void DrawVb( OpenGLApi graphics, int vbId, int verticesCount, DrawMode mode ) {
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
	
	public sealed class OptimisedMapShader : Shader {
		
		public OptimisedMapShader() {
			VertexSource = @"
#version 120
attribute vec3 in_position;
attribute vec2 in_flags;
varying vec2 out_texcoords;
varying vec4 out_colour;
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(in_position / 16.0, 1.0);
   vec2 texCoords = vec2(0.0, in_flags.x / 16.0 );
   float texCoordsType = floor(in_flags.y / 64.0 );
   if( texCoordsType == 1 ) {
      texCoords.s = 0.0625;
   } else if( texCoordsType == 2 ) {
      texCoords.t += 0.0625;
   } else if( texCoordsType == 3 ) {
      texCoords.s = 0.0625;
      texCoords.t += 0.0625;
   }   
   out_texcoords = texCoords;
   float rgb = mod(in_flags.y, 64.0) / 63.0;
   out_colour = vec4(rgb, rgb, rgb, 1.0 );
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
		
		public int positionLoc, flagsLoc;
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		public int texImageLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			flagsLoc = api.GetAttribLocation( ProgramId, "in_flags" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
			fogColLoc = api.GetUniformLocation( ProgramId, "fogColour" );
			fogEndLoc = api.GetUniformLocation( ProgramId, "fogEnd" );
			fogDensityLoc = api.GetUniformLocation( ProgramId, "fogDensity" );
			fogModeLoc = api.GetUniformLocation( ProgramId, "fogMode" );
		}
		
		const int stride = VertexMapPacked.Size;
		public void DrawVb( OpenGLApi graphics, int vbId, int verticesCount, DrawMode mode ) {
			graphics.BindVb( vbId );
			graphics.EnableAndSetVertexAttribPointer( positionLoc, 3, VertexAttribType.Int16, false, stride, 0 );
			graphics.EnableAndSetVertexAttribPointer( flagsLoc, 2, VertexAttribType.UInt8, false, stride, 6 );
			
			graphics.DrawVb( mode, 0, verticesCount );
			
			graphics.DisableVertexAttribArray( positionLoc );
			graphics.DisableVertexAttribArray( flagsLoc );
		}
	}
	
	public sealed class MapLiquidDepthPassShader : Shader {
		
		public MapLiquidDepthPassShader() {
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
		
		public int positionLoc, mvpLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
		}
	}
}