using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class TorchModel : CubeModel {
		
		public TorchModel( TextureAtlas2D atlas, BlockInfo info, byte block ) : base( atlas, info, block ) {
			min = new Vector3( 7 / 16f, 0f, 7 / 16f );
			max = new Vector3( 9 / 16f, 1f, 9 / 16f );
			
			TextureRectangle baseRec = recs[0];
			float scale = atlas.invVerElementSize;
			for( int face = 0; face < TileSide.Bottom; face++ ) {
				recs[face].U1 = baseRec.U1 + min.X * scale;
				recs[face].U2 = baseRec.U1 + max.X * scale;
			}
			
			TextureRectangle verRec = baseRec;
			verRec.U1 = baseRec.U1 + ( 7 / 16f ) * scale;
			verRec.U2 = baseRec.U1 + ( 9 / 16f ) * scale;
			verRec.V1 = baseRec.V1 + ( 8 / 16f ) * scale;
			verRec.V2 = baseRec.V1 + ( 10 / 16f ) * scale;
			recs[TileSide.Bottom] = verRec;
			
			verRec.V1 = baseRec.V1 + ( 6 / 16f ) * scale;
			verRec.V2 = baseRec.V1 + ( 8 / 16f ) * scale;
			recs[TileSide.Top] = verRec;
		}
		
		public override bool FaceHidden( int face, byte neighbour ) {
			return face == TileSide.Bottom && info.IsOpaque( neighbour );
		}
		
		public override int GetVerticesCount( int face, byte neighbour ) {
			return 6;
		}
		
		protected override void DrawTopFace( ref int index, float x, float y, float z,
		                           VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			TextureRectangle rec = recs[TileSide.Top];

			vertices[index++] = new VertexPos3fTex2fCol4b( x + max.X, y + 10 / 16f, z + min.Z, rec.U2, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + min.X, y + 10 / 16f, z + min.Z, rec.U1, rec.V1, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + min.X, y + 10 / 16f, z + max.Z, rec.U1, rec.V2, col );
			
			vertices[index++] = new VertexPos3fTex2fCol4b( x + min.X, y + 10 / 16f, z + max.Z, rec.U1, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + max.X, y + 10 / 16f, z + max.Z, rec.U2, rec.V2, col );
			vertices[index++] = new VertexPos3fTex2fCol4b( x + max.X, y + 10 / 16f, z + min.Z, rec.U2, rec.V1, col );
		}
	}
}
