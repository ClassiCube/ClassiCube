using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	/// <summary> Describes the starting index of this part within a model's array of vertices,
	/// and the number of vertices following the starting index that this part uses. </summary>
	public struct ModelPart {
		
		public int Offset, Count;
		
		public ModelPart( int vertexOffset, int vertexCount ) {
			Offset = vertexOffset;
			Count = vertexCount;
		}
	}
	
	/// <summary> Describes the type of skin that a humanoid model uses. </summary>
	public enum SkinType {
		Type64x32,
		Type64x64,
		Type64x64Slim,
	}
}