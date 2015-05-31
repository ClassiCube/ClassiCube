using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class ModelPart {
		
		public int VbId;
		public int Offset = 0;
		public int Count;
		public IGraphicsApi Graphics;
		
		public ModelPart( int vb, int offset, int count, IGraphicsApi graphics ) {
			Offset = offset;
			Count = count;
			Graphics = graphics;
			VbId = vb;
		}
		
		public void Render() {
			Graphics.DrawVb( DrawMode.Triangles, VertexFormat.Pos3fTex2fCol4b, VbId, Offset, Count );
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