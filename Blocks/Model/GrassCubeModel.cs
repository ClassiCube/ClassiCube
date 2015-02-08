using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class GrassCubeModel : CubeModel {
		
		public GrassCubeModel( Game game, byte block ) : base( game, block ) {
		}
		
		// TODO: We probably need a separate render pass for grass sides.
		public override void DrawFace( int face, byte meta, Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			if( face == TileSide.Top ) {
				int colour = info.GetColour( (float)( x % 64 ) / 63, (float)( z % 64 ) / 63 );
				col = new FastColour( ( colour & 0xFF0000 ) >> 16, ( colour & 0xFF00 ) >> 8, ( colour & 0xFF ) );
			}
			base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
		}
	}
}
