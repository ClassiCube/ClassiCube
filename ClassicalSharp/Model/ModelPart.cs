using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int Offset = 0;
		public int Count;
		
		public ModelPart( int offset, int count ) {
			Offset = offset;
			Count = count;
		}
		
		public void Render( IGraphicsApi api ) {
			api.DrawVb( DrawMode.Triangles, Offset, Count );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}