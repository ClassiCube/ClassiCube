using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class RailsModel : CubeModel {
		
		public RailsModel( TextureAtlas2D atlas, BlockInfo info, byte block ) : base( atlas, info, block ) {
			Pass = BlockPass.Sprite;
			max.Y = 1 / 16f;
		}
		
		public override bool HasFace( int face ) {
			return face == TileSide.Top;
		}
		
		public override bool FaceHidden( int face, byte neighbour ) {
			return false;
		}
		
		public override int GetVerticesCount( int face, byte neighbour ) {
			return 6;
		}
	}
}
