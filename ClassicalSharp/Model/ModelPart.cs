using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public struct ModelPart {
		
		public int Offset, Count;
		
		public ModelPart( int vertexOffset, int vertexCount ) {
			Offset = vertexOffset;
			Count = vertexCount;
		}
	}
	
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}