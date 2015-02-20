using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class LeverModel : TorchModel {
		
		CubeModel baseModel;
		public LeverModel( Game game, byte block ) : base( game, block ) {
			baseModel = new CubeModel( game, (byte)BlockId.Cobblestone );
			baseModel.min = new Vector3( 5 / 16f, 0, 4 / 16f );
            baseModel.max = new Vector3( 11 / 16f, 3 / 16f, 12 / 16f );
            
            TextureRectangle baseRec = baseModel.recs[0];
			float scale = atlas.invVerElementSize;
			
			TextureRectangle verRec = baseRec;
			verRec.U1 = baseRec.U1 + ( 5 / 16f ) * scale;
			verRec.U2 = baseRec.U1 + ( 11 / 16f ) * scale;
			verRec.V1 = baseRec.V1 + ( 4 / 16f ) * scale;
			verRec.V2 = baseRec.V1 + ( 12 / 16f ) * scale;
			baseModel.recs[TileSide.Bottom] = 
				baseModel.recs[TileSide.Top] = verRec;
			
			TextureRectangle nsRec = baseRec;
			nsRec.U1 = baseRec.U1 + ( 5 / 16f ) * scale;
			nsRec.U2 = baseRec.U1 + ( 11 / 16f ) * scale;
			nsRec.V1 = baseRec.V1 + ( 0 / 16f ) * scale;
			nsRec.V2 = baseRec.V1 + ( 3 / 16f ) * scale;
			baseModel.recs[TileSide.Front] = 
				baseModel.recs[TileSide.Back] = nsRec;
			
			TextureRectangle weRec = baseRec;
			weRec.U1 = baseRec.U1 + ( 4 / 16f ) * scale;
			weRec.U2 = baseRec.U1 + ( 12 / 16f ) * scale;
			weRec.V1 = baseRec.V1 + ( 0 / 16f ) * scale;
			weRec.V2 = baseRec.V1 + ( 3 / 16f ) * scale;
			baseModel.recs[TileSide.Left] = 
				baseModel.recs[TileSide.Right] = weRec;
		}
		
		public override bool FaceHidden( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return face == TileSide.Bottom && info.IsOpaque( neighbour );
		}
		
		public override int GetVerticesCount( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return 12;
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, 
		                              float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {			
			baseModel.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
			base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
		}
	}
}
