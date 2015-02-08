using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class RailsModel : CubeModel {
		
		public RailsModel( Game game, byte block ) : base( game, block ) {
			Pass = BlockPass.Sprite;
			max.Y = 1 / 16f;
		}
		
		public override bool HasFace( int face ) {
			return face == TileSide.Top;
		}
		
		public override bool FaceHidden( int face, byte meta, Neighbours state, byte neighbour ) {
			return false;
		}
		
		public override int GetVerticesCount( int face, byte meta, Neighbours state, byte neighbour ) {
			return 6;
		}
	}
}
