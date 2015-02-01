using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class GrassCubeModel : CubeModel {
		
		public GrassCubeModel( TextureAtlas2D atlas, BlockInfo info, byte block ) : base( atlas, info, block ) {
		}
		
		// TODO: This looks better than grey grass, but it's still not right.
		// It appears we have to over-saturate the colour, and that can't be done with normalised unsigned bytes.
		// We probably need a separate render pass for grass, foilage, etc.
		static readonly FastColour grassCol = new FastColour( 200, 255, 128 );
		public override void DrawFace( int face, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			if( face == TileSide.Top ) {
				col = grassCol;
			}
			base.DrawFace( face, ref index, x, y, z, vertices, col );
		}
	}
}
