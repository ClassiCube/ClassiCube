using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int VbId;
		public int Count;
		public OpenGLApi Graphics;
		
		public ModelPart( VertexPos3fTex2fCol4b[] vertices, int count, OpenGLApi graphics ) {
			Count = count;
			Graphics = graphics;
			VbId = Graphics.InitVb( vertices, VertexFormat.VertexPos3fTex2fCol4b, count );
		}
		
		public void Render( MapShader shader ) {
			shader.DrawVb( Graphics, VbId, Count );
		}
		
		public void Dispose() {
			Graphics.DeleteVb( VbId );
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}