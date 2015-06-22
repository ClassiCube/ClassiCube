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
			ProgramId = CreateProgram( vertexShader, fragmentShader );
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
		
		#region OpenGL api wrapper
		
		static int CreateProgram( string vertexShader, string fragShader ) {
			int vertexShaderId = LoadShader( vertexShader, ShaderType.VertexShader );
			int fragShaderId = LoadShader( fragShader, ShaderType.FragmentShader );
			int program = GL.CreateProgram();
			
			GL.AttachShader( program, vertexShaderId );
			GL.AttachShader( program, fragShaderId );
			GL.LinkProgram( program );
			
			string errorLog = GL.GetProgramInfoLog( program );
			if( !String.IsNullOrEmpty( errorLog ) ) {
				Console.WriteLine( "Program error for {0}:", program );
				Console.WriteLine( errorLog );
			}
			
			GL.DeleteShader( vertexShaderId );
			GL.DeleteShader( fragShaderId );
			return program;
		}
		
		static int LoadShader( string source, ShaderType type ) {
			int shaderId = GL.CreateShader( type );
			GL.ShaderSource( shaderId, source );
			GL.CompileShader( shaderId );
			
			string errorLog = GL.GetShaderInfoLog( shaderId );
			if( !String.IsNullOrEmpty( errorLog ) ) {
				Console.WriteLine( "Shader error for {0} ({1}):", shaderId, type );
				Console.WriteLine( errorLog );
			}
			return shaderId;
		}
		
		// Booooo, no direct state access for OpenGL 2.0.
		public void SetUniform( int uniformLoc, float value ) {
			GL.Uniform1( uniformLoc, value );
		}
		
		public void SetUniform( int uniformLoc, ref Vector2 value ) {
			GL.Uniform2( uniformLoc, value.X, value.Y );
		}
		
		public void SetUniform( int uniformLoc, ref Vector3 value ) {
			GL.Uniform3( uniformLoc, value.X, value.Y, value.Z );
		}
		
		public void SetUniform( int uniformLoc, ref Vector4 value ) {
			GL.Uniform4( uniformLoc, value.X, value.Y, value.Z, value.W );
		}
		
		public unsafe void SetUniform( int uniformLoc, ref Matrix4 value ) {
			fixed( float* ptr = &value.Row0.X )
				GL.UniformMatrix4( uniformLoc, 1, false, ptr );
		}
		
		public unsafe void SetUniformArray( int uniformLoc, float[] values ) {
			fixed( float* ptr = values )
				GL.Uniform1( uniformLoc, values.Length, ptr );
		}
		
		public unsafe void SetUniformArray( int uniformLoc, Vector2[] values ) {
			fixed( Vector2* ptr = values )
				GL.Uniform2( uniformLoc, values.Length, &ptr->X );
		}
		
		public unsafe void SetUniformArray( int uniformLoc, Vector3[] values ) {
			fixed( Vector3* ptr = values )
				GL.Uniform3( uniformLoc, values.Length, &ptr->X );
		}
		
		public unsafe void SetUniformArray( int uniformLoc, Vector4[] values ) {
			fixed( Vector4* ptr = values )
				GL.Uniform4( uniformLoc, values.Length, &ptr->X );
		}
		
		public int GetAttribLocation( string name ) {
			int loc = GL.GetAttribLocation( ProgramId, name );
			if( loc == -1 ) {
				Utils.LogWarning( "Attrib " + name + " returned -1 as its location." );
			}
			return loc;
		}
		
		public int GetUniformLocation( string name ) {
			int loc = GL.GetUniformLocation( ProgramId, name );
			if( loc == -1 ) {
				Utils.LogWarning( "Uniform " + name + " returned -1 as its location." );
			}
			return loc;
		}
		
		public unsafe void PrintAllAttribs() {
			int numAttribs;
			GL.GetProgram( ProgramId, ProgramParameter.ActiveAttributes, &numAttribs );
			for( int i = 0; i < numAttribs; i++ ) {
				int len, size;
				StringBuilder builder = new StringBuilder( 64 );
				ActiveAttribType type;
				GL.GetActiveAttrib( ProgramId, i, 32, &len, &size, &type, builder );
				Console.WriteLine( i + " : " + type + ", " + builder );
			}
		}
		
		public unsafe void PrintAllUniforms() {
			int numUniforms;
			GL.GetProgram( ProgramId, ProgramParameter.ActiveUniforms, &numUniforms );
			for( int i = 0; i < numUniforms; i++ ) {
				int len, size;
				StringBuilder builder = new StringBuilder( 64 );
				ActiveUniformType type;
				GL.GetActiveUniform( ProgramId, i, 32, &len, &size, &type, builder );
				Console.WriteLine( i + " : " + type + ", " + builder );
			}
		}
		
		#endregion
		
		public bool IsValid {
			get { return GL.IsProgram( ProgramId ); }
		}
		
		public void Bind() {
			GL.UseProgram( ProgramId );
		}
		
		public void Dispose() {
			GL.DeleteProgram( ProgramId );
		}
	}
	
	public abstract class FogAndMVPShader : Shader {
		
		public int mvpLoc, fogColLoc, fogEndLoc, fogDensityLoc, fogModeLoc;
		protected override void GetLocations() {
			mvpLoc = GetUniformLocation( "MVP" );
			
			fogColLoc = GetUniformLocation( "fogColour" );
			fogEndLoc = GetUniformLocation( "fogEnd" );
			fogDensityLoc = GetUniformLocation( "fogDensity" );
			fogModeLoc = GetUniformLocation( "fogMode" );
		}
		
		float lastFogDensity, lastFogEnd;
		int lastFogMode;
		Vector4 lastFogCol;
		public void UpdateFogAndMVPState( ref Matrix4 mvp ) {
			SetUniform( mvpLoc, ref mvp );
			// Cache fog state so we don't change uniforms needlessly.
			if( api.modernFogCol != lastFogCol ) {
				SetUniform( fogColLoc, ref api.modernFogCol );
				lastFogCol = api.modernFogCol;
			}
			if( api.modernFogDensity != lastFogDensity ) {
				SetUniform( fogDensityLoc, api.modernFogDensity );
				lastFogDensity = api.modernFogDensity;
			}
			if( api.modernFogEnd != lastFogEnd ) {
				SetUniform( fogEndLoc, api.modernFogEnd );
				lastFogEnd = api.modernFogEnd;
			}
			if( api.modernFogMode != lastFogMode ) {
				SetUniform( fogModeLoc, api.modernFogMode );
				lastFogMode = api.modernFogMode;
			}
		}
	}
}
