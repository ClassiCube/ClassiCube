using System;
using ClassicalSharp.Blocks.Model;

namespace ClassicalSharp.Blocks.Model {
	
	public abstract class IBlockModel {
		
		protected internal TextureAtlas2D atlas;
		protected internal BlockInfo info;
		protected internal byte block;
		
		public IBlockModel( TextureAtlas2D atlas, BlockInfo info, byte tile ) {
			this.atlas = atlas;
			this.block = tile;
			this.info = info;
		}
		
		public BlockPass Pass = BlockPass.Solid;
		
		public virtual bool HasFace( int face ) {
			return true;
		}
		
		public bool NeedsNeighbourState = false;
		
		public virtual bool FaceHidden( int face, byte meta, Neighbours state, byte neighbour ) {
			return info.IsFaceHidden( block, neighbour, face );
		}
		
		public abstract int GetVerticesCount( int face, byte meta, Neighbours state, byte neighbour );
		
		public abstract void DrawFace( int face, byte meta, Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col );
	}
	
	public enum BlockPass {
		Solid = 0,
		Sprite = 1,
		Transluscent = 2,
	}
	
	public struct Neighbours {
		public byte Above; // y + 1
		public byte Below; // y - 1
		public byte Left; // x - 1
		public byte Right; // x + 1
		public byte Front; // z - 1
		public byte Back; // z + 1
	}
}
