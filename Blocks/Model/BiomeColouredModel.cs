using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class BiomeColouredModel : IBlockModel {
		
		IBlockModel drawer;
		public BiomeColouredModel( IBlockModel drawer ) : base( drawer.game, drawer.block ) {
			this.drawer = drawer;
			Pass = drawer.Pass;
		}
		
		public override bool HasFace( int face ) {
			return drawer.HasFace( face );
		}
		
		public override bool FaceHidden( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return drawer.FaceHidden( face, meta, ref state, neighbour );
		}
		
		public override int GetVerticesCount( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return drawer.GetVerticesCount( face, meta, ref state, neighbour );
		}
		
		float[] scales = new float[] { 0.6f, 0.6f, 0.8f, 0.8f, 0.5f, 1f };
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			// TODO: Also according to the page, we have to average the eight neighbours for proper biome blending.
			// Though since we don't even use biomes yet, I guess that doesn't really matter.
			int colour = info.GetColour( (float)( x % 64 ) / 63, (float)( z % 64 ) / 63 );
			col = new FastColour( ( colour & 0xFF0000 ) >> 16, ( colour & 0xFF00 ) >> 8, ( colour & 0xFF ) );
			col = FastColour.Scale( col, scales[face] );			
			drawer.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
		}
	}
}
