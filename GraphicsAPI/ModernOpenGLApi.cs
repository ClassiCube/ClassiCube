using System;
using System.Text;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {

	public class ModernOpenGLApi {
		
		public int CreateProgram( string vertexShader, string fragShader ) {
			int vertexShaderId = LoadShader( vertexShader, ShaderType.VertexShader );
			int fragShaderId = LoadShader( fragShader, ShaderType.FragmentShader );
			int program = GL.CreateProgram();
			
			GL.AttachShader( program, vertexShaderId );
			GL.AttachShader( program, fragShaderId );
			GL.LinkProgram( program );
			
			string errorLog = GL.GetProgramInfoLog( program );
			Console.WriteLine( "Program error for {0}:", program );
			Console.WriteLine( errorLog );
			
			GL.DeleteShader( vertexShaderId );
			GL.DeleteShader( fragShaderId );
			return program;
		}
		
		static int LoadShader( string source, ShaderType type ) {
			int shaderId = GL.CreateShader( type );
			int len = source.Length;
			GL.ShaderSource( shaderId, 1, new [] { source }, ref len );
			GL.CompileShader( shaderId );
			
			string errorLog = GL.GetShaderInfoLog( shaderId );
			Console.WriteLine( "Shader error for {0} ({1}):", shaderId, type );
			Console.WriteLine( errorLog );
			return shaderId;
		}
		
		public void UseProgram( int program ) {
			GL.UseProgram( program );
		}
		
		// Booooo, no direct state access for OpenGL 2.0.
		
		public void SetUniform( int uniformLoc, ref float value ) {
			GL.Uniform1( uniformLoc, value );
		}
		
		public void SetUniform( int uniformLoc, ref Vector2 value ) {
			GL.Uniform2( uniformLoc, ref value );
		}
		
		public void SetUniform( int uniformLoc, ref Vector3 value ) {
			GL.Uniform3( uniformLoc, ref value );
		}
		
		public void SetUniform( int uniformLoc, ref Vector4 value ) {
			GL.Uniform4( uniformLoc, ref value );
		}
		
		public void SetUniform( int uniformLoc, ref Matrix4 value ) {
			GL.UniformMatrix4( uniformLoc, false, ref value );
		}
		
		public int GetAttribLocation( int program, string name ) {
			return GL.GetAttribLocation( program, name );
		}
		
		public int GetUniformLocation( int program, string name ) {
			return GL.GetUniformLocation( program, name );
		}
		
		public void PrintAllAttribs( int program ) {
			int numAttribs;
			GL.GetProgram( program, ProgramParameter.ActiveAttributes, out numAttribs );
			for( int i = 0; i < numAttribs; i++ ) {
				int len, size;
				StringBuilder builder = new StringBuilder( 64 );
				ActiveAttribType type;
				GL.GetActiveAttrib( program, i, 32, out len, out size, out type, builder );
				Console.WriteLine( i + " : " + type + ", " + builder );
			}
		}
		
		public void PrintAllUniforms( int program ) {
			int numUniforms;
			GL.GetProgram( program, ProgramParameter.ActiveUniforms, out numUniforms );
			for( int i = 0; i < numUniforms; i++ ) {
				int len, size;
				StringBuilder builder = new StringBuilder( 64 );
				ActiveUniformType type;
				GL.GetActiveUniform( program, i, 32, out len, out size, out type, builder );
				Console.WriteLine( i + " : " + type + ", " + builder );
			}
		}
	}
}