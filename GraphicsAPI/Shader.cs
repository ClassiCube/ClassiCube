using System;
using System.Text;
using System.IO;
using OpenTK;

namespace ClassicalSharp.GraphicsAPI {

	public abstract class Shader {
		
		public string VertexSource;
		
		public string FragmentSource;
		
		public int ProgramId;
		
		public void Init( OpenGLApi api ) {
			string vertexShader = ReadShader( VertexSource );
			string fragmentShader = ReadShader( FragmentSource );
			ProgramId = api.CreateProgram( vertexShader, fragmentShader );
			GetLocations( api );
		}
		
		const string fogUniformCode = @"
uniform vec4 fogColour;
uniform float fogEnd;
uniform float fogDensity;
uniform float fogMode;
// 0 = linear, 1 = exp
";
		
		const string pos3fTex2fCol4bAttributesCode = @"
in vec3 in_position;
in vec4 in_colour;
in vec2 in_texcoords;
flat out vec4 out_colour;
out vec2 out_texcoords;
";
		
		const string fogShaderCode = @"
   float alpha_start = finalColour.a;
   float depth = (gl_FragCoord.z / gl_FragCoord.w);
   float f = 0;
   if(fogMode == 0) {
      f = (fogEnd - depth) / fogEnd; // omit (end - start) since start is 0
   } else {
      f = exp(-fogDensity * depth);
   }
   f = clamp(f, 0.0, 1.0);
   finalColour = mix(fogColour, finalColour, f);
   finalColour.a = alpha_start;
";
		
		string ReadShader( string source ) {
			StringBuilder builder = new StringBuilder( source.Length );
			using( StringReader reader = new StringReader( source ) ) {
				string line;
				while( ( line = reader.ReadLine() ) != null ) {
					if( line == "--IMPORT fog_uniforms" ) {
						builder.Append( fogUniformCode );
					} else if( line == "--IMPORT fog_code" ) {
						builder.Append( fogShaderCode );
					} else if( line == "--IMPORT pos3fTex2fCol4b_attributes" ) {
						builder.Append( pos3fTex2fCol4bAttributesCode );
					} else if( line.StartsWith( "--IMPORT" ) ) {
						Utils.LogWarning( "Unrecognised import: " + line );
					} else {
						builder.AppendLine( line );
					}
				}
			}
			return builder.ToString();
		}
		
		public void Draw( OpenGLApi api, DrawMode mode, int stride, int vb, int startVertex, int verticesCount ) {
			api.BindModernVB( vb );
			EnableVertexAttribStates( api, stride );
			api.DrawModernVb( mode, vb, startVertex, verticesCount );
			DisableVertexAttribStates( api, stride );
		}
		
		public void DrawIndexed( OpenGLApi api, DrawMode mode, int stride, int vb, int ib, int indicesCount,
		                        int startVertex, int startIndex ) {
			api.BindModernIndexedVb( vb, ib );
			EnableVertexAttribStates( api, stride );
			api.DrawModernIndexedVb( mode, vb, ib, indicesCount, startVertex, startIndex );
			DisableVertexAttribStates( api, stride );
		}
		
		public void DrawDynamic<T>( OpenGLApi api, DrawMode mode, int stride, int vb, T[] vertices, int count ) where T : struct {
			api.BindModernVB( vb );
			EnableVertexAttribStates( api, stride );
			api.DrawModernDynamicVb( mode, vb, vertices, stride, count );
			DisableVertexAttribStates( api, stride );
		}
		
		protected virtual void GetLocations( OpenGLApi api ) {
		}
		
		protected virtual void EnableVertexAttribStates( OpenGLApi api, int stride ) {
		}
		
		protected virtual void DisableVertexAttribStates( OpenGLApi api, int stride ) {
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
		
		float lastFogDensity, lastFogEnd;
		int lastFogMode;
		Vector4 lastFogCol;
		public void UpdateFogAndMVPState( OpenGLApi api, ref Matrix4 mvp ) {
			api.SetUniform( mvpLoc, ref mvp );
			// Cache fog state so we don't change uniforms needlessly.
			if( api.modernFogCol != lastFogCol ) {
				api.SetUniform( fogColLoc, ref api.modernFogCol );
				lastFogCol = api.modernFogCol;
			}
			if( api.modernFogDensity != lastFogDensity ) {
				api.SetUniform( fogDensityLoc, api.modernFogDensity );
				lastFogDensity = api.modernFogDensity;
			}
			if( api.modernFogEnd != lastFogEnd ) {
				api.SetUniform( fogEndLoc, api.modernFogEnd );
				lastFogEnd = api.modernFogEnd;
			}
			if( api.modernFogMode != lastFogMode ) {
				api.SetUniform( fogModeLoc, api.modernFogMode );
				lastFogMode = api.modernFogMode;
			}
		}
	}
}
