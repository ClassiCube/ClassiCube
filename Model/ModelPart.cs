using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int Vb;
		public int Offset = 0;
		public int Count;
		public OpenGLApi Graphics;
		
		public ModelPart( int vb, int offset, int count, OpenGLApi graphics ) {
			Offset = offset;
			Count = count;
			Graphics = graphics;
			Vb = vb;
		}
		
		public void Render() {
			Graphics.DrawVb( DrawMode.Triangles, VertexFormat.Pos3fTex2fCol4b, Vb, Offset, Count );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}