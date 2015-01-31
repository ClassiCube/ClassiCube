using System;

namespace ClassicalSharp.Blocks.Model {
	
	public abstract class IBlockModel {
		
		protected internal TextureAtlas2D atlas;
		protected internal BlockInfo info;
		protected byte block;
		
		public IBlockModel( TextureAtlas2D atlas, BlockInfo info, byte tile ) {
			this.atlas = atlas;
			this.block = tile;
			this.info = info;
		}
		
		public BlockPass Pass = BlockPass.Solid;
		
		public virtual bool HasFace( int face ) {
			return true;
		}
		
		public virtual bool FaceHidden( int face, byte neighbour ) {
			return info.IsFaceHidden( block, neighbour, face );
		}
		
		public abstract int GetVerticesCount( int face, byte neighbour );
		
		public abstract void DrawFace( int face, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col );
	}
	
	public enum BlockPass {
		Solid = 0,
		Sprite = 1,
		Transluscent = 2,
	}
}
