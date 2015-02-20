using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class SaplingModel : SpriteModel {
		
		TextureRectangle[] recs = new TextureRectangle[4];
		public SaplingModel( Game game ) : base( game, (byte)BlockId.Sapling ) {
			recs[0] = recs[3] = atlas.GetTexRec( 14, 0 ); // oak
			recs[1] = atlas.GetTexRec( 10, 3 ); // spruce 
			recs[2] = atlas.GetTexRec( 7, 4 ); // birch
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			rec = recs[meta & 0x03];
			base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
		}
	}
}
