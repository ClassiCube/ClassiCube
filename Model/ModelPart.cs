using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int Vb;
		public int Offset = 0;
		public int Count;
		public OpenGLApi Graphics;
		public Shader Shader;
		
		public ModelPart( int vb, int offset, int count, OpenGLApi graphics, Shader shader ) {
			Offset = offset;
			Count = count;
			Graphics = graphics;
			Vb = vb;
			Shader = shader;
		}
		
		public void Render() {
			Shader.Draw( Graphics, DrawMode.Triangles, VertexPos3fTex2f.Size, Vb, Offset, Count );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}