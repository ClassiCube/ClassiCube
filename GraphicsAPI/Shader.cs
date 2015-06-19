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
		
		public void Draw( OpenGLApi api, DrawMode mode, VertexFormat format, int id, int startVertex, int verticesCount ) {
			EnableVertexAttribStates( api );
			api.DrawVb( mode, format, id, startVertex, verticesCount );
			DisableVertexAttribStates( api );
		}
		
		public void DrawIndexedVbBatch( OpenGLApi api, DrawMode mode, int vb, int ib, int indicesCount,
		                               int startVertex, int startIndex ) {
			EnableVertexAttribStates( api );
			api.DrawIndexedVbBatch( mode, vb, ib, indicesCount, startVertex, startIndex );
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
