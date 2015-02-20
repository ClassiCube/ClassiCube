using System;
using ClassicalSharp.Blocks.Model;

namespace ClassicalSharp.Blocks.Model {
	
	public abstract class IBlockModel {
		
		protected internal TextureAtlas2D atlas;
		protected internal BlockInfo info;
		protected internal Game game;
		protected internal byte block;
		
		public IBlockModel( Game game, byte tile ) {
			this.block = tile;
			this.game = game;
			this.atlas = game.TerrainAtlas;
			this.info = game.BlockInfo;
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
		public byte Above, AboveMeta; // y + 1
		public byte Below, BelowMeta; // y - 1
		public byte Left, LeftMeta; // x - 1
		public byte Right, RightMeta; // x + 1
		public byte Front, FrontMeta; // z - 1
		public byte Back, BackMeta; // z + 1
	}
}
