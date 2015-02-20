using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class GrassCubeModel : CubeModel {
		
		public GrassCubeModel( Game game, byte block ) : base( game, block ) {
		}
		
		// TODO: We probably need a separate render pass for grass sides.
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			if( face == TileSide.Top ) {
				//int colour = info.GetColour( (float)( x % 64 ) / 63, (float)( z % 64 ) / 63 );
				//col = new FastColour( ( colour & 0xFF0000 ) >> 16, ( colour & 0xFF00 ) >> 8, ( colour & 0xFF ) );
				col = BlendedColour( x, y, z );
			}
			base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
		}
		
		FastColour BlendedColour( float x, float y, float z ) {
			int sumR = 0, sumG = 0, sumB = 0;
			for( int xx = - 1; xx <= 1; xx++ ) {
				for( int zz = -1; zz <= 1; zz++ ) {
					int col = info.GetColour( (float)( ( x + xx ) % 64 ) / 63, 
					                      (float)( ( z + zz ) % 64 ) / 63 );
					sumR += ( col & 0xFF0000 ) >> 16;
					sumG += ( col & 0xFF00 ) >> 8;
					sumB += ( col & 0xFF );
				}
			}
			return new FastColour( sumR / 9, sumG / 9, sumB / 9 );
		}
	}
}
