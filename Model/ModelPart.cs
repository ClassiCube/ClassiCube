using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int Offset = 0;
		public int Count;
		public OpenGLApi Graphics;
		public Shader Shader;
		
		public ModelPart( int offset, int count, OpenGLApi graphics, Shader shader ) {
			Offset = offset;
			Count = count;
			Graphics = graphics;
			Shader = shader;
		}
		
		public void Render( int vb ) {
			Shader.Draw( Graphics, DrawMode.Triangles, VertexPos3fTex2f.Size, vb, Offset, Count );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}