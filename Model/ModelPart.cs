using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int Offset = 0;
		public int Count;
		public IGraphicsApi Graphics;
		
		public ModelPart( int offset, int count, IGraphicsApi graphics ) {
			Offset = offset;
			Count = count;
			Graphics = graphics;
		}
		
		public void Render( int vb ) {
			Graphics.DrawVb( DrawMode.Triangles, VertexFormat.Pos3fTex2fCol4b, vb, Offset, Count );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}