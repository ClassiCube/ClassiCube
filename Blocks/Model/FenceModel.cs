using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class FenceModel : CubeModel {
		
		public FenceModel( TextureAtlas2D atlas, BlockInfo info, byte block ) : base( atlas, info, block ) {
			min = new Vector3( 6 / 16f, 0f, 6 / 16f );
			max = new Vector3( 10 / 16f, 1f, 10 / 16f );
			
			TextureRectangle baseRec = recs[0];
			float scale = atlas.invVerElementSize;
			for( int face = 0; face < TileSide.Bottom; face++ ) {
				recs[face].U1 = baseRec.U1 + min.X * scale;
				recs[face].U2 = baseRec.U1 + max.X * scale;
			}
			
			TextureRectangle verRec = baseRec;
			verRec.U1 = baseRec.U1 + min.X * scale;
			verRec.U2 = baseRec.U1 + max.X * scale;
			verRec.V1 = baseRec.V1 + min.Z * scale;
			verRec.V2 = baseRec.V1 + max.Z * scale;
			recs[TileSide.Bottom] = verRec;
			recs[TileSide.Top] = verRec;
		}
		
		public override bool FaceHidden( int face, byte meta, byte neighbour ) {
			return face >= TileSide.Bottom && ( neighbour == block || info.IsOpaque( neighbour ) );
		}
		
		public override int GetVerticesCount( int face, byte meta, byte neighbour ) {
			return 6;
		}
	}
}
