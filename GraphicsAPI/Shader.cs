using System;

namespace ClassicalSharp.GraphicsAPI {

	public abstract class Shader {
		
		public string VertexSource;
		
		public string FragmentSource;
		
		public int ProgramId;
		
		public void Init( OpenGLApi api ) {
			ProgramId = api.CreateProgram( VertexSource, FragmentSource );
			GetLocations( api );
		}
		
		public void Draw( OpenGLApi api, DrawMode mode, VertexFormat format, int vb, int startVertex, int verticesCount ) {
			api.BindModernVB( vb );
			EnableVertexAttribStates( api );
			api.DrawModernVb( mode, vb, startVertex, verticesCount );
			DisableVertexAttribStates( api );
		}
		
		public void DrawIndexed( OpenGLApi api, DrawMode mode, int vb, int ib, int indicesCount,
		                               int startVertex, int startIndex ) {
			api.BindModernIndexedVb( vb, ib );
			EnableVertexAttribStates( api );
			api.DrawModernIndexedVb( mode, vb, ib, indicesCount, startVertex, startIndex );
			DisableVertexAttribStates( api );
		}
		
		public void DrawDynamic<T>( OpenGLApi api, DrawMode mode, VertexFormat format, int vb, T[] vertices, int count ) where T : struct {
			api.BindModernVB( vb );
			EnableVertexAttribStates( api );
			api.DrawModernDynamicVb( mode, vb, vertices, format, count );
			DisableVertexAttribStates( api );
		}
		
		protected virtual void GetLocations( OpenGLApi api ) {
		}
		
		protected virtual void EnableVertexAttribStates( OpenGLApi api ) {
		}
		
		protected virtual void DisableVertexAttribStates( OpenGLApi api ) {
		}
	}
}
