using System;
using System.Text;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {

	public partial class OpenGLApi {
		
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
		
		public void SetVertexAttrib( int attribLoc, float v0 ) {
			GL.VertexAttrib1( attribLoc, v0 );
		}
		
		public void SetVertexAttrib( int attribLoc, float v0, float v1 ) {
			GL.VertexAttrib2( attribLoc, v0, v1 );
		}
		
		public void SetVertexAttrib( int attribLoc, Vector2 v ) {
			GL.VertexAttrib2( attribLoc, v );
		}
		
		public void SetVertexAttrib( int attribLoc, float v0, float v1, float v2 ) {
			GL.VertexAttrib3( attribLoc, v0, v1, v2 );
		}
		
		public void SetVertexAttrib( int attribLoc, Vector3 v ) {
			GL.VertexAttrib3( attribLoc, v );
		}
		
		public void SetVertexAttrib( int attribLoc, float v0, float v1, float v2, float v3 ) {
			GL.VertexAttrib4( attribLoc, v0, v1, v2, v3 );
		}
		
		public void SetVertexAttrib( int attribLoc, Vector4 v ) {
			GL.VertexAttrib4( attribLoc, v );
		}
		
		public void SetVertexAttribPointerF( int attribLoc, int numComponents, int stride, int offset ) {
			GL.VertexAttribPointer( attribLoc, numComponents, VertexAttribPointerType.Float, false, stride, new IntPtr( offset ) );
		}
		
		public void EnableAndSetVertexAttribPointerF( int attribLoc, int numComponents, int stride, int offset ) {
			GL.EnableVertexAttribArray( attribLoc );
			GL.VertexAttribPointer( attribLoc, numComponents, VertexAttribPointerType.Float, false, stride, new IntPtr( offset ) );
		}
		
		VertexAttribPointerType[] attribMappings = new VertexAttribPointerType[] {
			VertexAttribPointerType.Float, VertexAttribPointerType.UnsignedByte,
			VertexAttribPointerType.UnsignedShort, VertexAttribPointerType.UnsignedInt,
			VertexAttribPointerType.Byte, VertexAttribPointerType.Short,
			VertexAttribPointerType.Int,
		};
		public void SetVertexAttribPointer( int attribLoc, int numComponents, VertexAttribType type, bool normalise, int stride, int offset ) {
			GL.VertexAttribPointer( attribLoc, numComponents, attribMappings[(int)type], normalise, stride, new IntPtr( offset ) );
		}
		
		public void EnableAndSetVertexAttribPointer( int attribLoc, int numComponents, VertexAttribType type, bool normalise, int stride, int offset ) {
			GL.EnableVertexAttribArray( attribLoc );
			GL.VertexAttribPointer( attribLoc, numComponents, attribMappings[(int)type], normalise, stride, new IntPtr( offset ) );
		}
		
		public void EnableVertexAttribArray( int attribLoc ) {
			GL.EnableVertexAttribArray( attribLoc );
		}
		
		public void DisableVertexAttribArray( int attribLoc ) {
			GL.DisableVertexAttribArray( attribLoc );
		}
		
		// Booooo, no direct state access for OpenGL 2.0.
		
		public void SetUniform( int uniformLoc, float value ) {
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
	
	public enum VertexAttribType : byte {
		Float32 = 0,
		UInt8 = 1,
		UInt16 = 2,
		UInt32 = 3,
		Int8 = 4,
		Int16 = 5,
		Int32 = 6,
	}
}