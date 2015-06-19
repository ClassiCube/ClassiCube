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
#version 130
in vec3 in_position;
in vec2 in_texcoords;
in uint in_colour;
in uint in_block;
smooth out vec2 out_texcoords;
flat out uint out_colour;
flat out uint out_block;
uniform mat4 MVP;
uniform float worldTime;

void main() {
   gl_Position = MVP * vec4(in_position, 1.0);
   if(in_block == 18u) {
      gl_Position.x += sin(worldTime) * 0.25 * fract( in_position.y / 2.0 );
   }
   out_texcoords = in_texcoords;
   out_colour = in_colour;
   out_block = in_block;
}";
			
			FragSource = @"
#version 130
smooth in vec2 out_texcoords;
flat in uint out_colour;
flat in uint out_block;
uniform sampler2D texImage;
uniform vec4 fogColour;
uniform float fogEnd;
uniform float fogDensity;
uniform float fogMode;
const vec4 lightCols[8] = vec4[8](
	vec4(1.0, 1.0, 1.0, 1.0 ),
	vec4(0.6, 0.6, 0.6, 1.0 ),
	
	vec4(0.6, 0.6, 0.6, 1.0 ),
	vec4(0.36, 0.36, 0.36, 1.0 ),
	
	vec4(0.8, 0.8, 0.8, 1.0 ),
	vec4(0.48, 0.48, 0.48, 1.0 ),
	
	vec4(0.5, 0.5, 0.5, 1.0 ),
	vec4(0.3, 0.3, 0.3, 1.0 )
);

void main() {
   vec4 finalColour = texture2D(texImage, out_texcoords) * lightCols[out_colour];
   if(finalColour.a < 0.5) {
      discard;
   }
   if(out_block == 18u) {
      finalColour.g += 0.0001;
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
		
		public int positionLoc, texCoordsLoc, colourLoc, tileLoc;
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		public int texImageLoc, worldTimeLoc;
		protected override void BindParameters( OpenGLApi api ) {			
			positionLoc = api.GetAttribLocation( ProgramId, "in_position" );
			texCoordsLoc = api.GetAttribLocation( ProgramId, "in_texcoords" );
			colourLoc = api.GetAttribLocation( ProgramId, "in_colour" );
			tileLoc = api.GetAttribLocation( ProgramId, "in_block" );
			
			texImageLoc = api.GetUniformLocation( ProgramId, "texImage" );
			worldTimeLoc = api.GetUniformLocation( ProgramId, "worldTime" );
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
			graphics.EnableAndSetVertexAttribPointer( colourLoc, 1, VertexAttribType.UInt8, true, stride, 20 );
			graphics.EnableAndSetVertexAttribPointer( tileLoc, 1, VertexAttribType.UInt8, true, stride, 21 );
			
			graphics.DrawVb( mode, 0, verticesCount );
			
			graphics.DisableVertexAttribArray( positionLoc );
			graphics.DisableVertexAttribArray( texCoordsLoc );
			graphics.DisableVertexAttribArray( colourLoc );
			graphics.DisableVertexAttribArray( tileLoc );
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