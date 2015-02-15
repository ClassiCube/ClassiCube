using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class SaplingModel : SpriteModel {
		
		TextureRectangle recO, recS, recB;
		public SaplingModel( Game game ) : base( game, (byte)BlockId.Sapling ) {
			recO = atlas.GetTexRec( 14, 0 ); // oak
			recS = atlas.GetTexRec( 10, 3 ); // spruce
			recB = atlas.GetTexRec( 7, 4 ); // birch
		}
		
		public override void DrawFace( int face, byte meta, Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			int type = meta & 0x03;
			rec = recO;
			if( type == 1 ) {
				rec = recS;
			} else if( type == 2 ) {
				rec = recB;
			}
			base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
		}
	}
}
