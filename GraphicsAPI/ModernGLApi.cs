using System;
using OpenTK.Graphics.OpenGL;

namespace ClassicalSharp.GraphicsAPI {

	public partial class OpenGLApi {
		
		public void EnableVertexAttribF( int attribLoc, int numComponents, int stride, int offset ) {
			GL.EnableVertexAttribArray( attribLoc );
			GL.VertexAttribPointer( attribLoc, numComponents, VertexAttribPointerType.Float, false, stride, new IntPtr( offset ) );
		}
		
		public void EnableVertexAttribF( int attribLoc, int numComponents, VertexAttribType type, bool normalise, int stride, int offset ) {
			GL.EnableVertexAttribArray( attribLoc );
			GL.VertexAttribPointer( attribLoc, numComponents, attribMappings[(int)type], normalise, stride, new IntPtr( offset ) );
		}
		
		public void EnableVertexAttribI( int attribLoc, int numComponents, VertexAttribType type, int stride, int offset ) {
			GL.EnableVertexAttribArray( attribLoc );
			GL.VertexAttribIPointer( attribLoc, numComponents, attribIMappings[(int)type], stride, new IntPtr( offset ) );
		}
		
		VertexAttribPointerType[] attribMappings = {
			VertexAttribPointerType.Float, VertexAttribPointerType.UnsignedByte,
			VertexAttribPointerType.UnsignedShort, VertexAttribPointerType.UnsignedInt,
			VertexAttribPointerType.Byte, VertexAttribPointerType.Short,
			VertexAttribPointerType.Int,
		};
		VertexAttribIPointerType[] attribIMappings = {
			(VertexAttribIPointerType)999, VertexAttribIPointerType.UnsignedByte,
			VertexAttribIPointerType.UnsignedShort, VertexAttribIPointerType.UnsignedInt,
			VertexAttribIPointerType.Byte, VertexAttribIPointerType.Short,
			VertexAttribIPointerType.Int,
		};
		
		public void DisableVertexAttrib( int attribLoc ) {
			GL.DisableVertexAttribArray( attribLoc );
		}
		
		public void DrawModernVb( DrawMode mode, int id, int startVertex, int verticesCount ) {
			GL.DrawArrays( modeMappings[(int)mode], startVertex, verticesCount );
		}
		
		public void DrawModernDynamicVb<T>( DrawMode mode, int vb, T[] vertices, int stride, int count ) where T : struct {
			int sizeInBytes = count * stride;
			GL.BufferSubData( BufferTarget.ArrayBuffer, IntPtr.Zero, new IntPtr( sizeInBytes ), vertices );
			GL.DrawArrays( modeMappings[(int)mode], 0, count );
		}
		
		const DrawElementsType indexType = DrawElementsType.UnsignedShort;
		public void DrawModernIndexedVb( DrawMode mode, int vb, int ib, int indicesCount,
		                                int startVertex, int startIndex ) {
			int offset = startVertex * VertexPos3fTex2fCol4b.Size;
			GL.DrawElements( modeMappings[(int)mode], indicesCount, indexType, new IntPtr( startIndex * 2 ) );
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