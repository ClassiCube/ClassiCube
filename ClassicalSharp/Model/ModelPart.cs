using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public struct ModelPart {
		
		public int Offset, Count;
		
		public ModelPart( int vertexOffset, int vertexCount ) {
			Offset = vertexOffset / 4 * 6;
			Count = vertexCount / 4 * 6;
		}
		
		public void Render( IGraphicsApi api ) {
			api.DrawIndexedVb_T2fC4b( DrawMode.Triangles, Count, 0, Offset );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}