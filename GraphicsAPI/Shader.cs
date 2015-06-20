using System;
using System.Text;
using System.IO;
using OpenTK;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {

	public abstract class Shader : IDisposable {
		
		public string VertexSource;
		
		public string FragmentSource;
		
		public int ProgramId;
		
		protected OpenGLApi api;
		
		public void Init( OpenGLApi api ) {
			string vertexShader = ReadShader( VertexSource );
			string fragmentShader = ReadShader( FragmentSource );
			ProgramId = api.CreateProgram( vertexShader, fragmentShader );
			this.api = api;
			GetLocations();
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
		
		public void Draw( DrawMode mode, int stride, int vb, int startVertex, int verticesCount ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, vb );
			EnableVertexAttribStates( stride );
			api.DrawModernVb( mode, vb, startVertex, verticesCount );
			DisableVertexAttribStates( stride );
		}
		
		public void DrawIndexed( DrawMode mode, int stride, int vb, int ib, int indicesCount,
		                        int startVertex, int startIndex ) {
			GL.BindBuffer( BufferTarget.ArrayBuffer, vb );
			GL.BindBuffer( BufferTarget.ElementArrayBuffer, ib );
			EnableVertexAttribStates( stride );
			api.DrawModernIndexedVb( mode, vb, ib, indicesCount, startVertex, startIndex );
			DisableVertexAttribStates( stride );
		}
		
		public void DrawDynamic<T>(  DrawMode mode, int stride, int vb, T[] vertices, int count ) where T : struct {
			GL.BindBuffer( BufferTarget.ArrayBuffer, vb );
			EnableVertexAttribStates( stride );
			api.DrawModernDynamicVb( mode, vb, vertices, stride, count );
			DisableVertexAttribStates( stride );
		}
		
		protected virtual void GetLocations() {
		}
		
		protected virtual void EnableVertexAttribStates( int stride ) {
		}
		
		protected virtual void DisableVertexAttribStates( int stride ) {
		}
		
		public void Dispose() {
			GL.DeleteProgram( ProgramId );
		}
	}
	
	public abstract class FogAndMVPShader : Shader {
		
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		protected override void GetLocations() {
			mvpLoc = api.GetUniformLocation( ProgramId, "MVP" );
			
			fogColLoc = api.GetUniformLocation( ProgramId, "fogColour" );
			fogEndLoc = api.GetUniformLocation( ProgramId, "fogEnd" );
			fogDensityLoc = api.GetUniformLocation( ProgramId, "fogDensity" );
			fogModeLoc = api.GetUniformLocation( ProgramId, "fogMode" );
		}
		
		float lastFogDensity, lastFogEnd;
		int lastFogMode;
		Vector4 lastFogCol;
		public void UpdateFogAndMVPState( ref Matrix4 mvp ) {
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
